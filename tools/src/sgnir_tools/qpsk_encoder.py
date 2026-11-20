# Python 3 port of stm_audio_bootloader/qpsk/encoder.py
# (Mutable Instruments, MIT; original by Emilie Gillet, 2013).
# Port kept line-for-line faithful so the output is byte-identical to the
# Python 2 original; only py2->py3 semantics were adapted (integer division,
# bytes instead of str, argparse instead of optparse).
#
# Qpsk encoder for converting firmware .bin files into bootloader .wav files.

import argparse
import sys

import numpy
import zlib

from sgnir_tools import audio_stream_writer


class Scrambler:

  def __init__(self, seed=0):
    self._state = seed

  def scramble(self, data):
    data = bytearray(data)
    for index in range(len(data)):
      data[index] = data[index] ^ (self._state >> 24)
      self._state = (self._state * 1664525 + 1013904223) & 0xffffffff
    return bytes(data)


class QpskEncoder:

  def __init__(
      self,
      sample_rate=48000,
      carrier_frequency=2000,
      bit_rate=8000,
      packet_size=256,
      scramble=False):
    period = sample_rate // carrier_frequency
    symbol_time = sample_rate // (bit_rate // 2)

    assert (symbol_time % period == 0) or (period % symbol_time == 0)
    assert (sample_rate % carrier_frequency) == 0
    assert (sample_rate % bit_rate) == 0

    self._sr = sample_rate
    self._br = bit_rate
    self._carrier_frequency = carrier_frequency
    self._sample_index = 0
    self._packet_size = packet_size
    self._scrambler = Scrambler(0) if scramble else None

  @staticmethod
  def _upsample(x, factor):
    return numpy.tile(x.reshape(len(x), 1), (1, factor)).ravel()

  def _encode_qpsk(self, symbol_stream):
    ratio = self._sr // self._br * 2
    symbol_stream = numpy.array(symbol_stream)
    bitstream_even = 2 * self._upsample(symbol_stream % 2, ratio) - 1
    bitstream_odd = 2 * self._upsample(symbol_stream // 2, ratio) - 1
    return bitstream_even / numpy.sqrt(2.0), bitstream_odd / numpy.sqrt(2.0)

  def _modulate(self, q_mod, i_mod):
    num_samples = len(q_mod)
    t = (numpy.arange(0.0, num_samples) + self._sample_index) / self._sr
    self._sample_index += num_samples
    phase = 2 * numpy.pi * self._carrier_frequency * t
    return (q_mod * numpy.sin(phase) + i_mod * numpy.cos(phase))

  def _encode(self, symbol_stream):
    return self._modulate(*self._encode_qpsk(symbol_stream))

  def _code_blank(self, duration):
    num_zeros = int(duration * self._br / 8) * 4
    symbol_stream = numpy.zeros((num_zeros, 1)).ravel().astype(int)
    return self._encode(symbol_stream)

  def _code_packet(self, data):
    assert len(data) <= self._packet_size
    if len(data) != self._packet_size:
      data = data + b'\xff' * (self._packet_size - len(data))

    if self._scrambler:
      data = self._scrambler.scramble(data)

    crc = zlib.crc32(data) & 0xffffffff

    # 16x 0 for the PLL ; 8x 21 for the edge detector ; 8x 3030 for syncing
    preamble = [0] * 8 + [0x99] * 4 + [0xcc] * 4
    crc_bytes = [crc >> 24, (crc >> 16) & 0xff, (crc >> 8) & 0xff, crc & 0xff]
    byte_stream = preamble + list(data) + crc_bytes

    symbol_stream = []
    for byte in byte_stream:
      symbol_stream.append((byte >> 6) & 0x3)
      symbol_stream.append((byte >> 4) & 0x3)
      symbol_stream.append((byte >> 2) & 0x3)
      symbol_stream.append((byte >> 0) & 0x3)

    return self._encode(symbol_stream)

  def code_intro(self):
    yield numpy.zeros((1 * self._sr, 1)).ravel()
    yield self._code_blank(1.0)

  def code_outro(self, duration=1.0):
    yield self._code_blank(duration)

  def code(self, data, page_size=1024, blank_duration=0.06):
    if len(data) % page_size != 0:
      tail = page_size - (len(data) % page_size)
      data += b'\xff' * tail

    offset = 0
    remaining_bytes = len(data)
    num_packets_written = 0
    while remaining_bytes:
      size = min(remaining_bytes, self._packet_size)
      yield self._code_packet(data[offset:offset + size])
      num_packets_written += 1
      if num_packets_written == page_size // self._packet_size:
        yield self._code_blank(blank_duration)
        num_packets_written = 0
      remaining_bytes -= size
      offset += size


STM32F4_SECTOR_BASE_ADDRESS = [
    0x08000000,
    0x08004000,
    0x08008000,
    0x0800C000,
    0x08010000,
    0x08020000,
    0x08040000,
    0x08060000,
    0x08080000,
    0x080A0000,
    0x080C0000,
    0x080E0000,
]

PAGE_SIZE = {'stm32f1': 1024, 'stm32f3': 2048}
PAUSE = {'stm32f1': 0.06, 'stm32f3': 0.15}
STM32F4_BLOCK_SIZE = 16384
STM32F4_APPLICATION_START = 0x08008000


def main():
  parser = argparse.ArgumentParser(
      description='Encode a firmware .bin into a QPSK audio bootloader .wav '
                  '(Mutable Instruments stm_audio_bootloader format).')
  parser.add_argument('bin_file', help='firmware .bin file to encode')
  parser.add_argument(
      '-k', '--scramble', action='store_true', default=False,
      help='randomize data stream')
  parser.add_argument(
      '-s', '--sample_rate', type=int, default=48000,
      help='sample rate in Hz (default: 48000)')
  parser.add_argument(
      '-c', '--carrier_frequency', type=int, default=6000,
      help='carrier frequency in Hz (default: 6000)')
  parser.add_argument(
      '-b', '--baud_rate', type=int, default=12000,
      help='baudrate in bps (default: 12000)')
  parser.add_argument(
      '-p', '--packet_size', type=int, default=256,
      help='packet size in bytes (default: 256)')
  parser.add_argument(
      '-o', '--output_file', default=None, metavar='FILE',
      help='write output to FILE (default: <bin_file> with .wav suffix)')
  parser.add_argument(
      '-t', '--target', default='stm32f1',
      choices=['stm32f1', 'stm32f3', 'stm32f4'],
      help='set page size and erase time for TARGET (default: stm32f1)')

  args = parser.parse_args()

  with open(args.bin_file, 'rb') as f:
    data = f.read()

  output_file = args.output_file
  if not output_file:
    if '.bin' in args.bin_file:
      output_file = args.bin_file.replace('.bin', '.wav')
    else:
      output_file = args.bin_file + '.wav'

  encoder = QpskEncoder(
      args.sample_rate,
      args.carrier_frequency,
      args.baud_rate,
      args.packet_size,
      args.scramble)
  writer = audio_stream_writer.AudioStreamWriter(
      output_file,
      args.sample_rate,
      16,
      1)

  # INTRO
  for block in encoder.code_intro():
    writer.append(block)

  blank_duration = 1.0
  if args.target in PAGE_SIZE:
    for block in encoder.code(data, PAGE_SIZE[args.target], PAUSE[args.target]):
      if len(block):
        writer.append(block)
  elif args.target == 'stm32f4':
    for x in range(0, len(data), STM32F4_BLOCK_SIZE):
      address = STM32F4_APPLICATION_START + x
      block = data[x:x + STM32F4_BLOCK_SIZE]
      pause = 3.5 if address in STM32F4_SECTOR_BASE_ADDRESS else 0.2
      for audio_block in encoder.code(block, STM32F4_BLOCK_SIZE, pause):
        if len(audio_block):
          writer.append(audio_block)
    blank_duration = 5.0
  for block in encoder.code_outro(blank_duration):
    writer.append(block)

  print('wrote %s (%d bytes of firmware)' % (output_file, len(data)))
  return 0


if __name__ == '__main__':
  sys.exit(main())

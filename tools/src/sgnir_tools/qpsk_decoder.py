# QPSK decoder for stm_audio_bootloader firmware .wav files.
#
# Counterpart to qpsk_encoder.py, used to verify the encoder port: decode an
# official firmware .wav back to the .bin payload (CRC-checked per packet),
# re-encode it, and compare byte-for-byte.
#
# This is NOT a general-purpose demodulator: it assumes a synthetically
# generated file (exact sample alignment, continuous carrier phase from sample
# 0), which holds for every .wav the encoder produces. The C++ reference is
# stm_audio_bootloader/qpsk/{demodulator,packet_decoder}.cc.

import argparse
import struct
import sys
import wave
import zlib

import numpy

PREAMBLE = bytes([0] * 8 + [0x99] * 4 + [0xcc] * 4)


def read_mono_wav(path):
  with wave.open(path, 'rb') as w:
    if w.getnchannels() != 1 or w.getsampwidth() != 2:
      raise SystemExit('expected a 16-bit mono wav, got %d ch / %d bytes' %
                       (w.getnchannels(), w.getsampwidth()))
    sample_rate = w.getframerate()
    raw = w.readframes(w.getnframes())
  samples = numpy.frombuffer(raw, dtype='<i2').astype(numpy.float64)
  return sample_rate, samples / 32767.0


def demodulate(samples, sample_rate, carrier_frequency, baud_rate):
  """Return the symbol stream (0..3 per symbol) of the whole file."""
  samples_per_symbol = sample_rate // (baud_rate // 2)
  num_symbols = len(samples) // samples_per_symbol
  samples = samples[:num_symbols * samples_per_symbol]

  # The encoder modulates with a phase derived from the absolute sample index,
  # so the matched filter can be built once for the whole file.
  t = numpy.arange(len(samples), dtype=numpy.float64) / sample_rate
  phase = 2 * numpy.pi * carrier_frequency * t
  sin_corr = (samples * numpy.sin(phase)).reshape(num_symbols, -1).sum(axis=1)
  cos_corr = (samples * numpy.cos(phase)).reshape(num_symbols, -1).sum(axis=1)

  q_bits = (sin_corr > 0).astype(int)   # symbol % 2
  i_bits = (cos_corr > 0).astype(int)   # symbol // 2
  return (i_bits << 1) | q_bits


def symbols_to_bytes(symbols):
  symbols = symbols[:len(symbols) // 4 * 4].reshape(-1, 4)
  packed = ((symbols[:, 0] << 6) | (symbols[:, 1] << 4) |
            (symbols[:, 2] << 2) | symbols[:, 3])
  return packed.astype(numpy.uint8).tobytes()


def extract_packets(byte_stream, packet_size):
  """Scan for preamble-framed packets, CRC-check, return the payload."""
  payload = bytearray()
  bad_crc = 0
  pos = 0
  while True:
    start = byte_stream.find(PREAMBLE, pos)
    if start < 0:
      break
    data_start = start + len(PREAMBLE)
    data_end = data_start + packet_size
    if data_end + 4 > len(byte_stream):
      break
    data = byte_stream[data_start:data_end]
    crc = struct.unpack('>L', byte_stream[data_end:data_end + 4])[0]
    if crc == (zlib.crc32(data) & 0xffffffff):
      payload += data
      pos = data_end + 4
    else:
      bad_crc += 1
      pos = start + 1
  return bytes(payload), bad_crc


def main():
  parser = argparse.ArgumentParser(
      description='Decode a QPSK audio bootloader .wav back to the firmware '
                  '.bin payload (CRC-checked).')
  parser.add_argument('wav_file', help='firmware .wav to decode')
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
      help='write payload to FILE (default: <wav_file> with .bin suffix)')

  args = parser.parse_args()

  sample_rate, samples = read_mono_wav(args.wav_file)
  symbols = demodulate(
      samples, sample_rate, args.carrier_frequency, args.baud_rate)
  byte_stream = symbols_to_bytes(symbols)
  payload, bad_crc = extract_packets(byte_stream, args.packet_size)

  if not payload:
    print('no valid packets found', file=sys.stderr)
    return 1

  output_file = args.output_file
  if not output_file:
    if '.wav' in args.wav_file:
      output_file = args.wav_file.replace('.wav', '.bin')
    else:
      output_file = args.wav_file + '.bin'

  with open(output_file, 'wb') as f:
    f.write(payload)

  print('wrote %s: %d bytes (%d packets, %d CRC failures)' %
        (output_file, len(payload), len(payload) // args.packet_size, bad_crc))
  return 1 if bad_crc else 0


if __name__ == '__main__':
  sys.exit(main())

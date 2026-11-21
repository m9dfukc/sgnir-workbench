# /// script
# requires-python = ">=3.10"
# dependencies = ["unicorn>=2.1", "capstone>=5.0"]
# ///
"""Trace-driven cycle model for the Schliere DSP on the Cortex-M4F.

Builds rings/dsp/schliere.cc with the firmware's compiler flags into a small
bare-metal ELF, runs Schliere::Process in the Unicorn emulator and adds up
the cycle counts of the Cortex-M4 Technical Reference Manual (tables 3-1 and
7-1) for every executed instruction.

The model is nominal: zero flash wait states, pipeline refill of 2 cycles per
taken branch. See README.md for how it compares with readings on the module.

    uv run cycle_model.py build --source <firmware root> --out <dir> [-- flags]
    uv run cycle_model.py run <elf> [--scenario noon] [--oversampled] [--lines]
    uv run cycle_model.py table [--ref <elf>] <elf> [<elf> ...]
"""

import argparse
import collections
import os
import struct
import subprocess
import sys

BLOCK_SIZE = 24
BLOCK_BUDGET_CYCLES = 84000  # 24 samples at 48 kHz, 168 MHz
PIPELINE_REFILL = 2  # cycles after a taken branch; the TRM says 1 to 3

ARCH_FLAGS = [
    '-mcpu=cortex-m4', '-mthumb', '-mfloat-abi=hard', '-mfpu=fpv4-sp-d16',
    '-mthumb-interwork', '-funroll-loops',
]
# CCFLAGS and CPPFLAGS of stmlib/makefile.inc plus the C++ dialect override
# from the firmware README.
COMPILE_FLAGS = [
    '-g', '-O2', '-Wall', '-Werror', '-Wno-unused-local-typedefs',
    '-Wno-strict-aliasing', '-fasm', '-finline',
    '-finline-functions-called-once', '-fdata-sections', '-ffunction-sections',
    '-fshort-enums', '-fno-move-loop-invariants', '-fno-unwind-tables',
    '-DGCC_ARMCM4', '-DSTM32F40_41xxx', '-DSTM32F4XX', '-DARM_MATH_CM4',
    '-D__FPU_PRESENT', '-DF_CPU=168000000L', '-DUSE_STDPERIPH_DRIVER',
    '-fno-exceptions', '-fno-rtti', '-std=gnu++11', '-Wno-deprecated',
]

FLASH_BASE = 0x08000000
RAM_BASE = 0x20000000
TEXT_ADDRESS = 0x08010000
STACK_TOP = 0x2001f000
RETURN_ADDRESS = 0x080ff000


def tool(name):
    prefix = os.environ.get('TOOLCHAIN_PATH', '')
    if prefix:
        return os.path.join(prefix, 'bin', 'arm-none-eabi-' + name)
    return 'arm-none-eabi-' + name


# ----------------------------------------------------------------------------
# Build
# ----------------------------------------------------------------------------

def build(source, out, extra_flags):
    """Compiles schliere.cc and the harness, links them. Returns the ELF path."""
    here = os.path.dirname(os.path.abspath(__file__))
    os.makedirs(out, exist_ok=True)
    flags = COMPILE_FLAGS + ARCH_FLAGS + ['-I' + source]
    objects = []
    for src, extra in (
            (os.path.join(source, 'rings', 'dsp', 'schliere.cc'), extra_flags),
            (os.path.join(here, 'harness.cc'), [])):
        obj = os.path.join(out, os.path.basename(src)[:-3] + '.o')
        subprocess.run([tool('g++'), '-c'] + flags + extra + [src, '-o', obj],
                       check=True)
        objects.append(obj)
    elf = os.path.join(out, 'schliere_harness.elf')
    subprocess.run(
        [tool('g++')] + ARCH_FLAGS + [
            '-nostartfiles', '-Wl,-e,h_init',
            '-Wl,-Ttext=0x%08x' % TEXT_ADDRESS, '-Wl,-Tdata=0x%08x' % RAM_BASE,
            '-Wl,--no-warn-rwx-segments', '-o', elf] + objects +
        ['-lm', '-lc', '-lgcc'], check=True)
    return elf


def symbols(elf):
    out = subprocess.run([tool('nm'), '-S', '-C', elf], capture_output=True,
                         text=True, check=True).stdout
    table = {}
    for line in out.splitlines():
        parts = line.split(None, 3)
        if len(parts) == 4:
            table[parts[3]] = (int(parts[0], 16), int(parts[1], 16))
        elif len(parts) == 3:
            table[parts[2]] = (int(parts[0], 16), 0)
    return table


# ----------------------------------------------------------------------------
# Cycle costs (Cortex-M4 TRM, tables 3-1 and 7-1)
# ----------------------------------------------------------------------------

FUSED = ('vfma', 'vfms', 'vfnma', 'vfnms', 'vmla', 'vmls', 'vnmla', 'vnmls')


def register_count(operands):
    if '{' in operands:
        operands = operands[operands.find('{') + 1:operands.find('}')]
    count = 0
    for part in operands.split(','):
        part = part.strip()
        if '-' in part:
            first, last = part.split('-')
            number = lambda r: int(''.join(c for c in r if c.isdigit()))
            count += number(last) - number(first) + 1
        elif part:
            count += 1
    return count


def base_cost(mnemonic, operands):
    """Cycles of one executed instruction, without branch effects."""
    m = mnemonic.split('.')[0]
    if m.startswith('v'):
        if m.startswith(FUSED):
            return 3
        if m.startswith(('vdiv', 'vsqrt')):
            return 14
        if m.startswith(('vldr', 'vstr')):
            return 2
        if m.startswith(('vpush', 'vpop', 'vldm', 'vstm')):
            return 1 + register_count(operands)
        return 1
    if m.startswith(('push', 'pop', 'ldm', 'stm')):
        return 1 + register_count(operands)
    if m.startswith(('ldr', 'str')):
        return 2
    if m.startswith(('udiv', 'sdiv')):
        return 6  # 2 to 12
    return 1


# ----------------------------------------------------------------------------
# Emulation
# ----------------------------------------------------------------------------

class Model(object):

    def __init__(self, elf):
        import capstone
        import unicorn
        from unicorn import arm_const
        self.arm_const = arm_const
        self.elf = elf
        self.symbols = symbols(elf)
        uc = unicorn.Uc(unicorn.UC_ARCH_ARM,
                        unicorn.UC_MODE_THUMB | unicorn.UC_MODE_MCLASS)
        uc.ctl_set_cpu_model(arm_const.UC_CPU_ARM_CORTEX_M4)
        uc.mem_map(FLASH_BASE, 0x100000)
        uc.mem_map(RAM_BASE, 0x20000)
        self._load(uc, elf)
        # FPU on.
        uc.reg_write(arm_const.UC_ARM_REG_C1_C0_2,
                     uc.reg_read(arm_const.UC_ARM_REG_C1_C0_2) | (0xf << 20))
        uc.reg_write(arm_const.UC_ARM_REG_FPEXC, 0x40000000)
        uc.mem_write(RETURN_ADDRESS, b'\xfe\xe7')  # b .
        uc.hook_add(unicorn.UC_HOOK_CODE, self._on_instruction)
        self.uc = uc
        self.disassembler = capstone.Cs(
            capstone.CS_ARCH_ARM,
            capstone.CS_MODE_THUMB | capstone.CS_MODE_MCLASS)
        self.decoded = {}
        self.trace = []
        self.recording = False

    @staticmethod
    def _load(uc, elf):
        data = open(elf, 'rb').read()
        phoff, = struct.unpack_from('<I', data, 28)
        phentsize, phnum = struct.unpack_from('<HH', data, 42)
        for i in range(phnum):
            p_type, offset, vaddr, _, filesz = struct.unpack_from(
                '<5I', data, phoff + i * phentsize)
            if p_type == 1 and filesz:
                uc.mem_write(vaddr, data[offset:offset + filesz])

    def _on_instruction(self, uc, address, size, user_data):
        if self.recording:
            self.trace.append(address)

    def address(self, name):
        return self.symbols[name][0]

    def decode(self, address):
        entry = self.decoded.get(address)
        if entry is None:
            code = bytes(self.uc.mem_read(address, 4))
            insn = next(self.disassembler.disasm(code, address, 1))
            entry = (insn.size, base_cost(insn.mnemonic, insn.op_str),
                     insn.mnemonic, insn.op_str)
            self.decoded[address] = entry
        return entry

    def call(self, name, r0=0, record=False):
        c = self.arm_const
        self.uc.reg_write(c.UC_ARM_REG_SP, STACK_TOP)
        self.uc.reg_write(c.UC_ARM_REG_LR, RETURN_ADDRESS | 1)
        self.uc.reg_write(c.UC_ARM_REG_R0, r0)
        self.trace = []
        self.recording = record
        self.uc.emu_start(self.address(name) | 1, RETURN_ADDRESS)
        self.recording = False
        return self.trace

    def cost(self, trace):
        """Returns (total cycles, cycles per address)."""
        per_address = collections.Counter()
        total = 0
        previous_load_store = False
        for i, address in enumerate(trace):
            size, cycles, mnemonic, _ = self.decode(address)
            m = mnemonic.split('.')[0]
            load_store = m.startswith(('ldr', 'str'))
            if load_store and previous_load_store:
                cycles = 1  # neighbouring loads/stores pipeline
            previous_load_store = load_store
            last = i + 1 == len(trace)
            if last or trace[i + 1] != address + size:
                cycles += PIPELINE_REFILL
            per_address[address] += cycles
            total += cycles
        return total, per_address

    def write_parameters(self, p):
        # Layout of rings::SchliereParameters (-fshort-enums).
        packed = struct.pack(
            '<9fBB', p['frequency'], p.get('octaves', 0.0), p['resonance'],
            p['tap'], p.get('tap_cv', 0.0), p['feedback'],
            p.get('feedback_cv', 0.0), p['spread'], p['mix'],
            1 if p.get('excite') else 0, p.get('excite_type', 0))
        self.uc.mem_write(self.address('params'), packed)

    def write_input(self, samples):
        self.uc.mem_write(self.address('in_'),
                          struct.pack('<%df' % BLOCK_SIZE, *samples))

    def read_output(self):
        fmt = '<%df' % BLOCK_SIZE
        size = 4 * BLOCK_SIZE
        return (
            struct.unpack(fmt, self.uc.mem_read(self.address('out_l'), size)) +
            struct.unpack(fmt, self.uc.mem_read(self.address('out_r'), size)))


# ----------------------------------------------------------------------------
# Scenarios
# ----------------------------------------------------------------------------

NOON = dict(frequency=0.5, resonance=0.5, tap=0.5, feedback=0.5, spread=0.5,
            mix=1.0)

# name: (parameters, moving, input amplitude)
SCENARIOS = {
    # All knobs at noon: the setting of the readings on the module.
    'noon': (NOON, False, 0.1),
    # Tap between two detents, nothing moves.
    'tap-between': (dict(NOON, tap=0.58), False, 0.1),
    # Loud input, feedback into the limiter, high resonance; tap, spread and
    # frequency move, a noise burst every 40 blocks.
    'worst': (dict(NOON, tap=0.58, spread=0.3, feedback=1.0, feedback_cv=0.2,
                   mix=0.75, resonance=0.9), True, 0.9),
}


def noise(seed):
    state = seed
    while True:
        state = (state * 1664525 + 1013904223) & 0xffffffff
        yield (state >> 8) / 8388608.0 - 1.0


def simulate(elf, scenario, oversampled, blocks, warm_up, keep_output=False):
    model = Model(elf)
    base, moving, amplitude = SCENARIOS[scenario]
    model.call('h_init', r0=1 if oversampled else 0)
    source = noise(12345)
    peak = 0
    total = 0
    per_address = collections.Counter()
    outputs = []
    for block in range(warm_up + blocks):
        p = dict(base)
        if moving:
            phase = (block % 400) / 400.0
            p['tap'] = 0.05 + 0.9 * (2 * phase if phase < 0.5 else 2 - 2 * phase)
            p['spread'] = 0.25 + 0.5 * ((block % 97) / 97.0)
            p['frequency'] = 0.3 + 0.4 * ((block % 53) / 53.0)
            p['excite'] = block % 40 == 0
            p['excite_type'] = 1
        model.write_parameters(p)
        model.write_input([amplitude * next(source) for _ in range(BLOCK_SIZE)])
        measured = block >= warm_up
        trace = model.call('h_block', record=measured)
        if keep_output:
            outputs.append(model.read_output())
        if measured:
            cycles, per = model.cost(trace)
            peak = max(peak, cycles)
            total += cycles
            per_address.update(per)
    return dict(model=model, peak=peak, mean=total / max(blocks, 1),
                per_address=per_address, blocks=blocks, outputs=outputs)


def percent(cycles):
    return 100.0 * cycles / BLOCK_BUDGET_CYCLES


def attribute(result, by_line):
    """Cycles per block by function, or by source line of Schliere::Process."""
    addresses = sorted(result['per_address'])
    out = subprocess.run(
        [tool('addr2line'), '-e', result['model'].elf, '-a', '-i', '-f', '-C'] +
        [hex(a) for a in addresses],
        capture_output=True, text=True, check=True).stdout.splitlines()
    chains = {}
    current = None
    for line in out:
        if line.startswith('0x'):
            current = int(line, 16)
            chains[current] = []
        else:
            chains[current].append(line)
    totals = collections.Counter()
    for a in addresses:
        chain = chains.get(a, [])
        # addr2line -i lists the inline chain innermost first; the last pair
        # is the function the code was emitted in.
        function = chain[-2] if len(chain) >= 2 else '?'
        location = chain[-1] if chain else '?'
        name = function.split('(')[0].replace('rings::', '')
        if by_line:
            if name != 'Schliere::Process':
                continue
            name = 'schliere.cc:' + location.rsplit(':', 1)[-1].split(' ')[0]
        totals[name] += result['per_address'][a]
    return [(k, v / float(result['blocks'])) for k, v in totals.most_common()]


# ----------------------------------------------------------------------------
# Commands
# ----------------------------------------------------------------------------

def command_build(args):
    elf = build(args.source, args.out, args.flags)
    size = symbols(elf)
    process = [v for k, v in size.items() if k.startswith(
        'rings::Schliere::Process(')]
    print('%s (Schliere::Process: %d bytes)' % (elf, process[0][1]))


def command_run(args):
    result = simulate(args.elf, args.scenario, args.oversampled, args.blocks,
                      args.warm_up)
    print('%s, 96 kHz path %s: mean %.0f cycles/block (%.1f %%), '
          'peak %.0f (%.1f %%)' % (
              args.scenario, 'on' if args.oversampled else 'off',
              result['mean'], percent(result['mean']),
              result['peak'], percent(result['peak'])))
    for name, cycles in attribute(result, args.lines)[:args.top]:
        print('  %-40s %8.0f  %5.1f %%' % (name, cycles, percent(cycles)))


def command_table(args):
    cases = (('noon', False), ('noon', True), ('worst', False), ('worst', True))
    reference = None
    if args.ref:
        reference = simulate(args.ref, 'worst', True, 0, args.compare_blocks,
                             keep_output=True)['outputs']
    print('%-34s %8s %8s %8s %8s%s' % (
        'build', 'noon 48', 'noon 96', 'worst 48', 'worst 96',
        '   max |diff| vs. ref' if args.ref else ''))
    for elf in args.elf:
        row = []
        for scenario, oversampled in cases:
            blocks = 400 if scenario == 'worst' else 30
            result = simulate(elf, scenario, oversampled, blocks, args.warm_up)
            row.append(percent(result['peak']))
        difference = ''
        if reference is not None:
            outputs = simulate(elf, 'worst', True, 0, args.compare_blocks,
                               keep_output=True)['outputs']
            worst = max(abs(a - b) for x, y in zip(reference, outputs)
                        for a, b in zip(x, y))
            difference = '   %.3g' % worst
        print('%-34s %7.1f%% %7.1f%% %7.1f%% %7.1f%%%s' % (
            (elf,) + tuple(row) + (difference,)))


def main():
    parser = argparse.ArgumentParser(
        description='Cycle model for the Schliere DSP on the Cortex-M4F '
                    '(peak load in percent of the 84 000-cycle block budget).')
    sub = parser.add_subparsers(dest='command', required=True)

    b = sub.add_parser(
        'build', help='build the harness ELF from a firmware source tree')
    b.add_argument('--source', required=True,
                   help='firmware root (contains rings/ and stmlib/)')
    b.add_argument('--out', required=True, help='output directory')
    b.add_argument('flags', nargs='*',
                   help='extra compiler flags for schliere.cc, after "--"')
    b.set_defaults(function=command_build)

    r = sub.add_parser('run', help='model one build, with a breakdown')
    r.add_argument('elf')
    r.add_argument('--scenario', choices=sorted(SCENARIOS), default='noon')
    r.add_argument('--oversampled', action='store_true',
                   help='96 kHz feedback path on')
    r.add_argument('--lines', action='store_true',
                   help='break Schliere::Process down by source line '
                        'instead of by function')
    r.add_argument('--top', type=int, default=20)
    r.add_argument('--blocks', type=int, default=60)
    r.add_argument('--warm-up', type=int, default=400)
    r.set_defaults(function=command_run)

    t = sub.add_parser('table', help='compare the peak load of several builds')
    t.add_argument('elf', nargs='+')
    t.add_argument('--ref', help='also report the largest output difference '
                                 'against this build (worst scenario, 96 kHz)')
    t.add_argument('--warm-up', type=int, default=400)
    t.add_argument('--compare-blocks', type=int, default=600)
    t.set_defaults(function=command_table)

    args = parser.parse_args()
    args.function(args)


if __name__ == '__main__':
    sys.exit(main())

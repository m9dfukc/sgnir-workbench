# Cheap SWD programmer for Mutable Instruments Rings (STM32F405RGT6) — 2026-11-20

Question: which cheap (≤ ~20 €) SWD probe to buy from amazon.de / German distributors as a safety net
for flashing/recovering a MI Rings, next to the primary audio-bootloader update path.
Host: macOS (Apple Silicon), tools OpenOCD / `st-flash` / probe-rs / STM32CubeProgrammer.

Markers: [sourced] = primary source checked today, [hypothesis] = inference or not verifiable right now.

---

## TL;DR recommendation

- **Buy: a ~6–10 € "ST-Link V2" clone (aluminium USB-stick style, e.g. AZDelivery or generic) from amazon.de**
  **plus a "2x5 1.27 mm (Cortex Debug) to 2.54 mm adapter board + 1.27 mm IDC ribbon" set (~5–10 €, amazon.de/eBay/AliExpress).**
  ST-Link V2 is literally the default programmer in Mutable's own build system (`PGM_INTERFACE ?= stlink-v2`
  in `stmlib/makefile.inc`) [sourced], and it is fully supported by OpenOCD, `st-flash` (Homebrew `stlink`)
  and probe-rs on macOS/Apple Silicon [sourced for OpenOCD/probe-rs support of ST-Link in general; the
  Apple-Silicon part is routine Homebrew practice, [hypothesis] only in the sense that I didn't re-test it today].
- **Fallback: Raspberry Pi Debug Probe, 12,20 € at Berrybase** [sourced] — official product, open firmware,
  CMSIS-DAP, works with OpenOCD and probe-rs. Downsides: only SWCLK/GND/SWDIO on its debug port (no NRST,
  no VTref) and you still need the same 1.27 mm adapter plus jumper wires.
- **Not worth it here: STLINK-V3MINIE.** Nice and official, but 31,60 € at Reichelt [sourced] (cheap only
  via Mouser, ~11–13 € [hypothesis, Mouser/ST pages timed out today]) and its STDC14 connector needs an
  extra STDC14→Cortex-10 adapter for Rings' header.
- The big official ST-LINK/V2 is 49,90 € at Berrybase [sourced] — exactly the overpriced option to avoid.

Whatever the probe: you additionally need to bridge from the probe to Rings' **2x5, 1.27 mm pitch**
mini-JTAG header, so the 1.27 mm adapter/ribbon is the one non-negotiable accessory.

---

## Rings debug header — facts from the schematic

Source: Rings v30 schematic PDF, sheet 1 and 5 (pichenettes/mutable-instruments-documentation,
`docs/modules/rings/downloads/rings_v30.pdf`) and the Eagle board file `rings_v30.brd`
(pichenettes/eurorack). All [sourced].

- MCU: **STM32F405RGT6** (Cortex-M4F, 1 MB flash) [sourced, sheet 1].
- JP1 = part `M05X2MINIJTAG`, board package **`2X5-1.27`** → 2x5 pins, **1.27 mm pitch**, labeled "JTAG"
  on the board [sourced, .brd strings]. This is physically the standard ARM Cortex Debug footprint.
- **JP1 pinout (read off the schematic, standard ARM Cortex 10-pin debug layout)** [sourced]:

  | Pin | Signal | Pin | Signal |
  |---|---|---|---|
  | 1 | **+3V3** (VTref) | 2 | **JTMS = SWDIO** |
  | 3 | GND | 4 | **JTCK = SWCLK** |
  | 5 | GND | 6 | JTDO (= SWO) |
  | 7 | n.c. | 8 | JTDI |
  | 9 | GND | 10 | **RESET (NRST)** |

  For SWD you only need pins 1 (reference only), 2, 4, 10 (optional) and any GND.
- JP2 = 6-pin **"Serial / System Bootloader"** header: pin 1 GND, pin 4 RX, pin 5 TX (3.3 V UART)
  [sourced, sheet 1]. Plus a **RESET** switch (SW3) and a **BOOT_FLASH/SYSBOOT** switch (SW4) on the board
  [sourced, sheet 1] — MI's docs describe entering the ROM serial bootloader with them [sourced,
  `docs/modules/rings/open_source.md`]. This is a second, SWD-free recovery path with any 3.3 V USB-UART dongle.
- Power: Rings' 3.3 V digital rail comes from an LM1117-3.3 fed from the +12 V Eurorack rail [sourced,
  sheet 5]. → **The module must be powered from the Eurorack PSU while flashing**; the probe cannot
  sensibly power the whole module, and pin 1 is meant as a voltage *reference*, not an input
  [sourced for the rail topology; "don't power from the probe" is standard practice, [hypothesis]].
- MI's official docs for Rings recommend JTAG + OpenOCD as *the* programming route ("the last — and
  recommended — solution"), naming the Olimex ARM-USB-OCD-H and an Embedded Artists adapter kit for the
  "mini-JTAG" connector [sourced, `open_source.md`]. The Embedded Artists kit is a standard 20-pin ARM
  JTAG → Cortex-10 (FTSH-105) adapter, consistent with the standard pinout above.
- Whether the header is factory-populated on a production Rings: v30 is the production schematic and the
  connector is in the BOM/board, and community flashing guides assume it is present, but I could not
  verify a photo today → **check your unit; expect a bare 10-pin 1.27 mm male header on the PCB back**
  [hypothesis].
- Firmware layout [sourced, `stmlib/makefile.inc` + `rings/makefile` + MI tech note `stm32_miscellania.md`]:
  bootloader at **0x08000000**, application at **0x08008000** (`APPLICATION_LARGE=TRUE` for Rings),
  the official upload target is `upload_combo_jtag_erase_first` = combined bootloader+app image written
  at 0x08000000.
- Read protection: nothing in MI's makefiles sets RDP, the standard workflow dumps flash freely
  (`image_jtag_1M` target) [sourced]; a factory Rings almost certainly has **RDP level 0** (no read
  protection) [hypothesis]. If a unit ever showed RDP1, `stm32f4x unlock 0` recovers it at the cost of a
  mass erase — acceptable since you'd re-flash anyway.

---

## Option comparison

| Option | Typical price (DE) | Probe-side connector | macOS/OpenOCD | Pros | Risks / gotchas |
|---|---|---|---|---|---|
| **ST-Link V2 clone** (metal USB stick, amazon.de: AZDelivery, HALJIA, generic "ST-Link V2 STM8/STM32") | ~6–10 € [hypothesis, typical amazon.de listings] | 2x5 0.1" male, jumper-wire friendly | OpenOCD `interface/stlink.cfg` (hla), `st-flash`, probe-rs, STM32CubeProgrammer — all fine on Apple Silicon [sourced for tool support in general] | Cheapest; the de-facto MI community standard — `stlink-v2` is the default in `stmlib/makefile.inc` [sourced]; SWD for F4 fully supported | Fake/relabeled MCUs; **the "3.3V" pins are a regulator output, not VTref — leave them unconnected on a rack-powered module** [hypothesis/community]; some batches have wrong silkscreen pinout on the case — verify pin names against the case printing and a continuity check [hypothesis/community reports]; **don't run ST's firmware-upgrade tools on a clone** (anti-clone bricking) [hypothesis/community reports]; no SWO/VTref sensing |
| **Raspberry Pi Debug Probe** | **12,20 € Berrybase** [sourced], also on amazon.de (~15–20 € [hypothesis]) | 3-pin JST-SH "D" port (SWCLK/GND/SWDIO) + supplied JST-SH→0.1" jumper cables [sourced, RPi docs] | CMSIS-DAP standard, explicitly works with OpenOCD [sourced, RPi docs]; probe-rs supports CMSIS-DAP [sourced] | Official, open-source firmware, well documented, UART port as bonus (usable for Rings' JP2 serial bootloader!) | Only 3 wires: **no NRST** (OpenOCD must reset via SWD `SYSRESETREQ` — fine for stock MI firmware [hypothesis]) and **no VTref**; 3.3 V targets only (Rings is 3.3 V — fine); still needs the 1.27 mm adapter |
| **STLINK-V3MINIE** | **31,60 € Reichelt** [sourced]; ~11–13 € Mouser [hypothesis, page unreachable today] | STDC14 (1.27 mm, MIPI-10 compatible middle pins), 14-pin ribbon included [hypothesis — ST page timed out, from memory of the data brief] | Full ST toolchain + OpenOCD, fast | Official, modern, USB-C | Needs an STDC14→Cortex-10 adapter (extra ~5 €, not common on amazon.de); **requires target VTref on pin 1** (Rings provides +3V3 there — OK); over budget at Reichelt |
| Official ST-LINK/V2 | 49,90 € Berrybase [sourced] | 20-pin 0.1" ARM JTAG | fine | — | Price; would also need 20→10 1.27 mm adapter (Olimex ARM-JTAG-20-10, ~6 € [hypothesis]) |
| Other CMSIS-DAP/DAPLink dongles (e.g. WCH-Link?, "DAPLink" sticks) | 5–15 € | varies | OpenOCD cmsis-dap | — | Quality/firmware lottery, no advantage over the two picks above [hypothesis] |

**Needed in every case:** an adapter from the probe to the 2x5 **1.27 mm** header. Easiest buy on
amazon.de/eBay: a "JTAG SWD adapter" set containing a small PCB (20-pin 2.54 mm + 10-pin 2.54 mm +
10-pin 1.27 mm footprints) **with a 1.27 mm 10-pin IDC ribbon cable**, usually marketed "for J-Link /
ST-Link", ~5–10 € [hypothesis, typical listings]. The Olimex ARM-JTAG-20-10 (~6 €, Olimex/Mouser) is the
branded equivalent but mates a 20-pin 2.54 mm plug, which the stick-style clone doesn't have — the generic
breakout + female-female jumper wires is the better match for the clone.

---

## Community usage

- Mutable's own build system defaults to `PGM_INTERFACE ?= stlink-v2` with OpenOCD, with Olimex FTDI
  probes as the commented-out alternative [sourced, `stmlib/makefile.inc`]. The official dev-environment
  README shows `export PGM_INTERFACE=stlink-v2; export PGM_INTERFACE_TYPE=hla` as the supported ST-Link
  path [sourced, pichenettes/mutable-dev-environment README].
- DIY clone projects (e.g. TOILmodular/Rings) keep the original MI JTAG wiring and flash
  bootloader+firmware hex files via programmer or UART bootloader [sourced, repo README].
- `forum.mutable-instruments.net` no longer resolves (DNS dead as of 2026-11-20) [sourced, fetch
  attempt], so individual forum threads about ST-Link clones couldn't be re-checked; the "everyone uses
  5-€ ST-Link clones for MI modules" claim is community lore consistent with the sources above
  [hypothesis].

---

## Wiring and flash procedure sketch (ST-Link V2 clone)

```
Rings JP1 (2x5, 1.27 mm, pin 1 = +3V3):      ST-Link V2 clone (2.54 mm header):
  1 +3V3   2 SWDIO   ───────────────────────  SWDIO
  3 GND    4 SWCLK   ───────────────────────  SWCLK
  5 GND    6 SWO     (n.c.)
  7 n.c.   8 TDI     (n.c.)
  9 GND   10 NRST    ───────────────────────  RST (optional)
  any GND ──────────────────────────────────  GND
  pin 1 +3V3: leave UNCONNECTED on a clone (its 3.3V pin is a supply output, not VTref)
```

1. **Power Rings from the Eurorack PSU** (bench: ±12 V; 3V3 is generated on-board) [sourced, sheet 5].
   Connect GND first, then SWDIO/SWCLK (/NRST).
2. **Backup first** (1 MB full flash dump):
   `st-flash read rings_backup_$(date +%Y%m%d).bin 0x8000000 0x100000`
   or OpenOCD: `openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c "init; reset halt; dump_image rings_backup.bin 0x08000000 0x100000; shutdown"`
3. **Flash** (combined bootloader+app image, as MI's `upload_combo_jtag_erase_first` does):
   `openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c "program rings_combo.bin 0x08000000 verify reset exit"`
   - Custom app only, keeping the stock bootloader: program at **0x08008000** instead
     (`APPLICATION_LARGE`) [sourced, makefile.inc].
   - Via MI makefiles: `PGM_INTERFACE=stlink-v2 PGM_INTERFACE_TYPE=hla make -f rings/makefile upload_combo_jtag_erase_first`
     — note `stmlib` references the old OpenOCD config name `stlink-v2.cfg`; OpenOCD ≥ 0.11 renamed it to
     `stlink.cfg`, so either set `PGM_INTERFACE=stlink` or symlink the cfg [sourced for the makefile side;
     OpenOCD rename from release notes, [hypothesis] not re-verified today].
4. Gotchas: expect **no read protection** on a factory Rings [hypothesis]; if OpenOCD reports locked
   flash, `stm32f4x unlock 0` + mass erase recovers the chip. Verify the clone's real pinout with a
   multimeter before first use [hypothesis/community]. The audio bootloader remains usable afterwards as
   long as the bootloader region 0x08000000–0x08007FFF is intact.

Extra safety net for ~8 €: any 3.3 V USB-UART (CP2102/FT232) on JP2 + the RESET/SYSBOOT buttons reaches
the STM32 ROM serial bootloader with zero SWD hardware [sourced, open_source.md + sheet 1].

---

## Sources (all fetched 2026-11-20)

- https://raw.githubusercontent.com/pichenettes/eurorack/master/rings/makefile (APPLICATION_LARGE, `upload_combo_jtag_erase_first`, STM32F4, wav target)
- https://raw.githubusercontent.com/pichenettes/stmlib/master/makefile.inc (`PGM_INTERFACE ?= stlink-v2`, OpenOCD targets, base addresses 0x08000000/0x08008000, jtag upload/dump targets)
- https://github.com/pichenettes/eurorack/tree/master/rings/hardware_design/pcb (`rings_v30.sch` / `rings_v30.brd`, Eagle; board package `2X5-1.27`, nets JTMS/JTCK/JTDI/NRST)
- https://raw.githubusercontent.com/pichenettes/mutable-instruments-documentation/main/docs/modules/rings/open_source.md (JTAG recommended, Olimex + mini-JTAG adapter, serial bootloader procedure, FTDI 3.3 V)
- https://raw.githubusercontent.com/pichenettes/mutable-instruments-documentation/main/docs/modules/rings/downloads/rings_v30.pdf (JP1 pinout, JP2, RESET/BOOT_FLASH switches, LM1117-3.3 supply)
- https://raw.githubusercontent.com/pichenettes/mutable-instruments-documentation/main/docs/tech_notes/stm32_miscellania.md (bootloader/app flash layout)
- https://github.com/pichenettes/mutable-dev-environment (README: `PGM_INTERFACE=stlink-v2`, `PGM_INTERFACE_TYPE=hla`)
- https://www.raspberrypi.com/documentation/microcontrollers/debug-probe.html (CMSIS-DAP, OpenOCD, 3-pin JST-SH cables, 3.3 V)
- https://www.berrybase.de/en/raspberry-pi-debug-probe (12,20 €, in stock)
- https://www.berrybase.de/en/search?search=stlink (official ST-LINK-V2 49,90 €; STLINK-V3MODS 16,10 €; no V3MINIE)
- https://www.reichelt.de/de/de/shop/suche/stlink-v3minie (STLINK-V3MINIE 31,60 €, in stock)
- https://github.com/TOILmodular/Rings (DIY clone, JTAG/UART programming notes)
- Unreachable today: st.com STLINK-V3MINIE product page and mouser.de (timeouts); forum.mutable-instruments.net (DNS gone)

---

## Addendum (2026-11-20, later): RP2040 as probe, JP2 pitch verified

- Decision (user): **no ST-Link purchase.** One of the user's RP2040 boards, flashed with the
  official **raspberrypi/debugprobe** firmware (https://github.com/raspberrypi/debugprobe,
  formerly picoprobe), serves as the SWD probe: CMSIS-DAP + USB-UART bridge, supported by
  OpenOCD (`interface/cmsis-dap.cfg`) and probe-rs, 3.3 V logic [sourced: debugprobe README /
  RPi docs for the Debug Probe, which runs the same firmware]. Default pin mapping to verify
  before wiring: GP2 = SWCLK, GP3 = SWDIO, GP4/GP5 = UART TX/RX [hypothesis]. Like the Pi
  Debug Probe it has no NRST line; OpenOCD resets via SWD (SYSRESETREQ).
- Only remaining purchase: the 2x5 1.27 mm Cortex-Debug adapter set for JP1.
- **JP2 pitch verified [code]:** in `rings_v30.brd` (binary Eagle, read via strings) JP1 has
  package `2X5-1.27` (1.27 mm — standard jumper wires do NOT fit) and JP2 has package `1X06`,
  the plain 2.54 mm 1x6 header — **standard jumper wires fit JP2 directly.** Together with the
  on-board RESET and SYSBOOT (BOOT_FLASH) switches this makes the ROM serial bootloader a
  zero-purchase recovery path (read and write, e.g. `stm32flash`), with the RP2040 usable as
  the UART bridge.

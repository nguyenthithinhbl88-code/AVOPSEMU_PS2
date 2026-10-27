# v39 — experimental BA0 bootstrap compatibility

The previous PS2 log printed `Initialize GS ...` and stopped at PC 8000D688,
SH A4440000 to BA000008 with value 3, after 15813763 instructions.
PMODE/GIF/primitives were all zero: this does not prove a rendered BIOS UI.

Add aligned halfword handlers for physical 1A000000..1A00000F and existing
aliases. Writes are sinks, last values retained only for diagnostics.
Reads return zero except offset6, which cycles 1,2,0. This is an experimental
compatibility policy based on PCSX2 1.6 Memory.cpp ba0R16 and its external
halfword write handler. Device identity/bit semantics are unverified.
Phase belongs to the instance and resets deterministically.
Byte, word, quadword, unaligned requests and adjacent regions still fail.
No GS drawing, PMODE enabling, IOP-ready reply or fake BIOS UI is generated.

The exact nine final attempted instructions from the log replay to 8000D68C,
including stack/return-address checks. This is not a full BIOS replay.
Tests cover aliases, counters, phase, reset, strict guards and diagnostics.

Reference: https://sources.debian.org/src/pcsx2/1.6.0%2Bdfsg-2/pcsx2/Memory.cpp/
New local implementation; no external source files incorporated.

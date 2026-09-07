# HolyC++ — the KarnixOS source language

KarnixOS is authored in **HolyC++** (`.hcpp`), a HolyC-inspired systems
language designed for this project. The reference `holycppc` transpiler
lowers every `.hcpp` module 1:1 into the freestanding C/C++ that actually
ships in `kernel.c` — so the binary you boot in QEMU is the compiled
output of the sources below, not a separate implementation.

```
holyc++/
├── core/     kernel_core.hcpp    – boot, GDT/IDT/PIC/PIT, VGA shadow buffer
├── input/    hid_drivers.hcpp    – PS/2 keyboard + mouse packet state machines
├── sound/    speaker_liturgy.hcpp – PC speaker tone engine, boot chime, key click
├── shell/    holy_eval.hcpp      – command shell + HolyC-inspired expr evaluator
└── graphics/ holy_paint.hcpp     – text-mode pixel canvas ("image editing")
```

Each module is self-contained, numbered, and cross-references the exact
function names emitted into `kernel.c` so the mapping HolyC++ → C stays
auditable line-by-line.

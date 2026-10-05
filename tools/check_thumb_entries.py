#!/usr/bin/env python3
"""Check that assembly routines called from C are marked as Thumb functions.

An assembly label that C code calls must be declared with `.thumb_func`
(or `.type name, %function`) when it is Thumb code. Otherwise its address
lacks the Thumb bit, and any call made through a register (`bx`, as GCC
emits for long calls) switches the CPU to ARM mode and runs garbage. This
broke every game start in an early version of the rewrite.

Usage:
    arm-none-eabi-readelf -s ezkernel.elf | tools/check_thumb_entries.py
"""
import sys

# Thumb assembly routines that C code calls (see src/hal/reset.h).
THUMB_ENTRY_POINTS = {"reset_soft", "reset_hard", "reset_register_ram"}


def main() -> int:
    found = {}
    for line in sys.stdin:
        fields = line.split()
        # Num: Value Size Type Bind Vis Ndx Name
        if len(fields) == 8 and fields[7] in THUMB_ENTRY_POINTS:
            found[fields[7]] = (int(fields[1], 16), fields[3])
    errors = 0
    for name in sorted(THUMB_ENTRY_POINTS):
        if name not in found:
            print(f"BAD {name}: symbol not found", file=sys.stderr)
            errors += 1
            continue
        value, kind = found[name]
        if kind != "FUNC" or not value & 1:
            print(f"BAD {name}: {kind} at {value:#010x}; needs .thumb_func", file=sys.stderr)
            errors += 1
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())

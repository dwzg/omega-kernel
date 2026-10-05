#!/usr/bin/env python3
"""Check that code in IWRAM only calls into ROM where that is known to be safe.

Functions marked IWRAM_CODE run while the cartridge ROM may be unmapped
(the Omega switches the ROM bus to the SD card, NOR flash or PSRAM). A call
from such a function back into ROM would then execute garbage. The linker
routes IWRAM -> ROM calls through "long call" veneers, so listing the
veneers in the IWRAM sections tells us every such call.

Usage:
    arm-none-eabi-objdump -d -j .iwram ezkernel.elf | tools/check_iwram_calls.py
"""
import re
import sys

# ROM functions that IWRAM code may call, and why.
ALLOWED = {
    # Busy-wait used by the SD and RTC routines. Its timing was tuned in
    # ROM, and it is only called while the kernel ROM is mapped.
    "delay_loop",
    # Drawing helpers run only from the menu, with the kernel ROM mapped.
    "gfx_framebuffer",
}

VENEER = re.compile(r"^[0-9a-f]+ <__(\w+?)_veneer>:")
CALL = re.compile(r"^\s*([0-9a-f]+):.*\sbl[x]?\s+[0-9a-f]+ <__(\w+?)_veneer>")
FUNC = re.compile(r"^[0-9a-f]+ <(\w+)>:")


def main() -> int:
    current = "?"
    callers = {}
    for line in sys.stdin:
        m = FUNC.match(line)
        if m:
            current = m.group(1)
            continue
        m = CALL.match(line)
        if m:
            callers.setdefault(m.group(2), set()).add(current)

    bad = {t: c for t, c in callers.items() if t not in ALLOWED}
    for target, sources in sorted(callers.items()):
        status = "ok " if target in ALLOWED else "BAD"
        print(f"{status} {target:24} <- {', '.join(sorted(sources))}")
    if bad:
        print("\nIWRAM code calls ROM functions that are not allowed (see the script).",
              file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())

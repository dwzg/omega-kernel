#!/usr/bin/env python3
"""Print the CHANGELOG.md section of one version (for release notes).

Usage: changelog_section.py VERSION [CHANGELOG.md]
"""
import re
import sys


def main():
    version = sys.argv[1]
    path = sys.argv[2] if len(sys.argv) > 2 else "CHANGELOG.md"
    lines, inside = [], False
    for line in open(path, encoding="utf-8"):
        if line.startswith("## "):
            if inside:
                break
            inside = re.match(rf"## \[?{re.escape(version)}\]?(\s|$)", line) is not None
            continue
        if inside:
            lines.append(line)
    if not inside and not lines:
        sys.exit(f"{path}: no section for version {version}")
    print("".join(lines).strip())


if __name__ == "__main__":
    main()

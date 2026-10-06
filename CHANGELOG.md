# Changelog

All notable changes to this project are documented here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and the project uses
[Semantic Versioning](https://semver.org/).

## [Unreleased]

### Added

- **Start where you left off.** The kernel opens the folder of the game you
  played last, with that game selected. **B** selects the folder you came
  from.
- **Favorites.** Mark games on their page; the Favorites screen in the main
  menu lists them by name.
- **Jump by letter.** **Left / Right** in the SD card browser go to the
  previous / next first letter.
- **Save backups.** Before a game starts, its save is copied to
  `/SAVER/<game>.bak`. **Restore save** on the game page goes back to it
  (and can be undone the same way).
- **Save type detection.** A game missing from the save type list is
  checked for the name of its save chip the first time it is started, so ROM
  hacks, prototypes and translations with new game codes save correctly
  without choosing the type by hand.
- **File names in other languages.** Accented letters (é, ü, ñ, ...), Greek,
  Cyrillic and Japanese kana are shown instead of `?`, and long names are
  kept in full (they scroll when selected).
- **Bigger folders.** A folder lists up to 2,048 entries instead of 512 games
  and 256 folders, in the same memory: names are kept in one shared pool.

### Changed

- Folders are sorted the way people expect: numbers by value ("Mega Man 2"
  before "Mega Man 10") and accented letters with their base letter.

### Fixed

- Gauntlet: Dark Legacy (AYGE) uses 512 B EEPROM, not 8 KB, so it saves again.

## [2.0.0] - 2026-10-05

A rewrite of the kernel with a new interface. Saves, save states, settings,
games in NOR flash, cheat files and thumbnails from the original kernel keep
working.

### Removed

- **Chinese language support.** The interface is English only. Cartridges set
  to Chinese switch to English.
- **The NES, Game Boy and Game Boy Color emulators.** The kernel starts GBA
  games only; `.nes`, `.gb` and `.gbc` files are no longer listed.
- The thumbnail on/off option of the old interface (box art is shown whenever it is on the card).

### Changed

- **New interface**: crisp pixel fonts (Galmuri), one list per screen, a
  title bar with clock, a bar at the bottom showing what each button does,
  and a game page with full-size box art, game code, size and save type.
  Settings show On / Off; dialogs say which button does what.
- The kernel opens the SD card directly; **B** in the top folder leads to the
  main menu (SD Card, NOR Flash, Recently Played, Settings, About).
- Settings are a single list; hotkeys and the clock have their own editors.
  The weekday is calculated from the date.
- File names are read with code page 437 instead of 936; characters outside
  ASCII are shown as `?`.
- The code base is reorganised into layers with documentation, unit tests,
  a UI simulator, CI and automatic releases.

### Fixed

- Games without the archive attribute, or marked read-only, were not listed.
  Hidden and system files are still left out.
- SD card read errors are reported instead of starting a corrupted game.
- Payloads crossing an 8 MB PSRAM page were cut off (affected save states and
  long cheat lists in games of 16-32 MB).
- Copying a second game to NOR in the same session could apply patches meant
  for the first one.
- Copying a full 32 MB game with add-ons to NOR could place the add-on code
  outside the cartridge; long cheat lists in NOR could be cut off while the
  game still expected them.
- A cheat value ending with `;` added a bogus code writing to address 0.
- Very fragmented ROMs or saves could overflow the table sent to the FPGA;
  they are now refused with a message.
- Many buffer overflows with long file names, paths and cheat files.
- The recently played list could lose or duplicate entries.
- The last page of a firmware update was padded with unrelated memory.
- Unknown settings words are preserved instead of being reset.
- Corrupt `.pat` cache files are ignored instead of being used.

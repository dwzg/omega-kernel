# User interface

This page describes the design of the kernel's interface and how it is
built. For using it, see the [user guide](user-guide.md).

<p>
  <img src="images/sd-root.png" width="320" alt="SD card browser">
  <img src="images/game-page.png" width="320" alt="Game page">
</p>

## Design principles

- **Simple and functional.** Every screen is one list or a short dialog.
  **A** goes in or does the thing, **B** goes back, always.
- **Say what the buttons do.** The hint bar at the bottom shows the buttons
  that work on the current screen and what they do, updated as the selection
  moves ("A Play · L+A BIOS · B Back", "<> Change").
- **Crisp text.** Pixel fonts (Galmuri) whose pixels are either on or off,
  so text is sharp on the GBA screen. Body text and titles are Galmuri11
  (11 px capitals on a 16 px line), small text Galmuri9.
- **Big pictures.** Box art is shown at its full 120 × 80 pixels.
- **Plain colours.** White background, near-black text, a dark title bar and
  a solid blue selection. Greyed-out rows are actions that are unavailable
  with the current settings; choosing one says why.
- **Clear words.** Settings show On / Off; dialogs say which button does
  what ("A Delete · B Cancel").

## Layout

```
 ┌──────────────────────────────────────────────┐  0
 │ SD Card                          3/13  09:41 │  title bar, 16 px
 ├──────────────────────────────────────────────┤
 │ Advance Wars                          4 MB   │  rows of 16 px,
 │▓Golden Sun▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓ 8 MB ▓│  8 visible
 │ Metroid Fusion                        8 MB  ▐│  scroll bar
 ├──────────────────────────────────────────────┤  146
 │ [A] Select  [B] Back  [START] Recent         │  hint bar, 14 px
 └──────────────────────────────────────────────┘  160
```

Game pages split the content area: the box art and details on the left
(`GAME_PANEL_WIDTH`, 128 px), the options on the right.

Metrics and colours are in `src/ui/theme.h`.

## Building blocks (`src/ui/widgets.h`)

| Widget | Purpose |
| --- | --- |
| `ui_title_bar()`, `ui_tick()` | Title bar with clock; `ui_tick()` keeps the clock current |
| `ui_hints()` | The button hint bar: `"A Open|B Back|START Recent"`, with `<>` and `^v` for the d-pad |
| `ui_list_t` | Scrolling list. Rows are produced on demand by a callback filling a `ui_row_t` (label, value, kind, checked, dimmed), so lists of any length cost no memory. Handles up/down, L/R paging, key repeat, headings that can't be selected, and scrolling of labels that don't fit |
| `ui_message()` | Message; A or B closes it |
| `ui_confirm()` | Question; A does the action, B cancels |
| `ui_progress_begin()` | Progress screen; returns a `progress_t` the loader reports to |

Row kinds: plain (label and optional value), chevron (opens a screen),
heading (small grey, not selectable) and check (shows a check mark).

## Screens

| File | Screens |
| --- | --- |
| `menu_screens.c` | Main menu, SD card browser, recently played |
| `game_screens.c` | Game page, cheat selection |
| `nor_screens.c` | NOR library, NOR game page, erase |
| `settings_screens.c` | Settings, hotkey and clock editors, About |
| `system_screens.c` | Firmware update, fatal errors |
| `app.c` | Settings persistence, entry point |

Screens are plain functions that run their own loop and return when the user
goes back. State shared between screens (settings, current folder, list
positions per folder depth) lives in `app_t`.

## Drawing (`src/gfx/`)

The display runs in mode 3: a 240 × 160 frame buffer of 15-bit colours.
`gfx.c` provides fills, frames, image blits (full and half size) and text.
Text is UTF-8 (`core/utf8.c`); glyphs are 1 bit per pixel.

Lists redraw only what changes, and never show anything half-drawn:

- Each row is built in an off-screen buffer (spare VRAM after the frame
  buffer, `gfx_offscreen_begin()`) and copied to the screen in one go. The
  title bar is built the same way.
- When a list scrolls by a row, the rows that stay visible are moved
  (`gfx_move_rows()`) and only the row that comes into view and the two whose
  selection changed are drawn. The simulator checks that this gives exactly
  the same pixels as a full redraw.
- A selected name that is too long scrolls by one pixel per frame, resting
  for a second at the start of each round. Each step is prepared off-screen
  and copied at the start of the next vertical blank, so it doesn't flicker.

## Fonts

The fonts are BDF bitmap fonts in `assets/fonts/src/` (Galmuri by Lee Minseo,
SIL Open Font License, reduced to the characters the kernel shows).
`tools/make_font.py` converts them, as listed in `assets/fonts/fonts.txt`:

```sh
python3 tools/make_font.py --all          # regenerate
python3 tools/make_font.py --all --check  # what CI runs
```

The format ("OFN2") is documented at the top of the script. Fonts cover
ASCII, Latin-1 and Latin Extended-A (accented letters), Greek, Cyrillic,
common punctuation, Japanese kana and full-width ASCII (`RANGES` in the
script); the bold title font takes characters it lacks from Galmuri11.
Other characters are drawn as `?`. To add a range, extend `RANGES`, subset
the full Galmuri BDF files again with `--subset` and run `--all`.

## Changing the interface

1. Make the change.
2. `make -C tests screenshots` and look at `tests/build/screens/*.png`.
3. `make -C tests update-golden` to accept the new screens, and commit
   `tests/sim/golden.txt` together with the change.
4. Add a scenario to `tests/sim/scenarios.c` for new screens.

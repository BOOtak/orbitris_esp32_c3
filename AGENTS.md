# AGENTS.md

## What this is

Monochrome (400x240, 1-bit) orbital Tetris for an ESP32-C3 handheld. The game core is shared with a desktop raylib simulator:

- `orbitris_esp32/` - shared game core plus the ESP32-Arduino implementation (`.ino`, `input.cpp`, `persist.cpp`, `sharp_display.cpp`, `trace.cpp`).
- `raylib_adapter/` - desktop simulator implementing the same platform symbols as the device.
- `lib/raylib` - git submodule, used only by the desktop build.

## Desktop build and run

Initialize the submodule first: `git submodule update --init --recursive`.

`build/` is already configured with the Visual Studio 2022 x64 generator. Rebuild with:

```
cmake --build build --config Debug
```

Then run `build\Debug\orbitris_esp32.exe`. To configure a fresh tree: `cmake -S . -B build -G "Visual Studio 17 2022" -A x64`. If `cmake` is not on PATH, use the CMake bundled with VS 2022 or a Developer PowerShell.

There are no automated tests, lint task, CI, or CMake presets. Building and running the desktop app is the verification step.

## ESP32 firmware

- Open `orbitris_esp32/orbitris_esp32.ino` in the Arduino IDE with an ESP32-C3 board. Arduino compiles every `.cpp` in that sketch folder automatically; there is no ESP-IDF or CMake firmware project.
- Pins and SPI/display setup are hardcoded in `sharp_display.cpp` and `input.h`.
- Keep Arduino/ESP headers out of shared core files.

## Platform boundary

The shared core must only call platform symbols implemented by both adapters: `lcd_draw_pixel`, `lcd_set_byte`, `lcd_fill_buffer`, `lcd_fill_line`, `is_key_down/pressed/released`, `input_init/input_update` (desktop no-ops), `load_*`/`persist_*`, `do_print`. Each adapter calls the shared `init_game()` and `update_draw_frame()`.

- `lcd_init`, `lcd_clear`, and `lcd_update` exist only on the device; desktop `main.cpp` calls `update_draw_frame` directly.
- Root `CMakeLists.txt` explicitly lists desktop sources and excludes the ESP-only files. When adding a shared `.cpp`, add it there. Never add `orbitris_esp32/{input,persist,sharp_display,trace}.cpp` or the `.ino`.
- New shared `.cpp` files are compiled automatically by Arduino, so they must stay portable (no `raylib.h`, no `<Arduino.h>`).

## Architecture notes

- `game_main.cpp` is the screen state machine. `rules[]` selects transitions; `init_game()` constructs screens. A new `Screen` subclass needs registration in `screen.h`/`screen.cpp`, construction in `init_game()`, and usually a transition rule.
- `draw.h` primitives honor global scale/mask state. Pair `begin_scale`/`end_scale` and `begin_mask`/`end_mask`; all drawing ultimately goes through `lcd_*`.
- Persistence stores raw struct blobs: desktop writes `stats.dat`/`highscores.dat` in the working directory, the device uses NVS `Preferences`. Changing `Stats` or `HighscoreTable` layout invalidates saved data.
- `default_scores_private.h` is gitignored and only used when compiled with `PRIVATE_BUILD`.

## Input

Desktop controls: arrow keys, `Z` = A, `X` = B. Device buttons are active-low with pull-ups; `ESP_KEY_*` values in `input.h` are GPIO numbers.

## Style

Follow `.clang-format`: 2-space indent, attached braces, no column limit, includes not sorted. Match the existing lowercase snake_case C-style APIs in the core.

# AGENTS.md

## What this is

Handheld game repo for a 400x240, 1-bit ESP32-C3 device. One platform layer is shared by the desktop raylib simulator and the ESP32 device:

- `orbitris_esp32/` - PlatformIO/Arduino sketch (`src_dir` in `platformio.ini`) containing the screens, shared gameplay modules, and device adapters.
- `lib/common/` - PlatformIO library of platform-independent drawing, math, and LCD primitives.
- `raylib_adapter/` - desktop adapter implementing the same platform symbols.
- `third_party/raylib` - git submodule used only by the desktop build.
- `DESIGN.md` - Cat Guard game design document and milestones M1-M6.

The current boot screen is `HouseScreen` (Cat Guard M1, subtractive darkness). The original Orbital Tetris screens remain.

## Build and run

Desktop (build/ is preconfigured with the Visual Studio 2022 x64 generator):

```
cmake --build build --config Debug
```

Run `build\Debug\orbitris_esp32.exe`. Initialize the submodule first if needed: `git submodule update --init --recursive`. Fresh configure: `cmake -S . -B build -G "Visual Studio 17 2022" -A x64`. If `cmake` is not on PATH, use the VS-bundled CMake or a Developer PowerShell.

Firmware: PlatformIO is configured in the root `platformio.ini` (`default_envs = esp32c3`, `src_dir = orbitris_esp32`). Use `pio run -e esp32c3` and `pio run -e esp32c3 -t upload` when the PlatformIO CLI is available; otherwise use the PlatformIO IDE/VS Code. The Arduino IDE can also open `orbitris_esp32/orbitris_esp32.ino`.

There are no automated tests, lint task, CI, or CMake presets. Building and running the desktop app is the verification step.

## Platform boundary

Shared code may call only symbols implemented by both adapters: `lcd_draw_pixel`, `lcd_set_byte`, `lcd_and_byte`, `lcd_fill_buffer`, `lcd_fill_line`, `is_key_down/pressed/released`, `input_init/input_update` (desktop no-ops), `load_*`/`persist_*`, `do_print`, `platform_micros`.

- `lcd_init`, `lcd_clear`, and `lcd_update` exist only on the device; desktop `main.cpp` calls `update_draw_frame` directly.
- `lcd_and_byte(cx, y, mask)` ANDs 8 pixels: 1 = keep, 0 = clear to black. It is the subtractive-darkness primitive.
- `lcd.h` lives in `lib/common/include`; `sharp_display.h` mirrors the device declarations. Keep them in sync.
- Keep shared headers portable: no `<Arduino.h>` and no `raylib.h`. Device-only files are `input.cpp`, `persist.cpp`, `sharp_display.cpp`, and `trace.cpp`.
- Root `CMakeLists.txt` lists desktop sources explicitly. Add every new shared `.cpp` there. Arduino/PlatformIO compile `orbitris_esp32/*.cpp` automatically; `lib/common` is a PlatformIO library.

## Architecture notes

- `game_main.cpp` is the screen state machine. `rules[]` selects transitions; `init_game()` constructs screens. A new `Screen` subclass needs registration in `screen.h`/`screen.cpp`, construction in `init_game()`, and usually a transition rule. Current boot is `screens::house_screen`.
- `world.{h,cpp}`: 15x15 maze (48px cells = 3x16px blocks), `is_wall_block`, wall-only collision (`deepest_contact_walls`, `move_with_collision`), and circle drawing. Edit the maze in `world.cpp`.
- `light.{h,cpp}`: fixed-point 360-degree light pipeline: DDA wall raycast, scanline polygon fill, Bayer dither. `draw_darkness_polygon` is the subtractive path used by the game (clears unlit pixels via `lcd_and_byte`); `fill_light_polygon`/`fill_solid_polygon` are additive/solid helpers kept for reuse.
- `house_screen.{h,cpp}`: Cat Guard M1 scaffold. Render order is white fill, black scene (outlines/player), then darkness. Cats, rugs, bed, and box are passable. Controls: arrows move, B runs. Player start/speed, cats, and target placements are tunable constants at the top of `house_screen.cpp`.
- The C3 has no FPU. Keep per-pixel light math fixed-point/integer and batch LCD writes through `lcd_set_byte`/`lcd_and_byte`; avoid adding float math to the lighting hot path.
- Persistence stores raw `Stats`/`HighscoreTable` blobs. Changing their layout invalidates `stats.dat`/`highscores.dat` and device NVS data.

## Input

Desktop controls: arrow keys, `Z` = A, `X` = B. Device buttons are active-low with pull-ups; `ESP_KEY_*` values in `input.h` are GPIO numbers. `HouseScreen` uses B for run.

## Style

Follow `.clang-format`: 2-space indent, attached braces, no column limit, includes not sorted. Match the existing lowercase snake_case C-style APIs in the core.

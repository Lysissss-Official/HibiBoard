# HibiBoard

A classroom desktop dashboard built with Qt6/C++. Provides a sleek top-bar overlay with a flip clock, class schedule countdown, semester countdown, and a duty roster sidebar — designed for classroom displays or teacher workstations.

## Features

- **Flip Clock** — retro-styled animated flip clock with millisecond precision
- **Class Schedule** — displays current/next class with countdown, supports 5-day × 12-period timetables
- **Semester Countdown** — shows remaining days until semester end
- **Duty Roster** — sidebar showing today's on-duty group and assigned students for 5 roles
- **Exam Mode** — fullscreen clock with animated rising triangles, triggered by clicking the clock area
- **Quick Buttons** — customizable shortcuts for settings, browser, file manager, clock toggle
- **Settings Panel** — fully editable schedule, duty groups, duty calendar, and semester info, persisted via QSettings
- **Wayland Support** — optional layer-shell integration for proper panel behavior on tiling WMs
- **Cross-Platform** — Linux (X11/Wayland) and Windows, with NSIS installer packaging

## Screenshots

<!-- TODO: add screenshots -->

## Build

### Requirements

- Qt 6.5+ (modules: Core, Gui, Widgets, Network)
- CMake 3.16+
- C++20 capable compiler (GCC 13+, Clang 16+, MSVC 2022+)
- (Optional) [LayerShellQt](https://github.com/nickguyer/LayerShellQt) for Wayland panel mode

### Linux

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Windows (MinGW cross-compile)

```bash
cmake -B cmake-build-mingw64-release \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/mingw-toolchain.cmake
cmake --build cmake-build-mingw64-release
cpack --config cmake-build-mingw64-release/CPackConfig.cmake   # NSIS installer
```

## Usage

The app launches a top bar docked at the screen top and an optional duty roster sidebar on the right.

| Interaction | Action |
|---|---|
| Click clock area | Enter fullscreen exam clock mode |
| Click anywhere in exam mode | Exit exam mode |
| Hover quick buttons | Show tooltip |
| Click settings button | Open settings panel |
| Drag duty roster | Reposition (when movable is enabled) |

## Configuration

All settings are persisted in `~/.config/HibiBoard/HibiBoard.conf` (Linux) or `%APPDATA%/HibiBoard/HibiBoard.ini` (Windows).

Editable via the built-in settings panel:
- Class name and semester end date
- Period definitions (name, start/end time)
- Weekly subject and teacher schedule
- Duty groups and roles
- Duty calendar (date → group mapping)
- Top bar exclusive zone and always-on-top behavior

## Project Structure

```
├── main.cpp              # Entry point, font loading, window initialization
├── config.h              # Default config, loadConfig(), saveConfig()
├── topbarwindow.h/cpp    # Main top bar with clock, schedule info, quick buttons
├── dutywindow.h/cpp      # Duty roster sidebar
├── settingswindow.h/cpp  # Settings panel with multi-page editor
├── resources.qrc         # Embedded resources (logo, icon)
├── assets/               # Logo and icon files
├── CMakeLists.txt        # Build configuration, CPack NSIS packaging
└── HibiBoard-Windows/    # Prebuilt Windows binaries
```

## License

MIT

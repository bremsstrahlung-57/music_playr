# **music_playr**

A lightweight cross-platform desktop music player written in C++17.

- Audio engine: [miniaudio](https://github.com/mackron/miniaudio)
- UI framework: [Dear ImGui](https://github.com/ocornut/imgui)

![C++](https://img.shields.io/badge/language-C++17-blue)
![Build](https://img.shields.io/badge/build-CMake-brightgreen)
![License: MIT](https://img.shields.io/badge/license-MIT-green)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux-lightgrey)


## Features

- Play audio files (MP3, WAV, FLAC, OGG, etc.)
- Playlist management
- Audio controls (play, pause, stop, seek)
- Add and Delete tracks
- Volume and Seek bar control
- Modern GUI interface
- Cross-platform support

## Building

```bash
git clone https://github.com/bremsstrahlung-57/music_playr.git
cd music_playr
mkdir build && cd build
cmake ..
make
./music_playr
```

### Dependencies:

- **miniaudio**: Low-level audio backend
- **Dear ImGui**: Immediate-mode GUI framework
- **OpenGL**: Graphics rendering
- **GLFW**: Window and input management
- **CMake**: Build system configuration

## Usage

Run the executable and use the GUI to:

- Add music files to your playlist
- Control playback
- Adjust volume and seek through tracks
- Make playlists
- Add and remove tracks from playlists

## Architecture Overview

### Core Components

- **Audio Engine and Controller**: _audio.cpp/hpp_ : Utilizes miniaudio for cross-platform audio playback with support for multiple formats. Manages playback state, volume control, and seeking functionality
- **UI Layer**: _player.cpp/hpp_ : Built with Dear ImGui for immediate-mode GUI rendering
- **Playlist Manager**: _db.cpp/hpp_ : Handles track management, queue operations, and playlist persistence

### Design Pattern

The project employs a component-based architecture where:

- Audio processing runs on a separate thread to prevent UI blocking
- State management is centralized for consistent playback control
- GUI components communicate through a simple event system
- File I/O operations are handled asynchronously

## Demo

<img width="405" height="315" alt="Screenshot 2025-11-05 203444" src="https://github.com/user-attachments/assets/d5593acd-0510-474e-bb45-6ea7dfb65701" />

<img width="405" height="315" alt="Screenshot 2025-11-05 203330" src="https://github.com/user-attachments/assets/7d3a64ca-44b9-4eb2-a6c8-f365bd44209f" />

<img width="405" height="315" alt="Screenshot 2025-11-05 203516" src="https://github.com/user-attachments/assets/0f2e668d-c42e-494d-aa96-afd1e5f10a11" />

<img width="405" height="315" alt="Screenshot 2025-11-05 203659" src="https://github.com/user-attachments/assets/19ae1047-3cae-4489-a6c3-5c05fa7ebc0e" />

---

https://github.com/user-attachments/assets/ffd5cc56-f62f-410b-b3d5-e4e8ffb3fb18

---

## Why

First I planned to make a CLI based music player but wanted to learn more about C++ and GUI development across Windows, Linux, and macOS. So I built this instead..

## License

MIT License © 2025 Sagar Sharma

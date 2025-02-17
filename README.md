# Tetrominoes
This game is my realization of Tetris. The project is written in C language using SDL2 libraries for rendering graphics and controlling input devices (currently keyboard).
# Installing
The game is already compiled for Windows and Linux operating systems with x64-bit architechture.
## Windows
Download the Windows release or the full project, find "./builds/Windows/Tetrominoes.exe" file and run it.
## Linux
Download and install SDL2 libraries on your machine.
- For Debian use: *sudo apt-get install libsdl2-2.0-0 libsdl2-dev libsdl2-image-2.0-0 libsdl2-ttf-dev libsdl2-image-dev libsdl2-mixer-dev*

Download the Linux release or the full project, find "./builds/Linux/Tetrominoes" file and execute it via terminal ("./Tetrominoes").
# Compiling
Use the Makefile in the “./” directory to simplify compilation.
## Windows
Use *make build_windows_dynamic* command to compile game with dynamic linking (.dll files are required).
The output file can be found in "./builds/Windows/Tetrominoes.exe".
All library files (.dll, .h, .a) can be found in "./libs/" directory.
## Linux
Before compiling, you will need to download and install SDL2 libraries on your machine.
- For Debian use: *sudo apt-get install libsdl2-2.0-0 libsdl2-dev libsdl2-image-2.0-0 libsdl2-ttf-dev libsdl2-image-dev libsdl2-mixer-dev*

Use *make build_linux_dynamic* command to compile game with dynamic linking (requires installed libraries in "/lib/" or "/usr/lib/" directories).
The output file can be found in "./builds/Linux/Tetrominoes" directory.

# Controls

- **ESC** - close the game
- **SPACE** - fall acceleration

**Arrows**
- **LEFT** - left move
- **RIGHT** - right move
- **UP** - clockwise rotation
- **DOWN** - counterclockwise rotation

# Gameplay

![](https://github.com/dencoderexe/Tetrominoes/blob/main/screenshots/gameplay.gif)

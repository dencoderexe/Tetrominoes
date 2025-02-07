TARGET = ./build/Tetrominoes
SRC = ./src/*.c
LINK_SDL2 = -Llibs\SDL2_dev\x86_64-w64-mingw32\lib -Ilibs\SDL2_dev\x86_64-w64-mingw32\include

build_linux_dynamic:
	gcc -Wall -DLINUX $(SRC) -o $(TARGET) -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer
	
build_windows_dynamic: #x64
	gcc -Wall -DWINDOWS ./src/*.c -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer -o $(TARGET).exe $(LINK_SDL2) -mwindows

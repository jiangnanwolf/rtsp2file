# RTSP to File with Motion Detection

This project allows you to capture RTSP streams and save only the motion-detected parts of the video to a file using FFmpeg libraries.

## Installation

To compile the project, you need to have the following dependencies installed:

- FFmpeg
- SDL2
- X11
- xcb
- alsa
- sndio
- Xv
- Xext
- lzma
- x264
- x265

You can install these dependencies on a Debian-based system using the following command:

```sh
sudo apt-get install libavdevice-dev libavfilter-dev libpostproc-dev libavformat-dev libavcodec-dev liblzma-dev libx264-dev libx265-dev libswresample-dev libswscale-dev libavutil-dev libsdl2-dev libxcb1-dev libasound2-dev libsndio-dev libxv-dev libxext-dev libx11-dev
```

## Compilation

To compile the project, use the following command:

```sh
g++ main.cpp rtsp.cpp -L/usr/local/lib -lavdevice -lxcb -lasound -lSDL2 -lsndio -lXv -lXext -lm -lavfilter -lpostproc -lavformat -lavcodec -llzma -lx264 -lx265 -lz -lswresample -lswscale -lavutil -pthread -latomic -lX11 -g -o remux
```

## Usage

To run the compiled program, use the following command:

```sh
./remux <RTSP_URL> <output_file>
```

Replace `<RTSP_URL>` with the URL of the RTSP stream you want to capture and `<output_file>` with the desired output file name.

## License

This project is licensed under the MIT License.

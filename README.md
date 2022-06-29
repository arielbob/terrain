Terrain generation using the [diamond-square algorithm](https://en.wikipedia.org/wiki/Diamond-square_algorithm).

## Building

- Have the 64-bit Microsoft C++ (MSVC) compiler initialized in your terminal. This can be done by finding and running `vcvars64.bat` in your terminal. Info on how to find that [here](https://docs.microsoft.com/en-us/cpp/build/building-on-the-command-line?view=msvc-170#developer_command_file_locations).
- Run `cd src`.
- Run `build.bat`.

## Running

- Open `main.exe` from the `build` directory.

## Program Instructions

- WASD to move
- Hold Shift to move faster
- T to hide textures
- Z to show wireframe made up of initial points

The file used as the initial points is in `data/initial_terrain1.txt'.

The program expects the parameters in initial heights files to be specified in this order:
- Low-resolution height map exponent (n)
- High-resolution height map exponent
- (2^n+1)^2 initial heights

For example, with the values 2, 10, then the initial heights, the program will create a 5x5 (2^n+1 = 5) low-resolution grid, thus the program will expect 25 values, and the resulting height map’s dimensions will be (2^10 + 1)x(2^10 + 1).
See the existing initial_heights text files for a template.

## Examples

| initial_terrain1.txt | initial_terrain2.txt | initial_terrain3.txt |
| -------------------- | -------------------- | -------------------- |
| ![image15](https://user-images.githubusercontent.com/23489769/176546842-35bd6249-53d9-4b29-8c15-36bffe2184c5.png) | ![image12](https://user-images.githubusercontent.com/23489769/176546874-7108963f-6bd7-4207-acec-be2174fa848d.png) | ![image20](https://user-images.githubusercontent.com/23489769/176546888-aab5a5e2-9b6e-4d10-bae3-e3ed0cefbf2a.png) |




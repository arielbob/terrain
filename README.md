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

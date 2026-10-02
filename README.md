Atomia : 2D showcase about bonding atoms together in C. WIP.

Built with Raylib.

## Build

Requires [Raylib](https://www.raylib.com/) headers and `lib/raylib.a` / `lib/raylib.dll` in the `lib/` folder (provided in this repo).

```sh
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
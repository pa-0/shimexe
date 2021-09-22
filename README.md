Under construction.

Based on [kiennq's c++ shim implementation](https://github.com/kiennq/scoop-better-shimexe)

## Installation

- In a Visual Studio command prompt, run `cl.exe /nologo /std:c++17 /DNDEBUG /MT /O2 /GF /GR- /GL shim.cpp`.
- Or using `clang++` with `clang++ shim.cpp -o shim.exe -m32 -O -std=c++17 -g`.
- Replace any `.exe` in `scoop\shims` by `shim.exe`.

An additional script, `repshims.bat`, is provided. It will replace all `.exe`s in the user's Scoop directory
by `shim.exe`.


## License

`SPDX-License-Identifier: MIT OR Unlicense`

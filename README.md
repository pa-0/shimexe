This is an attempt at a drop in replacement for the RealDimensions Softwarem LLC's (RDS) `shimgen.exe` that comes with Chocolatey. The shims are based on [kiennq's c++ shim implementation](https://github.com/kiennq/scoop-better-shimexe) for Scoop. 

The `.shim` file format is similar to the Scoop `.shim` format, as the `gui = ` option has been added so support the `shimgen.exe` `--gui` command line argument:
```
path = C:\path\to\file.exe
args = --extra-args-go-here
gui = force
```

### Advantages

- Free and Open Source
- Able to be freely used outside of "official" Chocolatey builds without a paid license.
- Faster shim creation
- Better support for `crtl+c`, as shim passes signal to child process
- Tries to terminate child processes if parent process is killed
- Consistent checksum for all shims
- No use of `csc.exe` by `shimgen.exe`

### Disadvantages

- No support for icons for the shims
- Creates a `.shim` file next to each shim `.exe`.
- Not thoroughly tested (yet)
- No support for extra shim options/arguments

## Build and Install

- In a Visual Studio (2019) command prompt, run:
```
cl.exe /nologo /std:c++17 /DNDEBUG /MT /O2 /GF /GR- /GL shim.cpp
rc shimgen-resources.rc
cl.exe /nologo /std:c++17 /DNDEBUG /MT /O2 /GF /GR- /GL /EHsc shimgen.cpp shimgen-resources.res
```

- Replace `shimgen.exe` in your choco source code tree with the build artifact. Don't forget to update the `shimgen.license.txt` as well.

## TODO
- Benchmark shim creation time and shim launching time
- Shim.exe metadata in file?
- Testing
- Shim Implement logging
- Shim add in error checking (working directory exists, not both waitforexit and exit)
- Shimgen add in path validation

## License

`SPDX-License-Identifier: MIT OR Unlicense`

# ShimGen 
ShimGen generates an executable '[shim](https://en.wikipedia.org/wiki/Shim_(computing))' that will in turn execute another file relative to its location. This is an attempt at a drop in replacement for the RealDimensions Softwarem LLC's (RDS) `shimgen.exe` that comes with Chocolatey, however has various other possible uses. The shims and generator have evolved from [kiennq's c++ shim implementation](https://github.com/kiennq/scoop-better-shimexe) for Scoop (see the history section for more info)

### Features
  - :exclamation: Executes GUI applications without the brief console window display
  - Free and Open Source
  - Able to be freely used outside of "official" Chocolatey builds without a paid license.
  - Faster shim creation
  - Better support for `crtl+c`, as shim passes signal to child process
  - Tries to terminate child processes if parent process is killed
  - Consistent checksum for all shims
  - No use of `csc.exe` by `shimgen.exe`
  - Supports icons for the shims

### To Do
  - Not thoroughly tested (yet)
  - Add support for extra shim options/arguments
  - Add changing icon
  - More testing
  - Add support to embed working directory into shim
  - Add option to create Scoop-style shims (`.shim` file next to each shim `.exe`)
  - **Shim**: add error checking
    - working directory exists
    - ~not both waitforexit and exit~
    - app path
  - **ShimGen**: add in path validation
  - **ShimGen**: add silent / less verbose execution
  - **ShimGen**: protection from creating a shim equal to the source thus overwriting the executable

## Using 
1. Download a release or build it yourself
2. Run `shimgen.exe <source>`

This will create an executable in the current directory named the same as `<source>` that will in turn execute it. More options can be viewed via `shimgen.exe -h`. The shim itself has additional options and can be viewed using `--shimgen-help`.

You *should* be able to replace `shimgen.exe` in your Chocolately source code tree, however it hasn't been thoroughly tested and will probably void your warranty. If you do, don't forget to update the `shimgen.license.txt` as well.


### Building
1. Get a copy of [Visual Studio](https://visualstudio.microsoft.com/downloads/). Installing just the "Desktop Development w/ C++" should suffice however the minimum I installed was:
    - Microsoft Visual Studio Workload - Core Editor
    - Microsoft Visual Studio Component - Core Editor
    - Microsoft Visual Studio Workload - Native Desktop
    - Microsoft Visual Studio Component Group - Native Desktop Core
    - Microsoft Visual Studio Component - Roslyn Compiler
    - Microsoft Visual Studio Component - Text Templating
    - Microsoft Visual Studio Component - VC Core IDE
    - Microsoft Visual Studio Component - VC Tools x86 x64
    - Microsoft Visual Studio Component - VC Redist 14 Latest 
    - Microsoft Visual Studio Component - Windows 11 SDK 22000
    - Microsoft Component - MSBuild
2. Build either using `nmake` or manually. Visual Studio should have installed `nmake` however other flavors should be able to process the included [makefile](./makefile). Either method, the steps are:
    1. Compile the skeleton shim via `cl.exe @compile_flags shim_win.cpp`
    2. Compile the resource (which requires the shim) via `rc shimgen.rc`
    3. Compile ShimGen with `cl.exe @compile_flags shim_win.cpp shimgen.res`


## Motivation & History
<todo>
<add contributers>

## License

`SPDX-License-Identifier: MIT OR Unlicense`

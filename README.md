# Shim Execuable 
Shim Execuable generates an executable '[shim](https://en.wikipedia.org/wiki/Shim_(computing))' that will in turn execute another file relative to its location. This is an attempt at a drop in replacement for the RealDimensions Softwarem LLC's (RDS) `shimgen.exe` that comes with Chocolatey, however has various other possible uses. The shims and generator have evolved from [kiennq's c++ shim implementation](https://github.com/kiennq/scoop-better-shimexe) for Scoop (see the history section for more info)

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
  - Compatible replacement of `shimgen.exe` (i.e., all documented command arguments are available)

### To Do
  - More testing
  - Impliment changing of icon
  - More testing
  - Add support to embedding working directory into shim
  - Add option to create Scoop-style shims (`.shim` file next to each shim `.exe`)
  

## Using 
1. Download a release or build it yourself
2. Run `shim_exec.exe <source>`

This will create an executable in the current directory named the same as `<source>` that will in turn execute it. More options can be viewed via `shim_exec.exe -h`. The shim itself has additional options and can be viewed using `--shim-help`.

You *should* be able to replace `shimgen.exe` in your Chocolately source code tree, however it hasn't been thoroughly tested and will probably void your warranty. The PowerShell script, [./tools/replace_shimgen.ps1](./tools/replace_shimgen.ps1), can be used to toggle this replacement.


## Building
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
2. Build using `nmake`. Visual Studio should have installed `nmake` however other flavors should be able to process the included [makefile](./makefile).


## Motivation & History
<todo>
<add contributers>

## License

`SPDX-License-Identifier: MIT OR Unlicense`

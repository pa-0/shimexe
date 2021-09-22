Under construction.

Based on [kiennq's c++ shim implementation](https://github.com/kiennq/scoop-better-shimexe)

## Build

- In a Visual Studio command prompt, run:
```
cl.exe /nologo /std:c++17 /DNDEBUG /MT /O2 /GF /GR- /GL shim.cpp
rc shimgen-resources.rc
cl.exe /nologo /std:c++17 /DNDEBUG /MT /O2 /GF /GR- /GL /EHsc shimgen.cpp shimgen-resources.res
```

- Replace `shimgen.exe` in your choco source code tree with the finished binary here. Don't forget to update the `shimgen.license.txt` as well.

## License

`SPDX-License-Identifier: MIT OR Unlicense`

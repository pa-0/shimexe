CPPFLAGS=-nologo -std:c++17 -DNDEBUG -MT -O2 -GF -GR- -GL -EHsc

all: .\bin\shim_executable.exe

shim_win.exe: shim_win.cpp shim_executable.h
	$(CPP) $(CPPFLAGS) $*.cpp
	rm $*.obj

shim_executable.res: shim_executable.rc shim_win.exe
	$(RC) $*.rc

.\bin\shim_executable.exe: shim_executable.cpp shim_executable.h shim_executable.res
	$(CPP) $(CPPFLAGS) -Fe$@ $(*B).cpp $(*B).res
# checksum $@ -t sha256 > $*.sha256
	rm $(*B).obj
	rm shim_win.exe
	rm shim_executable.res

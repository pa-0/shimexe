CPPFLAGS=-nologo -std:c++17 -DNDEBUG -MT -O2 -GF -GR- -GL -EHsc

all: .\bin\shimgen.exe

shim_win.exe: shim_win.cpp shimgen.h
	$(CPP) $(CPPFLAGS) $*.cpp
	rm $*.obj

shimgen.res: shimgen.rc shim_win.exe
	$(RC) $*.rc

.\bin\shimgen.exe: shimgen.cpp shimgen.h shimgen.res
	$(CPP) $(CPPFLAGS) -Fe$@ $(*B).cpp $(*B).res
	checksum $@ -t sha256 > $*.sha256
	rm $(*B).obj
	rm shim_win.exe
	rm shimgen.res

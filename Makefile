CPPFLAGS=/nologo /std:c++17 /DNDEBUG /MT /O2 /GF /GR- /GL /EHsc

all: shimgen.exe

shim_win.exe: shim_win.cpp shimgen.h
	$(CPP) $(CPPFLAGS) $*.cpp
	rm $*.obj

shimgen.res: shimgen.rc shim_win.exe
	$(RC) $*.rc

shimgen.exe: shimgen.cpp shimgen.h shimgen.res
	$(CPP) $(CPPFLAGS) $*.cpp $*.res
	rm $*.obj

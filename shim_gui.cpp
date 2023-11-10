#include "shim.h"

// ------------------------------------------------------------------------- //
// MAIN METHOD                                                               // 
// ------------------------------------------------------------------------- //
int APIENTRY WinMain(
    HINSTANCE hInst, HINSTANCE hInstPrev,
    PSTR cmdline, int cmdshow) {
  LPWSTR *argList;
  int nArgs;
  argList = CommandLineToArgvW(GetCommandLineW(), &nArgs);

  return shim(nArgs, argList);
}
  

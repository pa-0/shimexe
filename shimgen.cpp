#include <windows.h>
#include <string>
#include <vector>
#include <exception>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

using namespace std;

#include "shimgen.h"


// ------------------------- Copy Shim Executable -------------------------- // 
void unpack_shim_to_path(const filesystem::path& path, LPCSTR lpName) {
  DWORD   bytesWritten = 0;
  HANDLE  hFile        = INVALID_HANDLE_VALUE;
  HRSRC   hResource    = FindResource(NULL, lpName, RT_RCDATA);
  HGLOBAL hGlobal      = LoadResource(NULL, hResource);
  size_t  exeSiz       = SizeofResource(NULL, hResource); 
  void*   exeBuf       = LockResource(hGlobal);

  hFile = CreateFileW(
      path.c_str(),
      GENERIC_WRITE | GENERIC_READ, 0, NULL,
      CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  
  if (hFile != INVALID_HANDLE_VALUE) {
    DWORD bytesWritten = 0;
    WriteFile(hFile, exeBuf, exeSiz, &bytesWritten, NULL);
    CloseHandle(hFile);
    cout << "INFO - created shim, "
         << get_utf8(path) << ", from "
         << lpName << endl;
  }
  else {
    throw "ERROR - could not unpack shim\n";
  }

  return;
}


// ---------------------------- Copy Resources ----------------------------- // 
HANDLE hUpdateRes;  // update resource handle

BOOL EnumLangsFunc(HMODULE hModule, LPCTSTR lpType, LPCTSTR lpName,
                   WORD wLang, LONG lParam) {
    
  HRSRC hRes = FindResourceEx(hModule, lpType, lpName, wLang);
  HGLOBAL hResLoad = LoadResource(hModule, hRes);
  LPVOID lpResLock = LockResource(hResLoad);

  UpdateResource(
      hUpdateRes,
      lpType,
      lpName,
      wLang,
      lpResLock,                        // ptr to resource info
      SizeofResource(hModule, hRes));   // size of resource info

  cout << "INFO - copied ";
  
  if (lpType == RT_ICON)
    cout << "ICON";
  else if (lpType == RT_VERSION)
    cout << "VERSION";
  else if (lpType == RT_GROUP_ICON)
    cout << "ICON GROUP";

  cout << " resource ";
  if (!IS_INTRESOURCE(lpName))
    cout << lpName;
  else
    cout << (USHORT)lpName;

  cout << endl;

  return TRUE;
}

BOOL EnumNamesFunc(HMODULE hModule, LPCTSTR lpType, LPTSTR lpName,
                   LONG lParam) {
  EnumResourceLanguages(hModule, lpType, lpName,
                        (ENUMRESLANGPROC)EnumLangsFunc, 0);
  return TRUE;
}

BOOL EnumTypesFunc(HMODULE hModule, LPTSTR lpType, LONG lParam) {
  if(lpType == RT_ICON ||
      lpType == RT_VERSION ||
      lpType == RT_GROUP_ICON)
    EnumResourceNames(
        hModule,
        lpType,
        (ENUMRESNAMEPROC)EnumNamesFunc, 0);
  
  return TRUE;
}

BOOL copy_resources(filesystem::path sourcePath) { 
  HMODULE hExe =
    LoadLibraryExW(sourcePath.c_str(),
                   NULL,
                   LOAD_LIBRARY_AS_DATAFILE);
  
  if (!hExe) {
    cout << "ERROR - could not open "
         <<  '"' + get_utf8(sourcePath) + '"'
         << endl;
    return false;
  }
    
  EnumResourceTypes(hExe, (ENUMRESTYPEPROC)EnumTypesFunc, 0);
  
  BOOL bOut = FreeLibrary(hExe); 
  if (!bOut)
    cout << "ERROR - could not free application library" << endl;

  return bOut;
}


// ----------------------- Adding Arguments to Shim ------------------------ // 
BOOL add_shim_argument (HANDLE resource, LPCSTR name, wstring arg) {
  LPVOID lpArg = (LPVOID)arg.c_str();
  BOOL bUpdate = UpdateResource(
      resource,
      RT_RCDATA,
      name,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
      lpArg,
      arg.size() * sizeof(wchar_t));
  
  if (bUpdate)
    cout << "INFO - added resource "
         << name << " = "
         << get_utf8(arg) << endl;
  else
    cout << "ERROR - failed to add " << name << endl;
  
  return bUpdate;
}


// ----------------------------- Help Message ------------------------------ // 
void run_help() {
  const char *tHelp = R"V0G0N(
Usage:  SHIMGEN [-p PATH] [-o OUTPUT] [-c ARGS] [-i ICON] [--gui] [--debug]
        SHIMGEN PATH [options]
        SHIMGEN -h

ShimGen generates an executable 'shim' that will execute another file relative
to its location. For additional information, execute the shim with
--shimgen-help flag or visit: https://github.com/jphilbert/shimgen.

Options: 
    -?, --help, -h          show this help message and exit
    -p, --path PATH        the path to the executable. Can be relative or
                                absolute. If the first argument is not a path,
                                this argument is required. 
    -o, --output OUTPUT    path to the shim to create. By default, will
                                create a shim in the current directory with
                                the same name as the executable. 
    -c, --command ARGS     extra arguments the shim should pass to the original
                                executable. 
    -i, --iconpath ICON    UNIMPLEMENTED: Path to the executable for resolving
                                the icon. Can be relative or absolute (suggest
                                absolute).
    --gui                   force shim to launch app as GUI executable and it
                                should not wait for it to finish execution.
    --debug                 this instructs ShimGen to write out all messages.
)V0G0N";

  cout << tHelp << endl;
  exit(0);
}


// ------------------------------------------------------------------------- //
// MAIN METHOD                                                               // 
// ------------------------------------------------------------------------- //
int wmain(int argc, wchar_t* argv[], wchar_t* envp[]) {
  if (argc <= 1) {
    cerr
      << "At least one argument is required. Run with -h for more information."
      << endl;
    return 1;
    }
  
  vector<wstring> cmdArgs(argv + 1, argv + argc);
  
  int     exitcode    = 0;
  bool    argsGui     = false;
  bool    argsDebug   = false;
  wstring shimArgs    = L"";
  filesystem::path outputPath, inputPath, iconPath;

  // ----------------------- Parse Command Arguments ----------------------- // 
  if (cmdArgs[0][0] != '-')
    inputPath = cmdArgs[0];

  for (size_t i = !(inputPath.empty());
       i < cmdArgs.size();
       i++) {
    // Help
    if      (get_cmd_arg(cmdArgs, i, L"-?") ||
             get_cmd_arg(cmdArgs, i, L"-h") ||
             get_cmd_arg(cmdArgs, i, L"--help"))
      run_help();
    
    // Force GUI
    else if (get_cmd_arg(cmdArgs, i, L"--gui")) 
      argsGui = true;
    
    // Debug Info
    else if (get_cmd_arg(cmdArgs, i, L"--debug"))
      argsDebug = true;
    
    else if (
    // Get Output Path
        !(   get_cmd_arg(cmdArgs, i, outputPath, L"-o") ||
             get_cmd_arg(cmdArgs, i, outputPath, L"--output") ||
    // Get Input Path
             get_cmd_arg(cmdArgs, i, inputPath, L"-p") ||
             get_cmd_arg(cmdArgs, i, inputPath, L"--path") ||
    // Get Icon Path             
             get_cmd_arg(cmdArgs, i, iconPath, L"-i") ||
             get_cmd_arg(cmdArgs, i, iconPath, L"--iconpath") ||
    // Get Additional Arguments for Application             
             get_cmd_arg(cmdArgs, i, shimArgs, L"-c") ||
             get_cmd_arg(cmdArgs, i, shimArgs, L"--command")))
      
      wcerr << L"WARNING - invalid argument, "
            << cmdArgs[i]
            << ", skipping"
            << endl;
  }


  // Validate Input Path
  exitcode = 1;
  if (inputPath.empty())
    cerr << "ERROR - "
         << "a source executable must be specified."
         << endl;
  else if (!filesystem::exists(inputPath)) 
    cerr << "ERROR - "
         << get_utf8(inputPath)
         << " does not exists."
         << endl;
  else if (!filesystem::is_regular_file(inputPath)) 
    cerr << "ERROR - "
         << get_utf8(inputPath)
         << " is not a regular file."
         << endl;
  else if (inputPath.extension() != ".exe") 
    cerr << "ERROR - "
         << get_utf8(inputPath)
         << " is not an executable"
         << endl;
  else {
    inputPath = filesystem::weakly_canonical(inputPath);
    cout << "INFO - Source Application: "
         << get_utf8(inputPath)
         << endl;
    exitcode = 0;
  }

  if (exitcode != 0)
    return exitcode;

  
  // Validate Output Path
  if (outputPath.empty()) {
    outputPath = filesystem::current_path();
    outputPath /= inputPath.filename();
    cout << "WARNING - output path was not specified, using "
         << get_utf8(outputPath)
         << endl;
  }

  if (filesystem::is_directory(outputPath)) {
    outputPath /= inputPath.filename();
    cout << "WARNING - only output directory was specified, appending "
         << inputPath.filename()
         << endl;
  }

  outputPath = filesystem::weakly_canonical(outputPath);

  if (filesystem::exists(outputPath) &&
      filesystem::equivalent(outputPath, inputPath)) {
    cout << "ERROR - output and input must be different" << endl;
    return 1;
  }
  
  if (!iconPath.empty())
    cout << "WARNING - flags -i --icon not implemented, ignoring" << endl;

  if (argsDebug)
    cout << "WARNING - flag --debug not implemented, ignoring" << endl;

  
  // ------------------------ Unpack / Create Shim ------------------------- // 
  unpack_shim_to_path(outputPath, "SHIM_WIN");


  // --------------- Copy Resources from Source App to Shim ---------------- // 
  hUpdateRes = BeginUpdateResourceW(outputPath.c_str(), FALSE);
  copy_resources(inputPath);    

  
  // ------------------------- Add Shim Arguments -------------------------- //
  add_shim_argument(hUpdateRes, "SHIM_PATH", inputPath);
  
  if (shimArgs != L"") 
    add_shim_argument(hUpdateRes, "SHIM_ARGS", shimArgs);    
  
  if (argsGui) 
    add_shim_argument(hUpdateRes, "SHIM_GUI", L"T");

  EndUpdateResource(hUpdateRes, FALSE);

  
  cout << "ShimGen has successfully created " << get_utf8(outputPath) << endl;
  return exitcode;
}

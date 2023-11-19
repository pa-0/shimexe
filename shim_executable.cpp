#include <windows.h>
#include <vector>
#include <exception>

#include <string>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "SHELL32.LIB")

using namespace std;

#include "shim_resources.h"
#include "shim_executable.h"


// ------------------------- Copy Shim Executable -------------------------- // 
BOOL unpack_shim_to_path(const filesystem::path& path, wstring resName) {
  DWORD   bytesWritten = 0;
  HANDLE  hFile        = INVALID_HANDLE_VALUE;
  HRSRC   hResource    = FindResource(NULL,
                                      get_utf8(resName).c_str(),
                                      RT_RCDATA);
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
    return true;
  }

  return false;
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
    cerr << "ERROR - could not open "
         <<  '"' + get_utf8(sourcePath) + '"'
         << endl;
    return false;
  }
    
  EnumResourceTypes(hExe, (ENUMRESTYPEPROC)EnumTypesFunc, 0);
  
  BOOL bOut = FreeLibrary(hExe); 
  if (!bOut)
    cerr << "ERROR - could not free application library" << endl;

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
  
  if (!bUpdate)
    cerr << "ERROR - failed to add " << name << endl;
  else 
    cout << "INFO - added resource "
         << name << " = "
         << get_utf8(arg) << endl;
  
  return bUpdate;
}


// ----------------------------- Help Message ------------------------------ // 
void run_help() {
  const char *tHelp = R"V0G0N(
    SHIM_EXEC [-h | -?]
    SHIM_EXEC PATH [OUTPUT] [-c ARGS] [-i ICON] [--gui] [--debug]
    SHIM_EXEC -p PATH [...]
    SHIM_EXEC --path=PATH [...]


==============================
INFO
==============================
Generates a 'shim' file that has the sole purpose of executing another file,
similar to a shortcut, yet is a full fledged executable. During creation, the
resources of the source executable such as version info and icons are copied to
the shim. In addition to the source path, specific command line arguments can
be embedded and hence editable by a resource editor (e.g.
https://www.angusj.com/resourcehacker).

One specific option to take note of is denoting if the source application has a
GUI. Typically, this simply denotes if the shim process should end immediately
after starting the process for the source or to wait (these two options are
also selectable in the shim). In either case, the shim, originally built as a
console application, will utilize the current console when executed from the
command line. Otherwise it will spawn a console window to generate the child
process for the source executable. For GUI applications, where waiting is
unneeded, this console is immediately destroyed, albeit still noticeable. To
remedy this adverse effect, the --GUI option generates a shim built with the
GUI subsystem as opposed to the console subsystem. This in effect removes the
creation of a console for GUI source applications.

Shims created with and without the --GUI option still include the options to
wait or exit immediately and for the most part are indistinguishable. One
important yet practical difference is that GUI shims by default will exit
immediately after creating its child process whereas console shims will default
to wait. Of course GUI shims can be called from a console and flagged to wait
which should function similarly to console shims. Issues only arise if GUI
shims are called with any type of console logging turned on (i.e. --shim-help
or --shim-log). The type of CLI (e.g., powershell, cmd.exe, etc.) appears to
impact the output. If run outside of a console, in which case a console would
need to be created for the output stream, the shim instead writes to a
.SHIM.LOG file with the same path.

For additional information, execute the shim with --shim-help flag or visit
https://github.com/jphilbert/shim_executable.


==============================
EXAMPLES
==============================
The following all have the same behavior of creating a shim, APP.EXE, in the
working directory, C:\WORK\DIR:

    SHIM_EXEC --output=app.exe --path=..\app.exe
    
    SHIM_EXEC -o app.exe -p "C:\WORK\DIR\app.exe"
    
    SHIM_EXEC -p C:\WORK\DIR\app.exe
    
    SHIM_EXEC ..\app.exe

You cannot create a shim to itself. This will generate an error:
    SHIM_EXEC app.exe

however this will not:
    SHIM_EXEC app.exe app_shim.exe


==============================
ARGUMENTS
==============================
The application accepts the following arguments and they are not
case-sensitive. Argument flags can be shortened to a single dash and initial
letter (except for --GUI and --DEBUG) and values can be separated by either a
space or equal sign from its flag. Since PATH is required, it need not be
denoted by a flag if it is the first argument. Similarly, if the second
argument is also not denoted by a flag, it will be assumed to be OUTPUT.

    --help              Show this help message and exit.

    --path PATH         The path to the executable to shim. This can be a
                            relative path however it will be converted to an
                            absolute path before creation.

    --output OUTPUT     The path to the shim to create. If only a valid
                            directory is given, the name of the executable is
                            used for the shim. If omitted completely, the
                            current directory with the executable name is used.
                            This cannot be equal to PATH.

    --command ARGS      Additional arguments the shim should pass to the
                            original executable automatically. Should be quoted
                            for multiple arguments.

    --iconpath ICON     [UNIMPLEMENTED] Path to a file to use for the shim's
                            icon. By default, the executable's icon resources
                            are used.

    --gui               Explicitly sets shim to be created using the GUI or
    --console               console subsystem. GUI shims exit as soon as the
                            child process for the executable is created where
                            as console shims will wait. If neither is set, by
                            default the subsystem will be infered by the
                            executable, thus these options likely would be need
                            only for special cases.

    --debug             Print additional information when creating the shim to
                            the console.
)V0G0N";
  cout.clear();
  cout << "==============================" << endl
       << "Shim Executable - v" << VER_FILEVERSION_STR << endl
       << "=============================="
       << tHelp << endl;
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
  
  int     exitcode          = 0;
  wstring shimType          = L"";
  wstring shimArgs          = L"";
  bool isShimGen            = false;
  string exeName;
  filesystem::path
    outputPath,
    inputPath,
    iconPath,
    exePath;

  exePath = get_exe_path();
  exeName = exePath.stem().string();
  exePath = exePath.parent_path();
  //   case_insensitive_match(currentPath.stem().string(), "shimgen");
    
  cout.setstate(ios::failbit);  // start with no cout off
  
  // ----------------------- Parse Command Arguments ----------------------- //
  // Check for positional arguments
  if (cmdArgs[0][0] != '-') {
    inputPath = cmdArgs[0];
    if (cmdArgs[1][0] != '-')
      outputPath = cmdArgs[1];
  }

  // Loop over optional arguments (skipping positionals)
  for (size_t i = !(inputPath.empty()) + !(outputPath.empty());
       i < cmdArgs.size();
       i++) {
    // Help
    if      (get_cmd_arg(cmdArgs, i, L"-?") ||
             get_cmd_arg(cmdArgs, i, L"-h") ||
             get_cmd_arg(cmdArgs, i, L"--help"))
      run_help();
    
    // Force GUI
    else if (get_cmd_arg(cmdArgs, i, L"--gui")) 
      shimType = L"GUI";

    // Force Console
    else if (get_cmd_arg(cmdArgs, i, L"--console")) 
      shimType = L"CONSOLE";

    // Debug Info
    else if (get_cmd_arg(cmdArgs, i, L"--debug"))
      cout.clear();
    
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
      
      wcout << L"WARNING - invalid argument, "
            << cmdArgs[i]
            << ", skipping"
            << endl;
  }

  // ------------------------- Validate Arguments -------------------------- //
  
  // ---------- Input Path ---------- // 
  DWORD execType;
  exitcode = 1;
  
  // Check if EMPTY
  if (inputPath.empty()) {
    cerr << "ERROR - "
         << "a source executable must be specified."
         << endl;
    return exitcode;
  }


  // Check if EXIST
  exitcode = 0;
  if (!filesystem::exists(inputPath)) {
    exitcode = 1;
    
    if (inputPath.is_relative()) {
      isShimGen = true;
      cout << "INFO - path is relative" << endl;
      cout << "INFO - expanding from current path, "
           << get_utf8(filesystem::current_path()) << endl;
      cout << "WARNING - file, "
           << get_utf8(filesystem::weakly_canonical(inputPath))
           << ", does not exists." << endl;

      cout << "INFO - expanding from module path, "
           << get_utf8(exePath) << endl;

      inputPath = exePath / inputPath;    
      if (filesystem::exists(inputPath)) 
        exitcode = 0;
    }
  }

  inputPath = filesystem::weakly_canonical(inputPath);

  if (exitcode > 0) {
    cerr << "ERROR - target path, "
         << get_utf8(inputPath) << ", does not exists." << endl;
    return exitcode;
  }
  
  // Check if its a REGULAR FILE
  exitcode = 1;
  if (!filesystem::is_regular_file(inputPath)) {
    cerr << "ERROR - file, "
         << get_utf8(inputPath.filename())
         << ", is not a regular file."
         << endl;
    return exitcode;
  }

  
  execType = SHGetFileInfoW(inputPath.c_str(), NULL, NULL, NULL,
                            SHGFI_EXETYPE);

  // Check if EXECUTABLE
  if (execType == 0) {
    cerr << "ERROR - file, "
         << get_utf8(inputPath.filename())
         << ", is not an executable"
         << endl;
    return exitcode;
  }

  
  cout << "INFO - Source Application: "
       << get_utf8(inputPath)
       << endl;

  cout << "INFO -     Type: ";
    
  if (HIWORD(execType) != 0)
    cout << "Windows GUI application";
  else if (LOWORD(execType) == 0x5A4D)
    cout << "MS-DOS application";
  else
    cout << "Windows Console application (or .bat)";    
  cout << endl;

  cout << "INFO - Shim Type ";
  if (shimType == L"") {
    if (HIWORD(execType) != 0)
      shimType = L"GUI";
    else
      shimType = L"CONSOLE";
    cout << "implicitly set to " << get_utf8(shimType)
         << " (auto detected)" << endl;
  }
  else
    cout << "explicitly set to " << get_utf8(shimType)
         << " (command argument)" << endl;

  
  
  // ---------- Output Path ---------- // 
  // Check if EMPTY
  if (outputPath.empty()) {
    outputPath = filesystem::current_path();
    outputPath /= inputPath.filename();
    cout << "WARNING - output path was not specified, using "
         << get_utf8(outputPath)
         << endl;
  }

  // SHIMGEN is more strict and expands relative paths from the application 
  if (isShimGen) {
    // assume OUTPUTPATH is valid and a file regardless
    cout << "INFO - taking output path verbatim, ";

    // if OUTPUTPATH is relative expand from exe path
    if (outputPath.is_relative()) {
      outputPath = exePath / outputPath;
      cout << "expanding from module path, ";
    }

    outputPath = filesystem::weakly_canonical(outputPath);
    cout << get_utf8(outputPath) << "." << endl;
  }
  else
    if (filesystem::is_directory(outputPath)) {
      outputPath /= inputPath.filename();
      cout << "WARNING - only output directory was specified, appending "
           << inputPath.filename()
           << endl;
    }

  outputPath = filesystem::weakly_canonical(outputPath);

  // Check if output directory EXISTS
  if (!outputPath.parent_path().empty() &&
      !filesystem::is_directory(outputPath.parent_path())) {
    cerr << "ERROR - output directory, " 
         << outputPath.parent_path()
         << ", does not exist"
         << endl;
    return exitcode;
  }

  // Check if output path EQUALS input path
  if (filesystem::exists(outputPath) &&
           filesystem::equivalent(outputPath, inputPath)) {
    cerr << "ERROR - output and input must be different" << endl;
    return exitcode;
  }
  
  // ---------- Icon Path ---------- // 
  if (!iconPath.empty())
    cout << "WARNING - flags -i --icon not implemented, ignoring" << endl;

  
  // ------------------------ Unpack / Create Shim ------------------------- // 
  // unpack_shim_to_path(outputPath, "SHIM_CONSOLE");
  if (!unpack_shim_to_path(outputPath, L"SHIM_" + shimType)) {
    cerr << "ERROR - could not unpack shim" << endl;
    return 1;
  }
  
  cout << "INFO - created shim, "
       << get_utf8(outputPath.filename()) << ", from "
       << "SHIM_" << get_utf8(shimType) <<".EXE" << endl;


  // --------------- Copy Resources from Source App to Shim ---------------- // 
  hUpdateRes = BeginUpdateResourceW(outputPath.c_str(), FALSE);
  copy_resources(inputPath);    

  
  // ------------------------- Add Shim Arguments -------------------------- //
  add_shim_argument(hUpdateRes, "SHIM_PATH", inputPath);
  
  if (shimArgs != L"") 
    add_shim_argument(hUpdateRes, "SHIM_ARGS", shimArgs);    
  
  add_shim_argument(hUpdateRes, "SHIM_TYPE", shimType);

  EndUpdateResource(hUpdateRes, FALSE);

  cout.clear();
  cout << exeName
       << " has successfully created '"
       << get_utf8(outputPath) << "'" << endl;
  return exitcode;
}

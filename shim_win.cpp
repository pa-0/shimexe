#pragma comment(lib, "SHELL32.LIB")
#include <shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")
#include <string>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <optional>
#include <numeric>

using namespace std;

#include "shimgen.h"

#ifndef ERROR_ELEVATION_REQUIRED
#define ERROR_ELEVATION_REQUIRED 740
#endif

// --------------------------- Process Creation ---------------------------- // 
BOOL WINAPI CtrlHandler(DWORD ctrlType) {
  switch (ctrlType) {
    // Ignore all events, and let the child process
    // handle them.
  case CTRL_C_EVENT:
  case CTRL_CLOSE_EVENT:
  case CTRL_LOGOFF_EVENT:
  case CTRL_BREAK_EVENT:
  case CTRL_SHUTDOWN_EVENT:
    return TRUE;

  default:
    return FALSE;
  }
}

struct HandleDeleter {
  typedef HANDLE pointer;
  void operator()(HANDLE handle) {
    if (handle) {
      CloseHandle(handle);
    }
  }
};

namespace std {
  typedef unique_ptr<HANDLE, HandleDeleter> unique_handle;
  typedef optional<wstring> wstring_p;
}

tuple<unique_handle, unique_handle> MakeProcess(
    const wstring_p& path,
    const wstring_p& args,
    const wstring& workingDirectory) {
  STARTUPINFOW        startInfo   = {};
  PROCESS_INFORMATION processInfo = {};
  unique_handle       threadHandle;
  unique_handle       processHandle;
  
  // Set the Command Line
  vector<wchar_t> cmd(path->size() + args->size() + 2);
  wmemcpy(cmd.data(), path->c_str(), path->size());
  cmd[path->size()] = L' ';
  wmemcpy(cmd.data() + path->size() + 1, args->c_str(), args->size());
  cmd[path->size() + 1 + args->size()] = L'\0';

  // Set the Working Directory
  LPCWSTR workingDirectoryCSTR = nullptr;
  if (workingDirectory != L"") {
      workingDirectoryCSTR = workingDirectory.c_str();

      if (!PathFileExistsW(workingDirectoryCSTR)) {
        cout <<
          "Working directory does not exist, process may fail to start" <<
          endl;
      }
  }

  // Create the Process
  if (CreateProcessW(
          nullptr,                 // No module name (use command line)       
          cmd.data(),              // Command Line
          nullptr, nullptr, TRUE,  // Inheritance (Process, Thread, Handle)
          CREATE_SUSPENDED,        // Suspend threads on creation
          nullptr,                 // Use parent's environment block          
          workingDirectoryCSTR,    // Starting directory         
          &startInfo, &processInfo)) {
    // Set the handles
    threadHandle.reset(processInfo.hThread);
    processHandle.reset(processInfo.hProcess);
    // Start the thread
    ResumeThread(threadHandle.get());
  }
  else if (GetLastError() == ERROR_ELEVATION_REQUIRED) {
    // We must elevate the process, which is (basically) impossible with
    // CreateProcess, and therefore we fallback to ShellExecuteEx, which CAN
    // create elevated processes, at the cost of opening a new separate
    // window.  Theoretically, this could be fixed (or rather, worked around)
    // using pipes and IPC, but... this is a question for another day.

    SHELLEXECUTEINFOW sei = {};

    sei.cbSize = sizeof(SHELLEXECUTEINFOW);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpFile = path->c_str();
    sei.lpParameters = args->c_str();
    sei.nShow = SW_SHOW;
    sei.lpDirectory = workingDirectoryCSTR;

    if (!ShellExecuteExW(&sei)) {
      cout << "Unable to create elevated process: error "
           << GetLastError() << endl;
      return {move(processHandle), move(threadHandle)};
    }

    processHandle.reset(sei.hProcess);
  }
  else {
    cout << "Could not create process with command: " << endl
         << "  " << cmd.data() << endl;
    return {move(processHandle), move(threadHandle)};
  }

  // Ignore Ctrl-C and other signals
  if (!SetConsoleCtrlHandler(CtrlHandler, TRUE)) 
    cout << "Could not set control handler; Ctrl-C behavior may be invalid"
         << endl;

  return {move(processHandle), move(threadHandle)};
}


// ----------------------------- Logging Setup ----------------------------- //
// I'm not content with this but it works for now
BOOL hasConsole = FALSE;
static ofstream outStream;

void setup_stream() {
  hasConsole = AttachConsole(ATTACH_PARENT_PROCESS);
  // NOTE: Issue with AttachConsole
  //   Everything is normal, the output of cmd.exe and your program get
  //   arbitrarily intermingled. Cmd.exe got first, that's common, it displayed
  //   the command prompt before your program output got a chance. It has no
  //   reason to display it again unless you press the Enter key. It gets a lot
  //   worse if your program reads from the console. You'll have to start your
  //   app with start /wait yourapp.exe to tell cmd.exe to wait. Otherwise a
  //   good demonstration why AttachConsole() is such a bad idea.
  //    - Hans Passant Jun 11, 2021 at 16:22
  if(hasConsole) {
    outStream = ofstream("CONOUT$", ios::out);
    outStream << endl;
  }
  else {
    wchar_t applicationPath[MAX_PATH];
    const auto applicationPathSize = GetModuleFileNameW(
        nullptr, applicationPath, MAX_PATH);
  
    filesystem::path logPath = applicationPath;
    logPath.replace_extension(".log");

    outStream.open(logPath);
  }
  
  cout.rdbuf(outStream.rdbuf());
}

void close_console() {
  // Cleanup and Close the Stream
  if(hasConsole) {
    DWORD entityWritten;
    INPUT_RECORD inputs;
    inputs.EventType = KEY_EVENT;
    inputs.Event = { true, 0, 0, 0, VK_RETURN, 0 };
    WriteConsoleInputA(GetStdHandle(STD_INPUT_HANDLE),
                       &inputs, 1, &entityWritten);

    FreeConsole();
  }
  else 
    outStream.close();
}


// ----------------------------- Help Message ------------------------------ // 
void run_help() {
  const char *tHelp = R"V0G0N(
This is an application 'shim' that will execute another application (typically
named the same). Execute with --shimgen-NoOp to identify its target.

Execute shimgen.exe -h or visit https://github.com/jphilbert/shimgen for
additional information.

Options:
    --shimgen-Help          Prints out this help message
    --shimgen-Log           Turns on diagnostic messaging. These will be either
                                sent to the console or file depending on source
                                of execution. For files, they will be generated
                                with the same path (directory and file name)
                                with extension .LOG. 
    --shimgen-WaitForExit   Wait until the program exits to exit the shim
                                (default setting) 
    --shimgen-Exit          Exit the shim when the program starts
    --shimgen-GUI           Force shim to run as a GUI instead of auto-detecting
                                in the program. This will a exit the shim when
                                the program starts unless --shimgen-WaitForExit
                                is set explicitly. 
    --shimgen-UseTargetWorkingDirectory=<directory>
                                Run program from a custom working directory. 
    --shimgen-NoOp          Turns on diagnostic messaging but does not run the
                                shim 
)V0G0N";

  setup_stream();
  cout << tHelp << endl;
  close_console();
  exit(0);
}


// ------------------------------------------------------------------------- //
// MAIN METHOD                                                               // 
// ------------------------------------------------------------------------- //
int APIENTRY WinMain(
    HINSTANCE hInst, HINSTANCE hInstPrev,
    PSTR cmdline, int cmdshow) {

  // --------------------- Get Command Line Arguments ---------------------- // 
  LPWSTR *argv;
  int argc;
  
  argv = CommandLineToArgvW(GetCommandLineW(), &argc);
  vector<wstring> cmdArgs(argv + 1, argv + argc);

  DWORD   exitCode                = 0;
  bool    isWindowsApp            = false;
  bool    shimArgLog              = false;
  bool    shimArgNoop             = false;
  bool    shimArgExit             = false;
  bool    shimArgWait             = false;
  wstring shimArgWorkingDirectory = L"";
  TCHAR   shimPath[MAX_PATH];
  
  GetModuleFileName(nullptr, shimPath, MAX_PATH);

  for (size_t i = 0; i < cmdArgs.size(); i++) {
    if      (get_cmd_arg(cmdArgs, i, L"--shimgen-help"))
      run_help();
    else if (get_cmd_arg(cmdArgs, i, L"--shimgen-gui"))
      isWindowsApp      = true;
    else if (get_cmd_arg(cmdArgs, i, L"--shimgen-log"))
      shimArgLog        = true;
    else if (get_cmd_arg(cmdArgs, i, L"--shimgen-noop"))
      shimArgNoop       = true;
    else if (get_cmd_arg(cmdArgs, i, L"--shimgen-exit"))
      shimArgExit       = true;
    else if (get_cmd_arg(cmdArgs, i, L"--shimgen-waitforexit"))
      shimArgWait       = true;
    else
      get_cmd_arg(cmdArgs, i, shimArgWorkingDirectory,
                    L"--shimgen-usetargetworkingdirectory");
  }    

  if (shimArgNoop)
    shimArgLog = true;

  if (shimArgLog) {
    setup_stream();
    cout << "---------- Shim Logging ----------" << endl;
    cout << "Shim Path: " << shimPath << endl << endl;

    cout << "Command Line Arguments"  << endl;
    cout << "  GUI:     " << isWindowsApp << endl; 
    cout << "  Log:     " << shimArgLog   << endl; 
    cout << "  NoOp:    " << shimArgNoop  << endl;
    cout << "  Exit:    " << shimArgExit  << endl; 
    cout << "  Wait:    " << shimArgWait  << endl; 
    cout << "  Working Directory: "
         << get_utf8(shimArgWorkingDirectory).c_str()
         << endl; 
  }
  
  // ------------------------- Get Shim Arguments -------------------------- // 
  wstring appPath = L"";
  wstring appArgs = L"";
  exitCode = 1;
  
  if (!get_shim_info(appPath, "SHIM_PATH")) 
    cout << "ERROR - Shim has no application path. ";
  else if (!filesystem::exists(appPath)) 
    cout << "ERROR - Shim application path does not exist. ";
  else if (filesystem::equivalent(shimPath, appPath))
    cout << "ERROR - Shim points to itself. ";
  else
    exitCode = 0;

  if (exitCode == 1) {
    cout << "Regenerate shim with ShimGen." << endl;
    close_console();
    return exitCode;
  }

  get_shim_info(appArgs, "SHIM_ARGS");

  if (!isWindowsApp)
    isWindowsApp = get_shim_info("SHIM_GUI");

  if (shimArgLog) {
    cout << endl << "Embedded Arguments" << endl;
    cout << "  appPath:      " << get_utf8(appPath).c_str() << endl; 
    cout << "  appArgs:      " << get_utf8(appArgs).c_str() << endl; 
    cout << "  isWindowsApp: " << isWindowsApp              << endl << endl; 
  }

  // --------------------- Final Parameter Validation ---------------------- // 
  if (shimArgExit && shimArgWait) {
    cout
      << "Both --SHIMGEN-WAITFOREXIT and --SHIMGEN-EXIT cannot "
      << "be passed at the same time"
      << endl;
    close_console();
    return 1;
  }

  // Check if its *actually* a Windows application
  if (!isWindowsApp) {
    SHFILEINFOW sfi = {};
    isWindowsApp = HIWORD(
        SHGetFileInfoW(
            appPath.c_str(),
            -1,
            &sfi,
            sizeof(sfi),
            SHGFI_EXETYPE)) != 0;
    
    if (shimArgNoop && isWindowsApp) 
      cout
        << "Identified to be a Windows application, "
        << "changing isWindowsApp to TRUE"
        << endl;
  }

  // Append additional arguments
  if (!cmdArgs.empty()) {
    // Add command line arguments
    appArgs.append(
        L" " +
        accumulate(next(cmdArgs.begin()),
                   cmdArgs.end(),
                   cmdArgs[0],
                   [](wstring a, wstring b) {
                          return a + L" " + b;
                        }));
  }

  if (shimArgLog) 
    cout << "Full Arg List:      "
         << get_utf8(appArgs).c_str()
         << endl;
  
  if (shimArgNoop) {
    cout << "---------- Shim Exiting: NoOp ----------" << endl;
    close_console();
    return 0;
  }

  if (isWindowsApp)
    FreeConsole();

  
  // ----------------------------- Execute App ----------------------------- //
  cout << "Creating process for application" << endl;

  auto [processHandle, threadHandle] =
    MakeProcess(move(appPath), move(appArgs), shimArgWorkingDirectory);
  
  exitCode = processHandle ? 0 : 1;

  // Wait for app to finish when
  //    --> No flags
  //    --> Wait flag
  if (processHandle &&
      ((!isWindowsApp && !shimArgWait && !shimArgExit) ||
       shimArgWait)) {
    
    cout << "Waiting for process to complete" << endl;
    
    // A job object allows groups of processes to be managed as a unit.
    // Operations performed on a job object affect all processes associated with
    // the job object. Specifically here we attach to child processes to make
    // sure they terminate when the parent terminates as well.   
    unique_handle jobHandle(CreateJobObject(nullptr, nullptr));
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION jobInfo = {};

    jobInfo.BasicLimitInformation.LimitFlags = 
      JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE |
      JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK;
    
    SetInformationJobObject(
      jobHandle.get(),
      JobObjectExtendedLimitInformation,
      &jobInfo,
      sizeof(jobInfo));
    
    AssignProcessToJobObject(jobHandle.get(), processHandle.get());
    
    // Wait till end of process
    WaitForSingleObject(processHandle.get(), INFINITE);

    // Get the exit code
    GetExitCodeProcess(processHandle.get(), &exitCode);
  }

  cout << "---------- Shim Exiting: " << exitCode << " ----------" << endl;
  return exitCode;
}

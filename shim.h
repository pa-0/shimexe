#include <optional>
#include <numeric>

#include <string>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "SHELL32.LIB")

using namespace std;

#ifndef ERROR_ELEVATION_REQUIRED
#define ERROR_ELEVATION_REQUIRED 740
#endif

#define BUFSIZE 4096

#include "shim_resources.h"
#include "shim_executable.h"

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
  STARTUPINFOW        startInfo     = {};
  PROCESS_INFORMATION processInfo   = {};
  unique_handle       threadHandle;
  unique_handle       processHandle;

  // Set the Command Line
  wstring cmd = path->c_str();
  cmd += L" ";
  cmd += args->c_str();
  
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
static ofstream outStream;
DWORD consoleInfo = 0;
// 0 - unchecked
// 1 - console app                  - write to console
// 2 - windows app  - found console - write to console
// 3 - windows app  - no console    - write to file

void open_logging_stream() {
  if(consoleInfo > 0)
    return;
  
  if(AttachConsole(ATTACH_PARENT_PROCESS)) {
    consoleInfo = 2;
    outStream = ofstream("CONOUT$", ios::out);
    outStream << endl;
  }
  else if(GetLastError() == ERROR_INVALID_HANDLE) {
    consoleInfo = 3;
    wchar_t applicationPath[MAX_PATH];
    const auto applicationPathSize = GetModuleFileNameW(
        nullptr, applicationPath, MAX_PATH);
  
    filesystem::path logPath = applicationPath;
    logPath.replace_extension(".shim.log");
    outStream.open(logPath, ios::out | ios::trunc);
  }
  else {
    consoleInfo = 1;
    return;
  };
  
  cout.rdbuf(outStream.rdbuf());
}

void close_logging_stream() {
  // Cleanup and Close the Stream
  switch(consoleInfo) {
  case 2:
    // Mimics a console application to some extent...
    // ...from cmd.exe console, appears to add an extra new line
    // ...from powershell.exe, does not move the cursor (returns to position
    // prior to any output)
    DWORD entityWritten;
    INPUT_RECORD inputs;
    inputs.EventType = KEY_EVENT;
    inputs.Event.KeyEvent.bKeyDown = TRUE;
    inputs.Event.KeyEvent.dwControlKeyState = 0;
    inputs.Event.KeyEvent.uChar.UnicodeChar = '\r';
    inputs.Event.KeyEvent.wRepeatCount = 1;
    // inputs.Event.KeyEvent.wVirtualKeyCode = VK_RETURN;
    // inputs.Event.KeyEvent.wVirtualScanCode = MapVirtualKey(VK_RETURN, 0);
    WriteConsoleInput(GetStdHandle(STD_INPUT_HANDLE),
                       &inputs, 1, &entityWritten);
    
    FreeConsole();
    break;
    
  case 3:
    outStream.close();
    break;
  }
}


// ----------------------------- Help Message ------------------------------ // 
void run_help() {
  const char *tHelp = R"V0G0N(
==============================
INFO
==============================
This is an application 'shim' that will execute another application (typically
named the same located elsewhere). Execute with --shim-NoOp to identify its
target.

Execute SHIM_EXECUTABLE -h or visit
https://github.com/jphilbert/shim_executable for additional information.

==============================
ARGUMENTS
==============================
The following arguments can be passed to a shim and are not case-sensitive.
Each have an equivilent shimgen alias for Chocolately compatibility.

    --shim-Help     Shows this help menu and exits without running the target

    --shim-Log      Turns on diagnostic messaging in the console. If a windows
                        application executed without a console, a file (<shim
                        path>.SHIM.LOG) will be generated instead.
                        (alias --shimgen-log)

    --shim-Wait     Explicitly tell the shim to wait for target to exit. Useful
                        when something is calling a GUI and wanting to block
                        command line programs. This is the default behavior
                        unless the shim was created with the --GUI flag. Cannot
                        be used with --shim-Exit or --shim-GUI.
                        (alias --shimgen-waitforexit)

    --shim-Exit     Explicitly tell the shim to exit immediately after creating
                        the application process. This is the default behavior
                        when the shim was created with the --GUI flag. Cannot
                        be used with --shim-Wait.
                        (alias --shimgen-exit)

    --shim-GUI      Explicitly behave as if the target is a GUI application.
                        This is helpful in situations where the package did not
                        have a proper .gui file. This technically has the same
                        effect as --shim-Exit and is kept for legacy purposes.
                        (alias --shimgen-gui)

    --shim-Dir DIRECTORY
                    Set the working directory to <directory> or the target path
                        if omitted. By default the application will start in
                        the have the current drive and directory as the calling
                        process. Useful when programs need to be running from
                        where they are located (usually indicates programs that
                        have issues being run globally).
                        (alias --shimgen-UseTargetWorkingDirectory)

    --shim-NoOp     Executes the shim without calling the target application.
                        Logging is implicitly turned on.
                        (alias --shimgen-noop)
)V0G0N";

  open_logging_stream();
  cout << tHelp << endl;
  close_logging_stream();
  exit(0);
}


// ----------------------------- Main Function ----------------------------- // 
int shim(int argc, wchar_t* argv[]) {
    // --------------------- Get Command Line Arguments ---------------------- // 
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
    if      (get_cmd_arg(cmdArgs, i, L"--shim-help") ||
             get_cmd_arg(cmdArgs, i, L"--shimgen-help")) 
      run_help();
    else if (get_cmd_arg(cmdArgs, i, L"--shim-gui") ||
             get_cmd_arg(cmdArgs, i, L"--shimgen-gui"))
      isWindowsApp      = true;
    else if (get_cmd_arg(cmdArgs, i, L"--shim-log") ||
             get_cmd_arg(cmdArgs, i, L"--shimgen-log"))
      shimArgLog        = true;
    else if (get_cmd_arg(cmdArgs, i, L"--shim-noop") ||
             get_cmd_arg(cmdArgs, i, L"--shimgen-noop"))
      shimArgNoop = shimArgLog = true;
    else if (get_cmd_arg(cmdArgs, i, L"--shim-exit") ||
             get_cmd_arg(cmdArgs, i, L"--shimgen-exit"))
      shimArgExit       = true;
    else if (get_cmd_arg(cmdArgs, i, L"--shim-wait") ||
             get_cmd_arg(cmdArgs, i, L"--shimgen-waitforexit") )
      shimArgWait       = true;
    else
      get_cmd_arg(cmdArgs, i, shimArgWorkingDirectory,
                  L"--shim-dir") ||
      get_cmd_arg(cmdArgs, i, shimArgWorkingDirectory,
                  L"--shimgen-usetargetworkingdirectory");
  }    

  if (shimArgLog) {
    open_logging_stream();
    cout << "---------- Shim Info ----------" << endl;
    cout << "Shim Path: "  << endl;
    cout << "  " << shimPath << endl;
    cout << "  " << "built using Shim Executable - v"
         << VER_FILEVERSION_STR << endl << endl;

    cout << "Shim CL Parameters:" << endl;
    cout << "  GUI:          " << isWindowsApp << endl; 
    cout << "  Log:          " << shimArgLog   << endl; 
    cout << "  NoOp:         " << shimArgNoop  << endl;
    cout << "  Exit:         " << shimArgExit  << endl; 
    cout << "  Wait:         " << shimArgWait  << endl; 
    cout << "  Working Directory: " << endl; 
    cout << "    " << get_utf8(shimArgWorkingDirectory).c_str()
         << endl << endl;
  }

  shimArgExit = shimArgExit || isWindowsApp;

  if (shimArgExit && shimArgWait) {    
    open_logging_stream();
    cout << "ERROR - SHIM-WAIT cannot be used with SHIM-EXIT or SHIM-GUI"
         << endl;
    close_logging_stream();
    return 1;
  }

  
  // ------------------------- Get Shim Arguments -------------------------- // 
  wstring appPath   = L"";
  wstring appArgs   = L"";
  wstring shimType  = L"";
  exitCode          = 1;
  
  if (!get_shim_info(appPath, "SHIM_PATH")) { 
    open_logging_stream();
    cout << "ERROR - Shim has no application path. ";
    cout << "Regenerate shim." << endl;
    close_logging_stream();
    return exitCode;
  }
  else if (!filesystem::exists(appPath)) {
    open_logging_stream();
    cout << "ERROR - Shim application path does not exist. ";
    cout << "Regenerate shim." << endl;
    close_logging_stream();
    return exitCode;
  }
  else if (filesystem::equivalent(shimPath, appPath)) {
    open_logging_stream();
    cout << "ERROR - Shim points to itself. ";
    cout << "Regenerate shim." << endl;
    close_logging_stream();
    return exitCode;
  }
  else
    exitCode = 0;

  get_shim_info(appArgs, "SHIM_ARGS");

  get_shim_info(shimType, "SHIM_TYPE");

  if (shimArgLog) {
    cout << "Embedded Data" << endl;
    cout << "  appPath:      "
         << '"' << get_utf8(appPath).c_str() << '"' << endl; 
    cout << "  appArgs:      "
         << '"' << get_utf8(appArgs).c_str() << '"' << endl; 
    cout << "  shimType:     "
         << get_utf8(shimType).c_str() << endl << endl; 
  }

  
  // --------------------- Final Parameter Validation ---------------------- // 
  if (shimArgLog) {
    if (shimType == L"CONSOLE") {
      cout <<
        "Console application, waiting for process to finish by default";
      if (shimArgExit) 
        cout << ",\n BUT asked to exit immediately after starting";
    }
    else { 
      cout << "GUI application, exiting immediately by default";
      if (shimArgWait) 
        cout << ",\n BUT asked to wait for process to finish";
    }
    cout << endl << endl;
  }

  if (shimType == L"CONSOLE")
    shimArgWait = !shimArgExit;

  // Append additional arguments
  if (!cmdArgs.empty()) {
    if (!appArgs.empty())
      appArgs.append(L" ");
    
    // Add command line arguments
    appArgs.append(accumulate(next(cmdArgs.begin()),
                              cmdArgs.end(),
                              cmdArgs[0],
                              [](wstring a, wstring b) {
                                return a + L" " + b;
                              }));
  }

  if (shimArgLog) 
    cout << "Full list of arguments passing to application: " << endl
         << "  " << get_utf8(appArgs).c_str() << endl;

  if (shimArgNoop) {
    cout << "---------- Shim Exiting: NoOp ----------" << endl;
    close_logging_stream();
    return 0;
  }

  
  // ----------------------------- Execute App ----------------------------- //
  if (shimArgLog)
    cout << "Creating process for application" << endl;

  auto [processHandle, threadHandle] =
    MakeProcess(move(appPath), move(appArgs), shimArgWorkingDirectory);
  
  exitCode = processHandle ? 0 : 1;

  // Wait for app to finish when
  if (processHandle && shimArgWait) {
    if (shimArgLog)
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

  if (shimArgLog) {
    cout << "---------- Shim Exiting: " << exitCode << " ----------" << endl;
    close_logging_stream();
  }
  
  return exitCode;
}

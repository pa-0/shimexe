#include <corecrt_wstring.h>
#pragma comment(lib, "SHELL32.LIB")
#include <windows.h>
#include <stdio.h>
#include <shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

#include <string>
#include <string_view>
#include <tuple>
#include <optional>
#include <memory>
#include <vector>
#include <cwctype>
#include <algorithm>
#include <iostream>
#include <numeric>

#ifndef ERROR_ELEVATION_REQUIRED
#define ERROR_ELEVATION_REQUIRED 740
#endif

PWCHAR* CommandLineToArgvW(PWCHAR CmdLine, int* _argc) {
  PWCHAR* argv;
  PWCHAR  _argv;
  ULONG   len;
  ULONG   argc;
  WCHAR   a;
  ULONG   i, j;

  BOOLEAN  in_QM;
  BOOLEAN  in_TEXT;
  BOOLEAN  in_SPACE;

  len = wcslen(CmdLine);
  i = ((len+2)/2)*sizeof(PVOID) + sizeof(PVOID);

  argv = (PWCHAR*)GlobalAlloc(GMEM_FIXED,
                              i + (len+2)*sizeof(WCHAR));

  _argv = (PWCHAR)(((PUCHAR)argv)+i);

  argc = 0;
  argv[argc] = _argv;
  in_QM = FALSE;
  in_TEXT = FALSE;
  in_SPACE = TRUE;
  i = 0;
  j = 0;

  while( a = CmdLine[i] ) {
    if(in_QM) {
      if(a == '\"') {
        in_QM = FALSE;
      } else {
        _argv[j] = a;
        j++;
      }
    } else {
      switch(a) {
      case '\"':
        in_QM = TRUE;
        in_TEXT = TRUE;
        if(in_SPACE) {
          argv[argc] = _argv+j;
          argc++;
        }
        in_SPACE = FALSE;
        break;
      case ' ':
      case '\t':
      case '\n':
      case '\r':
        if(in_TEXT) {
          _argv[j] = '\0';
          j++;
        }
        in_TEXT = FALSE;
        in_SPACE = TRUE;
        break;
      default:
        in_TEXT = TRUE;
        if(in_SPACE) {
          argv[argc] = _argv+j;
          argc++;
        }
        _argv[j] = a;
        j++;
        in_SPACE = FALSE;
        break;
      }
    }
    i++;
  }
  _argv[j] = '\0';
  argv[argc] = NULL;

  (*_argc) = argc;
  return argv;
}

void WriteToFile (char *data, wchar_t *filename) {
  HANDLE hFile;
  DWORD dwBytesToWrite = strlen(data);
  DWORD dwBytesWritten ;
  BOOL bErrorFlag = FALSE;

  hFile =
    CreateFileW(filename,               // name of the write
                FILE_APPEND_DATA,       // open for appending
                FILE_SHARE_READ,        // share for reading only
                NULL,                   // default security
                OPEN_ALWAYS,            // open existing file or create new file 
                FILE_ATTRIBUTE_NORMAL,  // normal file
                NULL);                  // no attr. template

  if (hFile == INVALID_HANDLE_VALUE) {
    fprintf(stderr,
            "Terminal failure: Unable to create/open \"%ls\" for writing.\n",
            filename);
    return;
  }

  while (dwBytesToWrite > 0) {
    bErrorFlag =
      WriteFile(
                hFile,              // open file handle
                data,               // start of data to write
                dwBytesToWrite,     // number of bytes to write
                &dwBytesWritten,    // number of bytes that were written
                NULL);              // no overlapped structure

    data += dwBytesWritten;
    dwBytesToWrite -= dwBytesWritten;
  }

  CloseHandle(hFile);
}


BOOL WINAPI CtrlHandler(DWORD ctrlType)
{
    switch (ctrlType)
    {
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

struct HandleDeleter
{
    typedef HANDLE pointer;
    void operator()(HANDLE handle)
    {
        if (handle)
        {
            CloseHandle(handle);
        }
    }
};

namespace std {
typedef unique_ptr<HANDLE, HandleDeleter> unique_handle;
typedef optional<wstring> wstring_p;
} // namespace std

std::tuple<std::wstring_p, std::wstring_p, std::wstring_p> GetShimInfo()
{
    // Find filename of current executable.
    wchar_t filename[MAX_PATH + 2];
    const auto filenameSize = GetModuleFileNameW(nullptr, filename, MAX_PATH);

    if (filenameSize >= MAX_PATH)
    {
        fprintf(stderr, "The filename of the program is too long to handle.\n");
        return {std::nullopt, std::nullopt, std::nullopt};
    }

    // Use filename of current executable to find .shim
    wmemcpy(filename + filenameSize - 3, L"shim", 4U);
    filename[filenameSize + 1] = L'\0';
    FILE* fp = nullptr;

    if (_wfopen_s(&fp, filename, L"r,ccs=UTF-8") != 0)
    {
        fprintf(stderr, "Cannot open shim file for read.\n");
        return {std::nullopt, std::nullopt, std::nullopt};
    }

    std::unique_ptr<FILE, decltype(&fclose)> shimFile(fp, &fclose);

    // Read shim
    wchar_t linebuf[1 << 14];
    std::wstring_p path;
    std::wstring_p args;
    std::wstring_p gui;
    while (true)
    {
        if (!fgetws(linebuf, ARRAYSIZE(linebuf), shimFile.get()))
        {
            break;
        }

        std::wstring_view line(linebuf);

        if ((line.size() < 7) || ((line.substr(4, 3) != L" = ") && (line.substr(3, 3) != L" = ")))
        {
            continue;
        }

        if (line.substr(0, 4) == L"path")
        {
            path.emplace(line.data() + 7, line.size() - 7 - (line.back() == L'\n' ? 1 : 0));
            continue;
        }

        if (line.substr(0, 4) == L"args")
        {
            args.emplace(line.data() + 7, line.size() - 7 - (line.back() == L'\n' ? 1 : 0));
            continue;
        }

        if (line.substr(0, 3) == L"gui")
        {
            gui.emplace(line.data() + 6, line.size() - 6 - (line.back() == L'\n' ? 1 : 0));
            continue;
        }
    }

    return {path, args, gui};
}

std::tuple<std::unique_handle, std::unique_handle> MakeProcess(const std::wstring_p& path, const std::wstring_p& args, const std::wstring& workingDirectory)
{
    // Start subprocess
    STARTUPINFOW si = {};
    PROCESS_INFORMATION pi = {};

    std::vector<wchar_t> cmd(path->size() + args->size() + 2);
    wmemcpy(cmd.data(), path->c_str(), path->size());
    cmd[path->size()] = L' ';
    wmemcpy(cmd.data() + path->size() + 1, args->c_str(), args->size());
    cmd[path->size() + 1 + args->size()] = L'\0';

    std::unique_handle threadHandle;
    std::unique_handle processHandle;

    LPCWSTR workingDirectoryCSTR = nullptr;
    if (workingDirectory != L"")
    {
        workingDirectoryCSTR = workingDirectory.c_str();

        if (!PathFileExistsW(workingDirectoryCSTR)) {
            std::cerr << "Working directory does not exist, process may fail to start\n";
        }
    }

    if (CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, TRUE, CREATE_SUSPENDED, nullptr, workingDirectoryCSTR, &si, &pi))
    {
        threadHandle.reset(pi.hThread);
        processHandle.reset(pi.hProcess);

        ResumeThread(threadHandle.get());
    }
    else
    {
        if (GetLastError() == ERROR_ELEVATION_REQUIRED)
        {
            // We must elevate the process, which is (basically) impossible with
            // CreateProcess, and therefore we fallback to ShellExecuteEx,
            // which CAN create elevated processes, at the cost of opening a new separate
            // window.
            // Theorically, this could be fixed (or rather, worked around) using pipes
            // and IPC, but... this is a question for another day.
            SHELLEXECUTEINFOW sei = {};

            sei.cbSize = sizeof(SHELLEXECUTEINFOW);
            sei.fMask = SEE_MASK_NOCLOSEPROCESS;
            sei.lpFile = path->c_str();
            sei.lpParameters = args->c_str();
            sei.nShow = SW_SHOW;
            sei.lpDirectory = workingDirectoryCSTR;

            if (!ShellExecuteExW(&sei))
            {
                fprintf(stderr, "Unable to create elevated process: error %li.", GetLastError());
                return {std::move(processHandle), std::move(threadHandle)};
            }

            processHandle.reset(sei.hProcess);
        }
        else
        {
            fprintf(stderr, "Could not create process with command '%ls'.\n", cmd.data());
            return {std::move(processHandle), std::move(threadHandle)};
        }
    }

    // Ignore Ctrl-C and other signals
    if (!SetConsoleCtrlHandler(CtrlHandler, TRUE))
    {
        fprintf(stderr, "Could not set control handler; Ctrl-C behavior may be invalid.\n");
    }

    return {std::move(processHandle), std::move(threadHandle)};
}

bool wstring_starts_with(const std::wstring& checkstring, const std::wstring& comparestring)
{
    return ((checkstring.compare(0, comparestring.length(), comparestring)) == 0);
}

template<typename PathType>
void get_arg_value(const std::vector<std::wstring>& args, size_t& currentPos, PathType& argVariable, bool& argFound)
{
    size_t pos = args[currentPos].find('=');

    //If there is not a "=", a space was used, so check next argument in vector
    if (pos == std::wstring::npos)
    {
        size_t nextArg = currentPos + 1;
        // Short circuited check to make sure we are within vector
        if ((nextArg < args.size()) && (args[nextArg][0] != '-'))
        {
            argVariable = args[nextArg];
            argFound = true;

            // Increase iterator as the next arg was used
            currentPos++;
        }
    }
    else
    {
        argVariable = args[currentPos].substr(pos + 1);
        argFound = true;
    }

    return;
}

void run_help()
{
    std::cout << "This is a Scoop Style Shim\n";
    std::cout << "It is a drop in replacement for RealDimensions Software's Shims\n";
    std::cout << "\nAvailable arguments: \n";
    std::cout << "  --shimgen-help           Prints out this message\n";
    std::cout << "  --shimen-log             Not implemented, available for\n";
    std::cout << "                             compatibility with RDS shims\n";
    std::cout << "  --shimgen-waitforexit    Do not exit the shim until the\n";
    std::cout << "                             program exits\n";
    std::cout << "  --shimgen-exit           Exit the shim when the program starts\n";
    std::cout << "  --shimgen-gui            Force shim to run as a GUI instead\n";
    std::cout << "                             of autodetecting in the program\n";
    std::cout << "  --shimgen-usetargetworkingdirectory="
                 "<directory>\n";
    std::cout << "                             Run program from a custom\n";
    std::cout << "                             working directory\n";
    std::cout << "  --shimgen-noop           Do not run the shim\n";
    std::cout << "                             TODO log information to console" << std::endl;
}

int APIENTRY WinMain(HINSTANCE hInst,
                     HINSTANCE hInstPrev,
                     PSTR cmdline,
                     int cmdshow) { 
  LPWSTR *argv;
  int argc;
  int i;

  argv = CommandLineToArgvW(GetCommandLineW(), &argc);

  auto [path, shimArgs, gui] = GetShimInfo();

  std::vector<std::wstring> cmdArgs(argv + 1, argv + argc);

  bool argsLog = false;
  bool isWindowsApp = false;
  bool argsNoop = false;
  bool exitImmediately = false;
  bool waitForExit = false;
  std::wstring targetWorkingDirectory = L"";

  for (size_t i = 0; i < cmdArgs.size(); i++) {
    std::wstring currentArg = cmdArgs[i];
    std::transform(currentArg.begin(),
                   currentArg.end(),
                   currentArg.begin(),
                   std::towlower);
    if (currentArg[0] == '-') {
      if (wstring_starts_with(currentArg, L"--shimgen-help")) {
        run_help();
        return 0;
      }
      else if (wstring_starts_with(currentArg, L"--shimgen-log")) {
        //TODO: NO-OP at the moment, todo is adding in logging
        //get_arg_value(cmdArgs[i], i, outputPath, cmdArgsOutput);
        cmdArgs[i].clear();
      }
      else if (wstring_starts_with(currentArg, L"--shimgen-waitforexit")) {
        waitForExit = true;
        cmdArgs[i].clear();
      }
      else if (wstring_starts_with(currentArg, L"--shimgen-exit")) {
        exitImmediately = true;
        cmdArgs[i].clear();
      }
      else if (wstring_starts_with(currentArg, L"--shimgen-gui")) {
        isWindowsApp = true;
        cmdArgs[i].clear();
      }
      else if (wstring_starts_with(currentArg,
                                   L"--shimgen-usetargetworkingdirectory")) {
        if (wstring_starts_with(currentArg,
                                L"--shimgen-usetargetworkingdirectory=")) {
          targetWorkingDirectory = currentArg.substr(36, currentArg.length());
          cmdArgs[i].clear();
        }
        else {
          // If usetargetworkingdirectory is not used with an equals, then the
          // value will be in thenext argument
          cmdArgs[i].clear();
          // Increment i to consume next argument
          i++;
          targetWorkingDirectory = cmdArgs[i];
          cmdArgs[i].clear();
        }
      }
      else if (wstring_starts_with(currentArg, L"--shimgen-noop")) {
        argsNoop = true;
        cmdArgs[i].clear();
      }
    }
  }

  if (!path) {
    fprintf(stderr, "Could not read shim file.\n");
    return 1;
  }

  if (!shimArgs) {
    shimArgs.emplace();
  }

  if (!gui) {
    gui.emplace();
  }

  if (exitImmediately && waitForExit) {
    std::cerr << "Both waitforexit and exit cannot be passed at the same time\n";
    return 1;
  }

  //todo handle checking if target exists for logging
  if (!cmdArgs.empty()) {
    // Add command line arguments
    shimArgs->append(L" " +
                     std::accumulate(std::next(cmdArgs.begin()),
                                     cmdArgs.end(),
                                     cmdArgs[0],
                                     [](std::wstring a, std::wstring b) {
                                       return a + L" " + b; }));
  }

  if (gui == L"force") {
    isWindowsApp = true;
  }
  else {
    // Find out if the target program is a console app
    SHFILEINFOW sfi = {};
    isWindowsApp = HIWORD(SHGetFileInfoW(path->c_str(),
                                         -1,
                                         &sfi,
                                         sizeof(sfi),
                                         SHGFI_EXETYPE)) != 0;
  }

  if (argsNoop) {
    //TODO, add logging here about noop
    return 0;
  }

  if (isWindowsApp) {
    // Unfortunately, this technique will still show a window for a fraction of
    // time, but there's just no workaround.
    FreeConsole();
  }

  // Create job object, which can be attached to child processes
  // to make sure they terminate when the parent terminates as well.
  std::unique_handle jobHandle(CreateJobObject(nullptr, nullptr));
  JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli = {};

  jeli.BasicLimitInformation.LimitFlags =
    JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE | JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK;
  SetInformationJobObject(jobHandle.get(),
                          JobObjectExtendedLimitInformation,
                          &jeli,
                          sizeof(jeli));

  auto [processHandle, threadHandle] =
    MakeProcess(std::move(path), std::move(shimArgs), targetWorkingDirectory);
  
  if (processHandle && (!isWindowsApp || waitForExit) && !exitImmediately) {
    AssignProcessToJobObject(jobHandle.get(), processHandle.get());
    // Wait till end of process
    WaitForSingleObject(processHandle.get(), INFINITE);
    DWORD exitCode = 0;
    GetExitCodeProcess(processHandle.get(), &exitCode);
    return exitCode;
  }

  return processHandle ? 0 : 1;
}

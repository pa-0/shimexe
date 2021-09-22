#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <exception>
#include <filesystem>
#include <fstream>

#include "shimgen-resources.h"

using namespace std;

void run_help()
{
    cout << "Scoop Style ShimGen creates Scoop Style Shims for Chocolatey\n";
    cout << "It is a drop in replacement for Real Dimensions Software's ShimGen.exe\n";
    cout << "\nAvailable arguments: \n";
    cout << "  -?, -h, --help            Prints out this message\n";
    cout << "* -o, --output=STRING       Path of the shim to be created\n";
    cout << "* -p, --path=STRING         Path to the executable for\n";
    cout << "                              which to create a shim\n";
    cout << "  -i, --iconpath=STRING     Not implemented, available for\n";
    cout << "                              compatibility with RDS ShimGen\n";
    cout << "  -c, --command=STRING      Extra arguments the shim should\n";
    cout << "                              pass to the original executable\n";
    cout << "  --gui                     Not implemented, available for\n";
    cout << "                              compatibility with RDS ShimGen\n";
    cout << "  --debug                   Not implemented, available for\n";
    cout << "                              compatibility with RDS ShimGen\n";
    cout << "\n  Arguments with * are required" << endl;
}

bool wstring_starts_with(const wstring& checkstring, const wstring& comparestring)
{
    return ((checkstring.compare(0, comparestring.length(), comparestring)) == 0);
}

template<typename PathType>
void get_arg_value(const vector<wstring>& args, const size_t& currentPos, PathType& argVariable, bool& argFound)
{
    size_t pos = args[currentPos].find('=');

    //If there is not a "=", a space was used, so check next argument in vector
    if (pos == wstring::npos)
    {
        size_t nextArg = currentPos + 1;
        // Short circuited check to make sure we are within vector
        if ((nextArg < args.size()) && (args[nextArg][0] != '-'))
        {
            argVariable = args[nextArg];
            argFound = true;
        }
    }
    else
    {
        argVariable = args[currentPos].substr(pos + 1);
        argFound = true;
    }

    return;
}

bool validate_path(const wstring& path)
{
    //TODO
    return true;
}

void unpack_shim_to_path(const filesystem::path& path)
{
    DWORD bytesWritten = 0;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    HRSRC hResource = FindResource(NULL, MAKEINTRESOURCE(IDB_EMBEDEXE), MAKEINTRESOURCE(10));
    HGLOBAL hGlobal = LoadResource(NULL, hResource);
    size_t exeSiz = SizeofResource(NULL, hResource); // Size of the embedded data
    void* exeBuf = LockResource(hGlobal);

    hFile = CreateFileW(path.wstring().c_str(), GENERIC_WRITE | GENERIC_READ, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        DWORD bytesWritten = 0;
        WriteFile(hFile, exeBuf, exeSiz, &bytesWritten, NULL);
        CloseHandle(hFile);
    }
    else
    {
        throw "Could not unpack shim.exe file\n";
    }

    return;
}

std::string get_utf8(const std::wstring& wstr)
{
    if (wstr.empty())
        return std::string();
    int sz = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), 0, 0, 0, 0);
    std::string res(sz, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &res[0], sz, 0, 0);
    return res;
}

int wmain(int argc, wchar_t* argv[], wchar_t* envp[])
{
    vector<wstring> args(argv + 1, argv + argc);

    // Find filename of current executable.
    wchar_t filenameWChar[MAX_PATH + 2];
    const auto filenameSize = GetModuleFileNameW(nullptr, filenameWChar, MAX_PATH);

    if (filenameSize >= MAX_PATH)
    {
        throw "The filename of the program is too long to handle.\n";
    }

    filesystem::path shimgenPath(filenameWChar);
    shimgenPath = filesystem::weakly_canonical(shimgenPath);
    filesystem::current_path((filesystem::path(shimgenPath)).remove_filename());

    int exitcode = 0;

    bool argsHelp = false;
    bool argsOutput = false;
    bool argsPath = false;
    bool argsIcon = false;
    bool argsCommand = false;
    bool argsGui = false;
    bool argsDebug = false;

    wstring shimArgs;
    filesystem::path outputPath, inputPath, iconPath;

    for (size_t i = 0; i < args.size(); i++)
    {
        if (args[i][0] == '-')
        {
            if (wstring_starts_with(args[i], L"-h") || wstring_starts_with(args[i], L"-?") || wstring_starts_with(args[i], L"--help"))
            {
                argsHelp = true;
            }
            else if (wstring_starts_with(args[i], L"-o") || wstring_starts_with(args[i], L"--output"))
            {
                get_arg_value(args, i, outputPath, argsOutput);
            }
            else if (wstring_starts_with(args[i], L"-p") || wstring_starts_with(args[i], L"--path"))
            {
                get_arg_value(args, i, inputPath, argsPath);
            }
            else if (wstring_starts_with(args[i], L"-i") || wstring_starts_with(args[i], L"--iconpath"))
            {
                get_arg_value(args, i, iconPath, argsIcon);
            }
            else if (wstring_starts_with(args[i], L"-c") || wstring_starts_with(args[i], L"--command"))
            {
                get_arg_value(args, i, shimArgs, argsCommand);
            }
            else if (wstring_starts_with(args[i], L"--gui"))
            {
                argsGui = true;
            }
            else if (wstring_starts_with(args[i], L"--debug"))
            {
                argsDebug = true;
            }
            else
            {
                wcerr << L"Invalid Argument: " << args[i] << endl;
            }
        }
    }

    if ((argc == 1) || (argsHelp))
    {
        run_help();
    }
    else
    {
        if ((!argsOutput) || (outputPath == ""))
        {
            cerr << "An output path must be specified\n";
            exitcode = 1;
        }
        else if (!validate_path(outputPath))
        {
            cerr << "Invalid output argument\n";
            exitcode = 1;
        }
        else
        {
            outputPath = filesystem::weakly_canonical(outputPath);
        }

        if ((!argsPath) || (inputPath == L""))
        {
            cerr << "An path must be specified\n";
            exitcode = 1;
        }
        else if (!validate_path(inputPath))
        {
            cerr << "Invalid path argument\n";
            exitcode = 1;
        }
        else
        {
            inputPath = filesystem::weakly_canonical(inputPath);
        }

        if (argsIcon)
        {
            cout << "-i --icon not implemented yet, ignoring\n";
            //Edit resources of shim?
            //May be just unsupported as that would change the checksum.
        }

        // Would validate --command here, but no validation to do?

        if (argsGui)
        {
            cout << "--gui not implemented yet, ignoring\n";
            //TODO, add force gui option to shim.cpp?
        }

        if (exitcode == 0)
        {
            filesystem::path outputShimArgsPath = outputPath;
            outputShimArgsPath.replace_extension(".shim");

            unpack_shim_to_path(outputPath);

            ofstream outputShimArgsHandle;
            outputShimArgsHandle.open(outputShimArgsPath, ios::out);
            if (!outputShimArgsHandle.is_open())
            {
                cerr << outputShimArgsPath << " cannot be be opened\n";
                throw "Shim args file cannot be opened";
            }
            else
            {

                outputShimArgsHandle << "path = " << get_utf8(inputPath) << "\n";

                if (shimArgs != L"")
                {
                    outputShimArgsHandle << "args = " << get_utf8(shimArgs);
                }
                outputShimArgsHandle.close();
            }

            cout << "ShimGen has successfully created " << outputPath << endl;
        }
    }

    return exitcode;
}
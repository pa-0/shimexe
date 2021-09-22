#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <exception>
#include <filesystem>

#include "shimgen-resources.h"

using namespace std;

void run_help()
{
    cout << "TODO display help here" << endl;
    cout << "TODO display help here" << endl;
    cout << "TODO display help here" << endl;
}

bool wstring_starts_with(const wstring& checkstring, const wstring& comparestring)
{
    return ((checkstring.compare(0, comparestring.length(), comparestring)) == 0);
}

void get_arg_value(const vector<wstring>& args, const size_t& currentPos, wstring& argVariable, bool& argFound)
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

void unpack_shim(const filesystem::path& path)
{
    if (!filesystem::exists(path))
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
        } else {
            throw "Could not unpack shim.exe file\n";
        }
    }
    else
    {
        //Assume it exists and is valid, debug message here?
        //validate checksum?
    }

    return;
}

int wmain(int argc, wchar_t* argv[], wchar_t* envp[])
{
    vector<wstring> args(argv + 1, argv + argc);

    int exitcode = 0;

    bool argsHelp = false;
    bool argsOutput = false;
    bool argsPath = false;
    bool argsIcon = false;
    bool argsCommand = false;
    bool argsGui = false;
    bool argsDebug = false;

    wstring outputPath, inputPath, iconPath, shimArgs;

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
        if ((!argsOutput) || (outputPath == L""))
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
            // relative output paths based on $env:chocolateyinstall\bin?
            // check and convert to abs here?
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
            // relative input path, where relative too?
            // check and convert to abs here?
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
            filesystem::path shimExePath = filesystem::canonical(filesystem::path(argv[0])).replace_filename("shim.exe");
            filesystem::path shimShimPath = shimExePath.replace_extension(".shim");

            unpack_shim(shimExePath);

            // copy shim to output path

            // open shimShimPath as text file
            // write out inputPath into it
            // write out shimArgs into it
            // close file

            // cout that the shim was created.
        }
    }

    return exitcode;
}
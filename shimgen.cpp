#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <exception>

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

int wmain(int argc, wchar_t* argv[])
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

    wstring output, path, icon, command;

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
                argsOutput = true;
            }
            else if (wstring_starts_with(args[i], L"-p") || wstring_starts_with(args[i], L"--path"))
            {
                argsOutput = true;
            }
            else if (wstring_starts_with(args[i], L"-i") || wstring_starts_with(args[i], L"--iconpath"))
            {
                argsOutput = true;
            }
            else if (wstring_starts_with(args[i], L"-c") || wstring_starts_with(args[i], L"--command"))
            {
                argsOutput = true;
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

    run_help();

    return exitcode;
}
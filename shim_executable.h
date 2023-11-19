// ------------------------------------------------------------------------- //
// General Methods Used Across the Apps                                      // 
// ------------------------------------------------------------------------- //

// Convert Wide --> Narrow
string get_utf8(const wstring& wstr) {
  if (wstr.empty())
    return string();
  
  int sz = WideCharToMultiByte(
      CP_UTF8, 0, &wstr[0], (int)wstr.size(),
      0, 0, 0, 0);
  
  string res(sz, 0);
  WideCharToMultiByte(
      CP_UTF8, 0, &wstr[0], (int)wstr.size(),
      &res[0], sz, 0, 0);
  
    return res;
}

// Check if the beginning of string (A) contains string (B)
// EXAMPLE: (BBBaaaa,   BBB) --> T
// EXAMPLE: (Baaaa,     BBB) --> F
bool wstring_starts_with(const wstring& strA, const wstring& strB) {
  return ((strA.compare(0, strB.length(), strB)) == 0);
}


bool compare_insensitive(string s1, string s2) {
   //convert s1 and s2 into lower case strings
   transform(s1.begin(), s1.end(), s1.begin(), ::tolower);
   transform(s2.begin(), s2.end(), s2.begin(), ::tolower);
   return s1.compare(s2) == 0;
}

filesystem::path get_exe_path() {
  TCHAR   cExePath[MAX_PATH];  
  GetModuleFileName(NULL, cExePath, MAX_PATH);
  return filesystem::path(cExePath);
}  

// ---------------------- Get Command Line Arguments ----------------------- // 
bool get_cmd_arg(
    vector<wstring>& args,
    size_t& currentPos,
    const wstring& argString) {

  wstring currentArg = args[currentPos];
  transform(currentArg.begin(),
            currentArg.end(),
            currentArg.begin(),
            towlower);

  if (wstring_starts_with(currentArg, argString)) {
    args[currentPos].clear();
    return true;
  }

  return false;
}

template<typename T>
bool get_cmd_arg(
    vector<wstring>& args,
    size_t& currentPos,
    T& argVariable,
    const wstring& argString) {

  wstring currentArg = args[currentPos];
  if (!get_cmd_arg(args, currentPos, argString))
    return false;

  size_t pos = currentArg.find('=');

  //If there is not a "=", a space was used, so check next argument in vector
  if (pos == std::wstring::npos) {
    size_t nextArg = currentPos + 1;
    
    // Short circuited check to make sure we are within vector
    if ((nextArg < args.size()) && (args[nextArg][0] != '-')) {
      argVariable = args[nextArg];
    
      // Increase iterator as the next arg was used
      currentPos++;
      args[currentPos].clear();
    }
  }
  else {
    argVariable = currentArg.substr(pos + 1);
  }

  return true;
}


// ----------------------- Methods to Read Resources ----------------------- // 
bool get_shim_info(wstring& argVariable, LPCSTR argString) {
  HRSRC   hRes;                 // handle/ptr. to res. info. 
  HGLOBAL hResLoad;             // handle to loaded resource
  LPVOID  lpResLock;            // pointer to resource data

  hRes = FindResource(NULL, argString, RT_RCDATA);
  if (hRes) {    
    hResLoad = LoadResource(NULL, hRes);
    lpResLock = LockResource(hResLoad);
    argVariable = wstring((LPCWSTR)lpResLock,
                          SizeofResource(NULL, hRes) / sizeof(WCHAR));
    return true;
  }
  else
    return false;
}

bool get_shim_info(LPCSTR argString) {
  if (FindResource(NULL, argString, RT_RCDATA)) 
    return true;
  else
    return false;
}



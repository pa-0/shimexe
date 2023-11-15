# Path of Chocolatey ShimGen
$shimgen_path   = Join-Path $env:ChocolateyInstall 'tools'

# Path of Shim Executable Install
$exe_name       = "shim_exec.exe"
$app_path       = "$env:ChocolateyInstall\lib\shim_executable"

# Check if Backup Exists
if (Test-Path "$shimgen_path\shimgen.exe.bak") {
    Remove-Item "$shimgen_path\shimgen.exe"
    Remove-Item "$shimgen_path\shimgen.license.txt"
    Write-Host "Removed replaced shimgen.exe in $shimgen_path"
    
    Rename-Item "$shimgen_path\shimgen.exe.bak" "shimgen.exe"
    Rename-Item "$shimgen_path\shimgen.license.txt.bak" "shimgen.license.txt"
    Write-Host "Restored backup shimgen.exe"
}

else {
    Rename-Item "$shimgen_path\shimgen.exe" "shimgen.exe.bak"
    Rename-Item "$shimgen_path\shimgen.license.txt" "shimgen.license.txt.bak"
    Write-Host "Backed up shimgen.exe"
    
    Copy-Item "$app_path\$exe_name" "$shimgen_path\shimgen.exe"
    Copy-Item "$app_path\LICENSE" "$shimgen_path\shimgen.license.txt"
    Write-Host "Replaced shimgen.exe in $shimgen_path from $app_path"
}

# Path of Chocolatey ShimGen
$SG_CHO_path = Join-Path $env:ChocolateyInstall 'tools'

# Path of ShimGen Install
$SG_JPH_path = Get-ToolsLocation
$SG_simgen_path = Join-Path $env:ChocolateyInstall 'lib\shimgen'
$SG_simgen_backup_path = Join-Path $SG_simgen_path 'bak'

# Check if Backup Exists
if (Test-Path $SG_simgen_backup_path) {
    Remove-Item "$SG_CHO_path\shimgen.exe"
    Remove-Item "$SG_CHO_path\shimgen.license.txt"
    Write-Host "Removed replaced shimgen.exe in $SG_CHO_path"
    
    Move-Item "$SG_simgen_backup_path\shimgen.exe" $SG_CHO_path
    Move-Item "$SG_simgen_backup_path\shimgen.license.txt" $SG_CHO_path
    Remove-Item $SG_simgen_backup_path
    Write-Host "Restored backup shimgen.exe from $SG_simgen_backup_path"
}

else {
    $null = New-Item $SG_simgen_backup_path -ItemType Directory
    Move-Item "$SG_CHO_path\shimgen.exe" $SG_simgen_backup_path
    Move-Item "$SG_CHO_path\shimgen.license.txt" $SG_simgen_backup_path
    Write-Host "Backed up shimgen.exe to $SG_simgen_backup_path"
    
    Copy-Item "$SG_JPH_path\shimgen.exe" "$SG_CHO_path\shimgen.exe"
    Copy-Item "$SG_simgen_path\LICENSE" "$SG_CHO_path\shimgen.license.txt"
    Write-Host "Replaced shimgen.exe in $SG_CHO_path from $SG_JPH_path"
}

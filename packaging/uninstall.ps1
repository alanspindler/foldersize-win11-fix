#Requires -RunAsAdministrator
param()

$ErrorActionPreference = 'Stop'

$InstallDir = Join-Path ${env:ProgramFiles} 'FolderSize'
$svcExe = Join-Path $InstallDir 'FolderSizeSvc.exe'

Stop-Process -Name FolderSize -Force -ErrorAction SilentlyContinue

if (Test-Path $svcExe) {
    & $svcExe -remove
}

Remove-ItemProperty -Path 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Run' -Name FolderSize -ErrorAction SilentlyContinue
Remove-ItemProperty -Path 'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Control Panel\Cpls' -Name 'FolderSize.cpl' -ErrorAction SilentlyContinue

if (Test-Path $InstallDir) {
    Remove-Item -Path $InstallDir -Recurse -Force
}

Write-Host "Folder Size removido."

#Requires -Version 5.1
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent

$candidates = @(
    "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe",
    "$env:ProgramFiles\Inno Setup 6\ISCC.exe",
    "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe"
)
$iscc = $candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $iscc) {
    throw "Instale o Inno Setup 6 (winget install JRSoftware.InnoSetup) e rode de novo."
}

& $iscc (Join-Path $PSScriptRoot 'FolderSize-Win11-Setup.iss')
$out = Join-Path $root 'installer-output'
Get-ChildItem $out -Filter '*.exe' | Format-Table Name, Length -AutoSize
Write-Host "Pronto: $($out)"

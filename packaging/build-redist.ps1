# Copia o FolderSize.exe compilado para redist/ e opcionalmente gera ZIP da Release.
param(
    [switch] $Zip
)

$RepoRoot = Split-Path $PSScriptRoot -Parent
$BuildExe = Join-Path $RepoRoot 'build\FolderSize.exe'
$RedistExe = Join-Path $RepoRoot 'redist\FolderSize.exe'

if (-not (Test-Path $BuildExe)) {
    throw "Compile primeiro: cmake --build build (ver README)"
}

Copy-Item $BuildExe $RedistExe -Force
Write-Host "Atualizado: $RedistExe"

if ($Zip) {
    $version = '2.6.1.0'
    $zipPath = Join-Path $RepoRoot "FolderSize-Win11-Patch-$version.zip"
    if (Test-Path $zipPath) { Remove-Item $zipPath -Force }
    Compress-Archive -Path (Join-Path $RepoRoot 'redist\*'), (Join-Path $RepoRoot 'packaging\install.ps1'), (Join-Path $RepoRoot 'packaging\uninstall.ps1'), (Join-Path $RepoRoot 'LICENSE'), (Join-Path $RepoRoot 'NOTICE'), (Join-Path $RepoRoot 'README.md') -DestinationPath $zipPath
    Write-Host "ZIP: $zipPath"
}

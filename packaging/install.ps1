#Requires -RunAsAdministrator
<#
  Instala Folder Size (serviço + cliente popup patchado) a partir de redist/
  Não é necessário baixar o MSI original do SourceForge.
#>
param(
    [switch] $StartWithWindows
)

$ErrorActionPreference = 'Stop'

$RepoRoot = Split-Path $PSScriptRoot -Parent
$Redist = Join-Path $RepoRoot 'redist'
$InstallDir = Join-Path ${env:ProgramFiles} 'FolderSize'

$required = @(
    'FolderSize.exe',
    'FolderSizeSvc.exe',
    'FolderSize.cpl',
    'FolderSizeColumn.dll'
)

foreach ($file in $required) {
    $path = Join-Path $Redist $file
    if (-not (Test-Path $path)) {
        throw "Arquivo ausente em redist: $file — clone o repositório completo ou baixe o ZIP da Release."
    }
}

Stop-Process -Name FolderSize -Force -ErrorAction SilentlyContinue

if (-not (Test-Path $InstallDir)) {
    New-Item -ItemType Directory -Path $InstallDir | Out-Null
}

foreach ($file in $required) {
    Copy-Item (Join-Path $Redist $file) (Join-Path $InstallDir $file) -Force
}

# Instala/atualiza o serviço (implementação upstream: FolderSizeSvc.exe -install)
$svcExe = Join-Path $InstallDir 'FolderSizeSvc.exe'
& $svcExe -install

$svc = Get-Service -Name FolderSize -ErrorAction SilentlyContinue
if (-not $svc) {
    throw 'Serviço FolderSize não foi criado. Execute este script como Administrador.'
}

Set-Service -Name FolderSize -StartupType Automatic
Start-Service -Name FolderSize

$clientExe = Join-Path $InstallDir 'FolderSize.exe'
$runKey = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Run'
if ($StartWithWindows) {
    Set-ItemProperty -Path $runKey -Name FolderSize -Value "`"$clientExe`""
} else {
    Remove-ItemProperty -Path $runKey -Name FolderSize -ErrorAction SilentlyContinue
}

# Applet do Painel de Controle (opcional; caminho do instalador original)
$cplKey = 'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Control Panel\Cpls'
if (-not (Test-Path $cplKey)) {
    New-Item -Path $cplKey -Force | Out-Null
}
Set-ItemProperty -Path $cplKey -Name 'FolderSize.cpl' -Value (Join-Path $InstallDir 'FolderSize.cpl')

Start-Process $clientExe

Write-Host ""
Write-Host "Instalado em: $InstallDir"
Write-Host "Serviço FolderSize: $((Get-Service FolderSize).Status)"
Write-Host "Cliente iniciado. Use o ícone na bandeja: Show Folder Size popups."
Write-Host "Não execute o cliente como Administrador."

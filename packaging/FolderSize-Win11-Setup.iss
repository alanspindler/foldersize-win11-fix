; Folder Size Win11 Patch — instalador único (.exe)
; Requer Inno Setup 6+

#define MyAppName "Folder Size (Win11 patch)"
#define MyAppPublisher "Alan Spindler (community patch; GPLv2)"
#define MyAppURL "https://github.com/alanspindler/foldersize-win11-fix"
#define MyAppVersion "2.6.1.0"
#define MyInstallDir "{autopf}\FolderSize"

[Setup]
AppId={{A7C9F2E1-4B8D-4E6A-9F31-501C9F2E1A7B}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}/releases
DefaultDirName={#MyInstallDir}
DefaultGroupName=Folder Size
DisableProgramGroupPage=yes
LicenseFile=..\LICENSE
InfoBeforeFile=..\NOTICE
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputBaseFilename=FolderSize-Win11-Setup-{#MyAppVersion}
OutputDir=..\installer-output
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayIcon={app}\FolderSize.exe

[Languages]
Name: "brazilianportuguese"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "startup"; Description: "Iniciar o Folder Size com o Windows"; GroupDescription: "Opções adicionais:"; Flags: checkedonce
Name: "launchclient"; Description: "Abrir o Folder Size após instalar"; GroupDescription: "Opções adicionais:"; Flags: checkedonce

[Files]
Source: "..\redist\FolderSize.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\redist\FolderSizeSvc.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\redist\FolderSize.cpl"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\redist\FolderSizeColumn.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\redist\README.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\README.md"; DestDir: "{app}\docs"; DestName: "README.txt"; Flags: ignoreversion isreadme

[Icons]
Name: "{group}\Folder Size"; Filename: "{app}\FolderSize.exe"
Name: "{group}\Desinstalar Folder Size"; Filename: "{uninstallexe}"

[Registry]
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Windows\CurrentVersion\Control Panel\Cpls"; ValueType: string; ValueName: "FolderSize.cpl"; ValueData: "{app}\FolderSize.cpl"; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "FolderSize"; ValueData: """{app}\FolderSize.exe"""; Flags: uninsdeletevalue; Tasks: startup

[Run]
Filename: "{app}\FolderSizeSvc.exe"; Parameters: "-install"; StatusMsg: "Registrando o serviço Folder Size..."; Flags: runhidden waituntilterminated
Filename: "sc.exe"; Parameters: "start FolderSize"; StatusMsg: "Iniciando o serviço..."; Flags: runhidden waituntilterminated
Filename: "{app}\FolderSize.exe"; Description: "Iniciar o Folder Size agora"; Flags: postinstall nowait skipifsilent; Tasks: launchclient

[UninstallRun]
Filename: "{app}\FolderSizeSvc.exe"; Parameters: "-remove"; Flags: runhidden waituntilterminated

[Code]
procedure CurStepChanged(CurStep: TSetupStep);
var
  ResultCode: Integer;
begin
  if CurStep = ssInstall then
  begin
    { Encerra cliente antes de sobrescrever arquivos }
    Exec('taskkill.exe', '/IM FolderSize.exe /F', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
  end;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  ResultCode: Integer;
begin
  if CurUninstallStep = usPostUninstall then
  begin
    Exec('taskkill.exe', '/IM FolderSize.exe /F', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
  end;
end;

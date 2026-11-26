#ifndef AppVersion
#define AppVersion "18.0.0-dev.1"
#endif
#define AppExeName "GREEN_ION.exe"
[Setup]
AppId={{9CBA52F6-7688-4E5B-9C24-84E245D6F5BC}
AppName=GREEN ION
AppVersion={#AppVersion}
AppPublisher=GREEN ION
DefaultDirName={localappdata}\Programs\GREEN ION
DefaultGroupName=GREEN ION
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir=..\installer-output
OutputBaseFilename=GREEN_ION_Setup
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
SetupIconFile=..\build\GREEN_ION.ico
UninstallDisplayIcon={app}\{#AppExeName}
CloseApplications=yes
RestartApplications=yes
[Files]
Source: "..\dist\GREEN_ION.exe"; DestDir: "{app}"; Flags: ignoreversion
[Icons]
Name: "{autoprograms}\GREEN ION"; Filename: "{app}\{#AppExeName}"
Name: "{autodesktop}\GREEN ION"; Filename: "{app}\{#AppExeName}"; Tasks: desktopicon
[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Additional shortcuts:"; Flags: checkedonce
[Run]
Filename: "{app}\{#AppExeName}"; Description: "Launch GREEN ION"; Flags: nowait postinstall skipifsilent

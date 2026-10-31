; PostLive installer script (Inno Setup 6).
;
; Build and deploy Qt first:
;   cmake --build build/windows-x64 --config Release --target deploy-qt
; Then compile this script:
;   iscc packaging/PostLive.iss

#define MyAppName "PostLive"
; CI passes /DMyAppVersion=... parsed from CMakeLists; keep a fallback for local builds.
#ifndef MyAppVersion
#define MyAppVersion "1.1.0.0"
#endif
#define MyAppPublisher "ohto-ai"
#define MyAppURL "https://github.com/ohto-ai/PostLive"
#define MyAppExeName "PostLive.exe"

#define RepoRoot AddBackslash(SourcePath) + ".."
#define BuildDir RepoRoot + "\build\windows-x64\Release"
#define DistDir RepoRoot + "\dist"

[Setup]
; Keep the original AppId so existing installs can upgrade.
AppId={{5C99C655-E0FC-45F0-9660-C987543E4BAA}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf64}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=commandline
OutputDir={#DistDir}
OutputBaseFilename=PostLive-{#MyAppVersion}-setup
SetupIconFile={#RepoRoot}\src\resources\res\live.ico
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
UninstallDisplayIcon={app}\{#MyAppExeName}
VersionInfoVersion={#MyAppVersion}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
; Everything produced by CMake + windeployqt (FFmpeg binaries included when present).
Source: "{#BuildDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "PostLiveMockServer.exe,application.json,users.json,user\*,*.pdb,*.dmp,*.ilk"
; Never overwrite a user's local configuration on upgrade.
Source: "{#BuildDir}\application.json"; DestDir: "{app}"; Flags: onlyifdoesntexist uninsneveruninstall
Source: "{#BuildDir}\users.json"; DestDir: "{app}"; Flags: onlyifdoesntexist uninsneveruninstall

[Dirs]
; Avatar downloads need this directory to exist before the first login.
Name: "{app}\user\avatar"

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

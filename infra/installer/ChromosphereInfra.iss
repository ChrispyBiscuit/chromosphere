; Inno Setup script for the Chromosphere | INFRA Windows installer. Built by CI (.github/workflows/build.yml):
;   ISCC.exe /DAppVersion=0.1.123 infra\installer\ChromosphereInfra.iss
; Expects in ..\dist (next to this folder): ChromosphereInfra.vst3 (bundle) and ChromosphereInfra.exe (standalone).
;
; Automatic updates: the plug-in or the app runs
;   ChromosphereInfra-Setup.exe /VERYSILENT /SUPPRESSMSGBOXES /NORESTART /WAITPID=<host process id> [/RELAUNCH=1]
; Setup waits for that process to exit (its files are locked until then), installs without a window and, with
; /RELAUNCH=1 (sent by the standalone app), starts the app again.

#define AppName "Chromosphere | INFRA"
#define BundleName "ChromosphereInfra"
#define AppExe "ChromosphereInfra.exe"
#ifndef AppVersion
#define AppVersion "0.1.0"
#endif
#define SrcDir "..\dist"

[Setup]
AppId={{6C2B8E41-3F7A-4D95-B0E2-9A1D7C54F3B8}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher=Chromosphere
VersionInfoVersion={#AppVersion}
VersionInfoProductName={#AppName}
DefaultDirName={autopf}\Chromosphere INFRA
DisableDirPage=yes
DisableProgramGroupPage=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
CloseApplications=yes
OutputDir=..\out
OutputBaseFilename=ChromosphereInfra-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
SetupIconFile=ChromosphereInfra.ico
UninstallDisplayIcon={app}\{#AppExe}
UninstallDisplayName={#AppName}
InfoBeforeFile=BEFORE_INSTALL.txt
InfoAfterFile=AFTER_INSTALL.txt

[Types]
Name: "full"; Description: "Install everything"
Name: "custom"; Description: "Choose what to install"; Flags: iscustom

[Components]
Name: "vst3"; Description: "VST3 plug-in  (FL Studio, Ableton Live, Reaper, Cubase, Studio One, Bitwig)"; Types: full custom
Name: "app"; Description: "Standalone app  (play without a DAW: audio and MIDI settings built in)"; Types: full custom

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut for the standalone app"; Components: app

[InstallDelete]
; Replace an old copy completely so no stale files remain inside the bundle.
Type: filesandordirs; Name: "{commoncf64}\VST3\{#BundleName}.vst3"

[Files]
; VST3 bundles live in the folder every VST3 host scans: C:\Program Files\Common Files\VST3
Source: "{#SrcDir}\{#BundleName}.vst3\*"; DestDir: "{commoncf64}\VST3\{#BundleName}.vst3"; Components: vst3; Flags: recursesubdirs ignoreversion
Source: "{#SrcDir}\{#AppExe}"; DestDir: "{app}"; Components: app; Flags: ignoreversion
Source: "AFTER_INSTALL.txt"; DestDir: "{app}"; DestName: "README-ChromosphereInfra.txt"; Flags: ignoreversion
Source: "ChromosphereInfra.ico"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\{#AppName}"; Filename: "{app}\{#AppExe}"; Components: app
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppExe}"; Components: app; Tasks: desktopicon

[Run]
Filename: "{app}\{#AppExe}"; Description: "Start {#AppName}"; Components: app; Flags: nowait postinstall skipifsilent
; After a silent automatic update started from the standalone app, open it again.
Filename: "{app}\{#AppExe}"; Components: app; Flags: nowait; Check: ShouldRelaunch

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\{#BundleName}.vst3"
Type: dirifempty; Name: "{app}"

[Code]
function OpenProcess(dwDesiredAccess: DWORD; bInheritHandle: BOOL; dwProcessId: DWORD): THandle;
  external 'OpenProcess@kernel32.dll stdcall';
function CloseHandle(hObject: THandle): BOOL;
  external 'CloseHandle@kernel32.dll stdcall';
function GetExitCodeProcess(hProcess: THandle; var lpExitCode: DWORD): BOOL;
  external 'GetExitCodeProcess@kernel32.dll stdcall';

const
  PROCESS_QUERY_LIMITED_INFORMATION = $1000;
  STILL_ACTIVE = 259;

function ProcessIsRunning(pid: DWORD): Boolean;
var
  h: THandle;
  code: DWORD;
begin
  Result := False;
  h := OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, False, pid);
  if h <> 0 then
  begin
    if GetExitCodeProcess(h, code) then
      Result := (code = STILL_ACTIVE);
    CloseHandle(h);
  end;
end;

function ShouldRelaunch(): Boolean;
begin
  Result := WizardSilent() and (ExpandConstant('{param:RELAUNCH|0}') = '1');
end;

function InitializeSetup(): Boolean;
var
  pidStr: String;
  pid: Integer;
  waited: Integer;
begin
  Result := True;
  pidStr := ExpandConstant('{param:WAITPID|0}');
  pid := StrToIntDef(pidStr, 0);
  if pid > 0 then
  begin
    Log('Waiting for process ' + pidStr + ' to exit before installing');
    waited := 0;
    while ProcessIsRunning(pid) and (waited < 24 * 3600) do
    begin
      Sleep(2000);
      waited := waited + 2;
    end;
    if ProcessIsRunning(pid) then
    begin
      Log('Gave up waiting; not installing');
      Result := False;
    end
    else
    begin
      Log('Process gone; installing');
      Sleep(1500);
    end;
  end;
end;

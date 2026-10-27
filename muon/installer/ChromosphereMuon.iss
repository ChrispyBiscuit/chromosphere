; Inno Setup script for the Chromosphere | MUON Windows installer. Built by CI (.github/workflows/build.yml):
;   ISCC.exe /DAppVersion=0.1.123 muon\installer\ChromosphereMuon.iss
; Expects in ..\dist (next to this folder): ChromosphereMuon.vst3 (bundle). MUON is an effect: no standalone app.
;
; Automatic updates: the plug-in runs
;   ChromosphereMuon-Setup.exe /VERYSILENT /SUPPRESSMSGBOXES /NORESTART /WAITPID=<host process id>
; Setup waits for that process (the DAW) to exit, since its files are locked until then, and installs without a window.

#define AppName "Chromosphere | MUON"
#define BundleName "ChromosphereMuon"
#ifndef AppVersion
#define AppVersion "0.1.0"
#endif
#define SrcDir "..\dist"

[Setup]
AppId={{3E9A7D12-5B4C-4F8E-A61D-2C7B90E4F5A3}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher=Chromosphere
VersionInfoVersion={#AppVersion}
VersionInfoProductName={#AppName}
DefaultDirName={autopf}\Chromosphere MUON
DisableDirPage=yes
DisableProgramGroupPage=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
CloseApplications=yes
OutputDir=..\out
OutputBaseFilename=ChromosphereMuon-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
SetupIconFile=ChromosphereMuon.ico
UninstallDisplayIcon={app}\ChromosphereMuon.ico
UninstallDisplayName={#AppName}
InfoBeforeFile=BEFORE_INSTALL.txt
InfoAfterFile=AFTER_INSTALL.txt

[InstallDelete]
; Replace an old copy completely so no stale files remain inside the bundle.
Type: filesandordirs; Name: "{commoncf64}\VST3\{#BundleName}.vst3"

[Files]
; VST3 bundles live in the folder every VST3 host scans: C:\Program Files\Common Files\VST3
Source: "{#SrcDir}\{#BundleName}.vst3\*"; DestDir: "{commoncf64}\VST3\{#BundleName}.vst3"; Flags: recursesubdirs ignoreversion
Source: "AFTER_INSTALL.txt"; DestDir: "{app}"; DestName: "README-ChromosphereMuon.txt"; Flags: ignoreversion
Source: "ChromosphereMuon.ico"; DestDir: "{app}"; Flags: ignoreversion

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

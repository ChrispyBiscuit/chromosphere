; Inno Setup script for the Windows installer. Built by CI (.github/workflows/build.yml):
;   ISCC.exe installer\Chromosphere.iss            (VST3 only)
;   ISCC.exe /DWithAAX installer\Chromosphere.iss  (VST3 + AAX, when dist\Chromosphere.aaxplugin exists)
; Expects the plug-in bundles in ..\dist next to this folder.

#define AppName "Chromosphere | EXO"
#define BundleName "Chromosphere"
#ifndef AppVersion
#define AppVersion "0.1.0"
#endif
#define SrcDir "..\dist"

[Setup]
AppId={{A41F7C2E-9B5D-4E63-8F0A-2C6D1B7E9A35}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher=Chromosphere
DefaultDirName={commoncf64}\{#BundleName}
DisableDirPage=yes
DisableProgramGroupPage=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
OutputDir=..\out
OutputBaseFilename=Chromosphere-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
SetupIconFile=Chromosphere.ico
UninstallDisplayIcon={app}\Chromosphere.ico
UninstallDisplayName={#AppName} plug-ins
UninstallFilesDir={commoncf64}\{#BundleName}
InfoBeforeFile=BEFORE_INSTALL.txt
InfoAfterFile=AFTER_INSTALL.txt

[Types]
Name: "full"; Description: "Install everything"
Name: "custom"; Description: "Choose what to install"; Flags: iscustom

[Components]
Name: "vst3"; Description: "VST3 plug-in  (FL Studio, Reaper, Cubase, Live, Studio One, Bitwig)"; Types: full custom
#ifdef WithAAX
Name: "aax"; Description: "AAX plug-in  (Pro Tools)"; Types: full custom
#endif

[InstallDelete]
; Replace an old copy completely so no stale files remain inside the bundle.
Type: filesandordirs; Name: "{commoncf64}\VST3\{#BundleName}.vst3"
#ifdef WithAAX
Type: filesandordirs; Name: "{commoncf64}\Avid\Audio\Plug-Ins\{#BundleName}.aaxplugin"
#endif

[Files]
; VST3 bundles live in the folder every VST3 host scans: C:\Program Files\Common Files\VST3
Source: "{#SrcDir}\{#BundleName}.vst3\*"; DestDir: "{commoncf64}\VST3\{#BundleName}.vst3"; Components: vst3; Flags: recursesubdirs ignoreversion
#ifdef WithAAX
; Pro Tools loads AAX from C:\Program Files\Common Files\Avid\Audio\Plug-Ins
Source: "{#SrcDir}\{#BundleName}.aaxplugin\*"; DestDir: "{commoncf64}\Avid\Audio\Plug-Ins\{#BundleName}.aaxplugin"; Components: aax; Flags: recursesubdirs ignoreversion
#endif
Source: "AFTER_INSTALL.txt"; DestDir: "{app}"; DestName: "README-Chromosphere.txt"; Flags: ignoreversion
Source: "Chromosphere.ico"; DestDir: "{app}"; Flags: ignoreversion

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\{#BundleName}.vst3"
Type: filesandordirs; Name: "{commoncf64}\Avid\Audio\Plug-Ins\{#BundleName}.aaxplugin"
Type: dirifempty; Name: "{app}"

[Code]
// Waiting mode for automatic updates: the plug-in launches
//   Chromosphere-Setup.exe /VERYSILENT /SUPPRESSMSGBOXES /NORESTART /WAITPID=<DAW process id>
// while the DAW is still running. Setup then waits until that process has exited (the plug-in
// files are locked until then) and installs without any window. It gives up after 24 hours.
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

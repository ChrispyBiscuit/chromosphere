; Inno Setup script for the Windows installer. Built by CI (.github/workflows/build.yml):
;   ISCC.exe installer\FancyReverb.iss            (VST3 only)
;   ISCC.exe /DWithAAX installer\FancyReverb.iss  (VST3 + AAX, when dist\FancyReverb.aaxplugin exists)
; Expects the plug-in bundles in ..\dist next to this folder.

#define AppName "FancyReverb"
#define AppVersion "0.1.0"
#define SrcDir "..\dist"

[Setup]
AppId={{7D3C9A6E-2F41-4B8B-9C0D-1A2B3C4D5E6F}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher=FancyReverb
DefaultDirName={commoncf64}\{#AppName}
DisableDirPage=yes
DisableProgramGroupPage=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
OutputDir=..\out
OutputBaseFilename=FancyReverb-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName={#AppName} plug-ins
UninstallFilesDir={commoncf64}\{#AppName}
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
Type: filesandordirs; Name: "{commoncf64}\VST3\{#AppName}.vst3"
#ifdef WithAAX
Type: filesandordirs; Name: "{commoncf64}\Avid\Audio\Plug-Ins\{#AppName}.aaxplugin"
#endif

[Files]
; VST3 bundles live in the folder every VST3 host scans: C:\Program Files\Common Files\VST3
Source: "{#SrcDir}\{#AppName}.vst3\*"; DestDir: "{commoncf64}\VST3\{#AppName}.vst3"; Components: vst3; Flags: recursesubdirs ignoreversion
#ifdef WithAAX
; Pro Tools loads AAX from C:\Program Files\Common Files\Avid\Audio\Plug-Ins
Source: "{#SrcDir}\{#AppName}.aaxplugin\*"; DestDir: "{commoncf64}\Avid\Audio\Plug-Ins\{#AppName}.aaxplugin"; Components: aax; Flags: recursesubdirs ignoreversion
#endif
Source: "AFTER_INSTALL.txt"; DestDir: "{app}"; DestName: "README-FancyReverb.txt"; Flags: ignoreversion

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\{#AppName}.vst3"
Type: filesandordirs; Name: "{commoncf64}\Avid\Audio\Plug-Ins\{#AppName}.aaxplugin"
Type: dirifempty; Name: "{app}"

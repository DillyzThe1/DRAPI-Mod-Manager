; -- 64Bit.iss --
; Demonstrates installation of a program built for the x64 (a.k.a. AMD64)
; architecture.
; To successfully run this installation and the program it installs,
; you must have a "x64" edition of Windows.

; SEE THE DOCUMENTATION FOR DETAILS ON CREATING .ISS SCRIPT FILES!

[Setup]
AppName=DillyzRoleApi Mod Manager
AppVersion=1.0.0
AppPublisher=DillyzThe1
AppPublisherURL=https://github.com/DillyzThe1
WizardStyle=modern
DefaultDirName={autopf}\DillyzThe1\DRAPI-Mod-Manager
DefaultGroupName=DillyzThe1\DRAPI-Mod-Manager
UninstallDisplayIcon={app}\DRAPIMM_Uninstall.exe
Compression=lzma2
SolidCompression=yes
OutputDir=userdocs:inno setup
OutputBaseFilename=DRAPIMM-Installer-1.0.0
SetupIconFile=..\DRAPI-Mod-Manager\icon1.ico
; "ArchitecturesAllowed=x64" specifies that Setup cannot run on
; anything but x64.
ArchitecturesAllowed=x64
; "ArchitecturesInstallIn64BitMode=x64" requests that the install be
; done in "64-bit mode" on x64, meaning it should use the native
; 64-bit Program Files directory and the 64-bit view of the registry.
ArchitecturesInstallIn64BitMode=x64

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Files]
Source: "..\x64\Release\DRAPI-Mod-Manager.exe"; DestDir: "{app}"; DestName: "DRAPI-Mod-Manager.exe"
Source: "..\x64\Release\content\*"; DestDir: "{app}\content"; Flags: ignoreversion recursesubdirs
Source: "..\x64\Release\openal32.dll"; DestDir: "{app}"                  
Source: "..\x64\Release\sfml-audio-2.dll"; DestDir: "{app}"
Source: "..\x64\Release\sfml-graphics-2.dll"; DestDir: "{app}"
Source: "..\x64\Release\sfml-system-2.dll"; DestDir: "{app}"      
Source: "..\x64\Release\sfml-window-2.dll"; DestDir: "{app}"
Source: "readme.txt"; DestDir: "{app}"; Flags: isreadme

[Icons]
Name: "{group}\DillyzThe1\DRAPI-Mod-Manager"; Filename: "{app}\DRAPI-Mod-Manager.exe"
Name: "{commondesktop}\DRAPI-Mod-Manager"; Filename: "{app}\DRAPI-Mod-Manager"; Tasks: desktopicon; IconFilename: "{app}\DRAPI-Mod-Manager.exe"
; Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\DillyzThe1\DRAPI-Mod-Manager"; Filename: "{app}\DillyzThe1\DRAPI-Mod-Manager.exe"; Tasks: quicklaunchicon; IconFilename: "{app}\DillyzThe1\DRAPI-Mod-Manager.exe"

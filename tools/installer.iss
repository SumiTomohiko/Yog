[Setup]
AppName=Yog 0.0.4
AppVerName=Yog 0.0.4
OutputBaseFilename=Yog-0.0.4
DefaultDirName={sd}\Program Files\Yog-0.0.4
SourceDir=..
DefaultGroupName=Yog-0.0.4

[Files]
Source: "vs\2003\Yog\yog.exe"; DestDir: "{app}\bin"
Source: "lib\*.yg"; DestDir: "{app}\lib"
Source: "lib\ydoc\*.yg"; DestDir: "{app}\lib\ydoc"

[Icons]
Name: "{group}\yog"; Filename: "{app}\bin\yog.exe"
Name: "{group}\uninstall"; Filename: "{uninstallexe}"

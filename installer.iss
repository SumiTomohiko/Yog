[Setup]
AppName=Yog 0.0.4
AppVerName=Yog 0.0.4
OutputDir=.
OutputBaseFilename=Yog-0.0.4
DefaultDirName={sd}\Program Files\Yog-0.0.4
SourceDir=.
DefaultGroupName=Yog-0.0.4

[Files]
Source: "src\yog.exe"; DestDir: "{app}\bin"
Source: "lib\*.yg"; DestDir: "{app}\lib"
Source: "lib\ydoc\*.yg"; DestDir: "{app}\lib\ydoc"
Source: "src\yog.ico"; DestDir: "{app}"

[Icons]
Name: "{group}\Yog"; Filename: "{app}\bin\yog.exe"; IconFilename: "{app}\yog.ico"
Name: "{group}\uninstall"; Filename: "{uninstallexe}"

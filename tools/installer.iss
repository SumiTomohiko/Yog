[Setup]

AppName=Yog 0.0.4
AppVerName=Yog 0.0.4
OutputBaseFilename=Yog-0.0.4
DefaultDirName={sd}\Program Files\Yog-0.0.4
SourceDir=..

[Files]
Source: "vs\2003\Yog\Yog.exe"; DestDir: "{app}\bin"
Source: "lib\*.yg"; DestDir: "{app}\lib"
Source: "lib\ydoc\*.yg"; DestDir: "{app}\lib\ydoc"

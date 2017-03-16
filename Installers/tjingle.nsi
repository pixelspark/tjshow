; The name of the installer
Name "TJingle"
LoadLanguageFile "Dutch.nlf"

; The file to write
OutFile "Release\tjingle-setup.exe"
InstallDir "$PROGRAMFILES\TJ\TJingle"
LicenseData "license.rtf"

Page license
Page components
Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

BrandingText " "
LicenseBkColor FFFFFF

SetCompressor lzma
XPStyle on
SetDateSave on
SetDatablockOptimize on
CRCCheck on

VIAddVersionKey "ProductName" "TJingle"
VIAddVersionKey "Comments" ""
VIAddVersionKey "CompanyName" "Tommy van der Vorst"
VIAddVersionKey "LegalTrademarks" ""
VIAddVersionKey "LegalCopyright" "© Tommy van der Vorst, 2005-2010"
VIAddVersionKey "FileDescription" "TJingle"
VIAddVersionKey "ProductVersion" "4.0.0.0"
VIProductVersion "4.0.0.0"

Section "!TJingle"
    SectionIn RO
    
    ; Kill TJingle process
    Processes::FindProcess "tjingle.exe"
    StrCmp $R0 "1" process_found process_not_found
 
    process_found:
        MessageBox MB_YESNO|MB_ICONINFORMATION "TJingle is op dit moment actief; om het programma bij te kunnen werken, moet het worden gesloten. Klik op Ja om het programma te sluiten." IDNO nokill
        Processes::KillProcess "tjingle.exe"
	Sleep 1000
	
    nokill:
    process_not_found:

    ; Kill updater process
    Processes::FindProcess "tjupdater.exe"
    StrCmp $R0 "1" updater_process_found updater_process_not_found
    updater_process_found:
    Processes::KillProcess "tjupdater.exe"
    Sleep 1000
    updater_process_not_found:
    
    SetOutPath $INSTDIR
    File "..\build\Release\tjingle.exe"
    File "..\build\Release\bass.dll"
    File "..\build\Release\tinyxml.dll"
    File "..\build\Release\tjshared.dll"
    
    SetOutPath $INSTDIR\icons
    File /r "..\build\Release\icons\*.png"

    SetOutPath $INSTDIR\locale\nl
    File "..\build\Release\locale\nl\tjingle.tjs"
    File "..\build\Release\locale\nl\tjshared.tjs"
    File "..\build\Release\locale\nl\tjlocales.tjs"
    
    SetOutPath $INSTDIR\locale\en
    File "..\build\Release\locale\en\tjingle.tjs"
    File "..\build\Release\locale\en\tjshared.tjs"
    File "..\build\Release\locale\en\tjlocales.tjs"
    
    CreateDirectory $SMPROGRAMS\TJingle
    CreateShortCut "$SMPROGRAMS\TJingle\TJingle.lnk" "$INSTDIR\tjingle.exe" "" "$INSTDIR\tjingle.exe" 0 SW_SHOWNORMAL
    
    WriteRegStr HKCR ".jingle" "" "TJingle.jingle"
    WriteRegStr HKCR "TJingle.jingle" "" "TJingle-bestand"
    WriteRegStr HKCR "TJingle.jingle\shell" "" "open"
    WriteRegStr HKCR "TJingle.jingle\shell\open\command" "" '$INSTDIR\tjingle.exe "%1" '
    WriteRegStr HKCR "TJingle.jingle\DefaultIcon" "" "$INSTDIR\tjingle.exe,0"
SectionEnd

Section "Automatic updater"
   SectionIn 1 2
   
   Processes::KillProcess "tjupdater.exe"
   
   SetOutPath $INSTDIR
   File "..\build\Release\tjupdater.exe"

   SetOutPath $INSTDIR\updater
   File "..\build\Release\updater\tjingle.updater.xml"
SectionEnd

!include "vcredist.nsi"

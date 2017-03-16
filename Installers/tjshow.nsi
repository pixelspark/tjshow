!include "shellfolders.nsh"
!include LogicLib.nsh

; The name of the installer
!ifdef TJSHOW_DEMO_VERSION
   Name "TJShow (Demo version)"
   OutFile "Release\tjshow-demo-setup.exe"
   InstallDir "$PROGRAMFILES\TJ\TJShowDemo"
   BrandingText "DEMO Version"
!else
   Name "TJShow"
   OutFile "Release\tjshow-setup.exe"
   InstallDir "$PROGRAMFILES\TJ\TJShow"
   BrandingText " "
!endif

; Compressor settings
SetCompressor lzma
XPStyle on
SetDateSave on
SetDatablockOptimize on
CRCCheck on

; MUI includes and defines
!define MUI_LANGDLL_REGISTRY_ROOT HKLM
!define MUI_LANGDLL_REGISTRY_KEY Software\TJ\TJShow\Installer
!define MUI_LANGDLL_REGISTRY_VALUENAME InstallerLanguage
!include "MUI.nsh"

; Set up the abort warning and pages
!define MUI_ABORTWARNING
!ifdef TJSHOW_DEMO_VERSION
   !insertmacro MUI_PAGE_LICENSE "license-demo.rtf"
!else
   !insertmacro MUI_PAGE_LICENSE "license.rtf"
!endif
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

; Language selection
!insertmacro MUI_LANGUAGE Dutch     ; 1043
!insertmacro MUI_LANGUAGE English   ; 1033
!insertmacro MUI_LANGUAGE German    ;
!insertmacro MUI_LANGUAGE French    ; 1036
!insertmacro MUI_LANGUAGE Swedish   ; 
!insertmacro MUI_RESERVEFILE_LANGDLL 

; Before the installation starts, ask the user for a language
Function .onInit
   !insertmacro MUI_LANGDLL_DISPLAY 
FunctionEnd

LicenseBkColor FFFFFF

VIAddVersionKey "ProductName" "TJShow"
VIAddVersionKey "Comments" ""
VIAddVersionKey "CompanyName" "Pixelspark"
VIAddVersionKey "LegalTrademarks" ""
VIAddVersionKey "LegalCopyright" "© Tommy van der Vorst, 2005-2010"
VIAddVersionKey "FileDescription" "TJShow"
VIAddVersionKey "ProductVersion" "4.0.0.0"
VIAddVersionKey "FileVersion" "4.0.0.0"
VIProductVersion "4.0.0.0"

; Installatie-types
InstType "Volledig"
InstType "Basis"

Section "!TJShow"
    SectionIn RO
    
    ; Check if TJShow is running and if it is, terminate it
    Processes::FindProcess "tjshow.exe"
    StrCmp $R0 "1" process_found process_not_found
 
    process_found:
        MessageBox MB_YESNO|MB_ICONINFORMATION "TJShow is currently active. To complete the installation of this update, it needs to be closed. Click yes to close TJShow; click no to stop this installation." IDNO nokill
        Processes::KillProcess "tjingle.exe"
        
    nokill:
         Abort
    process_not_found:
    
    ; Kill the tjupdater.exe process, because otherwise, it uses/locks tinyxml.dll/tjshared.dll during installation
    Processes::FindProcess "tjupdater.exe"
    StrCmp $R0 "1" updater_process_found updater_process_not_found
    updater_process_found:
    Processes::KillProcess "tjupdater.exe"
    Sleep 1000
    updater_process_not_found:

    SetOutPath $INSTDIR
    
    ; Delete old plug-ins
    Delete "$INSTDIR\plugins\tj*.dll"
   
   !ifdef TJSHOW_DEMO_VERSION
      File "..\build\Release\tjshowdemo.exe"
      CreateDirectory "$SMPROGRAMS\TJShow Demo"
      CreateShortCut "$SMPROGRAMS\TJShow Demo\TJShow.lnk" "$INSTDIR\tjshowdemo.exe" "" "$INSTDIR\tjshowdemo.exe" 0 SW_SHOWNORMAL
    !else
      File "..\build\Release\tjshow.exe"
      CreateDirectory $SMPROGRAMS\TJShow
      CreateShortCut "$SMPROGRAMS\TJShow\TJShow.lnk" "$INSTDIR\tjshow.exe" "" "$INSTDIR\tjshow.exe" 0 SW_SHOWNORMAL
      CreateShortCut "$SMPROGRAMS\TJShow\TJShow (Client).lnk" "$INSTDIR\tjshow.exe" "C" "$INSTDIR\tjshow.exe" 0 SW_SHOWNORMAL
    !endif
    
    File "..\build\Release\tjcrashreporter.exe"
   
   SetOutPath $INSTDIR
   File "..\build\Release\dnssd.dll"
   File "..\build\Release\scintilla.dll"
   File "..\build\Release\tinyxml.dll"
   File "..\build\Release\tj*.dll"
   File "..\build\Release\epframework.dll"
   File "..\build\Release\player.fx"
   File "..\build\Release\d3dx9_42.dll"

   File "license.rtf"

   SetOutPath $INSTDIR\icons
   File /r /x .svn "..\build\Release\icons\*.*"
   
   SetOutPath $INSTDIR\resources
   File /r /x .svn "..\build\Release\resources\*.*"


   !ifdef TJSHOW_DEMO_VERSION
      SetOutPath $INSTDIR\locale\en
      File /x .svn "..\build\Release\locale\en\*.tjs"
   !else   
      SetOutPath $INSTDIR\locale
      File /r /x .svn "..\build\Release\locale\*.*"
   !endif
    
   SetOutPath $INSTDIR\plugins
   SetOutPath $INSTDIR\drivers

   WriteUninstaller "$INSTDIR\uninstall.exe"

   WriteRegStr HKCR ".tsx" "" "TJShow.tsx"
   WriteRegStr HKCR "TJShow.tsx" "" "TJShow-bestand"
   WriteRegStr HKCR "TJShow.tsx\shell" "" "open"

   !ifdef TJSHOW_DEMO_VERSION
      WriteRegStr HKCR "TJShow.tsx\shell\open\command" "" '$INSTDIR\tjshowdemo.exe M "%1" '
      WriteRegStr HKCR "TJShow.tsx\DefaultIcon" "" "$INSTDIR\tjshowdemo.exe,0"
      !else
      WriteRegStr HKCR "TJShow.tsx\shell\open\command" "" '$INSTDIR\tjshow.exe M "%1" '
      WriteRegStr HKCR "TJShow.tsx\DefaultIcon" "" "$INSTDIR\tjshow.exe,0"
      !endif

   !ifndef TJSHOW_DEMO_VERSION
      WriteRegStr HKCR ".ttx" "" "TJShow.ttx"
      WriteRegStr HKCR "TJShow.ttx" "" "TJShow spoor-bestand"
      WriteRegStr HKCR "TJShow.ttx\shell" "" "open"
      WriteRegStr HKCR "TJShow.ttx\DefaultIcon" "" "$INSTDIR\tjshow.exe,0"
      WriteRegStr HKCR "TJShow.ttx\shell\open\command" "" '$INSTDIR\tjshow.exe M "%1" '
   !endif
      
   ;; Conditionally write initial settings files with chosen locale to user folder
   System::Call 'shell32::SHGetSpecialFolderPathA(i $HWNDPARENT, t .r1, i ${CSIDL_APPDATA}, b 'false') i r0'
   SetOutPath $1\TJ\TJShow
   
   ${If} $LANGUAGE == 1043 ; Dutch
      File /x .svn "InitialSettings\nl\*.*"
   ${Else}
      File /x .svn "InitialSettings\en\*.*"
   ${EndIf}
      
   ;!ifndef TJSHOW_DEMO_VERSION
   ;   ExecShell "open" 'http://www.tjshow.com/licensing/?lang=$LANGUAGE'
   ;!endif
SectionEnd

Section "Automatic updater"
   SectionIn 1 2
   
   Processes::KillProcess "tjupdater.exe"
   
   SetOutPath $INSTDIR\updater
   File "..\build\Release\tjupdater.exe"

   SetOutPath $INSTDIR\updater
   File "..\build\Release\updater\tjshow.updater.xml"
SectionEnd

!include "vcredist.nsi"

SectionGroup Plugins
   Section "DMX"
      SectionIn 1 2
      SetOutPath $INSTDIR\plugins
      File "..\build\Release\plugins\tjdmx.dll"
      
      SetOutPath $INSTDIR\drivers
      File /x .svn "..\build\Release\drivers\*.*"
      
      # Patch-set file type
      WriteRegStr HKCR ".ttx" "" "TJShow.ttx"
      WriteRegStr HKCR "TJShow.TJDMX.tdp" "" "TJShow DMX-patchset"
      WriteRegStr HKCR "TJShow.ttx\shell" "" "open"
      WriteRegStr HKCR "TJShow.ttx\DefaultIcon" "" "$INSTDIR\tjshow.exe,0"
   SectionEnd

   !ifndef TJSHOW_DEMO_VERSION
      Section "Media"
         SectionIn 1 2
         SetOutPath $INSTDIR\plugins
         File "..\build\Release\plugins\tjmedia.dll"
      SectionEnd

      ;Section "Browser"
      ;   SectionIn 1
      ;   SetOutPath $INSTDIR\plugins
      ;   File "..\build\Release\plugins\tjbrowser.dll"
      ;   
      ;   SetOutPath $INSTDIR
      ;   File "..\build\Release\awesomium.dll"
      ;   File "..\build\Release\icudt38.dll"
      ;SectionEnd
      
      Section "Database"
         SectionIn 1
         SetOutPath $INSTDIR\plugins
         File "..\build\Release\plugins\tjdata.dll"
      SectionEnd
      
      Section "MIDI"
         SectionIn 1
         SetOutPath $INSTDIR\plugins
         File "..\build\Release\plugins\tjmidi.dll"
      SectionEnd
   !endif
SectionGroupEnd

SectionGroup Drivers
   Section /o "Enttec Pro DMX driver"
      SectionIn 1
      SetOutPath $INSTDIR\redist
      DetailPrint "Installeren van FTDI VCOM drivers voor Enttec Pro (naar c:\vcom)..."
      File "DMX\vcom_setup.exe"
      ExecWait "$INSTDIR\redist\vcom_setup.exe /silent"
      DetailPrint "Installatie van FTDI VCOM drivers voor Enttec Pro voltooid: $0"
   SectionEnd
SectionGroupEnd

 !ifndef TJSHOW_DEMO_VERSION
   SectionGroup Codecs
      Section /o "FFDShow (MPEG,XviD)"
         SectionIn 1
         SetOutPath $INSTDIR\redist
         DetailPrint "Installeren van FFDShow codec"
         File "Codecs\ffdshow.exe"
         ExecWait "$INSTDIR\redist\ffdshow.exe /S"
         DetailPrint "Installatie van FFDShow codec voltooid: $0"
      SectionEnd   

      Section /o "AC3Filter (AC3-geluid)"
         SectionIn 1
         DetailPrint "Installeren van AC3Filter codec"
         File "Codecs\ac3filter.exe"
         ExecWait "$INSTDIR\redist\ac3filter.exe /S"
         DetailPrint "Installatie van AC3Filter voltooid: $0"
      SectionEnd
      
      Section /o "OGG Codecs"
         SectionIn 1
         SetOutPath $INSTDIR\redist
         DetailPrint "Installeren van OGG codecs"
         File "Codecs\oggcodecs.exe"
         ExecWait "$INSTDIR\redist\oggcodecs.exe /S"
         DetailPrint "Installatie van OGG codecs voltooid: $0"
      SectionEnd  
   SectionGroupEnd
!endif

Section /o "TJShow client automatisch opstarten"
   WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Run" "TJShow Client" "'$INSTDIR\TJShow.exe' C"
SectionEnd

Section Uninstall
   Delete "$INSTDIR\*.dll"
   RMDir /r "$INSTDIR\locale"
   RMDir /r "$INSTDIR\examples"
   RMDir /r "$INSTDIR\plugins"
   RMDir /r "$INSTDIR\input"
   RMDir /r "$INSTDIR\icons"
   RMDir /r "$INSTDIR\fixtures"
   RMDir /r "$INSTDIR\parmon"
   RMDir /r "$INSTDIR\redist"
   RMDir /r "$INSTDIR\updater"
   RMDir /r "$INSTDIR\drivers"
   Delete "$INSTDIR\*.rtf"
   Delete "$INSTDIR\*.exe"
   DeleteRegValue HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Run" "TJShow Client"
   
SectionEnd

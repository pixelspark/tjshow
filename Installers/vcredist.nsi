Section "VC++ Runtime"
  SectionIn RO
  
  ; VC Runtime stuff
   SetOutPath $INSTDIR\redist
   File "vcredist_x86.exe"
   DetailPrint "Installeren van VC runtime..."
   ExecWait "$INSTDIR\redist\vcredist_x86.exe /Q"
   DetailPrint "VC runtime geinstalleerd!"
SectionEnd

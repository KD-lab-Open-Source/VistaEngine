; example2.nsi
;
; This script is based on example1.nsi, but it remember the directory, 
; has uninstall support and (optionally) installs start menu shortcuts.
;
; It will install example2.nsi into a directory that the user selects,

;--------------------------------

; The name of the installer
Name "Perimeter2"

; The file to write
OutFile "Perimeter2Setup.exe"

; The default installation directory
InstallDir $PROGRAMFILES\Perimeter2

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\NSIS_Perimeter2" "Install_Dir"

;--------------------------------

; Pages

Page components
Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

;--------------------------------

; The stuff to install
Section "Perimeter2 (required)"

  SectionIn RO
  
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  
  ; Put file there
  File "Perimeter2.nsi"
  
  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\NSIS_Perimeter2 "Install_Dir" "$INSTDIR"
  
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Perimeter2" "DisplayName" "NSIS Perimeter2"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Perimeter2" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Perimeter2" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Perimeter2" "NoRepair" 1
  WriteUninstaller "uninstall.exe"
  
SectionEnd

; Optional section (can be disabled by the user)
Section "Start Menu Shortcuts"

  CreateDirectory "$SMPROGRAMS\Perimeter2"
  CreateShortCut "$SMPROGRAMS\Perimeter2\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  CreateShortCut "$SMPROGRAMS\Perimeter2\Perimeter2 (MakeNSISW).lnk" "$INSTDIR\Perimeter2.nsi" "" "$INSTDIR\Perimeter2.nsi" 0
  
SectionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"
  
  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Perimeter2"
  DeleteRegKey HKLM SOFTWARE\NSIS_Perimeter2

  ; Remove files and uninstaller
  Delete $INSTDIR\Perimeter2.nsi
  Delete $INSTDIR\uninstall.exe

  ; Remove shortcuts, if any
  Delete "$SMPROGRAMS\Perimeter2\*.*"

  ; Remove directories used
  RMDir "$SMPROGRAMS\Perimeter2"
  RMDir "$INSTDIR"

SectionEnd

@echo off
if "%~1"=="/r" (
  echo Removing remap registry values and scheduled task...
  reg delete "HKLM\SYSTEM\CurrentControlSet\Control\Keyboard Layout" /v "Scancode Map" /f
  reg delete "HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\TimeZoneInformation" /v RealTimeIsUniversal /f
  reg add "HKEY_CURRENT_USER\Control Panel\Desktop" /v "ForegroundLockTimeout" /t REG_DWORD /d 200000 /f
  schtasks /delete /tn "key-remapper" /f
) else (
  echo Remap CapsLock to F13 via scanmap...
  reg add "HKLM\SYSTEM\CurrentControlSet\Control\Keyboard Layout" /v "Scancode Map" /t REG_BINARY /d 00000000000000000200000064003A0000000000 /f
  echo Use UTC for system clock...
  reg add "HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\TimeZoneInformation" /v RealTimeIsUniversal /t REG_DWORD /d 1 /f
  echo Disable ForegroundLockTimeout...
  reg add "HKEY_CURRENT_USER\Control Panel\Desktop" /v "ForegroundLockTimeout" /t REG_DWORD /d 0 /f
  echo Schedule remap.exe to run at logon...
  schtasks /create /sc onlogon /tn "key-remapper" /tr "%~dp0\remap.exe" /rl limited /f
)
exit /b

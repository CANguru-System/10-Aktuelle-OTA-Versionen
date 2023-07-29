@echo off
SET COMPORT=COM3


:loop
cls
echo.
echo CARguru - Helper
echo.
echo USB-Anschluesse:
call files
echo.
echo Bitte waehlen Sie eine der folgenden Optionen:
echo. 
echo  1 - COM-Port festlegen
echo  2 - Flash-Speicher loeschen
echo  3 - OTA-Upload vorbereiten auf %COMPORT%
echo  4 - Upload CANguru-Decoder (aus Ordner CANguru-Files) ueber %COMPORT%
echo  5 - Upload CANguru-Bridge (aus Ordner CANguru-Bridge) ueber %COMPORT%
echo  6 - Putty starten
echo. 
echo  x - Beenden
echo.
set /p SELECTED=Ihre Auswahl: 

if "%SELECTED%" == "x" goto :eof
if "%SELECTED%" == "1" goto :SetComPort
if "%SELECTED%" == "2" goto :ERASE_FLASH
if "%SELECTED%" == "3" goto :PREPARE_OTA
if "%SELECTED%" == "4" goto :UPLOAD_FIRMWARE
if "%SELECTED%" == "5" goto :UPLOAD_BRIDGE
if "%SELECTED%" == "6" goto :Putty

goto :errorInput 


:SetComPort
REM @echo OFF
REM FOR /L %%x IN (1, 1, 29) DO ECHO %%x - Setze COM-Port %%x
echo Bitte geben Sie die Nummer des COM-Anschlusses ein (z.B. 5 fuer COM5) oder x fuer Exit
echo.
set /p SELECTED=Ihre Auswahl: 

if "%SELECTED%" == "x" goto :loop

set COMPORT=COM%SELECTED%
goto :loop
goto :errorInput 

pause
goto :loop

:ERASE_FLASH
@echo on
esptool.exe --chip esp32 --port %COMPORT% erase_flash
@echo off
echo.
pause
goto :loop

:PREPARE_OTA
@echo on
esptool.exe --chip esp32 --port %COMPORT% --baud 460800 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size 4MB 0x1000 Prepare-OTA/bootloader.bin 0x8000 Prepare-OTA/partitions.bin 0xe000 Prepare-OTA/boot_app0.bin 0x10000 Prepare-OTA/firmware.bin
Putty\putty.exe -serial %COMPORT% -sercfg 115200,8,n,1,N
@echo off
echo.
pause
goto :loop

:UPLOAD_BRIDGE
@echo on
esptool.exe --chip esp32 --port %COMPORT% --baud 460800 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size 4MB 0x1000 CANguru-Bridge/bootloader.bin 0x8000 CANguru-Bridge/partitions.bin 0x10000 CANguru-Bridge/firmware.bin
@echo off
echo.
pause
goto :loop

:UPLOAD_FIRMWARE
@echo on
esptool.exe --chip esp32 --port %COMPORT% --baud 460800 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size 4MB 0x1000 Prepare-Upload/bootloader.bin 0x8000 Prepare-Upload/partitions.bin 0x10000 Prepare-Upload/firmware.bin
Putty\putty.exe -serial %COMPORT% -sercfg 115200,8,n,1,N
esptool.exe --chip esp32 --port %COMPORT% --baud 460800 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size 4MB 0x1000 CANguru-Files/bootloader.bin 0x8000 CANguru-Files/partitions.bin 0x10000 CANguru-Files/firmware.bin
@echo off
echo.
pause
goto :loop

:Putty
@echo on
Putty\putty.exe -serial %COMPORT% -sercfg 115200,8,n,1,N
@echo off
echo.
pause
goto :loop

:errorInput
echo.
echo Falsche Eingabe! Bitte erneut versuchen!
echo.
pause
goto :loop


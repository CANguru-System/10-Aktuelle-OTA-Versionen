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
echo  4 - Upload (aus Ordner CANguru-Files) ueber %COMPORT%
echo  5 - Putty starten
echo. 
echo  x - Beenden
echo.
set /p SELECTED=Ihre Auswahl: 

if "%SELECTED%" == "x" goto :eof
if "%SELECTED%" == "1" goto :SetComPort
if "%SELECTED%" == "2" goto :ERASE_FLASH
if "%SELECTED%" == "3" goto :PREPARE_OTA
if "%SELECTED%" == "4" goto :UPLOAD_FIRMWARE
if "%SELECTED%" == "5" goto :Putty

goto :errorInput 


:SetComPort
echo  1 - Setze COM-Port 1
echo  2 - Setze COM-Port 2
echo  3 - Setze COM-Port 3
echo  4 - Setze COM-Port 4
echo  5 - Setze COM-Port 5
echo  6 - Setze COM-Port 6
echo  7 - Setze COM-Port 7
echo  8 - Setze COM-Port 8
echo  9 - Setze COM-Port 9
echo  10 - Setze COM-Port 10
echo  x - Exit
echo.
set /p SELECTED=Ihre Auswahl: 

if "%SELECTED%" == "x" goto :loop
 
if "%SELECTED%" == "1" (
set COMPORT=COM1
goto :loop
)
if "%SELECTED%" == "2" (
set COMPORT=COM2
goto :loop
)
if "%SELECTED%" == "3" (
set COMPORT=COM3
goto :loop
)
if "%SELECTED%" == "4" (
set COMPORT=COM4
goto :loop
)
if "%SELECTED%" == "5" (
set COMPORT=COM5
goto :loop
)
if "%SELECTED%" == "6" (
set COMPORT=COM6
goto :loop
)
if "%SELECTED%" == "7" (
set COMPORT=COM7
goto :loop
)
if "%SELECTED%" == "8" (
set COMPORT=COM8
goto :loop
)
if "%SELECTED%" == "9" (
set COMPORT=COM9
goto :loop
)
if "%SELECTED%" == "10" (
set COMPORT=COM10
goto :loop
)
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


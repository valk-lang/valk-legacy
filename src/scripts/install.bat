@echo off

set VOLT_PATH=%~dp0\

echo ;%PATH%; | find /C /I ";%VOLT_PATH%;" > nul
if not errorlevel 1 goto jump
set PATH=%PATH%;%VOLT_PATH%
setx /M PATH "%PATH%;%VOLT_PATH%"
:jump

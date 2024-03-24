@echo off

set VALI_PATH=%~dp0\

echo ;%PATH%; | find /C /I ";%VALI_PATH%;" > nul
if not errorlevel 1 goto jump
set PATH=%PATH%;%VALI_PATH%
setx /M PATH "%PATH%;%VALI_PATH%"
:jump

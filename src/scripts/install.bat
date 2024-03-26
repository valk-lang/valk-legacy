@echo off

set VALK_PATH=%~dp0\

echo ;%PATH%; | find /C /I ";%VALK_PATH%;" > nul
if not errorlevel 1 goto jump
set PATH=%PATH%;%VALK_PATH%
setx /M PATH "%PATH%;%VALK_PATH%"
:jump

::@echo off

echo ***Vars
set "TargetFolder=MapEditor"

echo Config=%~1
echo TargetFolder=%TargetFolder%
echo ***

call .\CopyCommon.bat "%~1" "%TargetFolder%"
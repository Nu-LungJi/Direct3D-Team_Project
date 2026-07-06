::@echo off

echo ***Vars
set "TargetFolder=Client"

echo Config=%~1
echo TargetFolder=%TargetFolder%
echo ***

call .\CopyCommon.bat "%~1" "%TargetFolder%"
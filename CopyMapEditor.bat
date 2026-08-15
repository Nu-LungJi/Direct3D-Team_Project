::@echo off

echo ***Vars
set "TargetFolder=MapEditor"

echo Config=%~1
echo TargetFolder=%TargetFolder%
echo ***

call .\CopyCommon.bat "%~1" "%TargetFolder%"
:: MapEditor uses Client-authored pixel shaders. Decal materials load from Resources/json/DecalMaterials.
xcopy /E /I /Y /D .\Client\ShaderFiles\Decal\*.* .\%TargetFolder%\Bin\ShaderFiles\Decal\


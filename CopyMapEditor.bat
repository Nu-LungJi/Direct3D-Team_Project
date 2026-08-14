::@echo off

echo ***Vars
set "TargetFolder=MapEditor"

echo Config=%~1
echo TargetFolder=%TargetFolder%
echo ***

call .\CopyCommon.bat "%~1" "%TargetFolder%"
:: MapEditor uses the Client-authored decal materials and pixel shaders.
xcopy /E /I /Y /D .\Client\ShaderFiles\Decal\*.* .\%TargetFolder%\Bin\ShaderFiles\Decal\
xcopy /E /I /Y /D .\Client\DecalMaterials\*.* .\%TargetFolder%\Bin\DecalMaterials\


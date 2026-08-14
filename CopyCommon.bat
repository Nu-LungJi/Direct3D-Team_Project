::@echo off

:: Common Settings
set "TargetFolder=%~2"

:: Engine Copy
xcopy /E /I /Y /D .\Engine\public\*.* .\EngineSDK\Inc\
xcopy /E /I /Y /D .\Engine\Bin\Engine.dll .\%TargetFolder%\Bin\
xcopy /E /I /Y /D .\Engine\Bin\Engine.lib .\EngineSDK\Lib\

:: Shader Copy
xcopy /E /I /Y /D .\%TargetFolder%\ShaderFiles\*.* .\%TargetFolder%\Bin\ShaderFiles\
xcopy /E /I /Y /D .\Engine\ShaderFiles\*.* .\%TargetFolder%\Bin\ShaderFiles\

:: Lua Copy
xcopy /E /I /Y /D .\%TargetFolder%\LuaFiles\*.* .\%TargetFolder%\Bin\LuaFiles\
xcopy /E /I /Y /D .\Engine\LuaFiles\*.* .\%TargetFolder%\Bin\LuaFiles\

:: JSON Copy
:: [주의] 에디터로 생성하거나 수정하는 JSON 원본은 반드시 ../{Solution}/JsonFiles/ 아래에 저장한다.
:: [주의] Bin/JsonFiles/는 빌드할 때 구성되는 실행용 경로이므로 원본을 직접 저장하거나 수정하지 않는다.
:: Git 추적이 필요한 신규 JSON은 각 솔루션의 JsonFiles/에서 관리한다.
:: Engine 공용 JSON을 먼저 복사하고 같은 경로가 있으면 대상 프로젝트 JSON으로 덮어쓴다.
if exist ".\Engine\JsonFiles\" (
    xcopy /E /I /Y .\Engine\JsonFiles\*.* .\%TargetFolder%\Bin\JsonFiles\
)
if exist ".\%TargetFolder%\JsonFiles\" (
    xcopy /E /I /Y .\%TargetFolder%\JsonFiles\*.* .\%TargetFolder%\Bin\JsonFiles\
)

:: ThirdParty
xcopy /E /I /Y /D .\ThirdParty\fmod_2_03_12\lib\x64\fmod.dll .\%TargetFolder%\Bin\
xcopy /E /I /Y /D .\ThirdParty\HBAOPlus-3.1.0\lib\GFSDK_SSAO_D3D11.win64.dll .\%TargetFolder%\Bin\

if /I "%~1"=="Release" (
    xcopy /E /I /Y /D .\vcpkg_installed\x64-windows\x64-windows\bin\*.dll .\%TargetFolder%\Bin\
    xcopy /E /I /Y .\ThirdParty\physx-5.6.1\bin_release\*.dll .\%TargetFolder%\Bin\
    xcopy /E /I /Y .\ThirdParty\NvCloth_1.1.6\bin\Release\NvCloth_x64.dll .\%TargetFolder%\Bin\
) else (
    xcopy /E /I /Y /D .\vcpkg_installed\x64-windows\x64-windows\debug\bin\*.dll .\%TargetFolder%\Bin\
    xcopy /E /I /Y .\ThirdParty\physx-5.6.1\bin_debug\*.dll .\%TargetFolder%\Bin\
    xcopy /E /I /Y .\ThirdParty\NvCloth_1.1.6\bin\Release\NvCloth_x64.dll .\%TargetFolder%\Bin\
)

exit /b

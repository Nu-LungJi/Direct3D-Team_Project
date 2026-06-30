// 명령어				옵션			원본파일이 있는 위치			사본파일을 저장할 위치

xcopy				/y/I			.\Engine\public\*.*			.\EngineSDK\Inc\
xcopy				/y/I			.\Engine\Bin\Engine.dll		.\Client\Bin\
xcopy				/y/I			.\Engine\Bin\Engine.lib		.\EngineSDK\Lib\
//xcopy				/y/I /d			.\vcpkg_installed\x64-windows\x64-windows\bin\*.dll .\Client\Bin\
xcopy				/y/I			.\ThirdParty\fmod_2_03_12\lib\x64\fmod.dll		.\Client\Bin\
if "$(Configuration)" == "Release" (
    xcopy /y/I /d .\vcpkg_installed\x64-windows\x64-windows\bin\*.dll .\Client\Bin\
) else (
    xcopy /y/I /d .\vcpkg_installed\x64-windows\x64-windows\debug\bin\*.dll .\Client\Bin\
)
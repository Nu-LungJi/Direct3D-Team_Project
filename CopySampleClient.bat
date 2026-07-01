// 명령어				옵션			원본파일이 있는 위치			사본파일을 저장할 위치

xcopy				/y/I			.\Engine\public\*.*			.\EngineSDK\Inc\
xcopy				/y/I			.\Engine\Bin\Engine.dll		.\SampleClient\Bin\
xcopy				/y/I			.\Engine\Bin\Engine.lib		.\EngineSDK\Lib\
//xcopy				/y/I /d			.\vcpkg_installed\x64-windows\x64-windows\bin\*.dll .\SampleClient\Bin\
xcopy				/y/I			.\ThirdParty\fmod_2_03_12\lib\x64\fmod.dll		.\SampleClient\Bin\
if "$(Configuration)" == "Release" (
    xcopy /y/I /d .\vcpkg_installed\x64-windows\x64-windows\bin\*.dll .\SampleClient\Bin\
) else (
    xcopy /y/I /d .\vcpkg_installed\x64-windows\x64-windows\debug\bin\*.dll .\SampleClient\Bin\
)

//1. 첫 번째 폴더의 내용을 대상 폴더로 복사
xcopy /y /i /d /e .\SampleClient\ShaderFiles\*.* .\SampleClient\Bin\ShaderFiles\

// 2. 두 번째 폴더의 내용을 같은 대상 폴더로 복사 (이미 있는 폴더는 병합됨)
xcopy /y /i /d /e .\Engine\ShaderFiles\*.* .\SampleClient\Bin\ShaderFiles\
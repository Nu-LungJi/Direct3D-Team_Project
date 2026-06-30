1. 각자 컴퓨터에 vcpkg설치하기
git clone https://github.com/microsoft/vcpkg.git

2. 부트스트랩 실행:
bootstrap-vcpkg.bat

3. 부트스트랩 설치 확인:
vcpkg version

4. visual studio vcpkg 연동 확인
vcpkg integrate install

5. Bin폴더에 리소시즈 심볼릭 링크 달아주기
-리소스 심볼릭 링크 생성, 파워셀 관리자모드 켜서 클라이언트의 빈폴더로 가서 아래 입력 
New-Item -ItemType SymbolicLink -Path "./Resources" -Target "../../../JUSIN_160_FINAL_TEAM_RESOURCE"
-리소스 심볼릭 링크 생성, 파워셀 관리자모드 켜서 클라이언트의 빈폴더로 가서 아래 입력 
New-Item -ItemType SymbolicLink -Path "./Resources" -Target "../../../MyResource"
#include "pch.h"
#include "SerializeManager.h"

#include "SerDeTestCase.h"

NS_USING(Engine)

// CSerializeManager.cpp 상단

// 고급 컨테이너 직렬화 테스트용 더미 데이터
struct SAdvancedTestDummy : public ISerializable
{
	std::string playerName = "EpicPlayer";
	int level = 1;

	// 1. 동적 배열 (Vector)
	std::vector<int> inventory;

	// 2. 키-밸류 쌍 (Map)
	std::map<std::string, float> stats;

	// 생성자에서 기본 더미 데이터 세팅
	SAdvancedTestDummy() {
		inventory = { 1001, 1002, 1003 };
		stats["HP"] = 100.0f;
		stats["MP"] = 50.0f;
		stats["Speed"] = 5.5f;
	}

	virtual void Serialize(ISerializer& s) const override {
		s.Write("playerName", playerName);
		s.Write("level", level);
		s.Write("inventory", inventory); // Vector 한 줄 처리!
		s.Write("stats", stats);         // Map 한 줄 처리!
	}

	virtual void Deserialize(IDeserializer& d) override {
		d.Read("playerName", playerName);
		d.Read("level", level);
		d.Read("inventory", inventory);
		d.Read("stats", stats);
	}
};

CSerializeManager::CSerializeManager()
{
}

CSerializeManager::~CSerializeManager()
{
}

void CSerializeManager::UpdateGUI()
{
	ImGui::Begin("Advanced Serialize Tester");

	// 상태 유지를 위한 정적(static) 변수들
	static char szFileName[128] = "AdvancedSave";
	static int saveFormat = 0; // 0 = JSON, 1 = BINARY
	static std::string statusMsg = "Ready to test containers.";
	static ImVec4 statusColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

	// 고급 테스트 구조체 인스턴스
	static SAdvancedTestDummy dummyData;
	static char nameBuf[128] = "EpicPlayer";

	// =======================================================
	// 1. 포맷 및 파일 이름
	// =======================================================
	ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "[ 1. Save Settings ]");
	ImGui::RadioButton("JSON", &saveFormat, 0); ImGui::SameLine();
	ImGui::RadioButton("BINARY", &saveFormat, 1);
	ImGui::InputText("File Name", szFileName, sizeof(szFileName));
	ImGui::Separator();

	// =======================================================
	// 2. 기본 데이터 타입 수정
	// =======================================================
	ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "[ 2. Basic Types ]");
	if (ImGui::InputText("Player Name", nameBuf, sizeof(nameBuf))) {
		dummyData.playerName = nameBuf;
	}
	ImGui::InputInt("Level", &dummyData.level);
	ImGui::Separator();

	// =======================================================
	// 3. 컨테이너 데이터 수정 (Vector & Map)
	// =======================================================
	ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "[ 3. Containers ]");

	// --- A. std::vector 테스트 영역 ---
	if (ImGui::TreeNode("Inventory (std::vector<int>)"))
	{
		for (size_t i = 0; i < dummyData.inventory.size(); ++i) {
			ImGui::PushID((int)i); // [중요] ImGui가 각 항목을 구별할 수 있게 고유 ID 부여

			ImGui::SetNextItemWidth(100);
			ImGui::InputInt("##item", &dummyData.inventory[i]);

			ImGui::SameLine();
			if (ImGui::Button("X")) {
				// 삭제 버튼: 누르면 해당 원소 삭제 후 루프 탈출 (이터레이터 꼬임 방지)
				dummyData.inventory.erase(dummyData.inventory.begin() + i);
				ImGui::PopID();
				break;
			}
			ImGui::PopID();
		}

		if (ImGui::Button("+ Add Item")) {
			dummyData.inventory.push_back(9999); // 기본값으로 추가
		}
		ImGui::TreePop();
	}

	// --- B. std::map 테스트 영역 ---
	if (ImGui::TreeNode("Stats (std::map<string, float>)"))
	{
		std::string keyToRemove = "";

		// 기존 맵 데이터 출력 및 값 수정
		for (auto& [key, value] : dummyData.stats) {
			ImGui::PushID(key.c_str());

			ImGui::Text("%-10s", key.c_str()); // 키(Key) 출력
			ImGui::SameLine(100);

			ImGui::SetNextItemWidth(100);
			ImGui::InputFloat("##val", &value); // 값(Value) 수정

			ImGui::SameLine();
			if (ImGui::Button("X")) {
				keyToRemove = key; // 즉시 삭제하면 맵 순회 에러가 나므로 이름표만 기록
			}
			ImGui::PopID();
		}
		if (!keyToRemove.empty()) dummyData.stats.erase(keyToRemove);

		// 새로운 맵 데이터 추가 UI
		ImGui::Separator();
		static char newStatName[64] = "STR";
		static float newStatVal = 10.0f;

		ImGui::SetNextItemWidth(80);
		ImGui::InputText("##newStatKey", newStatName, sizeof(newStatName));
		ImGui::SameLine();
		ImGui::SetNextItemWidth(80);
		ImGui::InputFloat("##newStatVal", &newStatVal);
		ImGui::SameLine();
		if (ImGui::Button("+ Add Stat")) {
			dummyData.stats[newStatName] = newStatVal;
		}
		ImGui::TreePop();
	}
	ImGui::Separator();

	// =======================================================
	// 4. 저장 / 로드 액션 버튼
	// =======================================================
	ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "[ 4. Actions ]");

	std::string ext = (saveFormat == 0) ? ".json" : ".bin";
	std::string fullPath = std::string(szFileName) + ext;

	if (ImGui::Button("SAVE DATA", ImVec2(150, 40))) {
		HRESULT hr = (saveFormat == 0) ? JsonSerialize(fullPath, dummyData) : BinSerialize(fullPath, dummyData);
		if (SUCCEEDED(hr)) {
			statusMsg = "SAVE SUCCESS! -> " + fullPath;
			statusColor = ImVec4(0.2f, 1.0f, 0.2f, 1.0f);
		}
		else {
			statusMsg = "SAVE FAILED!";
			statusColor = ImVec4(1.0f, 0.2f, 0.2f, 1.0f);
		}
	}

	ImGui::SameLine();

	if (ImGui::Button("LOAD DATA", ImVec2(150, 40))) {
		HRESULT hr = (saveFormat == 0) ? JsonDeSerialize(fullPath, dummyData) : BinDeSerialize(fullPath, dummyData);
		if (SUCCEEDED(hr)) {
			strcpy_s(nameBuf, dummyData.playerName.c_str()); // 로드된 이름으로 UI 버퍼 갱신
			statusMsg = "LOAD SUCCESS! <- " + fullPath;
			statusColor = ImVec4(0.2f, 1.0f, 0.2f, 1.0f);
		}
		else {
			statusMsg = "LOAD FAILED!";
			statusColor = ImVec4(1.0f, 0.2f, 0.2f, 1.0f);
		}
	}

	ImGui::SameLine();

	// 데이터 꼬임 방지용 리셋 버튼
	if (ImGui::Button("RESET", ImVec2(80, 40))) {
		dummyData = SAdvancedTestDummy();
		strcpy_s(nameBuf, dummyData.playerName.c_str());
		statusMsg = "Data reset to default.";
		statusColor = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
	}

	// =======================================================
	// 5. 상태 표시창
	// =======================================================
	ImGui::Separator();
	ImGui::Text("Status: ");
	ImGui::SameLine();
	ImGui::TextColored(statusColor, "%s", statusMsg.c_str());

	ImGui::End();
}

HRESULT CSerializeManager::Initialize()
{
	return S_OK;
}

UPtr<CSerializeManager> CSerializeManager::Create()
{
	auto pInstance = ToUPtr(new CSerializeManager);
	if (FAILED(pInstance->Initialize()))
	{
		return nullptr;
	}
	return pInstance;
}

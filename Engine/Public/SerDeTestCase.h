#pragma once
#include "Engine_Defines.h"
#include "ISerializable.h"
#include "SerializerInterface.h"
#include "JsonSerializer.h"
#include "JsonDeSerializer.h"
#include "BinSerializer.h"
#include "BinDeSerializer.h"

#include <vector>
#include <list>
#include <map>
#include <unordered_map>
#include <string>
#include <fstream>

struct Float3Comparator {
	bool operator()(const _float3& a, const _float3& b) const {
		if (a.x != b.x) return a.x < b.x;
		if (a.y != b.y) return a.y < b.y;
		return a.z < b.z;
	}
};

struct Float3Hasher {
	size_t operator()(const _float3& v) const {
		// 해시 조합 기법 (boost::hash_combine과 유사한 방식)
		size_t h1 = std::hash<float>{}(v.x);
		size_t h2 = std::hash<float>{}(v.y);
		size_t h3 = std::hash<float>{}(v.z);

		// 비트 연산을 통해 해시값들을 섞습니다.
		// 0x9e3779b9는 골든 레이시오 기반의 매직 넘버입니다.
		h1 ^= h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2);
		h1 ^= h3 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2);
		return h1;
	}
};

// 필수: operator== (이미 정의되어 있다면 생략 가능)
struct Float3Equal {
	bool operator()(const _float3& a, const _float3& b) const {
		return (a.x == b.x) && (a.y == b.y) && (a.z == b.z);
	}
};

NS_BEGIN(Engine)

namespace TestEnum
{
	enum SomeEnum
	{
		One,
		Two,
		Three
	};

	enum class SomeClassEnum
	{
		First,
		Second,
		Third
	};
}

class CItemData final : public CEngineBase, public ISerializable
{
public:
	DECLARE_DERIVED_TYPE(CItemData, CEngineBase)

	int         itemID = 0;
	float       weight = 0.f;
	std::string itemName = "";

	_float2     position2D = { 0.f, 0.f };
	_float3     position3D = { 0.f, 0.f, 0.f };
	_float4     colorRGBA = { 0.f, 0.f, 0.f, 0.f };
	_float4x4   transformMatrix{};

	void Serialize(ISerializer& serializer) const override
	{
		serializer.Write("itemID", itemID);
		serializer.Write("weight", weight);
		serializer.Write("itemName", itemName);
		serializer.Write("position2D", position2D);
		serializer.Write("position3D", position3D);
		serializer.Write("colorRGBA", colorRGBA);
		serializer.Write("transformMatrix", transformMatrix);
	}

	void Deserialize(IDeserializer& deserializer) override
	{
		deserializer.Read("itemID", itemID);
		deserializer.Read("weight", weight);
		deserializer.Read("itemName", itemName);
		deserializer.Read("position2D", position2D);
		deserializer.Read("position3D", position3D);
		deserializer.Read("colorRGBA", colorRGBA);
		deserializer.Read("transformMatrix", transformMatrix);
	}

	bool operator==(const CItemData& rhs) const
	{
		return (itemID == rhs.itemID) &&
			(weight == rhs.weight) &&
			(itemName == rhs.itemName) &&
			(memcmp(&position2D, &rhs.position2D, sizeof(_float2)) == 0) &&
			(memcmp(&position3D, &rhs.position3D, sizeof(_float3)) == 0) &&
			(memcmp(&colorRGBA, &rhs.colorRGBA, sizeof(_float4)) == 0) &&
			(memcmp(&transformMatrix, &rhs.transformMatrix, sizeof(_float4x4)) == 0);
	}
};

class CCharacterData final : public CEngineBase, public ISerializable
{
public:
	DECLARE_DERIVED_TYPE(CCharacterData, CEngineBase)

	std::string charName = "";
	int         level = 1;

	std::vector<CItemData> inventory;
	std::list<int> questIDs; // 리스트 추가
	std::map<std::string, int> statsMap;
	std::unordered_map<int, float> cooldowns;

	void Serialize(ISerializer& serializer) const override
	{
		serializer.Write("charName", charName);
		serializer.Write("level", level);

		serializer.Write("Inventory", inventory);
		serializer.Write("QuestIDs", questIDs);
		serializer.Write("StatsMap", statsMap);
		serializer.Write("Cooldowns", cooldowns);
	}

	void Deserialize(IDeserializer& deserializer) override
	{
		deserializer.Read("charName", charName);
		deserializer.Read("level", level);

		deserializer.Read("Inventory", inventory);
		deserializer.Read("QuestIDs", questIDs);
		deserializer.Read("StatsMap", statsMap);
		deserializer.Read("Cooldowns", cooldowns);
	}

	bool operator==(const CCharacterData& rhs) const
	{
		return (charName == rhs.charName) &&
			(level == rhs.level) &&
			(inventory == rhs.inventory) && // vector 비교
			(questIDs == rhs.questIDs) &&   // list 비교
			(statsMap == rhs.statsMap) &&   // map 비교
			(cooldowns == rhs.cooldowns);   // unordered_map 비교
	}

};

class CWorldData final : public CEngineBase, public ISerializable
{
public:
	DECLARE_DERIVED_TYPE(CWorldData, CEngineBase)

	std::string worldName = "Mega_World";
	std::vector<CCharacterData> players;
	std::vector<CCharacterData> monsters;
	std::vector<_float4x4> mats;
	std::set<_float3, Float3Comparator> sets;
	std::unordered_set<_float3, Float3Hasher, Float3Equal> unorderSets;
	std::unordered_set<std::string> unorderStringSet;
	uint32_t ui32{};
	uint64_t ui64{};
	bool b{};
	TestEnum::SomeEnum eSomeEnum{};
	TestEnum::SomeClassEnum eSomeClassEnum{};

	void Serialize(ISerializer& serializer) const override
	{
		serializer.Write("worldName", worldName);
		serializer.Write("Players", players);
		serializer.Write("Monsters", monsters);
		serializer.Write("Mats", mats);
		serializer.Write("Sets", sets);
		serializer.Write("UnorderSets", unorderSets);
		serializer.Write("UnorderStringSet", unorderStringSet);
		serializer.Write("ui32", ui32);
		serializer.Write("ui64", ui64);
		serializer.Write("b", b);
		serializer.Write("eSomeEnum", eSomeEnum);
		serializer.Write("eSomeClassEnum", eSomeClassEnum);
	}

	void Deserialize(IDeserializer& deserializer) override
	{
		deserializer.Read("worldName", worldName);
		deserializer.Read("Players", players);
		deserializer.Read("Monsters", monsters);
		deserializer.Read("Mats", mats);
		deserializer.Read("Sets", sets);
		deserializer.Read("UnorderSets", unorderSets);
		deserializer.Read("UnorderStringSet", unorderStringSet);
		deserializer.Read("ui32", ui32);
		deserializer.Read("ui64", ui64);
		deserializer.Read("b", b);
		deserializer.Read("eSomeEnum", eSomeEnum);
		deserializer.Read("eSomeClassEnum", eSomeClassEnum);
	}

	bool operator==(const CWorldData& rhs) const
	{
		{
			auto it1 = sets.begin();
			auto it2 = rhs.sets.begin();

			while (it1 != sets.end())
			{
				if (it1->x != it2->x ||
					it1->y != it2->y ||
					it1->z != it2->z)
				{
					return false;
				}

				++it1;
				++it2;
			}
		}
		return (worldName == rhs.worldName) &&
			(players == rhs.players) &&
			(monsters == rhs.monsters) &&
			//(sets == rhs.sets) &&
			(unorderSets == rhs.unorderSets) &&
			(ui32 == rhs.ui32) &&
			(ui64 == rhs.ui64) &&
			(b == rhs.b) &&
			(memcmp(&mats[0], &rhs.mats[0], sizeof(_float4x4) * rhs.mats.size()) == 0);
	}
};

inline std::string GetFileSizeString(const std::string& path)
{
	std::ifstream in(path, std::ifstream::ate | std::ifstream::binary);
	if (in.is_open()) return std::to_string(in.tellg()) + " bytes";
	return "File Not Found";
}

// =========================================================================
// 테스트 실행
// =========================================================================
inline void RunMegaSerializationTest()
{
	// ----------------------------------------------------
	// 데이터 세팅
	// ----------------------------------------------------
	CWorldData originalWorld;
	originalWorld.worldName = "The_Ultimate_Test_World";

	const int NUM_PLAYERS = 18;
	const int NUM_ITEMS = 28;

	for (int p = 0; p < NUM_PLAYERS; ++p)
	{
		CCharacterData player;
		player.charName = "Player_" + std::to_string(p);
		player.level = 100 + p;

		player.questIDs.push_back(1001 + p);
		player.questIDs.push_back(2002 + p);

		for (int i = 0; i < NUM_ITEMS; ++i)
		{
			CItemData item;
			item.itemID = (p * 1000) + i;
			item.itemName = "Item_Lvl_" + std::to_string(i);
			item.weight = i * 1.5f;
			item.position3D = { (float)i, (float)p, 1.0f };
			player.inventory.push_back(item);
		}

		player.statsMap["STR"] = 10 + p;
		player.statsMap["DEX"] = 20 + p;
		player.cooldowns[1001] = 5.5f;
		player.cooldowns[2002] = 12.3f;

		originalWorld.players.push_back(player);
	}


	for (uint32_t i = 0; i < 18; ++i)
	{
		_float4x4 tmp;
		XMStoreFloat4x4(&tmp, XMMatrixScaling(1.f, 1.f, 2.f) * XMMatrixRotationX(1.f) * XMMatrixTranslation(i, i, i));
		originalWorld.mats.push_back(tmp);
	}

	originalWorld.sets.insert({ 1.f, 2.f, 3.f});

	originalWorld.unorderSets.insert({ 4.f, 5.f, 6.f });

	originalWorld.unorderStringSet.insert("ZZZZ");
	originalWorld.unorderStringSet.insert("ZZZZ");
	originalWorld.unorderStringSet.insert("ZZZZ123");

	originalWorld.ui32 = 1818;
	originalWorld.ui64 = 9999999999999;
	originalWorld.b = false;

	originalWorld.eSomeClassEnum = TestEnum::SomeClassEnum::Third;
	originalWorld.eSomeEnum = TestEnum::Two;

	// ---------------------------------
	// ---------------------------------
	// ---------------------------------

	// 상태 트래킹 변수 (구체적으로 나눔)
	bool bJsonSaveSuccess = false;
	bool bJsonLoadSuccess = false;
	bool bJsonDataMatch = false;

	bool bBinSaveSuccess = false;
	bool bBinLoadSuccess = false;
	bool bBinDataMatch = false;

	// ----------------------------------------------------
	// JSON 테스트
	// ----------------------------------------------------
	try {
		// [Save]
		auto pJsonSer = CJsonSerializer::Create();
		if (pJsonSer) {
			pJsonSer->Write("RootWorld", originalWorld);
			pJsonSer->SaveToFile("MegaTest_SaveData.json");
			bJsonSaveSuccess = true;
		}

		// [Load]
		if (bJsonSaveSuccess) {
			auto pJsonDeSer = CJsonDeSerializer::Create("MegaTest_SaveData.json");
			if (pJsonDeSer) {
				CWorldData restoredWorld;
				pJsonDeSer->Read("RootWorld", restoredWorld);
				bJsonLoadSuccess = true;

				// [Match]
				if (originalWorld == restoredWorld) bJsonDataMatch = true;
			}
		}
	}
	catch (...) { /* Crash Guard */ }

	// ----------------------------------------------------
	// BINARY 테스트
	// ----------------------------------------------------
	try {
		// [Save]
		auto pBinSer = CBinSerializer::Create();
		if (pBinSer) {
			pBinSer->Write("RootWorld", originalWorld);
			pBinSer->SaveToFile("MegaTest_SaveData.bin");
			bBinSaveSuccess = true;
		}

		// [Load]
		if (bBinSaveSuccess) {
			auto pBinDeSer = CBinDeSerializer::Create("MegaTest_SaveData.bin");
			if (pBinDeSer) {
				CWorldData restoredWorld;
				pBinDeSer->Read("RootWorld", restoredWorld);
				bBinLoadSuccess = true;

				// [Match]
				if (originalWorld == restoredWorld) bBinDataMatch = true;
			}
		}
	}
	catch (...) { /* Crash Guard */ }


	// ----------------------------------------------------
	// 상세 리포트 메시지 박스 생성
	// ----------------------------------------------------
	std::string resultMsg = "====== Serialization Test Report ======\n\n";

	auto GetMark = [](bool val) { return val ? "[ OK ]" : "[ FAIL ]"; };

	// JSON 파트
	resultMsg += " >JSON Format\n";
	resultMsg += " >>JsonSerializer (Save)  : " + std::string(GetMark(bJsonSaveSuccess)) + "\n";
	resultMsg += " >>JsonDeSerializer(Load) : " + std::string(GetMark(bJsonLoadSuccess)) + "\n";
	resultMsg += " >>Data Integrity Match   : " + std::string(GetMark(bJsonDataMatch)) + "\n\n";

	// BINARY 파트
	resultMsg += ">BINARY Format\n";
	resultMsg += ">>BinSerializer (Save)   : " + std::string(GetMark(bBinSaveSuccess)) + "\n";
	resultMsg += ">>BinDeSerializer(Load)  : " + std::string(GetMark(bBinLoadSuccess)) + "\n";
	resultMsg += ">>Data Integrity Match   : " + std::string(GetMark(bBinDataMatch)) + "\n\n";

	// 파일 크기 비교 (잘 써졌는지 덤으로 확인)
	resultMsg += " Generated File Size\n";
	resultMsg += " - JSON File : " + GetFileSizeString("MegaTest_SaveData.json") + "\n";
	resultMsg += " - BIN File  : " + GetFileSizeString("MegaTest_SaveData.bin") + "\n\n";

	// 최종 결론
	if (bJsonDataMatch && bBinDataMatch) {
		resultMsg += "=> RESULT: ALL PERFECT! NO BUGS FOUND.";
	}
	else {
		resultMsg += "=> RESULT: ERROR OCCURRED! Check the failed steps above.";
	}

	MessageBoxA(nullptr, resultMsg.c_str(), "Serializer Test Report", MB_OK | MB_ICONINFORMATION);
}

NS_END

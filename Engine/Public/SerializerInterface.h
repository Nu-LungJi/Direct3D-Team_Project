#pragma once
#include "Engine_Defines.h"
#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <type_traits> // type_traits 필수 포함

NS_BEGIN(Engine)

class ISerializable;

class ENGINE_DLL ISerializer {
public:
	virtual ~ISerializer() = default;

	virtual void Write(const std::string& key, int value) = 0;
	virtual void Write(const std::string& key, float value) = 0;
	virtual void Write(const std::string& key, const std::string& value) = 0;
	virtual void Write(const std::string& key, const _float2& value) = 0;
	virtual void Write(const std::string& key, const _float3& value) = 0;
	virtual void Write(const std::string& key, const _float4& value) = 0;
	virtual void Write(const std::string& key, const _float4x4& value) = 0;
	virtual void Write(const std::string& key, const ISerializable& value) = 0;


	template<typename InputIt>
	void WriteArray(const std::string& key, InputIt begin, InputIt end)
	{
		StartArray(key);

		for (auto it = begin; it != end; ++it)
		{
			using ValueType = std::decay_t<decltype(*it)>;

			if constexpr (std::is_pointer_v<ValueType>)
			{
				if (*it != nullptr) Write("", **it); 
			}
			else
			{
				Write("", *it);
			}
		}

		EndArray();
	}

	template<typename T>
	void WriteArray(const std::string& key, const T* startPtr, size_t size)
	{
		WriteArray(key, startPtr, startPtr + size);
	}

	template<typename Container>
	auto WriteArray(const std::string& key, const Container& container)
		-> decltype(std::begin(container), std::end(container), void()) // 컨테이너인지 SFINAE 검사
	{
		WriteArray(key, std::begin(container), std::end(container));
	}


	//template<typename MapType>
	//void WriteMap(const std::string& key, const MapType& mapData)
	//{
	//	StartMap(key);

	//	for (const auto& [itemKey, itemValue] : mapData)
	//	{
	//		using ValueType = std::decay_t<decltype(itemValue)>;

	//		if constexpr (std::is_pointer_v<ValueType>)
	//		{
	//			// 밸류가 포인터인 경우
	//			if (itemValue != nullptr) {
	//				Write(itemKey, *itemValue);
	//			}
	//		}
	//		else
	//		{
	//			// 밸류가 기본 타입
	//			Write(itemKey, itemValue);
	//		}
	//	}

	//	EndMap();
	//}


	template<typename K, typename V>
	void WriteMap(const std::string& key, const std::map<K, V>& mapData)
	{
		StartMap(key);
		for (const auto& [itemKey, itemValue] : mapData)
		{
			// 핵심: Key 타입을 문자열로 변환해야 JSON 키로 쓸 수 있습니다.
			std::string stringKey;
			if constexpr (std::is_same_v<K, std::string>) stringKey = itemKey;
			else if constexpr (requires { itemKey.GetDbgStr(); }) stringKey = itemKey.GetDbgStr(); // StringID 대응
			else stringKey = std::to_string(itemKey); // int 등 숫자 키 대응

			using ValueType = std::decay_t<decltype(itemValue)>;
			if constexpr (std::is_pointer_v<ValueType>) {
				if (itemValue != nullptr) Write(stringKey, *itemValue);
			}
			else {
				Write(stringKey, itemValue);
			}
		}
		EndMap();
	}

	template<typename K, typename V>
	void WriteMap(const std::string& key, const std::unordered_map<K, V>& mapData)
	{
		StartMap(key);
		for (const auto& [itemKey, itemValue] : mapData)
		{
			// 핵심: Key 타입을 문자열로 변환해야 JSON 키로 쓸 수 있습니다.
			std::string stringKey;
			if constexpr (std::is_same_v<K, std::string>) stringKey = itemKey;
			else if constexpr (requires { itemKey.GetDbgStr(); }) stringKey = itemKey.GetDbgStr(); // StringID 대응
			else stringKey = std::to_string(itemKey); // int 등 숫자 키 대응

			using ValueType = std::decay_t<decltype(itemValue)>;
			if constexpr (std::is_pointer_v<ValueType>) {
				if (itemValue != nullptr) Write(stringKey, *itemValue);
			}
			else {
				Write(stringKey, itemValue);
			}
		}
		EndMap();
	}

protected:
	virtual void StartArray(const std::string& key) = 0;
	virtual void EndArray() = 0;
	virtual void StartMap(const std::string& key) = 0;
	virtual void EndMap() = 0;
};

class ENGINE_DLL IDeserializer {
public:
	virtual ~IDeserializer() = default;

	// 1. 단일 원자 타입 및 객체 읽기
	virtual void Read(const std::string& key, int& outValue) = 0;
	virtual void Read(const std::string& key, float& outValue) = 0;
	virtual void Read(const std::string& key, std::string& outValue) = 0;
	virtual void Read(const std::string& key, _float2& outValue) = 0;
	virtual void Read(const std::string& key, _float3& outValue) = 0;
	virtual void Read(const std::string& key, _float4& outValue) = 0;
	virtual void Read(const std::string& key, _float4x4& outValue) = 0;
	virtual void Read(const std::string& key, ISerializable& outValue) = 0;

	// 2. 컨테이너 노드 제어 (public으로 열어두어야 외부에서 접근 가능)
	// 배열의 크기를 알아야 for문을 돌 수 있으므로 size_t 반환!
	virtual size_t StartArray(const std::string& key) = 0;
	virtual void EndArray() = 0;

	// 1. 맵도 배열처럼 개수(size_t)를 반환하도록 변경합니다.
	virtual size_t StartMap(const std::string& key) = 0;
	virtual void EndMap() = 0;

	// 2. 전체 키를 가져오는 GetMapKeys() 대신, 현재 위치의 단일 Key를 읽는 함수로 변경
	virtual std::string ReadMapKey() = 0;

	// =======================================================
	// [수정된] 맵(Map) 통째로 읽기 템플릿
	// =======================================================
	//template<typename V>
	//void ReadMap(const std::string& key, std::map<std::string, V>& outMap)
	//{
	//	// 1. 맵에 들어있는 아이템 개수를 받아옵니다.
	//	size_t count = StartMap(key);

	//	// 2. 개수만큼 순차적으로 [Key] -> [Value] 순서대로 읽습니다.
	//	for (size_t i = 0; i < count; ++i)
	//	{
	//		std::string k = ReadMapKey(); // 바이너리 스트림에서 문자열 1개 읽기
	//		Read(k, outMap[k]);           // 이어서 해당 밸류 읽기
	//	}

	//	EndMap();
	//}

	// unordered_map 버전도 동일하게 지원
	//template<typename V>
	//void ReadMap(const std::string& key, std::unordered_map<std::string, V>& outMap)
	//{
	//	size_t count = StartMap(key);
	//	for (size_t i = 0; i < count; ++i)
	//	{
	//		std::string k = ReadMapKey(); // 바이너리 스트림에서 문자열 1개 읽기
	//		Read(k, outMap[k]);           // 이어서 해당 밸류 읽기
	//	}
	//	EndMap();
	//}

	template<typename K, typename V>
	void ReadMap(const std::string& key, std::map<K, V>& outMap)
	{
		size_t count = StartMap(key);
		for (size_t i = 0; i < count; ++i)
		{
			std::string stringKey = ReadMapKey(); // 항상 문자열 키를 읽음

			// 문자열 키를 다시 K 타입으로 변환
			K k;
			if constexpr (std::is_same_v<K, std::string>) k = stringKey;
			else if constexpr (std::is_same_v<K, int>) k = std::stoi(stringKey);
			else k = K(stringKey.c_str()); // StringID(const char*) 생성자 활용

			Read(stringKey, outMap[k]); // Read 내부에서 키를 무시하고 밸류만 읽도록 처리
		}
		EndMap();
	}

	template<typename K, typename V>
	void ReadMap(const std::string& key, std::unordered_map<K, V>& outMap)
	{
		size_t count = StartMap(key);
		for (size_t i = 0; i < count; ++i)
		{
			std::string stringKey = ReadMapKey(); // 항상 문자열 키를 읽음

			// 문자열 키를 다시 K 타입으로 변환
			K k;
			if constexpr (std::is_same_v<K, std::string>) k = stringKey;
			else if constexpr (std::is_same_v<K, int>) k = std::stoi(stringKey);
			else k = K(stringKey.c_str()); // StringID(const char*) 생성자 활용

			Read(stringKey, outMap[k]); // Read 내부에서 키를 무시하고 밸류만 읽도록 처리
		}
		EndMap();
	}

	
	template<typename T>
	void ReadArray(const std::string& key, std::vector<T>& outVec)
	{
		// 1. JSON 배열에 진입하면서 원소 개수를 받아옵니다.
		size_t count = StartArray(key);

		if (count > 0)
		{
			// 2. 컨테이너 크기를 미리 세팅합니다.
			outVec.resize(count);

			// 3. 루프를 돌면서 순서대로 데이터를 채워 넣습니다.
			for (size_t i = 0; i < count; ++i)
			{
				// 배열 안에서는 키값이 무의미하므로 ""를 넘깁니다.
				// (CJsonDeSerializer 내부에서 m_arrayIndexStack을 통해 알아서 다음 원소를 꺼내줍니다)
				Read("", outVec[i]);
			}
		}

		// 4. 배열 노드에서 빠져나옵니다.
		EndArray();
	}

};

NS_END

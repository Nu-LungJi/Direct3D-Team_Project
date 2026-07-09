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
	void Write(const std::string& key, InputIt begin, InputIt end)
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
	void Write(const std::string& key, const T* startPtr, size_t size)
	{
		Write(key, startPtr, startPtr + size);
	}

	template<typename Container>
	auto Write(const std::string& key, const Container& container)
		-> decltype(std::begin(container), std::end(container), void()) // 컨테이너인지 SFINAE 검사
	{
		Write(key, std::begin(container), std::end(container));
	}

	template<typename K, typename V>
	void Write(const std::string& key, const std::map<K, V>& mapData)
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
	void Write(const std::string& key, const std::unordered_map<K, V>& mapData)
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

//protected:
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


	template<typename K, typename V>
	void Read(const std::string& key, std::map<K, V>& outMap)
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
	void Read(const std::string& key, std::unordered_map<K, V>& outMap)
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


	// =========================================================
		// 1. 동적 컨테이너 범용 템플릿 (std::vector, std::list, std::deque 등)
		// (resize() 함수와 반복자를 지원하는 모든 컨테이너 처리)
		// =========================================================
	template<typename Container>
	auto Read(const std::string& key, Container& outContainer)
		-> decltype(outContainer.resize(1), std::begin(outContainer), void()) // SFINAE: resize가 가능한 컨테이너만 매칭
	{
		size_t count = StartArray(key);

		if (count > 0)
		{
			outContainer.resize(count); // 컨테이너 크기를 JSON 배열 크기에 맞춤

			// std::list 등은 operator[]가 없으므로 Range-based for를 사용
			for (auto& item : outContainer)
			{
				Read("", item); // 각 원소 읽기
			}
		}

		EndArray();
	}

	// =========================================================
	// 2. Raw Pointer + Max Size 기반 배열 읽기
	// (동적 할당된 배열이나 버퍼 포인터를 넘길 때 사용, 오버플로우 방지)
	// =========================================================
	template<typename T>
	void Read(const std::string& key, T* outArray, size_t maxElements)
	{
		size_t count = StartArray(key);

		// JSON의 배열 크기가 버퍼보다 클 경우를 대비해 안전하게 작은 값을 선택
		size_t readCount = std::min(count, maxElements);

		for (size_t i = 0; i < readCount; ++i)
		{
			Read("", outArray[i]);
		}

		// 만약 JSON 데이터가 더 많더라도 스택 밸런스를 위해 남은 것은 무시하고 루프 종료
		EndArray();
	}

	// =========================================================
	// 3. 고정 크기 C-스타일 생배열 (T arr[N]) 편의성 래퍼
	// (예: int myArr[10]; -> ReadArray("Key", myArr); 한 줄로 처리)
	// =========================================================
	template<typename T, size_t N>
	void Read(const std::string& key, T(&outArray)[N])
	{
		// 배열의 크기 N을 컴파일러가 자동으로 추론하여 2번 함수로 토스합니다.
		Read(key, outArray, N);
	}
//protected:

	// 2. 컨테이너 노드 제어 (public으로 열어두어야 외부에서 접근 가능)
	// 배열의 크기를 알아야 for문을 돌 수 있으므로 size_t 반환!
	virtual size_t StartArray(const std::string& key) = 0;
	virtual void EndArray() = 0;

	// 1. 맵도 배열처럼 개수(size_t)를 반환하도록 변경합니다.
	virtual size_t StartMap(const std::string& key) = 0;
	virtual void EndMap() = 0;

	// 2. 전체 키를 가져오는 GetMapKeys() 대신, 현재 위치의 단일 Key를 읽는 함수로 변경
	virtual std::string ReadMapKey() = 0;
};

NS_END

#pragma once
#include "Engine_Defines.h"
#include <string>
#include <array>
#include <map>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include <type_traits>
#include "ISerializable.h"

#include <sstream>
NS_BEGIN(Engine)

class ENGINE_DLL ISerializer
{
public:
	DECLARE_RUNTIME_TYPE(ISerializer)

	virtual ~ISerializer() = default;

#pragma region PRIMITIVE
public:
	virtual void Write(const std::string& key, bool value) = 0;
	virtual void Write(const std::string& key, int8_t value) = 0;
	virtual void Write(const std::string& key, uint8_t value) = 0;
	virtual void Write(const std::string& key, int16_t value) = 0;
	virtual void Write(const std::string& key, uint16_t value) = 0;
	virtual void Write(const std::string& key, uint32_t value) = 0;
	virtual void Write(const std::string& key, uint64_t value) = 0;
	virtual void Write(const std::string& key, int64_t value) = 0;
	virtual void Write(const std::string& key, int value) = 0;
	virtual void Write(const std::string& key, float value) = 0;
	virtual void Write(const std::string& key, double value) = 0;
	virtual void Write(const std::string& key, const std::string& value) = 0;
	virtual void Write(const std::string& key, const _float2& value) = 0;
	virtual void Write(const std::string& key, const _float3& value) = 0;
	virtual void Write(const std::string& key, const _float4& value) = 0;
	virtual void Write(const std::string& key, const _float4x4& value) = 0;
	virtual void Write(const std::string& key, const StringID& value) = 0;
	virtual void Write(const std::string& key, const ISerializable& value) = 0;

	template<typename T>
	auto Write(const std::string& key, T value)
		-> std::enable_if_t<std::is_enum_v<T>>
	{
		using Underlying = std::underlying_type_t<T>;
		Write(key, static_cast<Underlying>(value));
	}

	template<typename T1, typename T2>
	void Write(const std::string& key, const std::pair<T1, T2>& pairData)
	{
		StartMap(key); 
		Write("First", pairData.first);
		Write("Second", pairData.second);
		EndMap();
	}

	template<typename T>
	void Write(const std::string& key, const std::optional<T>& optionalData)
	{
		StartMap(key);
		Write("HasValue", optionalData.has_value());
		if (optionalData) Write("Value", *optionalData);
		EndMap();
	}
#pragma endregion

#pragma region ARRAY
public:
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
protected:
	virtual void StartArray(const std::string& key) = 0;
	virtual void EndArray() = 0;
#pragma endregion

#pragma region MAP
public:
	template<typename K, typename V>
	void Write(const std::string& key, const std::map<K, V>& mapData)
	{
		StartMap(key);
		for (const auto& [itemKey, itemValue] : mapData)
		{
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
	virtual void StartMap(const std::string& key) = 0;
	virtual void EndMap() = 0;
#pragma endregion
};

class ENGINE_DLL IDeserializer
{
public:
	DECLARE_RUNTIME_TYPE(IDeserializer)

	virtual ~IDeserializer() = default;
	virtual bool HasValue(const std::string& key) const = 0;

#pragma region PRIMITIVE
public:
	virtual void Read(const std::string& key, bool& outValue) = 0;
	virtual void Read(const std::string& key, int8_t& outValue) = 0;
	virtual void Read(const std::string& key, uint8_t& outValue) = 0;
	virtual void Read(const std::string& key, int16_t& outValue) = 0;
	virtual void Read(const std::string& key, uint16_t& outValue) = 0;
	virtual void Read(const std::string& key, uint32_t& outValue) = 0;
	virtual void Read(const std::string& key, uint64_t& outValue) = 0;
	virtual void Read(const std::string& key, int64_t& outValue) = 0;
	virtual void Read(const std::string& key, int& outValue) = 0;
	virtual void Read(const std::string& key, float& outValue) = 0;
	virtual void Read(const std::string& key, double& outValue) = 0;
	virtual void Read(const std::string& key, std::string& outValue) = 0;
	virtual void Read(const std::string& key, _float2& outValue) = 0;
	virtual void Read(const std::string& key, _float3& outValue) = 0;
	virtual void Read(const std::string& key, _float4& outValue) = 0;
	virtual void Read(const std::string& key, _float4x4& outValue) = 0;
	virtual void Read(const std::string& key, ISerializable& outValue) = 0;
	virtual void Read(const std::string& key, StringID& outValue) = 0;

	template<typename T>
	auto Read(const std::string& key, T& outValue)
		-> std::enable_if_t<std::is_enum_v<T>>
	{
		if (!HasValue(key)) return;

		using Underlying = std::underlying_type_t<T>;
		Underlying temp = static_cast<Underlying>(outValue);
		Read(key, temp);
		outValue = static_cast<T>(temp);
	}

	template<typename T1, typename T2>
	void Read(const std::string& key, std::pair<T1, T2>& outPair)
	{
		if (!HasValue(key)) return;

		size_t count = StartMap(key);

		for (size_t i = 0; i < count; ++i)
		{
			std::string stringKey = ReadMapKey(); 

			if (stringKey == "First")
			{
				Read(stringKey, outPair.first);
			}
			else if (stringKey == "Second")
			{
				Read(stringKey, outPair.second);
			}
		}

		EndMap();
	}

	template<typename T>
	void Read(const std::string& key, std::optional<T>& outOptional)
	{
		if (!HasValue(key)) return;

		static_assert(std::is_default_constructible_v<T>,
			"Optional deserialization requires a default-constructible value type");
		static_assert(
			std::is_move_assignable_v<std::optional<T>> ||
			std::is_copy_assignable_v<std::optional<T>>,
			"Optional deserialization requires an assignable optional value type");

		const size_t count = StartMap(key);
		bool bHasValue = false;
		bool bReadHasValue = false;
		std::optional<T> loadedValue{};

		try
		{
			for (size_t i = 0; i < count; ++i)
			{
				const std::string field = ReadMapKey();
				if (field == "HasValue")
				{
					Read(field, bHasValue);
					bReadHasValue = true;
				}
				else if (field == "Value")
				{
					loadedValue.emplace();
					Read(field, *loadedValue);
				}
				else
				{
					throw std::runtime_error("Optional data contains an unknown field: " + field);
				}
			}
		}
		catch (...)
		{
			EndMap();
			throw;
		}

		EndMap();
		if (!bReadHasValue || bHasValue != loadedValue.has_value())
			throw std::runtime_error("Optional data has an inconsistent value state: " + key);

		if constexpr (std::is_move_assignable_v<std::optional<T>>)
			outOptional = std::move(loadedValue);
		else
			outOptional = loadedValue;
	}

#pragma endregion

#pragma region MAP
public:
	template<typename K, typename V>
	void Read(const std::string& key, std::map<K, V>& outMap)
	{
		if (!HasValue(key)) return;

		size_t count = StartMap(key);
		outMap.clear();
		for (size_t i = 0; i < count; ++i)
		{
			std::string stringKey = ReadMapKey(); // 항상 문자열 키를 읽음

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
		if (!HasValue(key)) return;

		size_t count = StartMap(key);
		outMap.clear();
		for (size_t i = 0; i < count; ++i)
		{
			std::string stringKey = ReadMapKey(); // 항상 문자열 키를 읽음

			K k;
			if constexpr (std::is_same_v<K, std::string>) k = stringKey;
			else if constexpr (std::is_same_v<K, int>) k = std::stoi(stringKey);
			else k = K(stringKey.c_str()); // StringID(const char*) 생성자 활용

			Read(stringKey, outMap[k]); // Read 내부에서 키를 무시하고 밸류만 읽도록 처리
		}
		EndMap();
	}

protected:
	// 맵도 배열처럼 개수를 반환
	virtual size_t StartMap(const std::string& key) = 0;
	virtual void EndMap() = 0;

	// 현재 위치의 단일 Key를 읽는 함수
	virtual std::string ReadMapKey() = 0;
#pragma endregion


#pragma region ARRAY
public:
	template<typename Container>
	auto Read(const std::string& key, Container& outContainer)
		-> decltype(outContainer.resize(1), std::begin(outContainer), void()) 
	{
		if (!HasValue(key)) return;

		size_t count = StartArray(key);
		outContainer.resize(count);

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

	// Set대응 Set은 insert
	template<typename Container>
	auto Read(const std::string& key, Container& outContainer)
		-> decltype(outContainer.insert(*std::begin(outContainer)), void())
	{
		if (!HasValue(key)) return;

		size_t count = StartArray(key);

		outContainer.clear();

		for (size_t i = 0; i < count; ++i)
		{
			typename Container::value_type item;

			Read("", item);

			outContainer.insert(std::move(item));
		}

		EndArray();
	}

	template<typename T>
	void Read(const std::string& key, T* outArray, size_t maxElements)
	{
		if (!HasValue(key)) return;

		size_t count = StartArray(key);

		// JSON의 배열 크기가 버퍼보다 클 경우를 대비해 안전하게 작은 값을 선택
		size_t readCount = std::min(count, maxElements);

		for (size_t i = 0; i < readCount; ++i)
		{
			Read("", outArray[i]);
		}

		for (size_t i = readCount; i < count; ++i)
		{
			T discard{};
			Read("", discard);
		}

		// 만약 JSON 데이터가 더 많더라도 스택 밸런스를 위해 남은 것은 무시하고 루프 종료
		EndArray();
	}


	template<typename T, size_t N>
	void Read(const std::string& key, T(&outArray)[N])
	{
		Read(key, outArray, N);
	}

	template<typename T, size_t N>
	void Read(const std::string& key, std::array<T, N>& outArray)
	{
		Read(key, outArray.data(), N);
	}

protected:
	// 배열의 크기를 알아야 for문을 돌 수 있으므로 size_t 반환
	virtual size_t StartArray(const std::string& key) = 0;
	virtual void EndArray() = 0;
#pragma endregion
};



#pragma region HELPER_MACRO

template<typename... Args>
void Helper_WriteAll(ISerializer& s, const std::string& names, const Args&... args)
{
	std::stringstream ss(names);
	std::string name;

	// C++17 Fold Expression: 인자로 받은 모든 변수(args...)에 대해 이 람다를 반복 실행
	auto write_single = [&](const auto& arg) {
		std::getline(ss, name, ','); // 쉼표 기준으로 문자열을 자름

		// 앞뒤 공백 제거
		size_t start = name.find_first_not_of(" \t");
		size_t end = name.find_last_not_of(" \t");

		s.Write(name.substr(start, end - start + 1), arg);
		};

	(write_single(args), ...); // 모든 변수를 순회하며 실행!
}

template<typename... Args>
void Helper_ReadAll(IDeserializer& d, const std::string& names, Args&... args)
{
	std::stringstream ss(names);
	std::string name;

	auto read_single = [&](auto& arg) {
		std::getline(ss, name, ',');

		size_t start = name.find_first_not_of(" \t");
		size_t end = name.find_last_not_of(" \t");

		d.Read(name.substr(start, end - start + 1), arg);
		};

	(read_single(args), ...);
}

// 한 줄 매크로
#define WRITE_ALL(serializer, ...)   Helper_WriteAll(serializer, #__VA_ARGS__, __VA_ARGS__)
#define READ_ALL(deserializer, ...)  Helper_ReadAll(deserializer, #__VA_ARGS__, __VA_ARGS__)

#pragma endregion


NS_END

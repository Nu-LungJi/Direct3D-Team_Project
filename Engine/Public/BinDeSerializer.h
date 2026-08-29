#pragma once
#include "Engine_Defines.h"
#include "BinSerializeFormat.h"
#include "SerializerInterface.h"
#include <vector>

NS_BEGIN(Engine)
class ISerializable;

class ENGINE_DLL CBinDeSerializer : public CEngineBase, public IDeserializer
{
public:
	DECLARE_DERIVED_TYPE_WITH_BASES(
		CBinDeSerializer,
		CEngineBase,
		IDeserializer)

	CBinDeSerializer();
	~CBinDeSerializer() override;

public:
	HRESULT LoadFromFile(const std::string& path);
	static UPtr<CBinDeSerializer> Create(const std::string& path);
	bool HasValue(const std::string& key) const override;
	bool IsFullyConsumed() const noexcept;

	void Read(const std::string& key, bool& outValue) override;
	void Read(const std::string& key, int8_t& outValue) override;
	void Read(const std::string& key, uint8_t& outValue) override;
	void Read(const std::string& key, int16_t& outValue) override;
	void Read(const std::string& key, uint16_t& outValue) override;
	void Read(const std::string& key, uint32_t& outValue) override;
	void Read(const std::string& key, uint64_t& outValue) override;
	void Read(const std::string& key, int64_t& outValue) override;
	void Read(const std::string& key, int& outValue) override;
	void Read(const std::string& key, float& outValue) override;
	void Read(const std::string& key, double& outValue) override;
	void Read(const std::string& key, std::string& outValue) override;
	void Read(const std::string& key, _float2& outValue) override;
	void Read(const std::string& key, _float3& outValue) override;
	void Read(const std::string& key, _float4& outValue) override;
	void Read(const std::string& key, _float4x4& outValue) override;
	void Read(const std::string& key, ISerializable& outValue) override;
	void Read(const std::string& key, StringID& outValue) override;

	size_t StartArray(const std::string& key) override;
	void EndArray() override;
	size_t StartMap(const std::string& key) override;
	void EndMap() override;
	std::string ReadMapKey() override;

private:
	std::vector<uint8_t> m_buffer;
	size_t m_readPos = 0; // 현재 읽고 있는 메모리 위치
	size_t RemainingBytes() const noexcept;

	template<typename T>
	void ReadBytes(T& outData);
};

NS_END

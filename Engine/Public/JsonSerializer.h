#pragma once
#include "Engine_Defines.h"
#include "SerializerInterface.h"

NS_BEGIN(Engine)
class ISerializable;
class ENGINE_DLL CJsonSerializer : public CEngineBase, public ISerializer
{
public:
	DECLARE_DERIVED_TYPE_WITH_BASES(
		CJsonSerializer,
		CEngineBase,
		ISerializer)

private:
	CJsonSerializer();
	~CJsonSerializer() override;

public:
	void Write(const std::string& key, bool value) override;
	void Write(const std::string& key, int8_t value) override;
	void Write(const std::string& key, uint8_t value) override;
	void Write(const std::string& key, int16_t value) override;
	void Write(const std::string& key, uint16_t value) override;
	void Write(const std::string& key, uint32_t value) override;
	void Write(const std::string& key, uint64_t value) override;
	void Write(const std::string& key, int64_t value) override;
	void Write(const std::string& key, int value) override;
	void Write(const std::string& key, const ISerializable& value) override;
	void Write(const std::string& key, float value) override;
	void Write(const std::string& key, double value) override;
	void Write(const std::string& key, const _float2& value) override;
	void Write(const std::string& key, const _float3& value) override;
	void Write(const std::string& key, const _float4& value) override;
	void Write(const std::string& key, const _float4x4& value)override;
	void Write(const std::string& key, const std::string& value) override;
	void Write(const std::string& key, const StringID& value) override;
public:
	void StartArray(const std::string& key) override;
	void EndArray() override;
	void StartMap(const std::string& key) override;
	void EndMap() override;

public:
	HRESULT SaveToFile(const std::string& path);

private:
	nlohmann::json m_json{};
	std::vector<nlohmann::json*> m_nodeStack;

private:
	HRESULT Initialize();

public:
	static UPtr<CJsonSerializer> Create();
};

NS_END

#pragma once
#include "Engine_Defines.h"
#include "SerializerInterface.h"

NS_BEGIN(Engine)
class ISerializable;
class CJsonSerializer : public CEngineBase, public ISerializer
{
public:
	DECLARE_DERIVED_TYPE(CJsonSerializer, CEngineBase)

private:
	CJsonSerializer();
	~CJsonSerializer() override;

public:
	void Write(const std::string& key, int value) override;
	void Write(const std::string& key, const ISerializable& value) override;
	void Write(const std::string& key, float value) override;
	void Write(const std::string& key, const _float2& value) override;
	void Write(const std::string& key, const _float3& value) override;
	void Write(const std::string& key, const _float4& value) override;
	void Write(const std::string& key, const _float4x4& value)override;
	void Write(const std::string& key, const std::string& value) override;

public:
	void StartArray(const std::string& key);
	void EndArray();
	void StartMap(const std::string& key);
	void EndMap();

public:
	void SaveToFile(const std::string& path);

private:
	nlohmann::json m_json{};
	std::vector<nlohmann::json*> m_nodeStack;

private:
	HRESULT Initialize();

public:
	static UPtr<CJsonSerializer> Create();
};

NS_END

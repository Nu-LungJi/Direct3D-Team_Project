#pragma once
#include "Engine_Defines.h"
#include "SerializerInterface.h"
#include <vector>

NS_BEGIN(Engine)
class ISerializable;

class ENGINE_DLL CBinSerializer : public CEngineBase, public ISerializer
{
public:
	DECLARE_DERIVED_TYPE(CBinSerializer, CEngineBase)

private:
	CBinSerializer();
	~CBinSerializer() override;

public:
	void Write(const std::string& key, bool value) override;
	void Write(const std::string& key, uint32_t value) override;
	void Write(const std::string& key, uint64_t value) override;
	void Write(const std::string& key, int value) override;
	void Write(const std::string& key, float value) override;
	void Write(const std::string& key, const std::string& value) override;
	void Write(const std::string& key, const _float2& value)  override;
	void Write(const std::string& key, const _float3& value)  override;
	void Write(const std::string& key, const _float4& value) override;
	void Write(const std::string& key, const _float4x4& value)override;
	void Write(const std::string& key, const ISerializable& value) override;
	void Write(const std::string& key, const StringID& value) override;

	void StartArray(const std::string& key) override;
	void EndArray() override;
	void StartMap(const std::string& key) override;
	void EndMap() override;

public:
	HRESULT SaveToFile(const std::string& path);
	static UPtr<CBinSerializer> Create();

private:
	// 바이너리 데이터가 쌓일 메모리 버퍼
	std::vector<uint8_t> m_buffer;

	// 현재 어떤 노드 안에 있는지 추적하기 위한 상태
	enum class ENodeType { Object, Array, Map };
	struct SNodeState {
		ENodeType type;
		size_t sizePos; // 배열/맵의 원소 개수(size)가 기록될 버퍼 인덱스 위치
		size_t count;   // 현재까지 기록된 원소 개수
	};
	std::vector<SNodeState> m_nodeStack;

private:
	// 키 쓰기 및 배열/맵 카운트 증가 처리기
	void PreWrite(const std::string& key);

	// 버퍼에 데이터를 밀어넣는 템플릿 헬퍼
	template<typename T>
	void WriteBytes(const T& data);
};

NS_END

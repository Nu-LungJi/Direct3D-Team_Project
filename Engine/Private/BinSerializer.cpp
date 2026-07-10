#include "BinSerializer.h"
#include "ISerializable.h"
#include <fstream>

NS_USING(Engine)

CBinSerializer::CBinSerializer() {}
CBinSerializer::~CBinSerializer() {}


UPtr<CBinSerializer> CBinSerializer::Create() {
	return ToUPtr(new CBinSerializer{});
}

template<typename T>
void CBinSerializer::WriteBytes(const T& data) {
	const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&data);
	m_buffer.insert(m_buffer.end(), ptr, ptr + sizeof(T));
}

// === 배열/맵 상태 추적기 (핵심 로직) ===
void CBinSerializer::PreWrite(const std::string& key)
{
	if (m_nodeStack.empty()) return;

	SNodeState& state = m_nodeStack.back();
	if (state.type == ENodeType::Map)
	{
		// 맵인 경우에만 Key 문자열을 바이너리에 기록 (일반 변수 Key는 용량 절약을 위해 무시!)
		size_t len = key.size();
		WriteBytes(len);
		if (len > 0) m_buffer.insert(m_buffer.end(), key.begin(), key.end());
		state.count++;
	}
	else if (state.type == ENodeType::Array)
	{
		state.count++;
	}
}

void CBinSerializer::Write(const std::string& key, bool value)
{
	PreWrite(key);
	WriteBytes(value);
}

void CBinSerializer::Write(const std::string& key, uint32_t value)
{
	PreWrite(key);
	WriteBytes(value);
}

void CBinSerializer::Write(const std::string& key, uint64_t value)
{
	PreWrite(key);
	WriteBytes(value);
}

void CBinSerializer::Write(const std::string& key, int value) {
	PreWrite(key);
	WriteBytes(value);
}

void CBinSerializer::Write(const std::string& key, float value) {
	PreWrite(key);
	WriteBytes(value);
}

void CBinSerializer::Write(const std::string& key, const std::string& value) {
	PreWrite(key);
	size_t len = value.size();
	WriteBytes(len);
	if (len > 0) m_buffer.insert(m_buffer.end(), value.begin(), value.end());
}

void CBinSerializer::Write(const std::string& key, const _float2& value)
{
	PreWrite(key);
	WriteBytes(value); // float x, y (8바이트)
}

void CBinSerializer::Write(const std::string& key, const _float3& value)
{
	PreWrite(key);
	WriteBytes(value); // float x, y, z (12바이트)
}

void CBinSerializer::Write(const std::string& key, const _float4& value)
{
	PreWrite(key);
	WriteBytes(value); // float x, y, z, w (16바이트)
}

void CBinSerializer::Write(const std::string& key, const _float4x4& value)
{
	PreWrite(key);
	WriteBytes(value);
}

void CBinSerializer::Write(const std::string& key, const ISerializable& value) {
	PreWrite(key);
	m_nodeStack.push_back({ ENodeType::Object, 0, 0 });
	value.Serialize(*this);
	m_nodeStack.pop_back();
}

// === 노드 제어 (스트림 패칭 기법) ===
void CBinSerializer::StartArray(const std::string& key) {
	PreWrite(key);
	m_nodeStack.push_back({ ENodeType::Array, m_buffer.size(), 0 }); // 현재 위치 기억
	size_t dummySize = 0;
	WriteBytes(dummySize); // 빈 깡통 크기를 먼저 기록해둠
}

void CBinSerializer::EndArray() {
	auto state = m_nodeStack.back();
	m_nodeStack.pop_back();
	// 기억해둔 위치로 돌아가 진짜 개수(count)를 덮어씀
	std::memcpy(m_buffer.data() + state.sizePos, &state.count, sizeof(size_t));
}

void CBinSerializer::StartMap(const std::string& key) {
	PreWrite(key);
	m_nodeStack.push_back({ ENodeType::Map, m_buffer.size(), 0 });
	size_t dummySize = 0;
	WriteBytes(dummySize);
}

void CBinSerializer::EndMap() {
	auto state = m_nodeStack.back();
	m_nodeStack.pop_back();
	std::memcpy(m_buffer.data() + state.sizePos, &state.count, sizeof(size_t));
}

// === 최종 파일 출력 ===
HRESULT CBinSerializer::SaveToFile(const std::string& path) {
	std::ofstream file(path, std::ios::binary);
	if (!file.is_open())
	{
		MSG_BOX("SaveToFile 파일 저장 실패");
		return E_FAIL;
	}
	if (file.is_open() && !m_buffer.empty()) {
		file.write(reinterpret_cast<const char*>(m_buffer.data()), m_buffer.size());
	}

	return S_OK;
}

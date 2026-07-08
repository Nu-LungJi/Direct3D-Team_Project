#include "BinDeSerializer.h"
#include "ISerializable.h"
#include <fstream>

NS_USING(Engine)

CBinDeSerializer::CBinDeSerializer() {}
CBinDeSerializer::~CBinDeSerializer() {}

HRESULT CBinDeSerializer::LoadFromFile(const std::string& path) {
	std::ifstream file(path, std::ios::binary | std::ios::ate); // 파일 끝에서 열어 크기 확인
	if (!file.is_open()) return E_FAIL;

	size_t fileSize = file.tellg();
	file.seekg(0, std::ios::beg); // 다시 처음으로 되돌림

	m_buffer.resize(fileSize);
	file.read(reinterpret_cast<char*>(m_buffer.data()), fileSize); // 한 방에 메모리로 카피!

	m_readPos = 0;
	return S_OK;
}

UPtr<CBinDeSerializer> CBinDeSerializer::Create(const std::string& path) {
	auto pInstance = ToUPtr(new CBinDeSerializer{});
	if (FAILED(pInstance->LoadFromFile(path))) return nullptr;
	return pInstance;
}

template<typename T>
void CBinDeSerializer::ReadBytes(T& outData) {
	std::memcpy(&outData, m_buffer.data() + m_readPos, sizeof(T));
	m_readPos += sizeof(T);
}

// === 초고속 순차 읽기 ===
void CBinDeSerializer::Read(const std::string& key, int& outValue) {
	ReadBytes(outValue);
}

void CBinDeSerializer::Read(const std::string& key, float& outValue) {
	ReadBytes(outValue);
}

void CBinDeSerializer::Read(const std::string& key, std::string& outValue) {
	size_t len = 0;
	ReadBytes(len);
	if (len > 0) {
		outValue.assign(reinterpret_cast<char*>(m_buffer.data() + m_readPos), len);
		m_readPos += len;
	}
}

void CBinDeSerializer::Read(const std::string& key, _float2& outValue)
{
	// _float2는 float 2개(8바이트)가 연속되어 있으므로 한 번에 읽기
	ReadBytes(outValue);
}

void CBinDeSerializer::Read(const std::string& key, _float3& outValue)
{
	// _float3는 float 3개(12바이트)가 연속되어 있으므로 한 번에 읽기
	ReadBytes(outValue);
}

void CBinDeSerializer::Read(const std::string& key, _float4& outValue)
{
	// _float4는 float 4개(16바이트)가 연속되어 있으므로 한 번에 읽기
	ReadBytes(outValue);
}

void CBinDeSerializer::Read(const std::string& key, _float4x4& outValue)
{
	ReadBytes(outValue); // 64바이트 그대로 읽기
}

void CBinDeSerializer::Read(const std::string& key, ISerializable& outValue) {
	outValue.Deserialize(*this); // 하위 객체는 알아서 순서대로 자기 것을 읽음
}

// === 배열 및 맵 제어 ===
size_t CBinDeSerializer::StartArray(const std::string& key) {
	size_t count = 0;
	ReadBytes(count);
	return count;
}
void CBinDeSerializer::EndArray() {} // 할 일 없음

size_t CBinDeSerializer::StartMap(const std::string& key) {
	size_t count = 0;
	ReadBytes(count);
	return count;
}
void CBinDeSerializer::EndMap() {} // 할 일 없음

std::string CBinDeSerializer::ReadMapKey() {
	std::string key;
	Read("", key); // 위에서 만든 string Read 함수 재활용
	return key;
}

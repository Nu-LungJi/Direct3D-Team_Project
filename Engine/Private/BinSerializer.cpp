#include "BinSerializer.h"
#include "ISerializable.h"

#include <cstring>
#include <fstream>
#include <stdexcept>
#include <type_traits>

NS_USING(Engine)

CBinSerializer::CBinSerializer() {}
CBinSerializer::~CBinSerializer() {}

UPtr<CBinSerializer> CBinSerializer::Create()
{
	return ToUPtr(new CBinSerializer{});
}

void CBinSerializer::WriteRawBytes(const void* pData, size_t iSize)
{
	if (iSize == 0) return;
	if (!pData) throw std::invalid_argument("Binary source buffer is null");

	const uint64_t iCurrentSize = static_cast<uint64_t>(m_buffer.size());
	const uint64_t iAppendSize = static_cast<uint64_t>(iSize);
	if (iCurrentSize > BinSerializeFormat::MAX_PAYLOAD_BYTES ||
		iAppendSize > BinSerializeFormat::MAX_PAYLOAD_BYTES - iCurrentSize)
	{
		throw std::length_error("Binary payload exceeds the configured size limit");
	}

	const auto* pBytes = static_cast<const uint8_t*>(pData);
	m_buffer.insert(m_buffer.end(), pBytes, pBytes + iSize);
}

template<typename T>
void CBinSerializer::WriteBytes(const T& data)
{
	static_assert(std::is_trivially_copyable_v<T>);
	WriteRawBytes(&data, sizeof(T));
}

void CBinSerializer::PreWrite(const std::string& key)
{
	if (m_nodeStack.empty()) return;

	SNodeState& state = m_nodeStack.back();
	if (state.type != ENodeType::Map && state.type != ENodeType::Array) return;

	if (state.count >= BinSerializeFormat::MAX_CONTAINER_ELEMENTS)
		throw std::length_error("Binary container exceeds the configured element limit");

	if (state.type == ENodeType::Map)
	{
		if (key.size() > BinSerializeFormat::MAX_STRING_BYTES)
			throw std::length_error("Binary map key exceeds the configured string limit");

		const uint64_t iLength = static_cast<uint64_t>(key.size());
		WriteBytes(iLength);
		WriteRawBytes(key.data(), key.size());
	}

	++state.count;
}

void CBinSerializer::Write(const std::string& key, bool value)
{
	PreWrite(key);
	const uint8_t iValue = value ? 1u : 0u;
	WriteBytes(iValue);
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

void CBinSerializer::Write(const std::string& key, int value)
{
	PreWrite(key);
	WriteBytes(value);
}

void CBinSerializer::Write(const std::string& key, float value)
{
	PreWrite(key);
	WriteBytes(value);
}

void CBinSerializer::Write(const std::string& key, const std::string& value)
{
	PreWrite(key);
	if (value.size() > BinSerializeFormat::MAX_STRING_BYTES)
		throw std::length_error("Binary string exceeds the configured size limit");

	const uint64_t iLength = static_cast<uint64_t>(value.size());
	WriteBytes(iLength);
	WriteRawBytes(value.data(), value.size());
}

void CBinSerializer::Write(const std::string& key, const StringID& value)
{
	Write(key, std::string(value.GetDbgStr()));
}

void CBinSerializer::Write(const std::string& key, const _float2& value)
{
	PreWrite(key);
	WriteBytes(value);
}

void CBinSerializer::Write(const std::string& key, const _float3& value)
{
	PreWrite(key);
	WriteBytes(value);
}

void CBinSerializer::Write(const std::string& key, const _float4& value)
{
	PreWrite(key);
	WriteBytes(value);
}

void CBinSerializer::Write(const std::string& key, const _float4x4& value)
{
	PreWrite(key);
	WriteBytes(value);
}

void CBinSerializer::Write(const std::string& key, const ISerializable& value)
{
	PreWrite(key);
	m_nodeStack.push_back({ ENodeType::Object, 0, 0 });
	try
	{
		value.Serialize(*this);
	}
	catch (...)
	{
		m_nodeStack.pop_back();
		throw;
	}
	m_nodeStack.pop_back();
}

void CBinSerializer::StartArray(const std::string& key)
{
	PreWrite(key);
	m_nodeStack.push_back({ ENodeType::Array, m_buffer.size(), 0 });
	WriteBytes(uint64_t{});
}

void CBinSerializer::EndArray()
{
	EndContainer(ENodeType::Array);
}

void CBinSerializer::StartMap(const std::string& key)
{
	PreWrite(key);
	m_nodeStack.push_back({ ENodeType::Map, m_buffer.size(), 0 });
	WriteBytes(uint64_t{});
}

void CBinSerializer::EndMap()
{
	EndContainer(ENodeType::Map);
}

void CBinSerializer::EndContainer(ENodeType eExpectedType)
{
	if (m_nodeStack.empty() || m_nodeStack.back().type != eExpectedType)
		throw std::logic_error("Binary serializer container stack mismatch");

	const SNodeState state = m_nodeStack.back();
	if (state.count > BinSerializeFormat::MAX_CONTAINER_ELEMENTS)
		throw std::length_error("Binary container exceeds the configured element limit");
	if (state.sizePos > m_buffer.size() || sizeof(uint64_t) > m_buffer.size() - state.sizePos)
		throw std::logic_error("Binary serializer container size position is invalid");

	const uint64_t iCount = static_cast<uint64_t>(state.count);
	std::memcpy(m_buffer.data() + state.sizePos, &iCount, sizeof(iCount));
	m_nodeStack.pop_back();
}

HRESULT CBinSerializer::SaveToFile(const std::string& path)
{
	if (!m_nodeStack.empty()) return E_FAIL;
	if (m_buffer.size() > BinSerializeFormat::MAX_PAYLOAD_BYTES) return E_FAIL;

	std::ofstream file(path, std::ios::binary | std::ios::trunc);
	if (!file.is_open()) return E_FAIL;

	BinSerializeFormat::HEADER header{};
	header.iPayloadSize = static_cast<uint64_t>(m_buffer.size());

	file.write(reinterpret_cast<const char*>(&header), sizeof(header));
	if (!m_buffer.empty())
	{
		file.write(
			reinterpret_cast<const char*>(m_buffer.data()),
			static_cast<std::streamsize>(m_buffer.size()));
	}
	file.flush();

	return file.good() ? S_OK : E_FAIL;
}

#include "BinDeSerializer.h"
#include "ISerializable.h"

#include <cstring>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <type_traits>

NS_USING(Engine)

CBinDeSerializer::CBinDeSerializer() {}
CBinDeSerializer::~CBinDeSerializer() {}

HRESULT CBinDeSerializer::LoadFromFile(const std::string& path)
{
	std::ifstream file(path, std::ios::binary | std::ios::ate);
	if (!file.is_open()) return E_FAIL;

	const std::streampos endPosition = file.tellg();
	if (endPosition < std::streampos{}) return E_FAIL;

	const uint64_t iFileSize = static_cast<uint64_t>(endPosition);
	const uint64_t iHeaderSize = sizeof(BinSerializeFormat::HEADER);
	if (iFileSize < iHeaderSize ||
		iFileSize - iHeaderSize > BinSerializeFormat::MAX_PAYLOAD_BYTES)
	{
		return E_FAIL;
	}

	file.seekg(0, std::ios::beg);
	if (!file.good()) return E_FAIL;

	BinSerializeFormat::HEADER header{};
	file.read(reinterpret_cast<char*>(&header), sizeof(header));
	if (!file.good()) return E_FAIL;

	if (header.iMagic != BinSerializeFormat::MAGIC ||
		header.iVersion != BinSerializeFormat::VERSION ||
		header.iFlags != BinSerializeFormat::FLAGS ||
		header.iPayloadSize != iFileSize - iHeaderSize ||
		header.iPayloadSize > BinSerializeFormat::MAX_PAYLOAD_BYTES ||
		header.iPayloadSize > static_cast<uint64_t>(std::numeric_limits<size_t>::max()))
	{
		return E_FAIL;
	}

	m_buffer.resize(static_cast<size_t>(header.iPayloadSize));
	if (!m_buffer.empty())
	{
		file.read(
			reinterpret_cast<char*>(m_buffer.data()),
			static_cast<std::streamsize>(m_buffer.size()));
		if (!file.good()) return E_FAIL;
	}

	m_readPos = 0;
	return S_OK;
}

UPtr<CBinDeSerializer> CBinDeSerializer::Create(const std::string& path)
{
	auto pInstance = ToUPtr(new CBinDeSerializer{});
	if (FAILED(pInstance->LoadFromFile(path))) return nullptr;
	return pInstance;
}

bool CBinDeSerializer::HasValue(const std::string& key) const
{
	return RemainingBytes() > 0;
}

bool CBinDeSerializer::IsFullyConsumed() const noexcept
{
	return m_readPos == m_buffer.size();
}

size_t CBinDeSerializer::RemainingBytes() const noexcept
{
	return m_readPos <= m_buffer.size() ? m_buffer.size() - m_readPos : 0;
}

template<typename T>
void CBinDeSerializer::ReadBytes(T& outData)
{
	static_assert(std::is_trivially_copyable_v<T>);
	if (m_readPos > m_buffer.size() || sizeof(T) > m_buffer.size() - m_readPos)
		throw std::out_of_range("Binary payload ended before the requested value");

	std::memcpy(&outData, m_buffer.data() + m_readPos, sizeof(T));
	m_readPos += sizeof(T);
}

void CBinDeSerializer::Read(const std::string& key, bool& outValue)
{
	uint8_t iValue{};
	ReadBytes(iValue);
	if (iValue > 1u) throw std::runtime_error("Binary bool value is invalid");
	outValue = iValue != 0;
}

void CBinDeSerializer::Read(const std::string& key, uint32_t& outValue)
{
	ReadBytes(outValue);
}

void CBinDeSerializer::Read(const std::string& key, uint64_t& outValue)
{
	ReadBytes(outValue);
}

void CBinDeSerializer::Read(const std::string& key, int& outValue)
{
	ReadBytes(outValue);
}

void CBinDeSerializer::Read(const std::string& key, float& outValue)
{
	ReadBytes(outValue);
}

void CBinDeSerializer::Read(const std::string& key, std::string& outValue)
{
	uint64_t iLength{};
	ReadBytes(iLength);
	if (iLength > BinSerializeFormat::MAX_STRING_BYTES ||
		iLength > static_cast<uint64_t>(RemainingBytes()) ||
		iLength > static_cast<uint64_t>(outValue.max_size()))
	{
		throw std::length_error("Binary string length is invalid");
	}

	const size_t iSize = static_cast<size_t>(iLength);
	if (iSize == 0)
	{
		outValue.clear();
		return;
	}

	outValue.assign(
		reinterpret_cast<const char*>(m_buffer.data() + m_readPos),
		iSize);
	m_readPos += iSize;
}

void CBinDeSerializer::Read(const std::string& key, _float2& outValue)
{
	ReadBytes(outValue);
}

void CBinDeSerializer::Read(const std::string& key, _float3& outValue)
{
	ReadBytes(outValue);
}

void CBinDeSerializer::Read(const std::string& key, _float4& outValue)
{
	ReadBytes(outValue);
}

void CBinDeSerializer::Read(const std::string& key, _float4x4& outValue)
{
	ReadBytes(outValue);
}

void CBinDeSerializer::Read(const std::string& key, ISerializable& outValue)
{
	outValue.Deserialize(*this);
}

void CBinDeSerializer::Read(const std::string& key, StringID& outValue)
{
	if (!HasValue(key)) return;

	std::string tempStr;
	Read(key, tempStr);
	outValue = StringID(tempStr);
}

size_t CBinDeSerializer::StartArray(const std::string& key)
{
	uint64_t iCount{};
	ReadBytes(iCount);
	if (iCount > BinSerializeFormat::MAX_CONTAINER_ELEMENTS ||
		iCount > static_cast<uint64_t>(std::numeric_limits<size_t>::max()))
	{
		throw std::length_error("Binary array element count is invalid");
	}

	return static_cast<size_t>(iCount);
}

void CBinDeSerializer::EndArray() {}

size_t CBinDeSerializer::StartMap(const std::string& key)
{
	uint64_t iCount{};
	ReadBytes(iCount);
	if (iCount > BinSerializeFormat::MAX_CONTAINER_ELEMENTS ||
		iCount > static_cast<uint64_t>(std::numeric_limits<size_t>::max()))
	{
		throw std::length_error("Binary map element count is invalid");
	}

	return static_cast<size_t>(iCount);
}

void CBinDeSerializer::EndMap() {}

std::string CBinDeSerializer::ReadMapKey()
{
	std::string key;
	Read("", key);
	return key;
}

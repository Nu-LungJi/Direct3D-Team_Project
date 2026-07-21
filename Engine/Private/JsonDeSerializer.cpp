#include "pch.h"
#include "JsonDeSerializer.h"
#include "ISerializable.h"
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <type_traits>

NS_USING(Engine)

namespace
{
	const nlohmann::json* FindValue(
		nlohmann::json& node,
		std::vector<size_t>& arrayIndexStack,
		const std::string& key)
	{
		if (node.is_array() && !arrayIndexStack.empty())
		{
			size_t& index = arrayIndexStack.back();
			if (index >= node.size()) return nullptr;
			const nlohmann::json* pValue = &node[index++];
			return pValue->is_null() ? nullptr : pValue;
		}

		if (!node.is_object() || !node.contains(key) || node[key].is_null())
			return nullptr;

		return &node[key];
	}

	template<typename T>
	void ReadIntegerValue(
		nlohmann::json& node,
		std::vector<size_t>& arrayIndexStack,
		const std::string& key,
		T& outValue)
	{
		static_assert(std::is_integral_v<T> && !std::is_same_v<T, bool>);

		const nlohmann::json* pValue = FindValue(node, arrayIndexStack, key);
		if (!pValue) return;
		if (!pValue->is_number_integer() && !pValue->is_number_unsigned())
			throw std::runtime_error("JSON integer field has an invalid type: " + key);

		if constexpr (std::is_signed_v<T>)
		{
			if (pValue->is_number_unsigned())
			{
				const uint64_t value = pValue->get<uint64_t>();
				if (value > static_cast<uint64_t>(std::numeric_limits<T>::max()))
					throw std::out_of_range("JSON signed integer field is out of range: " + key);
				outValue = static_cast<T>(value);
			}
			else
			{
				const int64_t value = pValue->get<int64_t>();
				if (value < static_cast<int64_t>(std::numeric_limits<T>::min()) ||
					value > static_cast<int64_t>(std::numeric_limits<T>::max()))
				{
					throw std::out_of_range("JSON signed integer field is out of range: " + key);
				}
				outValue = static_cast<T>(value);
			}
		}
		else
		{
			uint64_t value{};
			if (pValue->is_number_unsigned())
			{
				value = pValue->get<uint64_t>();
			}
			else
			{
				const int64_t signedValue = pValue->get<int64_t>();
				if (signedValue < 0)
					throw std::out_of_range("JSON unsigned integer field is negative: " + key);
				value = static_cast<uint64_t>(signedValue);
			}

			if (value > static_cast<uint64_t>(std::numeric_limits<T>::max()))
				throw std::out_of_range("JSON unsigned integer field is out of range: " + key);
			outValue = static_cast<T>(value);
		}
	}
}

CJsonDeSerializer::CJsonDeSerializer() {}
CJsonDeSerializer::~CJsonDeSerializer() {}

UPtr<CJsonDeSerializer> CJsonDeSerializer::Create(const std::string& path)
{
	auto pInstance = ToUPtr(new CJsonDeSerializer{});

	if (FAILED(pInstance->LoadFromFile(path))) return nullptr;
	if (FAILED(pInstance->Initialize())) return nullptr;

	return pInstance;
}

HRESULT CJsonDeSerializer::LoadFromFile(const std::string& path)
{
	std::ifstream file(path);
	if (!file.is_open()) return E_FAIL;

	try { file >> m_json; }
	catch (const nlohmann::json::parse_error&) { return E_FAIL; }

	return S_OK;
}

bool CJsonDeSerializer::HasValue(const std::string& key) const
{
	if (m_nodeStack.empty()) return false;

	const nlohmann::json& node = *m_nodeStack.back();
	if (node.is_array() && !m_arrayIndexStack.empty())
	{
		const size_t idx = m_arrayIndexStack.back();
		return idx < node.size() && !node[idx].is_null();
	}

	return node.is_object() && node.contains(key) && !node[key].is_null();
}

void CJsonDeSerializer::Read(const std::string& key, bool& outValue)
{
	nlohmann::json& node = *m_nodeStack.back();

	if (node.is_array() && !m_arrayIndexStack.empty())
	{
		size_t& idx = m_arrayIndexStack.back();
		if (idx < node.size())
		{
			if (!node[idx].is_null()) outValue = node[idx].get<bool>();
			idx++;
		}
	}
	else if (node.is_object() && node.contains(key) && !node[key].is_null())
	{
		outValue = node[key].get<bool>();
	}
}

void CJsonDeSerializer::Read(const std::string& key, int8_t& outValue)
{
	ReadIntegerValue(*m_nodeStack.back(), m_arrayIndexStack, key, outValue);
}

void CJsonDeSerializer::Read(const std::string& key, uint8_t& outValue)
{
	ReadIntegerValue(*m_nodeStack.back(), m_arrayIndexStack, key, outValue);
}

void CJsonDeSerializer::Read(const std::string& key, int16_t& outValue)
{
	ReadIntegerValue(*m_nodeStack.back(), m_arrayIndexStack, key, outValue);
}

void CJsonDeSerializer::Read(const std::string& key, uint16_t& outValue)
{
	ReadIntegerValue(*m_nodeStack.back(), m_arrayIndexStack, key, outValue);
}

void CJsonDeSerializer::Read(const std::string& key, uint32_t& outValue)
{
	ReadIntegerValue(*m_nodeStack.back(), m_arrayIndexStack, key, outValue);
}

void CJsonDeSerializer::Read(const std::string& key, uint64_t& outValue)
{
	ReadIntegerValue(*m_nodeStack.back(), m_arrayIndexStack, key, outValue);
}

void CJsonDeSerializer::Read(const std::string& key, int64_t& outValue)
{
	ReadIntegerValue(*m_nodeStack.back(), m_arrayIndexStack, key, outValue);
}

HRESULT CJsonDeSerializer::Initialize()
{
	m_nodeStack.push_back(&m_json);
	return S_OK;
}

void CJsonDeSerializer::Read(const std::string& key, int& outValue)
{
	ReadIntegerValue(*m_nodeStack.back(), m_arrayIndexStack, key, outValue);
}

void CJsonDeSerializer::Read(const std::string& key, float& outValue)
{
	nlohmann::json& node = *m_nodeStack.back();

	if (node.is_array() && !m_arrayIndexStack.empty())
	{
		size_t& idx = m_arrayIndexStack.back();
		if (idx < node.size())
		{
			if (!node[idx].is_null()) outValue = node[idx].get<float>();
			idx++;
		}
	}
	else if (node.is_object() && node.contains(key) && !node[key].is_null())
	{
		outValue = node[key].get<float>();
	}
}

void CJsonDeSerializer::Read(const std::string& key, double& outValue)
{
	nlohmann::json& node = *m_nodeStack.back();
	const nlohmann::json* pValue = FindValue(node, m_arrayIndexStack, key);
	if (!pValue) return;
	if (!pValue->is_number())
		throw std::runtime_error("JSON floating-point field has an invalid type: " + key);
	outValue = pValue->get<double>();
}

void CJsonDeSerializer::Read(const std::string& key, std::string& outValue)
{
	nlohmann::json& node = *m_nodeStack.back();

	if (node.is_array() && !m_arrayIndexStack.empty())
	{
		size_t& idx = m_arrayIndexStack.back();
		if (idx < node.size())
		{
			if (!node[idx].is_null()) outValue = node[idx].get<std::string>();
			idx++;
		}
	}
	else if (node.is_object() && node.contains(key) && !node[key].is_null())
	{
		outValue = node[key].get<std::string>();
	}
}

void CJsonDeSerializer::Read(const std::string& key, _float2& outValue)
{
	nlohmann::json& node = *m_nodeStack.back();
	std::vector<float> vec;

	if (node.is_array() && !m_arrayIndexStack.empty()) {
		size_t& idx = m_arrayIndexStack.back();
		if (idx < node.size()) { vec = node[idx++].get<std::vector<float>>(); }
	}
	else if (node.is_object() && node.contains(key)) {
		vec = node[key].get<std::vector<float>>();
	}

	if (vec.size() >= 2) { outValue.x = vec[0]; outValue.y = vec[1]; }
}

void CJsonDeSerializer::Read(const std::string& key, _float3& outValue)
{
	nlohmann::json& node = *m_nodeStack.back();
	std::vector<float> vec;

	if (node.is_array() && !m_arrayIndexStack.empty()) {
		size_t& idx = m_arrayIndexStack.back();
		if (idx < node.size()) { vec = node[idx++].get<std::vector<float>>(); }
	}
	else if (node.is_object() && node.contains(key)) {
		vec = node[key].get<std::vector<float>>();
	}

	if (vec.size() >= 3) { outValue.x = vec[0]; outValue.y = vec[1]; outValue.z = vec[2]; }
}

void CJsonDeSerializer::Read(const std::string& key, _float4& outValue)
{
	nlohmann::json& node = *m_nodeStack.back();
	std::vector<float> vec;

	if (node.is_array() && !m_arrayIndexStack.empty()) {
		size_t& idx = m_arrayIndexStack.back();
		if (idx < node.size()) { vec = node[idx++].get<std::vector<float>>(); }
	}
	else if (node.is_object() && node.contains(key)) {
		vec = node[key].get<std::vector<float>>();
	}

	if (vec.size() >= 4) { outValue.x = vec[0]; outValue.y = vec[1]; outValue.z = vec[2];  outValue.w = vec[3]; }
}

void CJsonDeSerializer::Read(const std::string& key, _float4x4& outValue)
{
	nlohmann::json& node = *m_nodeStack.back();
	nlohmann::json matrix;

	if (node.is_array() && !m_arrayIndexStack.empty()) {
		size_t& idx = m_arrayIndexStack.back();
		if (idx < node.size()) matrix = node[idx++];
	}
	else if (node.is_object() && node.contains(key)) {
		matrix = node[key];
	}

	// matrix가 2차원 배열(4x4) 형태를 갖추고 있는지 확인
	if (matrix.is_array() && matrix.size() == 4)
	{
		float* f = reinterpret_cast<float*>(&outValue);
		for (int i = 0; i < 4; ++i)
		{
			// 각 행(Row) 역시 배열이고 크기가 4인지 확인
			if (matrix[i].is_array() && matrix[i].size() == 4)
			{
				for (int j = 0; j < 4; ++j)
				{
					f[i * 4 + j] = matrix[i][j].get<float>();
				}
			}
		}
	}
}

void CJsonDeSerializer::Read(const std::string& key, ISerializable& outValue)
{
	nlohmann::json& node = *m_nodeStack.back();

	if (node.is_array() && !m_arrayIndexStack.empty())
	{
		size_t currentIdx = m_arrayIndexStack.back();

		if (currentIdx < node.size() && node[currentIdx].is_object())
		{
			m_nodeStack.push_back(&node[currentIdx]);
			outValue.Deserialize(*this);
			m_nodeStack.pop_back();
		}

		m_arrayIndexStack.back()++;
	}
	else if (node.is_object() && node.contains(key) && node[key].is_object())
	{
		m_nodeStack.push_back(&node[key]);
		outValue.Deserialize(*this);
		m_nodeStack.pop_back();
	}
}

void CJsonDeSerializer::Read(const std::string& key, StringID& outValue)
{
	if (!HasValue(key)) return;

	std::string tempStr;

	Read(key, tempStr);

	outValue = StringID(tempStr);
}

size_t CJsonDeSerializer::StartArray(const std::string& key)
{
	nlohmann::json& node = *m_nodeStack.back();
	bool bSuccess = false;

	if (node.is_array() && !m_arrayIndexStack.empty())
	{
		size_t& idx = m_arrayIndexStack.back();
		if (idx < node.size() && node[idx].is_array())
		{
			m_nodeStack.push_back(&node[idx]);
			++idx;
			bSuccess = true;
		}
	}
	else if (node.is_object() && node.contains(key) && node[key].is_array())
	{
		m_nodeStack.push_back(&node[key]);
		bSuccess = true;
	}

	// 실패하더라도 EndArray()의 pop_back()과 짝을 맞추기 위해 자기 자신을 더미로 푸시
	if (!bSuccess)
		throw std::runtime_error("JSON array field is missing or has an invalid type: " + key);

	m_arrayIndexStack.push_back(0);
	return bSuccess ? (*m_nodeStack.back()).size() : 0;
}

void CJsonDeSerializer::EndArray()
{
	if (m_nodeStack.size() > 1) m_nodeStack.pop_back();
	if (!m_arrayIndexStack.empty()) m_arrayIndexStack.pop_back();
}

size_t CJsonDeSerializer::StartMap(const std::string& key)
{
	nlohmann::json& node = *m_nodeStack.back();
	bool bSuccess = false;

	if (node.is_array() && !m_arrayIndexStack.empty())
	{
		size_t& idx = m_arrayIndexStack.back();
		if (idx < node.size() && node[idx].is_object())
		{
			m_nodeStack.push_back(&node[idx]);
			++idx;
			bSuccess = true;
		}
	}
	else if (node.is_object() && node.contains(key) && node[key].is_object())
	{
		m_nodeStack.push_back(&node[key]);
		bSuccess = true;
	}
	else
	{
		throw std::runtime_error("JSON map field is missing or has an invalid type: " + key);
	}

	if (!bSuccess)
		throw std::runtime_error("JSON map field is missing or has an invalid type: " + key);

	m_mapKeysStack.push_back(std::vector<std::string>());
	m_mapKeyIndexStack.push_back(0);

	// 진입에 성공했을 때만 현재 맵의 키들을 캐싱
	if (bSuccess)
	{
		for (auto& [k, v] : (*m_nodeStack.back()).items()) {
			m_mapKeysStack.back().push_back(k);
		}
	}

	return m_mapKeysStack.back().size();
}

std::string CJsonDeSerializer::ReadMapKey()
{
	// 스택이 비어있는 예외 상황 방어
	if (m_mapKeyIndexStack.empty() || m_mapKeysStack.empty()) return "";

	size_t& idx = m_mapKeyIndexStack.back();
	if (idx < m_mapKeysStack.back().size())
	{
		return m_mapKeysStack.back()[idx++];
	}
	return "";
}

void CJsonDeSerializer::EndMap()
{
	if (m_nodeStack.size() > 1) m_nodeStack.pop_back();
	if (!m_mapKeysStack.empty()) m_mapKeysStack.pop_back();
	if (!m_mapKeyIndexStack.empty()) m_mapKeyIndexStack.pop_back();
}

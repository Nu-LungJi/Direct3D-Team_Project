#include "pch.h"
#include "JsonDeSerializer.h"
#include "ISerializable.h"
#include <fstream>
#include <iostream>

NS_USING(Engine)

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
	catch (nlohmann::json::parse_error& e) { return E_FAIL; }

	return S_OK;
}

HRESULT CJsonDeSerializer::Initialize()
{
	m_nodeStack.push_back(&m_json);
	return S_OK;
}

// =======================================================
// 1. 원자 타입 읽기 (안전한 인덱스 접근)
// =======================================================
void CJsonDeSerializer::Read(const std::string& key, int& outValue)
{
	nlohmann::json& node = *m_nodeStack.back();

	if (node.is_array() && !m_arrayIndexStack.empty())
	{
		size_t& idx = m_arrayIndexStack.back();
		if (idx < node.size())
		{
			if (!node[idx].is_null()) outValue = node[idx].get<int>();
			idx++; // 값을 못 읽어도 인덱스는 증가시켜야 다음 원소로 넘어감
		}
	}
	else if (node.is_object() && node.contains(key) && !node[key].is_null())
	{
		outValue = node[key].get<int>();
	}
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

	// [수정됨] 크래시 방지 방어 코드 추가: matrix가 2차원 배열(4x4) 형태를 갖추고 있는지 확인
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

// =======================================================
// 2. 자식 ISerializable 객체 읽기
// =======================================================
void CJsonDeSerializer::Read(const std::string& key, ISerializable& outValue)
{
	nlohmann::json& node = *m_nodeStack.back();

	if (node.is_array() && !m_arrayIndexStack.empty())
	{
		size_t& idx = m_arrayIndexStack.back();
		if (idx < node.size() && node[idx].is_object())
		{
			m_nodeStack.push_back(&node[idx]);
			outValue.Deserialize(*this);
			m_nodeStack.pop_back();
		}
		idx++; // 안전한 순회를 위해 증가
	}
	else if (node.is_object() && node.contains(key) && node[key].is_object())
	{
		m_nodeStack.push_back(&node[key]);
		outValue.Deserialize(*this);
		m_nodeStack.pop_back();
	}
}

// =======================================================
// 3. 컨테이너 노드 제어 (스택 밸런스 완벽 보장)
// =======================================================
size_t CJsonDeSerializer::StartArray(const std::string& key)
{
	nlohmann::json& node = *m_nodeStack.back();
	bool bSuccess = false;

	if (node.is_array() && !m_arrayIndexStack.empty())
	{
		size_t idx = m_arrayIndexStack.back();
		if (idx < node.size() && node[idx].is_array())
		{
			m_nodeStack.push_back(&node[idx]);
			bSuccess = true;
		}
	}
	else if (node.is_object() && node.contains(key) && node[key].is_array())
	{
		m_nodeStack.push_back(&node[key]);
		bSuccess = true;
	}

	// [핵심] 실패하더라도 EndArray()의 pop_back()과 짝을 맞추기 위해 자기 자신을 더미로 푸시!
	if (!bSuccess) m_nodeStack.push_back(&node);

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

	// [수정됨] 누락되었던 실제 맵 노드 진입 로직 복구
	if (node.is_object() && node.contains(key) && node[key].is_object())
	{
		m_nodeStack.push_back(&node[key]);
		bSuccess = true;
	}
	else
	{
		m_nodeStack.push_back(&node); // 밸런스 유지용 더미 노드 푸시
	}

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
	// [수정됨] 누락되었던 노드 탈출 로직 복구
	if (m_nodeStack.size() > 1) m_nodeStack.pop_back();
	if (!m_mapKeysStack.empty()) m_mapKeysStack.pop_back();
	if (!m_mapKeyIndexStack.empty()) m_mapKeyIndexStack.pop_back();
}

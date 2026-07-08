#include "JsonSerializer.h"
#include "ISerializable.h"
#include <fstream>

NS_USING(Engine)

CJsonSerializer::CJsonSerializer()
{
}

CJsonSerializer::~CJsonSerializer()
{
}

HRESULT CJsonSerializer::Initialize()
{
	m_nodeStack.push_back(&m_json);
	return S_OK;
}

UPtr<CJsonSerializer> CJsonSerializer::Create()
{
	auto pInstance = ToUPtr(new CJsonSerializer{});
	if (FAILED(pInstance->Initialize()))
	{
		return nullptr;
	}
	return pInstance;
}

void CJsonSerializer::Write(const std::string& key, int value)
{
	nlohmann::json& node = *m_nodeStack.back();

	if (node.is_array()) node.push_back(value);
	else                 node[key] = value;
}

void CJsonSerializer::Write(const std::string& key, float value)
{
	nlohmann::json& node = *m_nodeStack.back();

	if (node.is_array()) node.push_back(value);
	else                 node[key] = value;
}

void CJsonSerializer::Write(const std::string& key, const _float2& value)
{
	nlohmann::json& node = *m_nodeStack.back();
	// _float2를 float 배열로 변환하여 저장
	std::vector<float> vec = { value.x, value.y };

	if (node.is_array()) node.push_back(vec);
	else                 node[key] = vec;
}

void CJsonSerializer::Write(const std::string& key, const _float3& value)
{
	nlohmann::json& node = *m_nodeStack.back();
	std::vector<float> vec = { value.x, value.y, value.z };

	if (node.is_array()) node.push_back(vec);
	else                 node[key] = vec;
}

void CJsonSerializer::Write(const std::string& key, const _float4& value)
{
	nlohmann::json& node = *m_nodeStack.back();
	std::vector<float> vec = { value.x, value.y, value.z, value.w };

	if (node.is_array()) node.push_back(vec);
	else                 node[key] = vec;
}

void CJsonSerializer::Write(const std::string& key, const _float4x4& value)
{
	nlohmann::json& node = *m_nodeStack.back();

	// 4x4 행렬을 float 배열의 배열로 변환
	nlohmann::json matrix = nlohmann::json::array();
	const float* f = reinterpret_cast<const float*>(&value);

	for (int i = 0; i < 4; ++i) {
		nlohmann::json row = nlohmann::json::array();
		for (int j = 0; j < 4; ++j) {
			row.push_back(f[i * 4 + j]);
		}
		matrix.push_back(row);
	}

	if (node.is_array()) node.push_back(matrix);
	else                 node[key] = matrix;
}
void CJsonSerializer::Write(const std::string& key, const std::string& value)
{
	nlohmann::json& node = *m_nodeStack.back();

	if (node.is_array()) node.push_back(value);
	else                 node[key] = value;
}

void CJsonSerializer::Write(const std::string& key, const ISerializable& value)
{
	nlohmann::json& node = *m_nodeStack.back();

	if (node.is_array())
	{
		node.push_back(nlohmann::json::object());
		m_nodeStack.push_back(&node.back());
	}
	else
	{
		node[key] = nlohmann::json::object();
		m_nodeStack.push_back(&node[key]);
	}

	value.Serialize(*this);
	m_nodeStack.pop_back();
}

void CJsonSerializer::StartArray(const std::string& key)
{
	nlohmann::json& node = *m_nodeStack.back();

	if (node.is_array())
	{
		node.push_back(nlohmann::json::array());
		m_nodeStack.push_back(&node.back());
	}
	else
	{
		node[key] = nlohmann::json::array();
		m_nodeStack.push_back(&node[key]);
	}
}

void CJsonSerializer::EndArray()
{
	if (m_nodeStack.size() > 1) m_nodeStack.pop_back();
}

void CJsonSerializer::StartMap(const std::string& key)
{
	nlohmann::json& currentNode = *m_nodeStack.back();

	currentNode[key] = nlohmann::json::object();
	m_nodeStack.push_back(&currentNode[key]);
}

void CJsonSerializer::EndMap()
{
	if (m_nodeStack.size() > 1)
	{
		m_nodeStack.pop_back();
	}
}

void CJsonSerializer::SaveToFile(const std::string& path)
{
	std::ofstream file(path);
	if (!file.is_open())
	{
		MSG_BOX("파일 저장 실패");
		return;
	}

	file << m_json.dump(4);
	file.close();

	// 세이브 후 인스턴스를 재활용할 수도 있으므로 스택을 초기 상태로 복구
	m_nodeStack.clear();
	m_nodeStack.push_back(&m_json);
}

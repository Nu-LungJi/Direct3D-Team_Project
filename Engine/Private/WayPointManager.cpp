#include "pch.h"
#include "WayPointManager.h"
#include "GameInstance.h"

NS_USING(Engine)
CWayPointManager::CWayPointManager()
{
}

CWayPointManager::~CWayPointManager()
{
}

void CWayPointManager::Update(_float fTimeDelta)
{

}

HRESULT CWayPointManager::Render()
{
	return S_OK;
}

void CWayPointManager::UpdateGUI()
{
	if (ImGui::Begin("WayPointManager"))
	{

		if (ImGui::TreeNode("Way"))
		{
			if (ImGui::Button("AddWay"))
			{
				auto pSrc = CGameInstance::Get().GetActiveCamera();
				if (nullptr == pSrc) return;

				m_WayPoint.push_back(pSrc->GetTransform().GetPosition());
			}
			if (ImGui::Button("UndoWay"))
			{
				if (!m_WayPoint.empty())
					m_WayPoint.pop_back();
			}
			if (ImGui::Button("SaveWay"))
				m_bPopup = true;
			if (ImGui::Button("LoadWay"))
			{
				m_bPopupL = true;
			}
			if (m_bPopupL)
			{
				ImGui::OpenPopup("LoadWay");
				_char NameBUffer[64]{};
				if (ImGui::BeginPopup("LoadWay", ImGuiWindowFlags_AlwaysAutoResize))
				{
					ImGui::Text("FileName");
					if (ImGui::InputText("##FileName", &NameBUffer[0], IM_ARRAYSIZE(NameBUffer))) //이름 입력
					{
						m_WayName = NameBUffer;

					}
					if (ImGui::Button("Ok"))
					{
						m_bPopupL = false;
						if (m_WayName.empty())
						{
							ImGui::CloseCurrentPopup();
							MSG_BOX("NoName");
						}
						else
						{
							m_WayPoint.clear();
							LoadWay(m_WayName, m_WayPoint);
						}
					} ImGui::SameLine(100.f);
					if (ImGui::Button("Cancle"))
					{
						m_bPopupL = false;
						ImGui::CloseCurrentPopup();
					}
					ImGui::EndPopup();
				}

			}
			if (m_bPopup)
			{
				_string Path = "./Resources/json/WayPoint/";
				ImGui::OpenPopup("SaveWay");
				_char NameBUffer[64]{};
				if (ImGui::BeginPopup("SaveWay", ImGuiWindowFlags_AlwaysAutoResize))
				{
					ImGui::Text("FileName");
					if (ImGui::InputText("##FileName", &NameBUffer[0], IM_ARRAYSIZE(NameBUffer))) //이름 입력
					{
						m_WayName = NameBUffer;

					}
					if (ImGui::Button("Ok"))
					{
						m_bPopup = false;
						if (m_WayPoint.empty())
						{
							ImGui::CloseCurrentPopup();
							MSG_BOX("NoName");
						}
						else
						{
							nlohmann::json j;
							JsonSaveLoadManager::SaveJsonTypeFloat3vector(j, m_WayName, m_WayPoint);
							Path += m_WayName + ".json";
							std::ofstream path(Path);
							path << j.dump(4);
							path.close();
						}
					} ImGui::SameLine(100.f);
					if (ImGui::Button("Cancle"))
					{
						m_bPopup = false;
						ImGui::CloseCurrentPopup();
					}
					ImGui::EndPopup();
				}
			}

			Way_Debug();
			ImGui::TreePop();
		}
		ImGui::End();
	}

	
}

void CWayPointManager::RegistWayTag(const _string& jsonFileName, const _string& MajorName, const _string& MinorName)
{
	auto iter = m_WayNames.find(jsonFileName);
	if (iter == m_WayNames.end())
	{
		WAY_NAME Way;
		Way.MajorName = MajorName;
		Way.MinorName = MinorName;
		m_WayNames[jsonFileName] = Way;
	}
}

void CWayPointManager::LoadWay(const _string& jsonFileName, std::vector<_float3>& OutName)
{
	auto& iter = FInd_Way(jsonFileName);
	if (iter.MajorName.empty() || iter.MinorName.empty()) return;
	auto pRes = CGameInstance::Get().GetResourceFirst<CResJson>(iter.MajorName, iter.MinorName);
	if (nullptr == pRes)
	{
		MSG_BOX("Load Failed Json To LoadWay");
		return;
	}
	auto json = pRes->Get_Json();
	JsonSaveLoadManager::LoadJsonTypeFloat3Vector(json, jsonFileName, OutName);
	
	return;
}

const WAY_NAME CWayPointManager::FInd_Way(const _string& WayName)
{
	auto iter = m_WayNames.find(WayName);

	if (iter != m_WayNames.end())
		return iter->second;

	WAY_NAME way{};
	return way;
}

void CWayPointManager::Way_Debug()
{
	uint32_t i = 0;
	for (auto& iter : m_WayPoint)
	{
		auto pDbgLineRender = CGameInstance::Get().GetDbgLineRender();

		const auto vPreviousColor = pDbgLineRender->GetColor();
		const auto ePreviousDepthMode = pDbgLineRender->GetDepthMode();
		pDbgLineRender->SetColor({ 0.f, 1.f, 1.f, 1.f });
		pDbgLineRender->SetDepthTest(true);
		pDbgLineRender->AddSphere(1.2f, XMMatrixTranslation(iter.x, iter.y, iter.z));
		pDbgLineRender->SetColor(vPreviousColor);
		pDbgLineRender->SetDepthMode(ePreviousDepthMode);
		WayPoint(iter, 0x44524750 + i++);
	}

}

void CWayPointManager::WayPoint(_float3& vPos, uint32_t iID)
{

	auto pCamera = CGameInstance::Get().GetActiveCamera();

	ImGuiViewport* pViewport = ImGui::GetMainViewport();

	if (!pCamera || !pViewport)
		return;

	_float4x4 View{};
	_float4x4 Projection{};
	_float4x4 World{};

	XMStoreFloat4x4(&View, pCamera->GetView());
	XMStoreFloat4x4(&Projection, pCamera->GetProj());

	XMStoreFloat4x4(&World, XMMatrixTranslation(vPos.x, vPos.y, vPos.z));

	ImGuizmo::SetOrthographic(false);
	ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList(pViewport));

	ImGuizmo::SetRect(pViewport->Pos.x, pViewport->Pos.y, pViewport->Size.x, pViewport->Size.y);;
	ImGuizmo::SetID(iID);
	if (!ImGuizmo::Manipulate(&View._11, &Projection._11, ImGuizmo::TRANSLATE, ImGuizmo::WORLD, &World._11))
		return;

	vPos = { World._41, World._42, World._43 };

	return;

}
UPtr<CWayPointManager> CWayPointManager::Create()
{
	return UPtr<CWayPointManager>(new CWayPointManager{});
}


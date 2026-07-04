#include "pch.h"
#include "MapManager.h"
#include "MapMeshObject.h"
#include <fstream>

NS_USING(Engine)

CMapManager::CMapManager()
{
}

CMapManager::~CMapManager()
{
}

HRESULT CMapManager::Initialize()
{
	return S_OK;
}

void CMapManager::PriorityUpdate(_float fTimeDelta)
{

}
void CMapManager::Update(_float fTimeDelta)
{

}
void CMapManager::LateUpdate(_float fTimeDelta)
{

}

void CMapManager::UpdateGUI()
{

}

HRESULT CMapManager::SaveMap(const std::string& path)
{
	nlohmann::ordered_json rootJson = {};
	rootJson["version"] = 1;
	rootJson["objects"] = nlohmann::ordered_json::array();

	const auto& layers = CGameInstance::Get().GetGameObjectLayers();

	for (const auto& pair : layers)
	{
		const auto& objects = pair.second;

		for(const auto& objectHandle : objects)
		{
			CMapMeshObject* pMeshObj = CGameInstance::Get().GetGameObjectByHandleT<CMapMeshObject>(objectHandle);
			
			if (pMeshObj == nullptr)
				continue;

			const auto& modelResourceGroupName = pMeshObj->GetModelResourceGroup();
			const auto& modelResourceTag = pMeshObj->GetModelResourceTag();
			const auto& layerName = pair.first;

			const auto& pTransform = pMeshObj->GetTransform();
			
			nlohmann::ordered_json objectJson =
			{
				{"type", "MapMeshObject"},
				{"objectTag", std::string(pMeshObj->GetObjectTag())},
				{"protoGroup", "PERMANENT"},
				{"prototype", "Prototype_GameObject_MapMeshObject"},
				{"modelGroup", modelResourceGroupName},
				{"model", modelResourceTag},
				{"layer", layerName},
				{"position", {
					pTransform.GetPosition().x,
					pTransform.GetPosition().y,
					pTransform.GetPosition().z
				}},
				{"rotation", {
					pTransform.GetQuaternion().x,
					pTransform.GetQuaternion().y,
					pTransform.GetQuaternion().z,
					pTransform.GetQuaternion().w,

				}},
				{"scale", {
					pTransform.GetScale().x,
					pTransform.GetScale().y,
					pTransform.GetScale().z
				}}
			};

			rootJson["objects"].push_back(objectJson);
		}
	}

	std::ofstream outFile(path + "TestMap.json");
	if (!outFile.is_open())
	{
		return E_FAIL;
	}

	outFile << rootJson.dump(4);
	outFile.close();

	return S_OK;
}

HRESULT CMapManager::LoadMap(const std::string& path, _bool clearBeforeLoad)
{
	if (clearBeforeLoad)
	{
		CGameInstance::Get().GameObjectAllReset();
	}

	std::ifstream inFile(path + "TestMap.json");
	if (!inFile.is_open())
	{
		return E_FAIL;
	}

	nlohmann::ordered_json rootJson;
	inFile >> rootJson;

	inFile.close();

	int version = rootJson["version"];

	for (const auto& objectJson : rootJson["objects"])
	{
		if (objectJson["type"] == "MapMeshObject")
		{
			std::string type = objectJson["type"];
			std::string objectTag = objectJson["objectTag"];
			std::string modelGroup = objectJson["modelGroup"];
			std::string model = objectJson["model"];
			std::string layer = objectJson["layer"];

			auto pos = objectJson["position"];
			_float x = pos[0];
			_float y = pos[1];
			_float z = pos[2];

			E::CMapMeshObject::MAP_MESH_OBJECT_DESC Desc{};
			Desc.sObjectTag = objectTag;
			Desc.modelGroupTag = modelGroup;
			Desc.modelResTag = model;
			Desc.protoGroupTag = "PERMANENT";
			Desc.prototypeTag = "Prototype_GameObject_MapMeshObject";

			auto hObject = E::CGameInstance::Get().AddGameObjectToLayer(
				Desc.protoGroupTag,
				Desc.prototypeTag,
				layer,
				&Desc);

			auto newObj = CGameInstance::Get().GetGameObjectByHandle(hObject.value());
			newObj->GetTransform().SetPosition(XMVectorSet(x,y,z,1.f));
		}
	}
	return S_OK;
}

UPtr<CMapManager> CMapManager::Create()
{
	auto pInstance = ToUPtr(new CMapManager{});
	if (FAILED(pInstance->Initialize()))
	{
		return nullptr;
	}
	return pInstance;
}

void CMapManager::Free()
{
	CEngineBase::Free();
}

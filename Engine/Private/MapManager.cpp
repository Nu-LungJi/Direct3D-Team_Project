#include "pch.h"
#include "MapManager.h"
#include "MapMeshObject.h"
#include <fstream>

NS_USING(Engine)

#ifdef _DEBUG
std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> CMapManager::_batch = nullptr;
std::unique_ptr<DirectX::BasicEffect> CMapManager::_effect = nullptr;
ComPtr<ID3D11InputLayout> CMapManager::_inputLayout = nullptr;
#endif

CMapManager::CMapManager()
{
}

CMapManager::~CMapManager()
{
#ifdef _DEBUG
	//-------DebugDraw-------
	_batch.reset();
	_effect.reset();
	_inputLayout.Reset();
	//-----------------------
#endif
}

HRESULT CMapManager::Initialize()
{

#ifdef _DEBUG
	// DebugDraw
	// static 객체들이 아직 생성되지 않았다면 최초 1회 생성
	if (_batch == nullptr)
	{
		// Batch 생성
		_batch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(CGameInstance::Get().GetGraphicDeviceContext().Get());

		// Effect 생성 및 설정
		_effect = std::make_unique<DirectX::BasicEffect>(CGameInstance::Get().GetGraphicDevice().Get());
		_effect->SetVertexColorEnabled(true); // 정점 색상 사용 활성화

		// InputLayout 생성 (BasicEffect의 셰이더 정보와 VertexPositionColor 구조를 연결)
		void const* shaderByteCode;
		size_t byteCodeLength;
		_effect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

		HRESULT hr = CGameInstance::Get().GetGraphicDevice()->CreateInputLayout(
			DirectX::VertexPositionColor::InputElements,
			DirectX::VertexPositionColor::InputElementCount,
			shaderByteCode,
			byteCodeLength,
			_inputLayout.GetAddressOf()
		);

		assert(SUCCEEDED(hr));
	}
#endif

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

HRESULT CMapManager::SaveMap(const std::string& path)
{
	RebuildChunks();

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
			const auto& pos = pTransform.GetPosition();
			const auto& quat = pTransform.GetQuaternion();
			const auto& scale = pTransform.GetScale();

			MAPCHUNK_COORD coord = WorldToChunkCoord(pos);
			
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
					pos.x,
					pos.y,
					pos.z
				}},
				{"rotation", {
					quat.x,
					quat.y,
					quat.z,
					quat.w,

				}},
				{"scale", {
					scale.x,
					scale.y,
					scale.z
				}},
				{"chunk", {
					{"x", coord.x},
					{"y", coord.y},
					{"z", coord.z}
					}
				}
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

			const auto& pos = objectJson["position"];
			const auto& rot = objectJson["rotation"];
			const auto& scale = objectJson["scale"];

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
			auto& newObjTransform = newObj->GetTransform();

			newObjTransform.SetPosition(XMVectorSet(pos[0], pos[1], pos[2], 1.f));
			newObjTransform.SetQuaternion(_float4{ rot[0], rot[1], rot[2], rot[3] });
			newObjTransform.SetScale(_float3{ scale[0], scale[1], scale[2] });

			//const auto& chunkJson = objectJson["chunk"];
			// MAPCHUNK_COORD coord = {};
			//coord.x = chunkJson["x"];
			//coord.y = chunkJson["y"];
			//coord.z = chunkJson["z"];

			// json 수정이나 chunkSize 변경 시 달라질 수 있기 때문에 재 계산하여 로드
			MAPCHUNK_COORD coord = WorldToChunkCoord({ pos[0], pos[1], pos[2] });
			auto& chunk = m_Chunks[coord];
			chunk.coord = coord;
			chunk.hObjects.push_back(hObject.value());
			chunk.bounds = MakeChunkBoundingBox(coord);
			chunk.m_bDirty = true;
		}
	}
	return S_OK;
}

_float3 CMapManager::GetChunkCenter(const MAPCHUNK_COORD& coord)
{
	return _float3(
			{
				(coord.x + 0.5f) * m_vChunkSize.x,
				(coord.y + 0.5f) * m_vChunkSize.y,
				(coord.z + 0.5f) * m_vChunkSize.z,
			});
}

BoundingBox CMapManager::MakeChunkBoundingBox(const MAPCHUNK_COORD& coord)
{
	_float3 center = GetChunkCenter(coord);
	_float3 extents = {
		m_vChunkSize.x * 0.5f,
		m_vChunkSize.y * 0.5f,
		m_vChunkSize.z * 0.5f
	};
	return BoundingBox(center, extents);
}

MAPCHUNK_COORD CMapManager::WorldToChunkCoord(const _float3& pos) const
{
	return {
		static_cast<int64_t>(std::floor(pos.x / m_vChunkSize.x)),
		static_cast<int64_t>(std::floor(pos.y / m_vChunkSize.y)),
		static_cast<int64_t>(std::floor(pos.z / m_vChunkSize.z))
	};
}

void CMapManager::RebuildChunks()
{
	m_Chunks.clear();

	const auto& layers = CGameInstance::Get().GetGameObjectLayers();

	for (const auto& [layerName, layer] : layers)
	{
		for (const auto& handle : layer)
		{
			CGameObject* pObj = CGameInstance::Get().GetGameObjectByHandle(handle);

			if (pObj == nullptr)
				continue;

			const auto& pos = pObj->GetTransform().GetPosition();

			MAPCHUNK_COORD coord = WorldToChunkCoord(pos);

			auto& chunk = m_Chunks[coord];
			chunk.coord = coord;
			chunk.hObjects.push_back(handle);
			chunk.bounds = MakeChunkBoundingBox(coord);
			chunk.m_bDirty = true;
		}
	}

}

#ifdef _DEBUG

HRESULT CMapManager::RenderDebugMapChunk()
{
	if (m_bDebugDrawMapChunk == false)
		return E_FAIL;

	CCameraObject* cam = CGameInstance::Get().GetActiveCamera();

	if (cam == nullptr)
		return E_FAIL;

	for (const auto& [coord, mapChunk] : m_Chunks)
	{
		_float3 center = GetChunkCenter(coord);
		DrawBox(mapChunk.bounds, Colors::Lime, cam->GetView(), cam->GetProj(), XMMatrixIdentity());
	}

	return S_OK;
}

void CMapManager::DrawBox(const DirectX::BoundingBox& box, DirectX::FXMVECTOR color, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection, DirectX::CXMMATRIX world)
{
	_effect->SetView(view);
	_effect->SetProjection(projection);
	_effect->SetWorld(world);
	CGameInstance::Get().GetGraphicDeviceContext()->IASetInputLayout(_inputLayout.Get());

	// 그리기
	_effect->Apply(CGameInstance::Get().GetGraphicDeviceContext().Get());
	_batch->Begin();

	// DirectXTK의 내장 Draw 함수 호출
	DX::Draw(_batch.get(), box, color);

	_batch->End();
}
#endif

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


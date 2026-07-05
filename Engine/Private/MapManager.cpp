#include "pch.h"
#include "MapManager.h"
#include "MapMeshObject.h"
#include <fstream>
#include <filesystem>

NS_USING(Engine)

namespace
{
	std::string ChunkFileName(const MAPCHUNK_COORD& coord)
	{
		return std::to_string(coord.x) + "_" + std::to_string(coord.y) + "_" + std::to_string(coord.z) + ".json";
	}

	nlohmann::ordered_json MakeCoordJson(const MAPCHUNK_COORD& coord)
	{
		return nlohmann::ordered_json
		{
			{"x", coord.x},
			{"y", coord.y},
			{"z", coord.z}
		};
	}

	MAPCHUNK_COORD ReadCoordJson(const nlohmann::ordered_json& coordJson)
	{
		return MAPCHUNK_COORD
		{
			coordJson["x"].get<int64_t>(),
			coordJson["y"].get<int64_t>(),
			coordJson["z"].get<int64_t>()
		};
	}

	nlohmann::ordered_json MakeObjectJson(CMapMeshObject* pMeshObj, const std::string& layerName, const MAPCHUNK_COORD& coord)
	{
		const auto& pTransform = pMeshObj->GetTransform();
		const auto& pos = pTransform.GetPosition();
		const auto& quat = pTransform.GetQuaternion();
		const auto& scale = pTransform.GetScale();

		return nlohmann::ordered_json
		{
			{"type", "MapMeshObject"},
			{"objectTag", std::string(pMeshObj->GetObjectTag())},
			{"protoGroup", "PERMANENT"},
			{"prototype", "Prototype_GameObject_MapMeshObject"},
			{"modelGroup", pMeshObj->GetModelResourceGroup()},
			{"model", pMeshObj->GetModelResourceTag()},
			{"layer", layerName},
			{"position", { pos.x, pos.y, pos.z }},
			{"rotation", { quat.x, quat.y, quat.z, quat.w }},
			{"scale", { scale.x, scale.y, scale.z }},
			{"chunk", MakeCoordJson(coord)}
		};
	}

	bool HasHandle(const std::vector<CHandle>& handles, const CHandle& target)
	{
		return std::find(handles.begin(), handles.end(), target) != handles.end();
	}

	bool CanAutoUnload(const MAPCHUNK& chunk)
	{
		return chunk.loadState == EChunkLoadState::Loaded
			&& chunk.saveState != EChunkSaveState::Unsaved;
	}

	bool CanAutoLoad(const MAPCHUNK& chunk)
	{
		return chunk.loadState == EChunkLoadState::Unloaded
			&& chunk.saveState != EChunkSaveState::Unsaved;
	}

	std::optional<CHandle> CreateMapMeshObjectFromJson(const nlohmann::ordered_json& objectJson)
	{
		if (!objectJson.contains("type") || objectJson["type"] != "MapMeshObject")
		{
			return std::nullopt;
		}

		const std::string objectTag = objectJson["objectTag"];
		const std::string modelGroup = objectJson["modelGroup"];
		const std::string model = objectJson["model"];
		const std::string layer = objectJson["layer"];

		E::CMapMeshObject::MAP_MESH_OBJECT_DESC desc{};
		desc.sObjectTag = objectTag;
		desc.modelGroupTag = modelGroup;
		desc.modelResTag = model;
		desc.protoGroupTag = objectJson.value("protoGroup", "PERMANENT");
		desc.prototypeTag = objectJson.value("prototype", "Prototype_GameObject_MapMeshObject");

		auto hObject = E::CGameInstance::Get().AddGameObjectToLayer(
			desc.protoGroupTag,
			desc.prototypeTag,
			layer,
			&desc);
		if (!hObject.has_value())
		{
			return std::nullopt;
		}

		auto* newObj = E::CGameInstance::Get().GetGameObjectByHandle(hObject.value());
		if (newObj == nullptr)
		{
			return std::nullopt;
		}

		const auto& pos = objectJson["position"];
		const auto& rot = objectJson["rotation"];
		const auto& scale = objectJson["scale"];

		auto& newObjTransform = newObj->GetTransform();
		newObjTransform.SetPosition(XMVectorSet(pos[0], pos[1], pos[2], 1.f));
		newObjTransform.SetQuaternion(_float4{ rot[0], rot[1], rot[2], rot[3] });
		newObjTransform.SetScale(_float3{ scale[0], scale[1], scale[2] });

		return hObject;
	}
}

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

// 저장해놨던 Chunk들을 심리스 로딩/언로딩
// (저장해놓은 Chunk들만 작동하기때문에 에디터에서 실시간으로 Rebuild한 Cunk는 심리스의 적용대상이 되지않음)
void CMapManager::Update(_float fTimeDelta)
{
	if (!m_bChunkStreaming)
	{
		return;
	}

	if (m_sMapRootPath.empty() || m_Chunks.empty())
	{
		return;
	}

	CCameraObject* pCamera = CGameInstance::Get().GetActiveCamera();
	if (pCamera == nullptr)
	{
		return;
	}

	const auto& pos = pCamera->GetTransform().GetPosition();
	const MAPCHUNK_COORD cameraChunkCoord = WorldToChunkCoord(pos); // 카메라가 속한 Chunk 구함

	std::vector<MAPCHUNK_COORD> neededChunks;
	neededChunks.reserve(27);

	// 카메라가 속한 Chunk 주변의 3*3*3 Chunk들
	for (int64_t y = -1; y <= 1; ++y)
	{
		for (int64_t z = -1; z <= 1; ++z)
		{
			for (int64_t x = -1; x <= 1; ++x)
			{
				neededChunks.push_back(
					MAPCHUNK_COORD
					{
						cameraChunkCoord.x + x,
						cameraChunkCoord.y + y,
						cameraChunkCoord.z + z
					});
			}
		}
	}

	auto isNeededChunk = [&neededChunks](const MAPCHUNK_COORD& coord)
		{
			return std::find(neededChunks.begin(), neededChunks.end(), coord) != neededChunks.end();
		};

	// 모든 Chunk들 중 주변3*3*3 Chunk가 아니라면 UnLoad
	for (auto& [coord, chunk] : m_Chunks)
	{
		if (CanAutoUnload(chunk) && !isNeededChunk(coord))
		{
			UnLoadChunk(coord);
		}
	}

	// 주변 3*3*3 Chunk들 중, m_Chunks에 존재하는거라면 Load
	for (const auto& coord : neededChunks)
	{
		auto iter = m_Chunks.find(coord);
		if (iter == m_Chunks.end())
		{
			continue;
		}

		if (CanAutoLoad(iter->second))
		{
			LoadChunk(coord);
		}
	}
}
void CMapManager::LateUpdate(_float fTimeDelta)
{

}

HRESULT CMapManager::SaveMap(const std::string& path)
{
	RebuildChunks();

	const std::filesystem::path mapDir(path);
	const std::filesystem::path chunkDir = mapDir / "chunks";
	std::error_code ec;
	std::filesystem::create_directories(chunkDir, ec);
	if (ec)
	{
		return E_FAIL;
	}

	nlohmann::ordered_json rootJson = {};
	rootJson["version"] = 1;
	rootJson["chunkSize"] = { m_vChunkSize.x, m_vChunkSize.y, m_vChunkSize.z };
	rootJson["chunks"] = nlohmann::ordered_json::array();
	rootJson["objects"] = nlohmann::ordered_json::array();

	const auto& layers = CGameInstance::Get().GetGameObjectLayers();

	std::vector<MAPCHUNK_COORD> originallyUnloadedChunks;
	for (auto& [coord, chunk] : m_Chunks)
	{
		if (chunk.loadState == EChunkLoadState::Unloaded && chunk.saveState != EChunkSaveState::Unsaved)
		{
			originallyUnloadedChunks.push_back(coord);
			if (FAILED(LoadChunk(coord)))
			{
				return E_FAIL;
			}
		}
	}

	for (auto& [coord, chunk] : m_Chunks)
	{
		const std::string fileName = ChunkFileName(coord);
		const std::filesystem::path relativePath = std::filesystem::path("chunks") / fileName;
		const std::filesystem::path chunkPath = mapDir / relativePath;

		chunk.filePath = relativePath.generic_string();

		if (FAILED(SaveChunk(coord, chunkPath.generic_string())))
		{
			return E_FAIL;
		}

		chunk.saveState = EChunkSaveState::Saved;

		rootJson["chunks"].push_back(nlohmann::ordered_json
		{
			{"coord", MakeCoordJson(coord)},
			{"file", chunk.filePath},
			{"objectCount", chunk.hObjects.size()}
		});
	}

	for (const auto& pair : layers)
	{
		const auto& objects = pair.second;

		for(const auto& objectHandle : objects)
		{
			CMapMeshObject* pMeshObj = CGameInstance::Get().GetGameObjectByHandleT<CMapMeshObject>(objectHandle);
			
			if (pMeshObj == nullptr)
				continue;

			const auto& layerName = pair.first;

			const auto& pos = pMeshObj->GetTransform().GetPosition();
			const MAPCHUNK_COORD coord = WorldToChunkCoord(pos);
			rootJson["objects"].push_back(MakeObjectJson(pMeshObj, layerName, coord));
		}
	}

	for (const auto& coord : originallyUnloadedChunks)
	{
		UnLoadChunk(coord);
	}

	std::ofstream outFile((mapDir / "map.json").string());
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
		m_Chunks.clear();
	}

	const std::filesystem::path mapDir(path);
	std::filesystem::path mapFilePath = mapDir / "map.json";
	if (std::filesystem::exists(mapFilePath))
	{
		if (FAILED(LoadMapData(path)))
		{
			return E_FAIL;
		}

		std::vector<MAPCHUNK_COORD> chunkCoords;
		chunkCoords.reserve(m_Chunks.size());
		for (const auto& [coord, chunk] : m_Chunks)
		{
			chunkCoords.push_back(coord);
		}

		for (const auto& coord : chunkCoords)
		{
			if (FAILED(LoadChunk(coord)))
			{
				return E_FAIL;
			}
		}

		return S_OK;
	}

	mapFilePath = mapDir / "TestMap.json";
	std::ifstream inFile(mapFilePath.string());
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
		if (auto hObject = CreateMapMeshObjectFromJson(objectJson))
		{
			const auto& pos = objectJson["position"];
			MAPCHUNK_COORD coord = WorldToChunkCoord({ pos[0], pos[1], pos[2] });
			auto& chunk = m_Chunks[coord];
			chunk.coord = coord;
			chunk.hObjects.push_back(hObject.value());
			chunk.bounds = MakeChunkBoundingBox(coord);
			chunk.loadState = EChunkLoadState::Loaded;
			chunk.saveState = EChunkSaveState::Saved;
		}
	}
	return S_OK;
}

HRESULT CMapManager::SaveChunk(const MAPCHUNK_COORD& coord, const std::string& chunkPath)
{
	const auto iter = m_Chunks.find(coord);
	if (iter == m_Chunks.end())
	{
		return E_FAIL;
	}

	const std::filesystem::path filePath(chunkPath);
	std::error_code ec;
	std::filesystem::create_directories(filePath.parent_path(), ec);
	if (ec)
	{
		return E_FAIL;
	}

	const MAPCHUNK& chunk = iter->second;
	nlohmann::ordered_json chunkJson = {};
	chunkJson["version"] = 1;
	chunkJson["coord"] = MakeCoordJson(coord);
	chunkJson["bounds"] =
	{
		{"center", { chunk.bounds.Center.x, chunk.bounds.Center.y, chunk.bounds.Center.z }},
		{"extents", { chunk.bounds.Extents.x, chunk.bounds.Extents.y, chunk.bounds.Extents.z }}
	};
	chunkJson["objects"] = nlohmann::ordered_json::array();

	const auto& layers = CGameInstance::Get().GetGameObjectLayers();
	for (const auto& [layerName, layer] : layers)
	{
		for (const auto& handle : layer)
		{
			if (!HasHandle(chunk.hObjects, handle))
			{
				continue;
			}

			CMapMeshObject* pMeshObj = CGameInstance::Get().GetGameObjectByHandleT<CMapMeshObject>(handle);
			if (pMeshObj == nullptr)
			{
				continue;
			}

			chunkJson["objects"].push_back(MakeObjectJson(pMeshObj, layerName, coord));
		}
	}

	std::ofstream outFile(filePath.string());
	if (!outFile.is_open())
	{
		return E_FAIL;
	}

	outFile << chunkJson.dump(4);
	outFile.close();

	return S_OK;
}

HRESULT CMapManager::LoadMapData(const std::string& path)
{
	const std::filesystem::path mapDir(path);
	const std::filesystem::path mapFilePath = mapDir / "map.json";

	std::ifstream inFile(mapFilePath.string());
	if (!inFile.is_open())
	{
		return E_FAIL;
	}

	nlohmann::ordered_json rootJson;
	inFile >> rootJson;
	inFile.close();

	m_sMapRootPath = mapDir.generic_string();
	m_Chunks.clear();

	if (rootJson.contains("chunkSize"))
	{
		const auto& chunkSize = rootJson["chunkSize"];
		m_vChunkSize = _float3{ chunkSize[0], chunkSize[1], chunkSize[2] };
	}

	if (!rootJson.contains("chunks"))
	{
		return S_OK;
	}

	for (const auto& chunkMetaJson : rootJson["chunks"])
	{
		const MAPCHUNK_COORD coord = ReadCoordJson(chunkMetaJson["coord"]);
		MAPCHUNK chunk{};
		chunk.coord = coord;
		chunk.bounds = MakeChunkBoundingBox(coord);
		chunk.filePath = chunkMetaJson.value("file", (std::filesystem::path("chunks") / ChunkFileName(coord)).generic_string());
		chunk.loadState = EChunkLoadState::Unloaded;
		chunk.saveState = EChunkSaveState::Saved;

		m_Chunks.emplace(coord, std::move(chunk));
	}

	return S_OK;
}

HRESULT CMapManager::LoadChunk(const MAPCHUNK_COORD& coord)
{
	auto iter = m_Chunks.find(coord);
	if (iter == m_Chunks.end())
	{
		return E_FAIL;
	}

	MAPCHUNK& chunk = iter->second;
	if (chunk.loadState == EChunkLoadState::Loaded)
	{
		return S_OK;
	}

	std::filesystem::path chunkPath = std::filesystem::path(m_sMapRootPath) / chunk.filePath;
	if (chunk.filePath.empty())
	{
		chunk.filePath = (std::filesystem::path("chunks") / ChunkFileName(coord)).generic_string();
		chunkPath = std::filesystem::path(m_sMapRootPath) / chunk.filePath;
	}

	std::ifstream inFile(chunkPath.string());
	if (!inFile.is_open())
	{
		return E_FAIL;
	}

	nlohmann::ordered_json chunkJson;
	inFile >> chunkJson;
	inFile.close();

	chunk.hObjects.clear();
	chunk.loadState = EChunkLoadState::Loading;

	for (const auto& objectJson : chunkJson["objects"])
	{
		if (auto hObject = CreateMapMeshObjectFromJson(objectJson))
		{
			chunk.hObjects.push_back(hObject.value());
		}
	}

	chunk.bounds = MakeChunkBoundingBox(coord);
	chunk.loadState = EChunkLoadState::Loaded;
	chunk.saveState = EChunkSaveState::Saved;

	return S_OK;
}

HRESULT CMapManager::UnLoadChunk(const MAPCHUNK_COORD& coord)
{
	auto iter = m_Chunks.find(coord);
	if (iter == m_Chunks.end())
	{
		return E_FAIL;
	}

	MAPCHUNK& chunk = iter->second;
	if (chunk.saveState == EChunkSaveState::Unsaved)
	{
		return E_FAIL;
	}

	chunk.loadState = EChunkLoadState::Unloading;

	for (const auto& handle : chunk.hObjects)
	{
		if (auto* pObj = CGameInstance::Get().GetGameObjectByHandle(handle))
		{
			pObj->SetPendingDestroyCascade(true);
		}
	}

	chunk.hObjects.clear();
	chunk.loadState = EChunkLoadState::Unloaded;

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
	std::unordered_map<MAPCHUNK_COORD, MAPCHUNK, tagMapChunkCoordHash> prevChunks;
	prevChunks.reserve(m_Chunks.size());
	prevChunks.insert(m_Chunks.begin(), m_Chunks.end());

	m_Chunks.clear();
	m_Chunks.reserve(prevChunks.size());

	for (const auto& [coord, prevChunk] : prevChunks)
	{
		MAPCHUNK rebuiltChunk = prevChunk;
		rebuiltChunk.hObjects.clear();
		rebuiltChunk.loadState = prevChunk.loadState; //EChunkLoadState::Unloaded;
		m_Chunks.emplace(coord, std::move(rebuiltChunk));
	}

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

			auto prevIter = prevChunks.find(coord);
			const bool isKnownChunk = prevIter != prevChunks.end();

			auto& chunk = m_Chunks[coord];
			if (!isKnownChunk)
			{
				chunk.coord = coord;
				chunk.bounds = MakeChunkBoundingBox(coord);
				chunk.saveState = EChunkSaveState::Unsaved;
				chunk.filePath.clear();
			}

			chunk.hObjects.push_back(handle);
			chunk.loadState = EChunkLoadState::Loaded;
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

	const auto view = cam->GetView();
	const auto proj = cam->GetProj();

	for (const auto& [coord, mapChunk] : m_Chunks)
	{
		_float3 center = GetChunkCenter(coord);

		FXMVECTOR color = mapChunk.loadState == EChunkLoadState::Loaded ? Colors::Lime : Colors::Red;
		DrawBox(mapChunk.bounds, color, view, proj, XMMatrixIdentity());
	}

	//// 카메라 주변 3x3x3 스트리밍 대상 청크 표시
	//const auto& camPos = cam->GetTransform().GetPosition();
	//const MAPCHUNK_COORD camCoord = WorldToChunkCoord(camPos);

	//for (int64_t y = -1; y <= 1; ++y)
	//{
	//	for (int64_t z = -1; z <= 1; ++z)
	//	{
	//		for (int64_t x = -1; x <= 1; ++x)
	//		{
	//			MAPCHUNK_COORD coord
	//			{
	//				camCoord.x + x,
	//				camCoord.y + y,
	//				camCoord.z + z
	//			};

	//			BoundingBox bounds = MakeChunkBoundingBox(coord);

	//			// 빨간색 : 현재 카메라 주변 스트리밍 범위
	//			DrawBox(bounds, Colors::Red, view, proj, XMMatrixIdentity());
	//		}
	//	}
	//}

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


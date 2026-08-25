#pragma once

#include "Engine_Base.h"
#include "MapChunkSerializer.h"

NS_BEGIN(Engine)

class CMapMeshObject;
class CDecalVolume;

// 런타임 맵 오브젝트와 저장 전용 데이터 사이의 변환을 담당한다.
// 게임 오브젝트 생성과 Transform 복원까지 한곳에서 처리해 로드 경로의 중복을 막는다.
class ENGINE_DLL CMapRuntimeObjectFactory final
{
public:
	MAP_MESH_OBJECT_FILE_DATA MakeMapMeshObjectFileData(
		const CMapMeshObject& object,
		const std::string& layerName) const;
	// 스트리밍 분할 적용에서는 Transform 갱신을 다음 게임 오브젝트 갱신 시점까지 미룰 수 있다.
	std::optional<CHandle> CreateMapMeshObject(
		const MAP_MESH_OBJECT_FILE_DATA& objectData,
		_bool updateTransformImmediately = true) const;

	MAP_DECAL_FILE_DATA MakeDecalFileData(
		const CDecalVolume& decal,
		const std::string& layerName) const;
	std::optional<CHandle> CreateDecal(
		const MAP_DECAL_FILE_DATA& decalData) const;
};

NS_END

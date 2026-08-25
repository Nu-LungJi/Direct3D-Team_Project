#pragma once
#include "Engine_Base.h"
#include "MapChunk.h"
#include <nlohmann/json.hpp>

NS_BEGIN(Engine)

// MapMeshObject 하나를 파일에 저장하거나 파일에서 읽을 때 사용하는 순수 데이터다.
struct MAP_MESH_OBJECT_FILE_DATA
{
	std::string objectTag;
	std::string protoGroup;
	std::string prototype;
	std::string modelGroup;
	std::string model;
	std::string layer;

	_float3 position{};
	_float4 rotation{ 0.f, 0.f, 0.f, 1.f };
	_float3 scale{ 1.f, 1.f, 1.f };
	WIND_DESC windDesc{};
};

// 데칼의 머티리얼 파라미터 하나를 이름과 값 목록으로 보관한다.
struct MAP_DECAL_PARAMETER_DATA
{
	std::string name;
	std::vector<_float> values;
};

// 데칼 텍스처 슬롯에 덮어쓸 리소스 정보를 보관한다.
struct MAP_DECAL_TEXTURE_OVERRIDE_DATA
{
	UINT slot{};
	std::string group;
	std::string tag;
	std::string path;
};

// DecalVolume 하나를 저장하고 복원하는 데 필요한 파일 데이터다.
struct MAP_DECAL_FILE_DATA
{
	std::string objectTag = "MapDecal";
	std::string protoGroup = "PERMANENT";
	std::string prototype = "Prototype_GameObject_DecalVolume";
	std::string layer;
	std::string materialPath;
	std::string textureGroup;
	std::string textureTag;
	std::string texturePath;

	_float3 position{};
	_float4 rotation{ 0.f, 0.f, 0.f, 1.f };
	_float3 scale{ 10.f, 2.f, 10.f };
	_float opacity = 1.f;
	_float normalThreshold = 0.4f;
	_float edgeSoftness = 0.05f;

	_bool hasMaterialParameters{};
	std::vector<MAP_DECAL_PARAMETER_DATA> materialParameters;
	std::vector<MAP_DECAL_TEXTURE_OVERRIDE_DATA> textureOverrides;

	// 구버전 데칼 파일의 고정 파라미터를 읽기 위한 호환 데이터다.
	_float4 legacyAlbedo{ 1.f, 1.f, 1.f, 1.f };
	_float3 legacyEmissive{ 1.f, 0.f, 0.f };
	_float legacyEmissiveIntensity = 1.f;
};

// map.json에 기록되는 청크 하나의 메타데이터다.
struct MAP_CHUNK_METADATA
{
	MAPCHUNK_COORD coord{};
	std::string filePath;
	size_t objectCount{};
};

// map.json 전체를 런타임 객체와 분리해 표현한다.
struct MAP_FILE_DATA
{
	int version = 2;
	_float3 chunkSize{ 150.f, 150.f, 150.f };
	std::vector<MAP_CHUNK_METADATA> chunks;
	std::vector<MAP_MODEL_RESOURCE_KEY> requiredModels;
	std::vector<MAP_DECAL_FILE_DATA> decals;
};

// 청크 파일 하나의 내용을 표현한다.
struct MAP_CHUNK_FILE_DATA
{
	int version = 1;
	MAPCHUNK_COORD coord{};
	BoundingBox bounds{};
	std::vector<MAP_MESH_OBJECT_FILE_DATA> objects;
};

// 맵과 청크의 JSON 파일 형식만 담당한다.
// 게임 오브젝트 생성, 리소스 로드, 청크 상태 변경은 수행하지 않는다.
class ENGINE_DLL CMapChunkSerializer final
{
public:
	std::string MakeChunkFileName(const MAPCHUNK_COORD& coord) const;

	HRESULT SaveMapFile(const std::filesystem::path& filePath, const MAP_FILE_DATA& mapData) const;
	HRESULT LoadMapFile(const std::filesystem::path& filePath, MAP_FILE_DATA& outMapData) const;

	HRESULT SaveChunkFile(const std::filesystem::path& filePath, const MAP_CHUNK_FILE_DATA& chunkData) const;
	HRESULT LoadChunkFile(const std::filesystem::path& filePath, MAP_CHUNK_FILE_DATA& outChunkData) const;

	// 청크 파일로 분리되기 전 TestMap.json의 오브젝트 목록을 읽는다.
	HRESULT LoadLegacyMapFile(const std::filesystem::path& filePath, std::vector<MAP_MESH_OBJECT_FILE_DATA>& outObjects) const;

private:
	nlohmann::ordered_json WriteCoord(const MAPCHUNK_COORD& coord) const;
	MAPCHUNK_COORD ReadCoord(const nlohmann::ordered_json& json) const;
	nlohmann::ordered_json WriteWind(const WIND_DESC& windDesc) const;
	WIND_DESC ReadWind(const nlohmann::ordered_json& json) const;
	nlohmann::ordered_json WriteMapMeshObject(const MAP_MESH_OBJECT_FILE_DATA& object, const MAPCHUNK_COORD& coord) const;
	std::optional<MAP_MESH_OBJECT_FILE_DATA> ReadMapMeshObject(const nlohmann::ordered_json& json) const;
	nlohmann::ordered_json WriteDecal(const MAP_DECAL_FILE_DATA& decal) const;
	std::optional<MAP_DECAL_FILE_DATA> ReadDecal(const nlohmann::ordered_json& json) const;

	_float3 ReadFloat3(const nlohmann::ordered_json& json,const char* key, const _float3& fallback) const;
	_float4 ReadFloat4(const nlohmann::ordered_json& json, const char* key, const _float4& fallback) const;
};

NS_END

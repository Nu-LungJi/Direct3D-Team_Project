#pragma once
#include <iostream>
#include <fstream>
#include <filesystem>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <nlohmann/json.hpp>
#include  <any>
#include <vector>
#include <list>
#include <map>
#include <algorithm>
#include <functional>
#include <string>
#include <unordered_map>
#include <ctime>
#include <memory>
#include <wrl/client.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
using namespace DirectX;
using namespace std;

enum MATERIALTYPE {
	NONE = 0, DIFFUSE = 1, SPECULAR = 2, AMBIENT = 3, EMISSIVE = 4, HEIGHT = 5, NORMALS = 6, SHININESS = 7, OPACITY = 8, DISPLACEMENT = 9, LIGHTMAP = 10, REFLECTION = 11, BASE_COLOR = 12,
	NORMAL_CAMERA = 13, EMISSION_COLOR = 14, METALNESS = 15, DIFFUSE_ROUGHNESS = 16, AMBIENT_OCCLUSION = 17, UNKNOWN = 18, SHEEN = 19, CLEARCOAT = 20, TRANSMISSION = 21, MAYA_BASE = 22,
	MAYA_SPECULAR = 23, MAYA_SPECULAR_COLOR = 24, MAYA_SPECULAR_ROUGHNESS = 25, ANISOTROPY = 26, GLTF_METALLIC_ROUGHNESS = 27, MATERIAL_END
};

typedef struct ChunkHeader
{
	uint32_t type;
	uint32_t size;

}CHUCKHEADER;

enum ChunkType { CHUNK_MESH, CHUNK_MATERIAL, CHUNK_TEXTURE, CHUNK_BONE, CHUNK_ANIM };
struct MODEL_FILE_HEADER
{
	bool bHasBone;
	bool bHasAnimation;

	uint32_t MeshCount;
	uint32_t MaterialCount;
	uint32_t AnimationCount;
	uint32_t BoneCount;
};
struct ANIMATION_FILE_HEADER {

};





typedef struct tagTexture {
	uint32_t m_textureType;
	uint32_t m_textureNum;
	std::string File;
	std::string Ext;
}TEXTUREINFO;

typedef struct tagBone {
	std::string m_name;
	int32_t m_patrentBoneIndex;
	XMFLOAT4X4 m_TransformationMatrix;
}BONEINFO;

typedef struct tagModel
{
	bool bHasBone;
	bool bHasAnimation;

	uint32_t MeshCount;
	uint32_t MaterialCount;
	uint32_t AnimationCount;
}MODEL;

/* 애니메이션이 없는 메시용 정점. */
typedef struct tagVertexMesh
{
	XMFLOAT3	vPosition;
	XMFLOAT3	vNormal;
	XMFLOAT3	vTangent;
	XMFLOAT3	vBinormal;
	XMFLOAT2	vTexcoord;

}VTXMESH;

/* 애니메이션이 있는 메시용 정점. */
typedef struct tagVertexAnimMesh
{
	XMFLOAT3	vPosition;
	XMFLOAT3	vNormal;
	XMFLOAT3	vTangent;
	XMFLOAT3	vBinormal;
	XMFLOAT2	vTexcoord;

	XMUINT4		vBlendIndices;
	XMFLOAT4	vBlendWeights;

}VTXANIMMESH;

typedef struct tagKeyFrame {
	XMFLOAT3  vScale;
	XMFLOAT4  vRotation;
	XMFLOAT3  vTranslation;
	float	  fTrackPosition;
}KEYFRAME;

typedef struct tagChannel {
	int32_t BoneIndex;
	uint32_t KeyFrameCount;
	shared_ptr<vector<KEYFRAME>> KeyFrames;
}CHANNELDATA;

constexpr uint32_t MORPH_BINARY_MAGIC = 0x4850524D; // "MRPH"

typedef struct tagMorphVertexDelta {
	uint32_t VertexIndex{};
	XMFLOAT3 PositionDelta{};
	XMFLOAT3 NormalDelta{};
	XMFLOAT3 TangentDelta{};
	XMFLOAT3 BinormalDelta{};
} MORPHVERTEXDELTA;

typedef struct tagMorphTargetData {
	string Name;
	vector<MORPHVERTEXDELTA> Deltas;
} MORPHTARGETDATA;

typedef struct tagMorphKeyData {
	float TrackPosition{};
	vector<uint32_t> TargetIndices;
	vector<float> Weights;
} MORPHKEYDATA;

typedef struct tagMorphChannelData {
	string MeshName;
	vector<MORPHKEYDATA> Keys;
} MORPHCHANNELDATA;

typedef struct tagAnimation {
	string sAnimName;
	float AnimationDuration;
	float AnimtaionTickPerSecond;
	uint32_t ChannelCount;
	shared_ptr<vector<CHANNELDATA>> Channels;
	shared_ptr<vector<MORPHCHANNELDATA>> MorphChannels;
}ANIMATIONDATA;

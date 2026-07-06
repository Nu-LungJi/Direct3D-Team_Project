#pragma once
namespace Engine
{
	typedef struct tagVertexCol
	{
		_float3 pos;
		_float4 color;
	} VTX_COL;

	typedef struct tagVertexTex
	{
		_float3 pos;
		_float2 texCoord;
	} VTX_TEX;

	typedef struct tagVertexNormal
	{
		_float3 pos;
		_float3 normal;
	} VTX_NORMAL;

	typedef struct tagVertexNormalTex
	{
		_float3 pos;
		_float3 normal;
		_float2 texCoord;
	} VTX_NORMAL_TEX;

	typedef struct tagVertexPointParticle
	{
		_float3 pos{};   // 12 bytes
		_float2 texCoord{}; //  8 bytes
		_float2 uvSize{}; // textureSize
		_float2 size{}; //  world size
		_float rotation{};
		_float4 color{};
		uint32_t texIndex{};  //  4 bytes
		uint32_t frameIndex{};
		uint32_t light{ 0xFF };
		uint32_t flag;
	} VTX_POINT_PARTICLE;

	typedef struct tagParticle {
		_float3 position;
		_float  pad1;
		_float3 velocity;
		_float life;
		_float maxLife;
		_float size;
		uint32_t alive;
		uint32_t loop;
		_float4 color;
	}PARTICLE;
	typedef struct tagFireInstancedData
	{
		_float4x4 matWorld{};
		uint32_t   texIndexs[6]{};
		uint32_t   light{ 0xFF };
		_float4 vColor{ 1.f, 1.f, 1.f, 1.f };
	}VTX_FIRE_INSTANCED_DATA;


	/* 애니메이션이 없는 메시용 정점. */
	typedef struct tagVertexMesh
	{
		XMFLOAT3	vPosition;
		XMFLOAT3	vNormal;
		XMFLOAT3	vTangent;
		XMFLOAT3	vBinormal;
		XMFLOAT2	vTexcoord;


		static constexpr uint32_t		iNumElements = { 5 };
		static constexpr D3D11_INPUT_ELEMENT_DESC		Elements[iNumElements] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{ "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0}
		};
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


		//static constexpr uint32_t		iNumElements = { 7 };
		//static constexpr D3D11_INPUT_ELEMENT_DESC		Elements[iNumElements] = {
		//	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		//	{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		//	{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
		//	{ "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0},
		//	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0},
		//	{ "BLENDINDEX", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, 56, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		//	{ "BLENDWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 72, D3D11_INPUT_PER_VERTEX_DATA, 0 }
		//};
	}VTXANIMMESH;


}
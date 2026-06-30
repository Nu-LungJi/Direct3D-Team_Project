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
}
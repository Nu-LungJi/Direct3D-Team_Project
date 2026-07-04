#pragma once

namespace Engine
{
	typedef struct tagEngineDesc
	{
		HWND hWnd;
		HINSTANCE hInstance;
		WINMODE eWinMode;
		uint32_t iWinSizeX, iWinSizeY;
		uint32_t		iNumLevels;
	} ENGINE_DESC;


	typedef struct tagRenderContext
	{
		RENDERPASS pass;
		_vector eye{};
		_matrix matView{};
		_matrix matProj{};
		_matrix matViewProj{};
	} RENDER_CTX;

	typedef struct tagWorkerTask
	{
		_string sTaskName;
		_Func func;
	} WORKER_TASK;

	typedef struct tagMaterial
	{
		_float4 ambient{};
		_float4 diffuse{};
		_float4 specular{};
		_float4 reflect{};
	} MATERIAL;

	typedef struct tagDirectionalLight
	{
		_float4 ambient{};
		_float4 diffuse{};
		_float4 specular{};
		_float3 direction{};
		_float _pad{};
	} DIRECTIONAL_LIGHT;

	typedef struct tagPointLight
	{
		_float4 ambient{};
		_float4 diffuse{};
		_float4 specular{};
		_float3 pos{};
		_float range{};
		_float3 att{};//감쇠
		_float _pad{};
	} POINT_LIGHT;

	typedef struct tagSpotLight
	{
		_float4 ambient{};
		_float4 diffuse{};
		_float4 specular{};
		_float3 pos{};
		_float range{};
		_float3 direction{};
		_float spot{};
		_float3 att{};//감쇠
		_float _pad{};
	} SPOT_LIGHT;

	typedef struct tagPostProcess
	{
		_float DistortionIntensity;  // 왜곡 강도
		_float ChromaticIntensity;   // 색수차 강도
		_float VignetteIntensity;    // 비네팅 강도
		_float VignetteSmoothness;   // 비네팅
	} POSTPROCESS;
	typedef struct tagUiDesc
	{
		_string name;
		_float2 Pos;
		_float2 Scale;
	} UI_DESC;
	typedef struct tagKeyFrame
	{
		XMFLOAT3	vScale;
		XMFLOAT4	vRotation;
		XMFLOAT3	vTranslation;
		float		fTrackPosition;
	}KEYFRAME;

	typedef struct tagParticleSpawnData
	{
		_float3 position;
		_float3 velocity;
		_float  life;
		_float  size;
		_float4 color;
	}PARTICLE_SPAWN_DATA;

	typedef struct tagParticleEmitRequest
	{
		uint32_t count;
		_bool    bLoop;
		_float   fSpawnInterval;
	} PARTICLE_EMIT_REQUEST;

	typedef struct tagBeamVertex
	{
		_float3 vPosition;
		_float2 vUV;
	}BEAM_VERTEX;

}
#pragma once
namespace Engine
{
	typedef struct tagConstantBufferPerPass
	{
		//DIRECTIONAL_LIGHT dirLight{};
		_float4x4  matView{};            // 뷰 행렬
		_float4x4  matProj{};            // 투영 행렬 (Perspective 또는 Ortho)
		_float4x4  matViewProj{};        // 곱해진 행렬 (VS에서 연산 절약)
		_float4x4  matInvView{};			// 뷰 역행렬 (빌보드 계산이나 월드 좌표 복원용)
		_float4x4  matInvViewProj{};
		_float3 vCamPos{};
		_float4x4  matShadowLightViewProj{};
		_float3 vShadowLightDir{};
		_float2 _pad{};
	} CB_PER_PASS;
	static_assert(sizeof(CB_PER_PASS) % 16 == 0);

	typedef struct tagConstantBufferPerObject
	{
		_float4x4 matWorld{};
		_float4x4 matWVP{};
		_float4   vBaseColor{ 1.f, 1.f, 1.f, 1.f };
		uint32_t light{ 0xFF };
		_float3 _pad{};
	} CB_PER_OBJECT;
	static_assert(sizeof(CB_PER_OBJECT) % 16 == 0);

	typedef struct tagConstantBufferPerUI
	{
		_float2  texCoord{};
		_float2  uvSize{};
		_float4  color{ 1.f, 1.f, 1.f, 1.f };
		uint32_t texIndex{};
		_float2  borderUV{};
		float    _pad0{};
		_float2  borderPx{};
		_float2  rectSizePx{};
	} CB_PER_UI;
	static_assert(sizeof(CB_PER_UI) % 16 == 0);

	typedef struct tagConstantBufferObjectPBR
	{
		_float3  AlbedoValue;
		_float   RoughnessValue;
		_float   MetallicValue;
		_float3  Padding;
	} CB_OBJECT_PBR;
	static_assert(sizeof(CB_OBJECT_PBR) % 16 == 0);

	typedef struct tagConstantBufferLight
	{
		DYNAMIC_LIGHT AffectedLight[MAX_LIGHT_COUNT];
		int           g_iLightCount;
		XMFLOAT3      g_LightPadding;
	} CB_LIGHT;
	static_assert(sizeof(CB_LIGHT) % 16 == 0);

	typedef struct tagInitParticle
	{
		uint32_t g_iMaxParticles;
		_float3 pad;
	}CB_INIT_PARTICLE;
	static_assert(sizeof(CB_INIT_PARTICLE) % 16 == 0);


	typedef struct CB_ParticleUpdate
	{
		float    g_fTimeDelta;
		uint32_t g_iNumInstances;
		uint32_t g_iBehaviorType;
		float    g_fPadding;
	}CB_PER_PARTICLE;;
	static_assert(sizeof(CB_PER_PARTICLE) % 16 == 0);

	typedef struct CB_ParticleSpawn
	{
		uint32_t    g_iSpawnCount;
		uint32_t	g_iMaxParticles;
		_float2     pad;
		PARTICLE_SPAWN_DATA  g_SpawnData[MAX_SPAWN_PER_CALL];
	}CB_PARTICLE_SPAWN;
	static_assert(sizeof(CB_PARTICLE_SPAWN) % 16 == 0);

	typedef struct CB_RibbonParticle
	{
		uint32_t    g_iSpawnCount;
		_float3     pad;
		PARTICLE_SPAWN_DATA  g_SpawnData[MAX_SPAWN_PER_CALL];
	}CB_RIBBON_PARTICLE;
	static_assert(sizeof(CB_RIBBON_PARTICLE) % 16 == 0);

}
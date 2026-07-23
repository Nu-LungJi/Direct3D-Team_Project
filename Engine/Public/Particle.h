#pragma once
#include "Engine_Defines.h"
#include "GameObject.h"
#include "ComStaticModelInstance.h"
NS_BEGIN(Engine)
class ENGINE_DLL CParticle : public CEngineBase
{
public:
	DECLARE_DERIVED_TYPE(CParticle, CEngineBase)

	enum PARTICLE_BEHAVIOR : uint32_t
	{
		BEHAVIOR_NONE = 0,
		BEHAVIOR_DISTORTION = 1 << 1, 
		BEHAVIOR_BILLBOARD = 1 << 2,
		BEHAVIOR_GRAVITY = 1 << 3,
		BEHAVIOR_CIRCLE_TO_WAVE = 1 << 4,
		BEHAVIOR_SMOKE = 1 << 5,
		BEHAVIOR_SMOKEJUMP = 1<<6,
	};

protected:
	explicit CParticle();
	virtual ~CParticle();
public:
	virtual HRESULT Initialize(void* pArg) = 0;
	virtual void PriorityUpdate(E::_float fTimeDelta) = 0;
	virtual void Update(E::_float fTimeDelta) = 0;
	virtual void LateUpdate(E::_float fTimeDelta) = 0;
	virtual HRESULT Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) = 0;
	virtual HRESULT Spawn(uint32_t count, const PARTICLE_SPAWN_DATA* pSpawnData) = 0;
	virtual void ClearByOwner(uint32_t ownerID) = 0;
	virtual void TranslateOwner(uint32_t ownerId, const _float3& delta);
	virtual void TransformOwner(uint32_t ownerId, const _float4x4& deltaMatrixData);
public:
	virtual void SetPosition(const _float3& pos) {}
	virtual void SetVelocity(const _float3& vel) {}
	virtual void SetSize(const _float3& size) {}
	virtual void SetColor(const _float4& color) {}

	void RequestSpawn(const std::vector<PARTICLE_SPAWN_DATA>& spawnList);
	HRESULT Set_BlendState(BLENDTYPE blendNum);
	uint32_t Get_BlendState() { return m_iBlendIndex; }

public:
	PARTICLE_TYPE& Get_Type() { return m_eType; }
	HRESULT LoadParticleTexture(std::pair<StringID, StringID> textureId);

protected:
	void ProcessPendingSpawns(E::_float fTimeDelta);

protected:
	PARTICLE_TYPE m_eType;
	SPtr<class CResPixelShader> m_pResPixelShader{};
	SPtr<class CResVertexShader> m_pResVertexShader{};
	UPtr<class CComStaticModelInstance> m_pComModelInstance{};
	SPtr<class CResTexture2D> m_pParticleTexture;
	SPtr<class CResTexture2D> m_pNormalTexture;
	SPtr<class CResTexture2D> m_pDistortionTexture;
	SPtr<class CResTexture2D> m_pNoiseTexture;
	SPtr<class CResTexture2D> m_pHdrPositionTexture;
	SPtr<class CResTexture2D> m_pHdrNormalTexture;
	SPtr<class CResTexture2D> m_pAnyTexture;
	SPtr<class CResBlendState> m_pBlendState;

private:

	struct PENDING_SPAWN
	{
		PARTICLE_SPAWN_DATA data;
		E::_float remainingDelay;
	};
	std::vector<PENDING_SPAWN> m_PendingSpawns;
	
	uint32_t m_iBlendIndex = 0;
};
NS_END

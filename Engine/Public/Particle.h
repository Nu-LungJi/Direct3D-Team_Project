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
		BEHAVIOR_SHRINK = 1 << 0, // 시간에 따라 크기 축소
		// BEHAVIOR_FADE = 1 << 1, // 나중에 다른 행동 추가 시 이런 식으로 확장
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
public:

	void RequestSpawn(const std::vector<PARTICLE_SPAWN_DATA>& spawnList);

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
	SPtr<class CResTexture2D> m_pNoiseTexture;

private:

	struct PENDING_SPAWN
	{
		PARTICLE_SPAWN_DATA data;
		E::_float remainingDelay;
	};
	std::vector<PENDING_SPAWN> m_PendingSpawns;
};
NS_END

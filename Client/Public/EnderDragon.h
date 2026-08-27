#pragma once
#include "Monster.h"
#include "Client_Defines.h"
enum class DRAGON_SKILL{BOOM,BREATH,FIREBALL,PULSE,RANDOMBALL, TURNBREATH, THREEBALL, BLACKBALL,LONGBREATH,GASI,GASIBREATH,GROUND, END};
enum class DRAGON_PHASE{PHASE1, PHASE2, PHASE3, PHASE4, PHASE5, PHASE6, PHASE7, END};
enum class EDG_SPAWN_NUMBER { FIRST, SECOND, THIRD, FOUR };

// 투명 드래곤이 울부 짖었다
typedef struct stredganimfsm
{
	int32_t iAnimIndex{};
	_float	fBlend{};
}EDG_ANIM_FSM;
typedef struct stractiveskilltable
{
	_string		  SkillName{};
	_float		  fLifeTime{}, fDist{};
	int32_t		  iBoneOffset{};
	DRAGON_SKILL eType{};
}EDG_ACSKT_DESC;

NS_BEGIN(Client)
typedef struct stredgskillInfo
{
	CHandle handle{};
	_bool	bPool;
	int32_t iBoneIndex{};
	_string LevelTag{};
	PROTO_GAMEOBJECT ProtoTag;
	_string NameTag{};
	int32_t iOffsetBoneIndex{-1};
	DRAGON_SKILL eType{DRAGON_SKILL::END};

}EDG_SKILL_INFO;
class CEnderDragon final : public CMonster
{
public:
	DECLARE_DERIVED_TYPE(CEnderDragon, CMonster)

public:
	typedef struct tagDragonDesc : public CMonster::MONSTER_DESC
	{

	}DRAGON_DESC;

private:
	CEnderDragon();
	~CEnderDragon() override;

public:
	void UpdateGUI() override;
public:
	HRESULT						InitializePrototype(void* pArg = nullptr) override;
	HRESULT						Initialize(void* pArg) override;
	void						PriorityUpdate(E::_float fTimeDelta) override;
	void						FixedUpdate(E::_float fTimeDelta) override;
	void						Update(E::_float fTimeDelta) override;
	void						LateUpdate(E::_float fTimeDelta) override;
	HRESULT						Ready_Fsm(const _string& LevelTag);
	HRESULT						Ready_Skill(const _string& LevelTag);
	void						Ready_BBKeyValue();
	void						ReadySound();
	/*----------- 광윤 추가 -----------*/ // MaskMap Test
	HRESULT						Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) override;
	HRESULT						Render_Instanced(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx, const E::MODEL_INSTANCE_BATCH& Batch) override;

private:
	HRESULT						Render_WingFXForward(ID3D11DeviceContext* pContext, const E::MODEL_INSTANCE_BATCH& Batch);
	HRESULT						Prepare_DragonInstancing(ID3D11DeviceContext* pContext, const E::MODEL_INSTANCE_BATCH& Batch, SPtr<E::CResModel>& pOutModel, uint32_t& iOutInstanceCount);
	HRESULT						Bind_SkinnedMeshConstantBuffer(ID3D11DeviceContext* pContext, SPtr<E::CResModel>& pModel, SPtr<CResModelMesh>& pMesh, uint32_t iMeshIndex);
private:
	const E::MODEL_INSTANCE_BATCH* m_pPendingWingFXBatch = nullptr;
	_bool m_bWingFXQueued = false;

private:
	SPtr<CResTexture2D>		m_pBodyMaskTexture{};
	SPtr<CResTexture2D>		m_pWingsMaskTexture{};

	SPtr<CResTexture2D>		m_pBodyMROTexture{};
	SPtr<CResTexture2D>		m_pWingsMROTexture{};

	SPtr<CResTexture2D>		m_pEtherealWingsTexture{};

	SPtr<CResTexture2D>		m_pMarbleNoiseTexture{};
	SPtr<CResTexture2D> 	m_pRiverNoiseTexture{};
	SPtr<CResTexture2D> 	m_pCausticNoiseTexture{};
	SPtr<CResTexture2D> 	m_pDetailNoiseTexture{};

	SPtr<CResPixelShader>	m_pResDragonEyePixelShader{};
	SPtr<CResPixelShader>	m_pResDragonBodyPixelShader{};
	SPtr<CResPixelShader>	m_pResDragonWingPixelShader{};
	SPtr<CResPixelShader>	m_pResDragonWingFXPixelShader{};

	SPtr<CResDepthStencilState>	m_pResWingFXDSS{};
	SPtr<CResRasterizerState>	m_pResWingFXRasterizer{};

	CComModelInstance*		m_pComOutlineModelInstance{};
	/*---------------------------------*/

public:
	_string						Get_SkillName(ATTMON SkillNode)override;
	
	void						Set_StateFinished(_bool bFinished);
	void						Set_Break(_bool bHit) { m_bIsBreak = bHit; }

	_bool						Is_StateFinished();
	
	_bool						Check_Table(PLAYER_SKILL_TYPE eType) override;
	void						Check_Phase();
	void						Set_AttTable(ATTMON eType, _float2 fSkillRatio) override;
	void						Set_Dissolve(_float fDissolve) { m_fDissolve = fDissolve; }
	void						Set_WingParticlesEnabled(_bool bEnabled) { m_bWingParticlesEnabled = bEnabled; if (!bEnabled) m_fWingParticleSpawnAcc = 0.f; }
	EDG_SKILL_INFO&				Get_SkillInfo(DRAGON_SKILL eType) { return m_SkillHandle[ETOUI(eType)]; }
	const _string&				Get_SkillNmae(DRAGON_SKILL eType) { return m_EffectNames[ETOUI(eType)]; }
	void						Heal(uint32_t iHp) { m_iHp += iHp; if (m_iHp >= m_iMaxHp) m_iHp = m_iMaxHp; }
	void						Set_EndGame() { m_bEndGame = true; }
private:
	void						Update_BBToFsm();
	void						Flag_Check(_float fTimeDelta) override;
	_bool						BreakSkillType(PLAYER_SKILL_TYPE eType);
	
	void						Phase_Debug();
	void						Picking(_float3& vPos,uint32_t iID);

	void						InitializeEffects();
	void						Update_EnvironmentParticles(_float fTimeDelta);
	void						Spawn_EnvironmentParticles(uint32_t iParticleIndex, uint32_t iCount);
	void						Update_WingParticles(_float fTimeDelta);
	void						Spawn_WingParticle(int32_t iBoneIndex);

	void						Stuck() override;
private:
	class CEnderDragon_State* m_pFsm{ nullptr };
	
	_string			m_EffectNames[ETOUI(DRAGON_SKILL::END)]{};
	EDG_SKILL_INFO	m_SkillHandle[ETOUI(DRAGON_SKILL::END)]{};
	DRAGON_SKILL	m_eDragonSkill{};
	DRAGON_PHASE	m_ePhase{};
	_bool			m_bIsBreak{ false }, m_bActiveSKill{ false }, m_bPopup{ false }, m_bPopupL{ false }, m_bEndGame{ false };

	_string						m_WayName{};
	std::list<_float3>			m_DebugPoint;
	std::array<_bool, ETOUI(DRAGON_PHASE::END)>	m_bPhaseLock{ false };
	_float						m_fBlobEnvSpawnAcc{};
	_float						m_fSwirlEnvSpawnAcc{};
	_float						m_fBlobEnvSpawnInterval{ 0.25f };
	_float						m_fSwirlEnvSpawnInterval{ 0.6f };
	int32_t						m_iLeft1WingParticleBoneIndex{ -1 };
	int32_t						m_iRight1WingParticleBoneIndex{ -1 };
	int32_t						m_iLeft2WingParticleBoneIndex{ -1 };
	int32_t						m_iRight2WingParticleBoneIndex{ -1 };
	_float						m_fWingParticleSpawnAcc{};
	_float						m_fWingParticleSpawnInterval{ 0.1f };
	_bool						m_bWingParticlesEnabled{ true };
	uint32_t					m_iDefaultEffectID{};
public:
	static E::UPtr<CEnderDragon> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END

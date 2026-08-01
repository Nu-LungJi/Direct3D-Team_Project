#pragma once
#include "AnimationObject.h"
#include "Client_Defines.h"
NS_BEGIN(Engine)
class CComConstantBuffer;
class CResTexture2D;
class CResVertexShader;
class CResPixelShader;
class CResSamplerState;
class CResModel;
class CComModelInstance;
class CComAnimator;
class CComBeHavior;
class CComCollider;
class CComPxCharacterController;
class CComCharacterMoveIntent;
class CComCharacterMotor;
class CComPxRigidBody;
class CComPxSphereCollider;
NS_END


NS_BEGIN(Client)
typedef struct HitTable
{
	_bool operator == (const HitTable& rhs) const
	{
		return (eAttType == rhs.eAttType) && (eHitType == rhs.eHitType);
	}

	ATTMON						eAttType{ ATTMON::END };
	PLAYER_SKILL_TYPE			eHitType{ PLAYER_SKILL_TYPE::DEFAULT };
	int32_t						iAnimIndex{ -1 };
	_float						fBlend{ 0.1f };

}HITTABLE;

typedef struct MonsterHitInfo
{
	PLAYER_SKILL_TYPE eHitType{ PLAYER_SKILL_TYPE::DEFAULT};
	ATTMON    eAttType{ ATTMON::END };
	int32_t iPriority{ 0 };
}MON_HIT_INFO;
class CMonster : public CAnimationObject
{
public:
	DECLARE_DERIVED_TYPE(CMonster, CAnimationObject)
public:
	typedef struct tagGoblnedesc : CAnimationObject::GAMEOBJECT_DESC
	{
		_string SocketName{}, LevelTag{}, ReSourceTag{}, BeHaviorTag{};
		_bool	bDonMove{ false };
		_float3 vPos{}, vScale{ 1.f,1.f,1.f }, vRot{1.f,1.f,1.f};
		_float fAngle{};

		_float3 vWeaponScale{ 1.f,1.f,1.f };
		_string resBeHaviorMajor{}, resBeHaviorMinor{};
		_string WeaponResourceName{};
		_string WeaponProtoName{};
		MONSTER_TYPE				MonType{MONSTER_TYPE::BOSS};
		CHandle						TargetHandle{};
		PX_FILTER_DESC tFilter{
			.iLayer = ETOUI(COLLISION_LAYER::ENEMY_BODY),
			.iSimulationMask = PX_ALL_LAYERS,
			.iQueryMask = PX_ALL_LAYERS
		};


	}MONSTER_DESC;
protected:
	CMonster();
	~CMonster() override;

public:
	void UpdateGUI();
	HRESULT InitializePrototype(void* pArg);
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;
	HRESULT Render_Instanced(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx, const E::MODEL_INSTANCE_BATCH& Batch);
	HRESULT Update_InstanceBuffer(ID3D11DeviceContext* pContext, const std::vector<GPU_ANIM_INSTANCE_DATA>& Instances);
	HRESULT Bind_InstanceBuffer(ID3D11DeviceContext* pContext);

	/*----------- 광윤 추가 -----------*/
	HRESULT Render_Shadow(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) override;
	/*---------------------------------*/
public:
	void OnTriggerEnter(CGameObject* pObj,const PX_ON_TRIGGER_DATA& info) override;
public:
	void Set_Partes(PARTES eType, CHandle Handle) { m_Partes[ETOUI(eType)] = Handle; };
	const int32_t				Get_CurrentHp() const { return m_iHp; }
	const int32_t				Get_MaxHp()		const { return m_iMaxHp; }
	void						Set_Damage(int32_t iDamage) { m_iHp -= iDamage; }
	void						Set_Emissive(_float fEmissive) { m_fPreEmissive = m_fIntensive = fEmissive; }
	_bool						Activate_PendingHit();
	const MON_HIT_INFO			Get_ActiveHitInfo()const { return m_ActiveMonTable; }
	const MON_HIT_INFO			Get_PendingHitInfo() const { return m_PendingMonTable; }
	_bool						Is_PendingHit() { return m_bPending; }
	_bool						Is_ActiveHit() { return m_bActiveHit; }
	void						ReActiveTable();
	_bool						Check_Table(PLAYER_SKILL_TYPE eType);
	_bool						Is_Grounded();
	_bool						Monster_Type(MONSTER_TYPE eType) { if (m_eMonType == eType)return true;  return false; }
	uint32_t					GetHitCnt() { return m_iHitCnt; }
	uint32_t					GetNormalCnt() {return m_iNormalHitCnt;}
	CGameObject*				Get_Target() { return CGameInstance::Get().GetGameObjectByHandle(m_TargetHandle); }

	virtual _string				Get_SkillName(ATTMON SkillNode) { return ""; };
	virtual void				Set_AttTable(ATTMON eType, _float2 fSkillRatio) {};
protected:
	uint32_t					Find_SkillNum(ATTMON eType);
	_bool						Check_Flag(uint32_t iFlag);
private:
	void						Damaged(PLAYER_SKILL_TYPE eType);
	void						RunningSkill(_float fTimeDelta);
	void						Flag_Check(_float fTimeDelta);
	void						StartEmissive() { if (m_bWork) return;  m_bEmissive = true; }
	void						EmissiveFadeOut(_float fTimeDelta);
protected:
	CComModelInstance* m_pComModelInstance{};
	CComAnimator* m_pModelAnimator{};
	CComBeHavior* m_pBeHavior;
	CComCollider* m_pComCollider{};
	CComPxCharacterController* m_pCharacterController{};
	CComCharacterMoveIntent* m_pMoveIntent{};
	CComCharacterMotor* m_pCharacterMotor{};
	CComPxRigidBody* m_pComRigidBody{};
	CComPxSphereCollider* m_pComSphereCol{};

	CHandle m_Partes[ETOUI(PARTES::END)]{};

	
	// Anim
	SPtr<CResPixelShader> m_pResPixelShader{};
	SPtr<CResVertexShader> m_pResVertexCPUSkinningInstancedShader{};
	SPtr<CResCBuffer> m_pResSkinMeshCBuffer{};



	CComConstantBuffer* m_pComCBufferPerObject{};
	_float3 m_fEMissiveColor{};
	_float ff{};

	_float2								m_fSkillRatio{ };
	uint32_t							m_iCurrentInstanceCount{}, m_iHitCnt{}, m_iNormalHitCnt{}, m_iCurEffectID{};
	_float								m_fIntensive{}, m_fPreEmissive{}, m_fAlpha{}, m_fTimeTick{};
	int32_t								m_iHp{}, m_iMaxHp{};
	_bool								m_bEmissive{ false }, m_bWork{ false },m_bSkillLoop{ false }, m_bSkipAtt{false};
	_string								m_SocketName{}, m_CurEffectName{};
	ATTMON								m_eAttType{};

	_bool								m_bPending{ false };
	MON_HIT_INFO						m_PendingMonTable{};

	_bool								m_bActiveHit{ false };
	MON_HIT_INFO						m_ActiveMonTable{};

	MONSTER_TYPE						m_eMonType{ MONSTER_TYPE::NORMAL };
	std::vector<E::SPAWN_COMMAND>		m_Effects[ETOUI(ATTMON::END)];
	CHandle								m_TargetHandle{};

	std::map<ATTMON, uint32_t>			m_MonSkillLists;
	//파티클 재설정용
	_bool								m_bDonMove{ false };
	std::map<ATTMON, _string>			m_ParticleData;
public:
	E::UPtr<E::CPrototype> Clone(void* pArg) PURE;
};

NS_END



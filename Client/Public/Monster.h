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
NS_END



NS_BEGIN(Client)
typedef struct HitTable
{
	_bool operator == (const HitTable& rhs) const
	{
		return (eAttType == rhs.eAttType) && (eHitType == rhs.eHitType);
	}

	ATTMON			eAttType{ ATTMON::END };
	HITMON			eHitType{ HITMON::END };
}HITTABLE;
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

public:
	void Set_Partes(PARTES eType, CHandle Handle) { m_Partes[ETOUI(eType)] = Handle; };
	const int32_t			Get_CurrentHp() const { return m_iHp; }
	const int32_t			Get_MaxHp()		const { return m_iMaxHp; }
	void					Set_Damage(int32_t iDamage) { m_iHp -= iDamage; }
	void					Set_Emissive(_float fEmissive) { m_fEmissive = fEmissive; }
	void					Set_AttTable(ATTMON eType, _float2 fSkillRatio)
	{
		if (!m_bSkill) {
			m_MonTable.eAttType = eType;
			m_fSkillRatio = fSkillRatio;
			m_bSkill = true;
		}
	}
	const HITTABLE			Get_HitTable()const { return m_MonTable; }
private:
	void					RunningSkill(_float fTimeDelta);
	void					IsHit();
	void					Flag_Check(_float fTimeDelta);
	void					StartEmissive() { if (m_bWork) return;  m_fPreEmissive = m_fEmissive; m_bEmissive = true; }
	void					EmissiveFadeOut(_float fTimeDelta);
protected:
	CComModelInstance* m_pComModelInstance{};
	CComAnimator* m_pModelAnimator{};
	CComBeHavior* m_pBeHavior;
	CComCollider* m_pComCollider{};
	CComPxCharacterController* m_pCharacterController{};
	CComCharacterMoveIntent* m_pMoveIntent{};
	CComCharacterMotor* m_pCharacterMotor{};
	CHandle m_Partes[ETOUI(PARTES::END)]{};


	// Anim
	SPtr<CResPixelShader> m_pResPixelShader{};
	SPtr<CResVertexShader> m_pResVertexCPUSkinningInstancedShader{};
	SPtr<CResCBuffer> m_pResSkinMeshCBuffer{};



	CComConstantBuffer* m_pComCBufferPerObject{};
	_float3 m_f{};
	_float ff{};

	_float2						m_fSkillRatio{ };
	uint32_t					m_iCurrentInstanceCount = 0;
	_float						m_fEmissive{}, m_fPreEmissive{}, m_fAlpha{}, m_fTimeTick{};
	int32_t						m_iHp{}, m_iMaxHp{};
	_bool						m_bDead{ false }, m_bEmissive{ false }, m_bWork{ false }, m_bSkill{ false };
	_string						m_SocketName{};
	HITTABLE					m_MonTable{};

	
	std::vector<E::SPAWN_COMMAND> m_Effects[ETOUI(ATTMON::END)];


	//파티클 재설정용
	_bool								m_bDonMove{ false };
	std::map<ATTMON, _string>			m_ParticleData;
public:
	E::UPtr<E::CPrototype> Clone(void* pArg) PURE;
};

NS_END



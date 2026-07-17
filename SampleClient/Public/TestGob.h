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
NS_END

NS_BEGIN(Client)

enum class HITMON { HIT_1, HIT_2, HIT_3, HIT_4, END };
class CTestGob final : public CAnimationObject
{
public:
	DECLARE_DERIVED_TYPE(CTestGob, CAnimationObject)

public:
	typedef struct tagMonsterDesc : public CGameObject::GAMEOBJECT_DESC
	{
		_string SocketName{}, LevelTag{}, ReSourceTag{}, BeHaviorTag{};
		_float3 vPos{};
	}MONSTER_DESC;

private:
	CTestGob();
	~CTestGob() override;

public:
	void UpdateGUI() override;
	void Set_Partes(PARTES eType, CHandle Handle) { m_Partes[ETOUI(eType)] = Handle; };
public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) override;
	HRESULT Render_Instanced(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx, const E::MODEL_INSTANCE_BATCH& Batch) override;
	HRESULT Update_InstanceBuffer(ID3D11DeviceContext* pContext, const std::vector<GPU_ANIM_INSTANCE_DATA>& Instances);

	HRESULT Bind_InstanceBuffer_CS(ID3D11DeviceContext* pContext);
	HRESULT Bind_FinalBoneUAV_CS(ID3D11DeviceContext* pContext);

	HRESULT Unbind_AnimationCompute(ID3D11DeviceContext* pContext);

	HRESULT Bind_InstanceBuffer_VS(ID3D11DeviceContext* pContext);

	HRESULT Bind_FinalBoneSRV_VS(ID3D11DeviceContext* pContext);

	HRESULT Unbind_AnimationVS(ID3D11DeviceContext* pContext);

public:	
	const int32_t			Get_CurrentHp() { return m_iHp; }
	const int32_t			Get_MaxHp()	  { return m_iMaxHp; }
	void					Set_Damage(int32_t iDamage) { m_iHp -= iDamage; }
	void					Set_Emissive(_float fEmissive) { m_fEmissive = fEmissive; }
	const HITMON			Get_HitMon() const { return m_eHitType; }
	_bool					Check_HitCnt(int32_t iHit) { if (iHit >= m_iHitCnt)return true;   return false; }
	void					ResetHitcnt() { m_iHitCnt = 0; }
private:			
	void					IsHit();
	void					Flag_Check(_float fTimeDelta);
	void					StartEmissive() { if (m_bWork) return;  m_fPreEmissive = m_fEmissive; m_bEmissive = true; }
	void					EmissiveFadeOut(_float fTimeDelta);
	
private:
	CComModelInstance* m_pComModelInstance{};
	CComAnimator* m_pModelAnimator{};
	CComBeHavior* m_pBeHavior;
	CComCollider* m_pComCollider{};
	CHandle m_Partes[ETOUI(PARTES::END)]{};


	// Anim
	SPtr<CResPixelShader> m_pResPixelShader{};
	SPtr<CResVertexShader> m_pResVertexShader{};
	SPtr<CResVertexShader> m_pResVertexInstancedShader{};
	SPtr<CResCBuffer> m_pResSkinMeshCBuffer{};


	SPtr<CResComputeShader> m_pAnimComputeShader{};


	CComConstantBuffer* m_pComCBufferPerObject{};

	_float4 m_fAlbedoColor = { 1.f, 1.f, 1.f, 1.f };
	_float	m_fNormalIntensity = 1.f;
	_float	m_fRoughnessIntensity = 1.f;
	_float	m_fMetallicIntensity = 1.f;
	_float	m_fAmbientIntensity = 1.f;
	_float	m_fSpecularIntensity = 1.f;
	_float3 m_fEmissiveColor = { 1.f, 1.f, 0.f };
	_float	m_fEmissiveIntensity = 0.f;

	uint32_t m_iCurrentInstanceCount = 0.f;
	HITMON						m_eHitType{ HITMON::END };
	_float						m_fEmissive{}, m_fPreEmissive{}, m_fAlpha{}, m_fTimeTick{};
	int32_t						m_iHp{}, m_iMaxHp{}, m_iHitCnt{};
	_bool						m_bDead{ false }, m_bEmissive{ false }, m_bWork{false};
	_string						m_SocketName{};
public:
	static E::UPtr<CTestGob> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END

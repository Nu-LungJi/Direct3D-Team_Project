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
class CResCBuffer;
class CComModelInstance;
class CComAnimator;
class CResComputeShader;

// 물리
class CComPxCharacterController;
class CComLocomotion;
class CComCharacterMotor;
NS_END

NS_BEGIN(Client)
class CPlayer_StateMachine;
class CPlayer final : public CAnimationObject
{
public:
	DECLARE_DERIVED_TYPE(CPlayer, CAnimationObject)

public:
	typedef struct tagPlayerDesc : public CGameObject::GAMEOBJECT_DESC
	{
		StringID sGroupTag;
		StringID sResTag;

		_float3 vInitialPosition{ 50.f, 50.f, 10.f };
		PX_FILTER_DESC tFilter{
			.iLayer = ETOUI(COLLISION_LAYER::PLAYER_BODY),
			.iSimulationMask = PX_ALL_LAYERS,
			.iQueryMask = PX_ALL_LAYERS
		};

	}DESC;	

private:
	CPlayer();
	CPlayer(const CPlayer& rhs);
	~CPlayer() override;

public:
	void UpdateGUI() override;
public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;

	void FixedUpdate(_float fTimeDelta) override;

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
	int32_t FindAnimationIndex(_string_view sAnimationName) const;


public:

	CComAnimator* GetAnimator() const { return m_pModelAnimator; }
	CComLocomotion* GetLocomotion() const { return m_pLocomotion; }

private:
	CComModelInstance* m_pComModelInstance{};
	CComAnimator* m_pModelAnimator{};

	// Anim
	SPtr<CResPixelShader> m_pResPixelShader{};
	SPtr<CResVertexShader> m_pResVertexShader{};
	SPtr<CResVertexShader> m_pResVertexInstancedShader{};
	SPtr<CResCBuffer> m_pResSkinMeshCBuffer{};
	CHandle m_Partes[ETOUI(PARTES::END)]{};

	SPtr<CResComputeShader> m_pAnimComputeShader{};


	CComConstantBuffer* m_pComCBufferPerObject{};

	_float4 m_fAlbedoColor = { 1.f, 1.f, 1.f, 1.f };
	_float	m_fNormalIntensity = 1.f;
	_float	m_fRoughnessIntensity = 1.f;
	_float	m_fMetallicIntensity = 1.f;
	_float	m_fAmbientIntensity = 1.f;
	_float	m_fSpecularIntensity = 1.f;
	_float3 m_fEmissiveColor = { 1.f, 1.f, 1.f };
	_float	m_fEmissiveIntensity = 0.f;

	uint32_t m_iCurrentInstanceCount = 0.f;

private:
	_bool   m_bStateInitailzie = false;
private:
	CComPxCharacterController* m_pCharacterController{};
	CComLocomotion* m_pLocomotion{};
	CComCharacterMotor* m_pCharacterMotor{};
	CPlayer_StateMachine* m_pStateMachine{};

public:
	static E::UPtr<CPlayer> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END

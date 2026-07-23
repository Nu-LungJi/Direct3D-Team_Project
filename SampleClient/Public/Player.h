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
class CComCharacterMoveIntent;
class CComCharacterMotor;
NS_END

NS_BEGIN(Client)
class CPlayer_StateMachine;
class CPlayer final : public CAnimationObject
{
public:
	enum class LOCOMOTION_MODE : uint32_t
	{
		FREE,
		HOVER,
	};

	enum class LOCOMOTION_GAIT : uint32_t
	{
		IDLE,
		WALK,
		JOG,
		SPRINT,
	};

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
	CComCharacterMoveIntent* GetMoveIntent() const { return m_pMoveIntent; }
	CComCharacterMotor* GetCharacterMotor() const { return m_pCharacterMotor; }
	LOCOMOTION_MODE GetLocomotionMode() const { return m_eLocomotionMode; }
	LOCOMOTION_GAIT GetDesiredGait() const { return m_eDesiredGait; }
	_bool HasMoveInput() const { return m_bMoveInput; }
	const _float3& GetDesiredMoveDirection() const { return m_vDesiredMoveDirection; }
	const _float3& GetCameraFacingDirection() const { return m_vCameraFacingDirection; }
	_bool IsRootMotionRotationActive() const { return m_bRootMotionRotationActive; }
	void SetRootMotionRotationActive(_bool bActive) { m_bRootMotionRotationActive = bActive; }
	void SetLocomotionAngleDebug(_float fForward, _float fRight, _float fAngle, _float fSpeed, _string_view sDirection);
	void ClearLocomotionAngleDebug();

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
	CComCharacterMoveIntent* m_pMoveIntent{};
	CComCharacterMotor* m_pCharacterMotor{};
	CPlayer_StateMachine* m_pStateMachine{};

	_bool m_bHasLocomotionAngleDebug = false;
	_float m_fLocomotionForward = 0.f;
	_float m_fLocomotionRight = 0.f;
	_float m_fLocomotionAngle = 0.f;
	_float m_fLocomotionSpeed = 0.f;
	_string m_sLocomotionDirection;

private:
	_float m_fCurrentMoveSpeed = 0.f;
	_float m_fWalkSpeed = 2.f;
	_float m_fJogSpeed = 5.f;
	_float m_fSprintSpeed = 8.5f;
	_float m_fAcceleration = 12.f;
	_float m_fDeceleration = 18.f;
	_float3 m_vLastMoveDirection{};
	_float3 m_vDesiredMoveDirection{};
	_float3 m_vCameraFacingDirection{ 0.f, 0.f, 1.f };
	_bool m_bMoveInput = false;
	_bool m_bRootMotionRotationActive = false;
	LOCOMOTION_MODE m_eLocomotionMode = LOCOMOTION_MODE::FREE;
	LOCOMOTION_GAIT m_eDesiredGait = LOCOMOTION_GAIT::IDLE;



public:
	static E::UPtr<CPlayer> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END

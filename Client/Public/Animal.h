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
class CComSound;
class CBTBlackBoard;
NS_END


NS_BEGIN(Client)

class CAnimal : public CAnimationObject
{
public:
	DECLARE_DERIVED_TYPE(CAnimal, CAnimationObject)
public:
	typedef struct tagAnimaldesc : CAnimationObject::GAMEOBJECT_DESC
	{
		_string LevelTag{}, ReSourceTag{}, BeHaviorTag{};
		_bool	bDonMove{ false };
		_float3 vPos{}, vScale{ 1.f,1.f,1.f }, vRot{ 1.f,1.f,1.f };
		_float fAngle{};
		// 모델 로컬 크기이며 생성 시 vScale을 적용해 CCT 월드 크기로 변환한다.
		_float fCCTHeight{ 2.1f };
		_float fCCTRadius{ 0.45f };
		_float fCCTStepOffset{ 0.4f };
		_float3 vCCTCenterOffset{ 0.f, 1.5f, 0.f };

		_string resBeHaviorMajor{}, resBeHaviorMinor{};
		_float3 vStartPos{}, vEndPos{};
		CHandle						TargetHandle{};
		PX_FILTER_DESC tFilter{
			.iLayer = ETOUI(COLLISION_LAYER::NPC_BODY),
			.iSimulationMask = PX_ALL_LAYERS,
			// [LSY] 캐릭터 CCT끼리는 충돌하되 전투용 HurtBox는 이동 Query에서 제외한다.
			.iQueryMask =
				ETOUI(COLLISION_LAYER::WORLD_DYNAMIC)
		};


	}ANIMAL_DESC;
protected:
	CAnimal();
	~CAnimal() override;

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
	void OnTriggerEnter(CGameObject* pObj, const PX_ON_TRIGGER_DATA& info) override;
public:
	void						Get_SoundKey(_string& CursoundName);
	const _float4x4*			Get_CombineBoneMatrix(int32_t iBoneIndex);
	CComAnimator*				Get_Animator();
	CComCharacterMoveIntent*	Get_MoveIntent();
	void						SetRootMotionActive(_bool bActive) { m_bRootMotionTranslationActive = bActive; }
	void						SetRootMotionRotationActive(_bool bActive) { m_bRootMotionRotationActive = bActive; }
	void						SetRootMotionTranslationScale(_float fScale) { m_fRootMotionTranslationScale = std::max(0.f, fScale); }
	CBTBlackBoard*				Get_BlackBoard();
	int32_t						Find_AnimIndex(const _string& AnimName);

private:
	void						Update_Animation(_float fTimeDelta);

protected:
	CComModelInstance* m_pComModelInstance{};
	CComAnimator* m_pModelAnimator{};
	CComBeHavior* m_pBeHavior;
	CComPxCharacterController* m_pCharacterController{};
	CComCharacterMoveIntent* m_pMoveIntent{};
	CComCharacterMotor* m_pCharacterMotor{};
	CComPxRigidBody* m_pComRigidBody{};
	CComPxSphereCollider* m_pComSphereCol{};
	CComSound* m_pComSound{};

	// Anim
	SPtr<CResPixelShader> m_pResPixelShader{};
	SPtr<CResVertexShader> m_pResVertexCPUSkinningInstancedShader{};
	SPtr<CResCBuffer> m_pResSkinMeshCBuffer{};



	CComConstantBuffer* m_pComCBufferPerObject{};

	uint32_t							m_iCurrentInstanceCount{};
	_bool							m_bRootMotionTranslationActive{ false }, m_bRootMotionRotationActive{ false };
	_float								m_fRootMotionTranslationScale{ 1.f };
	_string								m_SocketName{};

	CHandle								m_TargetHandle{};
	std::unordered_map<_string, std::vector<_string>> m_SoundTable;
public:
	E::UPtr<E::CPrototype> Clone(void* pArg) PURE;
};

NS_END



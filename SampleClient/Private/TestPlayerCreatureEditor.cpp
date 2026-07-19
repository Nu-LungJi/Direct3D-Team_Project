#include "pch.h"
#include "TestPlayerCreatureEditor.h"

#include "ComCharacterMotor.h"
#include "ComLocomotion.h"
#include "ComPxCharacterController.h"
#include "DbgLineRender.h"
#include "GameInstance.h"
#include "Resources.h"
#include "TestPlayer3CameraCreatureEditor.h"

NS_USING(Client)

CTestPlayerCreatureEditor::CTestPlayerCreatureEditor() = default;

CTestPlayerCreatureEditor::CTestPlayerCreatureEditor(const CTestPlayerCreatureEditor& rhs)
	: CGameObject{ rhs }
{
}

CTestPlayerCreatureEditor::~CTestPlayerCreatureEditor() = default;

HRESULT CTestPlayerCreatureEditor::Initialize(void* pArg)
{
	auto* pDesc = static_cast<DESC*>(pArg);
	if (!pDesc || FAILED(CGameObject::Initialize(pArg)))
		return E_FAIL;

	{
		CComPxCharacterController::DESC Desc{};
		Desc.pResMaterial = CResPhysXMaterial::CreateAndLoad({});
		Desc.vPosition = pDesc->vInitialPosition;
		Desc.tFilter = pDesc->tFilter;
		if (FAILED(AddComponentFromProto(
			ES_EngineProtoMajorType::PHYSX,
			ES_EngineProtoPhysXComponent::Prototype_Component_ComPxCharacterController,
			"ComPxCharacterController", &Desc, &m_pCharacterController)))
		{
			return E_FAIL;
		}
	}

	{
		CComLocomotion::DESC Desc{};
		if (FAILED(AddComponentFromProto(
			ES_EngineProtoMajorType::PERMANENT,
			ES_EngineProtoComponent::Prototype_Component_ComLocomotion,
			"ComLocomotion", &Desc, &m_pLocomotion)))
		{
			return E_FAIL;
		}
	}

	{
		CComCharacterMotor::DESC Desc{};
		Desc.pLocomotion = m_pLocomotion;
		Desc.pCharacterController = m_pCharacterController;
		Desc.fGravity = -9.81f;
		Desc.fJumpVelocity = 5.f;
		Desc.bUseGravity = true;
		Desc.bSyncTransform = true;
		if (FAILED(AddComponentFromProto(
			ES_EngineProtoMajorType::PERMANENT,
			ES_EngineProtoComponent::Prototype_Component_ComCharacterMotor,
			"ComCharacterMotor", &Desc, &m_pCharacterMotor)))
		{
			return E_FAIL;
		}
	}

	GetTransform().SetPosition(pDesc->vInitialPosition);
	GetTransform().Update();
	return S_OK;
}

void CTestPlayerCreatureEditor::PriorityUpdate(_float)
{
	auto* pCamera = CGameInstance::Get().GetActiveCamera("CREATURE_PLAYER_CAMERA");
	if (!pCamera)
	{
		m_pLocomotion->ClearMoveIntent();
		return;
	}

	_float fForward{};
	_float fRight{};
	if (CGameInstance::Get().KeyPressing(DIK_W))
		fForward += 1.f;
	if (CGameInstance::Get().KeyPressing(DIK_S))
		fForward -= 1.f;
	if (CGameInstance::Get().KeyPressing(DIK_D))
		fRight += 1.f;
	if (CGameInstance::Get().KeyPressing(DIK_A))
		fRight -= 1.f;

	_float3 vForward{};
	_float3 vRight{};
	XMStoreFloat3(&vForward, pCamera->GetTransform().GetState(STATE::LOOK));
	XMStoreFloat3(&vRight, pCamera->GetTransform().GetState(STATE::RIGHT));
	vForward.y = 0.f;
	vRight.y = 0.f;

	const _float3 vMoveDirection{
		vForward.x * fForward + vRight.x * fRight,
		0.f,
		vForward.z * fForward + vRight.z * fRight };

	if (vMoveDirection.x != 0.f || vMoveDirection.z != 0.f)
		m_pLocomotion->SetMoveIntent(vMoveDirection, 5.f);
	else
		m_pLocomotion->ClearMoveIntent();

	if (CGameInstance::Get().KeyDown(DIK_SPACE))
		m_pLocomotion->RequestJump();
}

void CTestPlayerCreatureEditor::FixedUpdate(_float fTimeDelta)
{
	m_pCharacterMotor->FixedUpdate(fTimeDelta);
}

void CTestPlayerCreatureEditor::LateUpdate(_float)
{
	GetTransform().Update();

	if (auto* pCamera = Cast<CTestPlayer3CameraCreatureEditor>(
		CGameInstance::Get().GetActiveCamera("CREATURE_PLAYER_CAMERA")))
	{
		pCamera->UpdateFollow();
	}

	if (auto* pDbgLineRender = CGameInstance::Get().GetDbgLineRender())
	{
		const auto vPreviousColor = pDbgLineRender->GetColor();
		const auto ePreviousDepthMode = pDbgLineRender->GetDepthMode();
		const _float3 vPosition = GetTransform().GetPosition();

		pDbgLineRender->SetColor({ 0.2f, 0.7f, 1.f, 1.f });
		pDbgLineRender->SetDepthTest(true);
		pDbgLineRender->AddCapsule(
			0.5f,
			1.f,
			XMMatrixTranslation(vPosition.x, vPosition.y, vPosition.z));

		pDbgLineRender->SetColor(vPreviousColor);
		pDbgLineRender->SetDepthMode(ePreviousDepthMode);
	}
}

HRESULT CTestPlayerCreatureEditor::Render(ID3D11DeviceContext*, const RENDER_CTX&)
{
	return S_OK;
}

UPtr<CTestPlayerCreatureEditor> CTestPlayerCreatureEditor::Create()
{
	auto pInstance = ToUPtr(new CTestPlayerCreatureEditor{});
	if (FAILED(pInstance->InitializePrototype()))
		return nullptr;
	return pInstance;
}

UPtr<CPrototype> CTestPlayerCreatureEditor::Clone(void* pArg)
{
	auto pInstance = ToUPtr(new CTestPlayerCreatureEditor{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
		return nullptr;
	return pInstance;
}

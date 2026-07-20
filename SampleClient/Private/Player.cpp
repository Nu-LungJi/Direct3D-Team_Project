#include "pch.h"
#include "Player.h"
#include "Client_Resources.h"
#include "ComConstantBuffer.h"
#include "ComModelInstance.h"
#include "ComAnimator.h"
#include "Resources.h"
#include "GameInstance.h"
#include "TestPartObject.h"

#include "ComCharacterMotor.h"
#include "ComCharacterMoveIntent.h"
#include "ComPxCharacterController.h"
#include "TestPlayer3CameraCreatureEditor.h"
#include "Player_StateMachine.h"
#include "Player_Locomotion_State.h"

NS_USING(Client)

CPlayer::CPlayer()
	: CAnimationObject{}
{
}

CPlayer::CPlayer(const CPlayer& rhs)
	: CAnimationObject{ rhs }
{
	m_pResVertexShader = rhs.m_pResVertexShader;
	m_pResPixelShader = rhs.m_pResPixelShader;
	m_pResVertexInstancedShader = rhs.m_pResVertexInstancedShader;
	m_pResSkinMeshCBuffer = rhs.m_pResSkinMeshCBuffer;
	m_pAnimComputeShader = rhs.m_pAnimComputeShader;
}
CPlayer::~CPlayer()
{
}

void CPlayer::UpdateGUI()
{
	CAnimationObject::UpdateGUI();

}

HRESULT CPlayer::InitializePrototype(void* pArg)
{
	m_pResVertexShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelAnim");
	//m_pResVertexShader = CResVertexShader::Create("./ShaderFiles/Shader_VtxNorTex.hlsl");
	if (FAILED(m_pResVertexShader->Load()))
	{
		return E_FAIL;
	}
	m_pResPixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_TestModelAnim");
	//m_pResPixelShader = CResPixelShader::Create("./ShaderFiles/Shader_VtxNorTex.hlsl");
	if (FAILED(m_pResPixelShader->Load()))
	{
		return E_FAIL;
	}
	m_pResVertexInstancedShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelAnim_Instanced");
	if (!m_pResVertexInstancedShader || FAILED(m_pResVertexInstancedShader->Load()))
	{
		return E_FAIL;
	}
	m_pResSkinMeshCBuffer = CGameInstance::Get().GetResourceFirst<CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "CB_GPU_SKIN_MESH");
	if (!m_pResSkinMeshCBuffer)
	{
		return E_FAIL;
	}


	m_pAnimComputeShader = CGameInstance::Get().GetResourceFirst<CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_Animation");
	if (FAILED(m_pAnimComputeShader->Load()))
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CPlayer::Initialize(void* pArg)
{
	if (FAILED(CGameObject::Initialize(pArg)))
	{
		return E_FAIL;
	}
	
	auto* pDesc = static_cast<DESC*>(pArg);
	auto pGroup = pDesc->sGroupTag;
	auto pRes = pDesc->sResTag;

	{
		CComConstantBuffer::DESC Desc{};
		Desc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_OBJECT };
		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ConstantBuffer", "ComCBufferPerObject", &Desc, &m_pComCBufferPerObject)))
		{
			return E_FAIL;
		};
	}

	{
	
		CComModelInstance::DESC Desc{};
		Desc.sGroupTag = pGroup;
		Desc.sResTag = pRes;

		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ModelInstance", "ComCModelIntance", &Desc, &m_pComModelInstance)))
		{
			return E_FAIL;
		};
	}

	{
		CComAnimator::DESC DescAnim{};
		DescAnim.sComTag = "ComCModelIntance";

		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_Animator", "ComCModelAnimator", &DescAnim, &m_pModelAnimator)))
		{
			return E_FAIL;
		};


		m_pModelAnimator->Play_Anim(1.f, true, 0.2f);
	}


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
		CComCharacterMoveIntent::DESC Desc{};
		if (FAILED(AddComponentFromProto(
			ES_EngineProtoMajorType::PERMANENT,
			ES_EngineProtoComponent::Prototype_Component_ComCharacterMoveIntent,
			"ComLocomotion", &Desc, &m_pMoveIntent)))
		{
			return E_FAIL;
		}
	}

	{
		CComCharacterMotor::DESC Desc{};
		Desc.pMoveIntent = m_pMoveIntent;
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

	{
		CStateMachine::DESC Desc{};

		if (FAILED(AddComponentFromProto(pGroup,"Prototype_Component_PlayerStateMachine","ComPlayerStateMachine", &Desc, &m_pStateMachine)))
		{


			return E_FAIL;
		}

	
	}
	GetTransform().SetScale(_float3{2.f,2.f,2.f });
	GetTransform().SetPosition(pDesc->vInitialPosition);
	GetTransform().Update();

	//CTestPartObject::DESC WeaponDesc{};
	//WeaponDesc.sObjectTag = "Weapon";
	//WeaponDesc.hOwner = GetHandle();
	//WeaponDesc.iBoneIndex = m_pComModelInstance->GetModel()->Get_BoneIndex("SKT_RightHandSocket");
	//WeaponDesc.vBoneOffset = {0.f,0.f,0.f};
	//WeaponDesc.sGroupTag = "TEST"; 
	//WeaponDesc.sResTag = "Static_Axe_Model_Resource";

	//auto Weapon = E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_TEST", "Prototype_GameObject_TestPartObject", "Weapon", &WeaponDesc);
	//if (!Weapon.has_value())
	//{
	//	MSG_BOX("Create Failed Weapon");
	//	return E_FAIL;
	//}

	//m_Partes[ETOUI(PARTES::WEAPON)] = Weapon.value();
	return S_OK;

}

void CPlayer::PriorityUpdate(E::_float fTimeDelta)
{
	if (!m_bStateInitailzie) {
		m_pStateMachine->AddPlayerState(PLAYER_STATE::LOCOMOTION, CPlayer_Locomotion_State::Create());
		m_pStateMachine->SetInitialState(PLAYER_STATE::LOCOMOTION);
		m_bStateInitailzie = true;
	}


	auto* pCamera = CGameInstance::Get().GetActiveCamera("CREATURE_ANIM_PLAYER_CAMERA");
	if (!pCamera)
	{
		m_pMoveIntent->ClearMoveIntent();
		
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
		m_pMoveIntent->SetMoveIntent(vMoveDirection, 5.f);
	else
		m_pMoveIntent->ClearMoveIntent();

	if (CGameInstance::Get().KeyDown(DIK_SPACE))
		m_pMoveIntent->RequestJump();

	if (m_pStateMachine)
		m_pStateMachine->PriorityUpdate(fTimeDelta);
}

void CPlayer::FixedUpdate(_float fTimeDelta)
{
	m_pCharacterMotor->FixedUpdate(fTimeDelta);
}


void CPlayer::Update(E::_float fTimeDelta)
{
	ZoneScopedN("Update CPlayer");

	if (m_pStateMachine)
		m_pStateMachine->Update(fTimeDelta);

	if (m_pComModelInstance->GetModel()->GetAnimations().size() != 0) {

		m_pModelAnimator->Update(fTimeDelta);

	}

}

int32_t CPlayer::FindAnimationIndex(_string_view sAnimationName) const
{
	if (!m_pComModelInstance || !m_pComModelInstance->GetModel())
		return -1;

	const auto& animations = m_pComModelInstance->GetModel()->GetAnimations();
	for (uint32_t i = 0; i < animations.size(); ++i)
	{
		if (animations[i] && animations[i]->GetAnimName() == sAnimationName)
			return static_cast<int32_t>(i);
	}

	return -1;
}

void CPlayer::LateUpdate(E::_float fTimeDelta)
{
	if (m_pStateMachine)
		m_pStateMachine->LateUpdate(fTimeDelta);

	GetTransform().Update();

	if (auto* pCamera = Cast<CTestPlayer3CameraCreatureEditor>(
		CGameInstance::Get().GetActiveCamera("CREATURE_ANIM_PLAYER_CAMERA")))
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

	const auto& pModel = m_pComModelInstance->GetModel();

	if (!pModel)
		return;

	if (!pModel->GetAnimations().empty())
	{
		CGameInstance::Get().Add_Instance(m_pComModelInstance, m_pModelAnimator, *GetTransform().GetCombinedWorldMatrix());

		return;
	}


	CGameInstance::Get().AddRenderObject(RENDERGROUP::NONBLEND, this);
}

HRESULT CPlayer::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{

	{
		E::CB_PER_OBJECT cbPerObject{};
		cbPerObject.matWorld = *GetTransform().GetCombinedWorldMatrix();
		XMStoreFloat4x4(&cbPerObject.matWVP, GetTransform().GetLoadedCombinedWorldMatrix() * ctx.matViewProj);
		if (FAILED(m_pComCBufferPerObject->MapDiscard(pContext, &cbPerObject, sizeof(cbPerObject))))
		{
			return E_FAIL;
		}
		pContext->VSSetConstantBuffers(0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());
		pContext->PSSetConstantBuffers(0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());


		m_pComModelInstance->DebugDraw_Bones(cbPerObject.matWorld);

	}
	const auto& vs = m_pResVertexShader;

	const auto& ps = m_pResPixelShader;


	pContext->IASetInputLayout(vs->GetInputLayout().Get());
	pContext->VSSetShader(vs->GetVertexShader().Get(), nullptr, 0);
	pContext->PSSetShader(ps->GetPixelShader().Get(), nullptr, 0);

	auto pModel = m_pComModelInstance->GetModel();

	uint32_t	iNumMeshes = pModel->Get_NumMeshes();
	for (uint32_t i = 0; i < iNumMeshes; ++i) {
		const auto& viBuffer = pModel->GetMeshes()[i];


		ID3D11Buffer* vertexBuffers[] = {
				viBuffer->GetVertexBuffer().Get()
		};
		uint32_t strides[] = {
			viBuffer->GetVertexStride()
		};
		uint32_t offsets[] = {
			0
		};
		pContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
		pContext->IASetIndexBuffer(viBuffer->GetIndexBuffer().Get(), viBuffer->GetIndexFormat(), 0);
		pContext->IASetPrimitiveTopology(viBuffer->GetPrimitiveType());

		{
			if (!m_pComModelInstance->GetModel()->GetAnimations().empty())
				if (FAILED(m_pComModelInstance->Bind_BoneMatrices(pContext, i))) {
					return E_FAIL;
				}
		}

		{
			m_pComModelInstance->Bind_Textures(pContext, i);
			m_pComModelInstance->Bind_Materials(pContext, { 1.f, 1.f, 1.f }, 0.f, { 1.f, 1.f, 1.f }, 0.f, 1.f);	// EmissiveColor -> EmissiveIntensity -> Alpha 순
		}

		pContext->DrawIndexed(viBuffer->GetNumIndices(), 0, 0);
		//pContext->DrawIndexedInstancedIndirect(viBuffer->GetNumIndices(), 0, 0);
	}



	return S_OK;
}

HRESULT CPlayer::Render_Instanced(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx, const E::MODEL_INSTANCE_BATCH& Batch)
{
	ZoneScopedN("Render TestModel");

	if (!pContext)
		return E_INVALIDARG;

	const uint32_t iInstanceCount = Batch.Instances.size();

	if (iInstanceCount == 0)
		return S_OK;



	if (!m_pAnimComputeShader || !m_pAnimComputeShader->GetComputeShader())
	{
		return E_FAIL;
	}

	// -------------------------------------------------
	// Compute Shader
	// -------------------------------------------------

	if (FAILED(Update_InstanceBuffer(pContext, Batch.Instances)))
	{
		return E_FAIL;
	}

	// CS t0 ~ t5
	if (FAILED(m_pComModelInstance->Bind_GPUAnimationSRVs_CS(pContext)))
	{
		return E_FAIL;
	}

	// CS t6
	if (FAILED(Bind_InstanceBuffer_CS(pContext)))
	{
		return E_FAIL;
	}

	// CS u0
	if (FAILED(Bind_FinalBoneUAV_CS(pContext)))
	{
		return E_FAIL;
	}

	pContext->CSSetShader(m_pAnimComputeShader->GetComputeShader().Get(), nullptr, 0);

	/*
	 * 한 Thread Group = 한 인스턴스라는 전제.
	 */
	pContext->Dispatch(iInstanceCount, 1, 1);

	if (FAILED(Unbind_AnimationCompute(pContext)))
	{
		return E_FAIL;
	}

	// -------------------------------------------------
	// Vertex Shader용 SRV
	// -------------------------------------------------

	// VS t6 = InstanceData
	if (FAILED(Bind_InstanceBuffer_VS(pContext)))
	{
		return E_FAIL;
	}

	// VS t7 = Compute 결과 FinalBoneMatrix
	if (FAILED(Bind_FinalBoneSRV_VS(pContext)))
	{
		return E_FAIL;
	}
	if (FAILED(m_pComModelInstance->Bind_GPUSkinBones_VS(pContext)))
	{
		return E_FAIL;
	}

	// -------------------------------------------------
	// Graphics Shader
	// -------------------------------------------------

	const auto& vs = m_pResVertexInstancedShader;

	const auto& ps = m_pResPixelShader;

	if (!vs || !ps)
		return E_FAIL;

	pContext->IASetInputLayout(vs->GetInputLayout().Get());

	pContext->VSSetShader(vs->GetVertexShader().Get(), nullptr, 0);

	pContext->PSSetShader(ps->GetPixelShader().Get(), nullptr, 0);

	auto pModel = CGameInstance::Get().GetResourceFirst<CResModel>(Batch.Key.modelGroup, Batch.Key.modelTag);

	const uint32_t iNumMeshes = pModel->Get_NumMeshes();

	for (uint32_t iMeshIndex = 0; iMeshIndex < iNumMeshes; ++iMeshIndex)
	{
		const auto& viBuffer = pModel->GetMeshes()[iMeshIndex];

		if (!viBuffer)
			continue;

		ID3D11Buffer* vertexBuffers[] =
		{
			viBuffer->GetVertexBuffer().Get()
		};

		UINT strides[] =
		{
			viBuffer->GetVertexStride()
		};

		UINT offsets[] =
		{
			0
		};

		pContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);

		pContext->IASetIndexBuffer(viBuffer->GetIndexBuffer().Get(), viBuffer->GetIndexFormat(), 0);

		pContext->IASetPrimitiveTopology(viBuffer->GetPrimitiveType());

		E::GPU_SKIN_MESH_CONSTANTS skinConstants{};
		skinConstants.iSkinBoneOffset = pModel->Get_GPUMeshSkinRange(iMeshIndex).iSkinBoneOffset;
		D3D11_MAPPED_SUBRESOURCE mappedResource{};
		if (FAILED(pContext->Map(m_pResSkinMeshCBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
			return E_FAIL;
		memcpy(mappedResource.pData, &skinConstants, sizeof(skinConstants));
		pContext->Unmap(m_pResSkinMeshCBuffer->GetCBuffer().Get(), 0);
		ID3D11Buffer* pSkinMeshCB = m_pResSkinMeshCBuffer->GetCBuffer().Get();
		pContext->VSSetConstantBuffers(5, 1, &pSkinMeshCB);


		m_pComModelInstance->Bind_Textures(pContext, iMeshIndex);
		m_pComModelInstance->Bind_Materials(pContext, { 1.f, 1.f, 1.f }, 0.f, { 1.f, 1.f, 1.f }, 0.f, 1.f);

		pContext->DrawIndexedInstanced(viBuffer->GetNumIndices(), iInstanceCount, 0, 0, 0);
	}



	if (FAILED(Unbind_AnimationVS(pContext)))
	{
		return E_FAIL;
	}

	return S_OK;
}
HRESULT CPlayer::Update_InstanceBuffer(ID3D11DeviceContext* pContext, const std::vector<GPU_ANIM_INSTANCE_DATA>& Instances)
{

	m_iCurrentInstanceCount = static_cast<uint32_t>(Instances.size());

	if (Instances.empty())
		return S_OK;

	constexpr uint32_t MAX_INSTANCE_COUNT = 512;

	if (m_iCurrentInstanceCount > MAX_INSTANCE_COUNT)
		return E_FAIL;

	auto pStructuredBuffer = CGameInstance::Get().GetResourceFirst<CResStructuredBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "SBUFFER_ANIMAITON");

	if (!pStructuredBuffer)
		return E_FAIL;

	ID3D11Buffer* pBuffer = pStructuredBuffer->GetBuffer().Get();

	if (!pBuffer)
		return E_FAIL;

	/*
	 * 이전 Batch에서 VS/CS에 연결되어 있을 수 있으므로
	 * Map 전에 SRV를 해제한다.
	 */
	ID3D11ShaderResourceView* pNullSRV = nullptr;

	pContext->CSSetShaderResources(6, 1, &pNullSRV);

	pContext->VSSetShaderResources(6, 1, &pNullSRV);

	const size_t iCopySize = sizeof(GPU_ANIM_INSTANCE_DATA) * m_iCurrentInstanceCount;

	// pDstBox가 nullptr이면 D3D11은 버퍼 전체(현재 512개)를 복사한다.
	// Instances에는 이번 배치의 원소만 있으므로, 유효한 원소 범위만 갱신해야 한다.
	D3D11_BOX updateBox{};
	updateBox.left = 0;
	updateBox.right = static_cast<UINT>(iCopySize);
	updateBox.top = 0;
	updateBox.bottom = 1;
	updateBox.front = 0;
	updateBox.back = 1;

	pContext->UpdateSubresource(pBuffer, 0, &updateBox, Instances.data(), 0, 0);

	return S_OK;

}

HRESULT CPlayer::Bind_InstanceBuffer_CS(ID3D11DeviceContext* pContext)
{
	auto pStructuredBuffer = CGameInstance::Get().GetResourceFirst<CResStructuredBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "SBUFFER_ANIMAITON");

	if (!pStructuredBuffer)
		return E_FAIL;

	ID3D11ShaderResourceView* pSRV = pStructuredBuffer->GetSRV().Get();

	if (!pSRV)
		return E_FAIL;

	pContext->CSSetShaderResources(6, 1, &pSRV);

	return S_OK;
}
HRESULT CPlayer::Bind_FinalBoneUAV_CS(ID3D11DeviceContext* pContext)
{
	auto pStructuredBuffer = CGameInstance::Get().GetResourceFirst<CResStructuredBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "SBUFFER_FINALBONEMATRIX");

	if (!pStructuredBuffer)
		return E_FAIL;

	ID3D11UnorderedAccessView* pUAV = pStructuredBuffer->GetUAV().Get();

	if (!pUAV)
		return E_FAIL;

	// 이전 Draw에서 FinalBone 버퍼가 VS의 SRV로
	// 연결되어 있었다면 먼저 연결 해제
	ID3D11ShaderResourceView* pNullSRV = nullptr;

	pContext->VSSetShaderResources(7, 1, &pNullSRV);

	// CS의 u0 슬롯에 출력 UAV 연결
	pContext->CSSetUnorderedAccessViews(0, 1, &pUAV, nullptr);

	return S_OK;
}
HRESULT CPlayer::Unbind_AnimationCompute(ID3D11DeviceContext* pContext)
{
	// CS t0 ~ t6 SRV 해제
	ID3D11ShaderResourceView* pNullSRVs[7] =
	{
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr
	};

	pContext->CSSetShaderResources(0, 7, pNullSRVs);

	// CS u0 UAV 해제
	ID3D11UnorderedAccessView* pNullUAV = nullptr;

	pContext->CSSetUnorderedAccessViews(0, 1, &pNullUAV, nullptr);

	// Compute Shader 자체도 해제
	pContext->CSSetShader(nullptr, nullptr, 0);

	return S_OK;
}

HRESULT CPlayer::Bind_InstanceBuffer_VS(ID3D11DeviceContext* pContext)
{

	auto pStructuredBuffer = CGameInstance::Get().GetResourceFirst<CResStructuredBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "SBUFFER_ANIMAITON");

	if (!pStructuredBuffer)
		return E_FAIL;

	ID3D11ShaderResourceView* pSRV = pStructuredBuffer->GetSRV().Get();

	if (!pSRV)
		return E_FAIL;

	// VS의 t6 슬롯에 InstanceData 연결
	pContext->VSSetShaderResources(6, 1, &pSRV);

	return S_OK;
}

HRESULT CPlayer::Bind_FinalBoneSRV_VS(ID3D11DeviceContext* pContext)
{

	auto pStructuredBuffer = CGameInstance::Get().GetResourceFirst<CResStructuredBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "SBUFFER_FINALBONEMATRIX");

	if (!pStructuredBuffer)
		return E_FAIL;

	ID3D11ShaderResourceView* pSRV = pStructuredBuffer->GetSRV().Get();

	if (!pSRV)
		return E_FAIL;

	// VS의 t7 슬롯에 Compute 결과 연결
	pContext->VSSetShaderResources(7, 1, &pSRV);

	return S_OK;
}
HRESULT CPlayer::Unbind_AnimationVS(ID3D11DeviceContext* pContext)
{
	if (!pContext)
		return E_INVALIDARG;

	ID3D11ShaderResourceView* pNullSRVs[3]{};

	pContext->VSSetShaderResources(6, 3, pNullSRVs);

	return S_OK;
}

E::UPtr<CPlayer> CPlayer::Create()
{
	auto pInstance = E::ToUPtr(new CPlayer{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CPlayer");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CPlayer::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CPlayer{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CPlayer");
		return nullptr;
	}

	return pInstance;
}

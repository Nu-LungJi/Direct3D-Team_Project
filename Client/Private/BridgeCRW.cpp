#include "pch.h"
#include "BridgeCRW.h"
#include "Client_Resources.h"
#include "ComConstantBuffer.h"
#include "ComModelInstance.h"
#include "ComAnimator.h"
#include "Resources.h"
#include "GameInstance.h"
#include "ComSocket.h"

NS_USING(Client)

void CBridgeCRW::UpdateGUI()
{
	__super::UpdateGUI();

	auto& animations = m_pComModelInstance->GetModel()->GetAnimations();

	ImGui::Begin("Animation List");

	if (ImGui::TreeNode("Animation"))
	{
		for (uint32_t i = 0; i < animations.size(); ++i)
		{
			auto pAnim = animations[i];

			if (!pAnim)
				continue;

			bool bSelected = (m_pModelAnimator->GetPlayAnimIndex() == i);


			if (ImGui::Selectable(pAnim->GetAnimName().c_str(), bSelected))
			{
				m_pModelAnimator->Play_Anim(i,false, 0.1f);

			}
		}

		ImGui::TreePop();
	}

	ImGui::End();
}

CBridgeCRW::CBridgeCRW()
	: CAnimationObject{}
{
}

CBridgeCRW::~CBridgeCRW()
{
}

HRESULT CBridgeCRW::InitializePrototype(void* pArg)
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

HRESULT CBridgeCRW::Initialize(void* pArg)
{
	if (FAILED(CGameObject::Initialize(pArg)))
	{
		return E_FAIL;
	}

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
		Desc.sGroupTag = "TEST";
		Desc.sResTag = "Model_Resource";

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

		// TestModel은 생성 직후부터 CPU pose + VS skinning 경로를 사용한다.
		m_pModelAnimator->SetEvaluationMode(
			CComAnimator::EVALUATION_MODE::GPU);
		m_eState = STATE::FLOATING;
		m_pModelAnimator->Play_Anim(
			ETOUI(STATE::FLOATING), true, 0.f);
	}


	return S_OK;

}

void CBridgeCRW::PriorityUpdate(E::_float fTimeDelta)
{
}

void CBridgeCRW::Update(E::_float fTimeDelta)
{
	ZoneScopedN("Update TestModel");

	if (m_pComModelInstance->GetModel()->GetAnimations().size() != 0) {

		m_pModelAnimator->Update(fTimeDelta);

		if (m_eState == STATE::DESCENDING &&
			m_pModelAnimator->GetPlayAnimRatio() >= 1.f)
		{
			m_eState = STATE::IDLE;
			m_pModelAnimator->Play_Anim(
				static_cast<int32_t>(STATE::IDLE), true, 0.f);
		}
	}

}

_bool CBridgeCRW::RequestBring()
{
	if (!m_pModelAnimator || m_eState != STATE::FLOATING)
		return false;

	m_eState = STATE::DESCENDING;
	m_pModelAnimator->Play_Anim(
		static_cast<int32_t>(STATE::DESCENDING), false, 0.1f);

	CGameInstance::Get().GetSoundManager()->Play3D(
		"./Resources/SampleClient/Sound/Bridge/Bridge_Start.wav",
		SOUND_3D_DESC{
			.vPosition = GetTransform().GetPosition(),
			.fMinDistance = 5.f,
			.fMaxDistance = 60.f,
			.eRolloff = SOUND_3D_ROLLOFF::LINEAR
		},
		SOUND_PLAY_DESC{
			.sBusID = SOUND_BUS::SFX,
			.fVolume = 1.f,
			.fPitch = 1.f,
			.iPriority = 64,
			.bLoop = false
		});
	return true;
}

_bool CBridgeCRW::RequestFix()
{
	if (!m_pModelAnimator || m_eState != STATE::IDLE)
		return false;

	m_eState = STATE::FIXING;
	m_pModelAnimator->Play_Anim(
		static_cast<int32_t>(STATE::FIXING), false, 0.1f);

	CGameInstance::Get().GetSoundManager()->Play3D(
		"./Resources/SampleClient/Sound/Bridge/Bridge_Bind.wav",
		SOUND_3D_DESC{
			.vPosition = GetTransform().GetPosition(),
			.fMinDistance = 5.f,
			.fMaxDistance = 60.f,
			.eRolloff = SOUND_3D_ROLLOFF::LINEAR
		},
		SOUND_PLAY_DESC{
			.sBusID = SOUND_BUS::SFX,
			.fVolume = 1.f,
			.fPitch = 1.f,
			.iPriority = 64,
			.bLoop = false
		});
	return true;
}

void CBridgeCRW::LateUpdate(E::_float fTimeDelta)
{
	GetTransform().Update();

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

HRESULT CBridgeCRW::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
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
			m_pComModelInstance->Bind_Materials(pContext, { 1.f, 1.f, 1.f }, 0.f, { 1.f, 1.f, 1.f }, 0.f, 1.f);	// EmissiveColor -> EmissiveIntensity -> Alpha ??
		}

		pContext->DrawIndexed(viBuffer->GetNumIndices(), 0, 0);
		//pContext->DrawIndexedInstancedIndirect(viBuffer->GetNumIndices(), 0, 0);
	}



	return S_OK;
}

HRESULT CBridgeCRW::Render_Instanced(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx, const E::MODEL_INSTANCE_BATCH& Batch)
{
	const auto evaluationMode =
		static_cast<CComAnimator::EVALUATION_MODE>(Batch.Key.iEvaluationMode);

	// GPU: 기존 Animation CS -> FinalBone SRV -> GPU skinning VS 경로만 허용한다.
	if (evaluationMode != CComAnimator::EVALUATION_MODE::GPU)
		return E_FAIL;


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
	 * ??Thread Group = ???�스?�스?�는 ?�제.
	 */
	pContext->Dispatch(iInstanceCount, 1, 1);

	if (FAILED(Unbind_AnimationCompute(pContext)))
	{
		return E_FAIL;
	}

	// -------------------------------------------------
	// Vertex Shader??SRV
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
HRESULT CBridgeCRW::Update_InstanceBuffer(ID3D11DeviceContext* pContext, const std::vector<GPU_ANIM_INSTANCE_DATA>& Instances)
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

	ID3D11ShaderResourceView* pNullSRV = nullptr;


	pContext->VSSetShaderResources(6, 1, &pNullSRV);

	const size_t iCopySize = sizeof(GPU_ANIM_INSTANCE_DATA) * m_iCurrentInstanceCount;


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

HRESULT CBridgeCRW::Bind_InstanceBuffer_CS(ID3D11DeviceContext* pContext)
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
HRESULT CBridgeCRW::Bind_FinalBoneUAV_CS(ID3D11DeviceContext* pContext)
{
	auto pStructuredBuffer = CGameInstance::Get().GetResourceFirst<CResStructuredBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "SBUFFER_FINALBONEMATRIX");

	if (!pStructuredBuffer)
		return E_FAIL;

	ID3D11UnorderedAccessView* pUAV = pStructuredBuffer->GetUAV().Get();

	if (!pUAV)
		return E_FAIL;


	ID3D11ShaderResourceView* pNullSRV = nullptr;

	pContext->VSSetShaderResources(7, 1, &pNullSRV);


	pContext->CSSetUnorderedAccessViews(0, 1, &pUAV, nullptr);

	return S_OK;
}
HRESULT CBridgeCRW::Unbind_AnimationCompute(ID3D11DeviceContext* pContext)
{

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


	ID3D11UnorderedAccessView* pNullUAV = nullptr;

	pContext->CSSetUnorderedAccessViews(0, 1, &pNullUAV, nullptr);


	pContext->CSSetShader(nullptr, nullptr, 0);

	return S_OK;
}

HRESULT CBridgeCRW::Bind_InstanceBuffer_VS(ID3D11DeviceContext* pContext)
{

	auto pStructuredBuffer = CGameInstance::Get().GetResourceFirst<CResStructuredBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "SBUFFER_ANIMAITON");

	if (!pStructuredBuffer)
		return E_FAIL;

	ID3D11ShaderResourceView* pSRV = pStructuredBuffer->GetSRV().Get();

	if (!pSRV)
		return E_FAIL;


	pContext->VSSetShaderResources(6, 1, &pSRV);

	return S_OK;
}

HRESULT CBridgeCRW::Bind_FinalBoneSRV_VS(ID3D11DeviceContext* pContext)
{

	auto pStructuredBuffer = CGameInstance::Get().GetResourceFirst<CResStructuredBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "SBUFFER_FINALBONEMATRIX");

	if (!pStructuredBuffer)
		return E_FAIL;

	ID3D11ShaderResourceView* pSRV = pStructuredBuffer->GetSRV().Get();

	if (!pSRV)
		return E_FAIL;


	pContext->VSSetShaderResources(7, 1, &pSRV);

	return S_OK;
}
HRESULT CBridgeCRW::Unbind_AnimationVS(ID3D11DeviceContext* pContext)
{
	if (!pContext)
		return E_INVALIDARG;

	ID3D11ShaderResourceView* pNullSRVs[3]{};

	pContext->VSSetShaderResources(6, 3, pNullSRVs);

	return S_OK;
}

E::UPtr<CBridgeCRW> CBridgeCRW::Create()
{
	auto pInstance = E::ToUPtr(new CBridgeCRW{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CBridgeCRW");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CBridgeCRW::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBridgeCRW{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBridgeCRW");
		return nullptr;
	}

	return pInstance;
}

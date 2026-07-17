#include "pch.h"
#include "TestModel.h"
#include "Client_Resources.h"
#include "ComConstantBuffer.h"
#include "ComModelInstance.h"
#include "ComAnimator.h"
#include "Resources.h"
#include "GameInstance.h"
#include "TestPartObject.h"



NS_USING(Client)

CTestModel::CTestModel()
	: CAnimationObject{}
{
}

CTestModel::~CTestModel()
{
}

void CTestModel::UpdateGUI()
{
	CAnimationObject::UpdateGUI();

}

HRESULT CTestModel::InitializePrototype(void* pArg)
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

HRESULT CTestModel::Initialize(void* pArg)
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
	}

	CTestPartObject::DESC WeaponDesc{};
	WeaponDesc.sObjectTag = "Weapon";
	WeaponDesc.hOwner = GetHandle();
	WeaponDesc.iBoneIndex = m_pComModelInstance->GetModel()->Get_BoneIndex("SKT_RightHandSocket");
	WeaponDesc.vBoneOffset = {0.f,0.f,0.f};
	WeaponDesc.sGroupTag = "TEST"; 
	WeaponDesc.sResTag = "Static_Axe_Model_Resource";

	auto Weapon = E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_TEST", "Prototype_GameObject_TestPartObject", "Weapon", &WeaponDesc);
	if (!Weapon.has_value())
	{
		MSG_BOX("Create Failed Weapon");
		return E_FAIL;
	}

	m_Partes[ETOUI(PARTES::WEAPON)] = Weapon.value();
	return S_OK;

}

void CTestModel::PriorityUpdate(E::_float fTimeDelta)
{ 
}

void CTestModel::Update(E::_float fTimeDelta)
{
	ZoneScopedN("Update TestModel");

	if (m_pComModelInstance->GetModel()->GetAnimations().size() != 0) {
		
		m_pModelAnimator->Update(fTimeDelta);
	}

}

void CTestModel::LateUpdate(E::_float fTimeDelta)
{
	GetTransform().Update();

	const auto& pModel = m_pComModelInstance->GetModel();

	if (!pModel)
		return;

	if (!pModel->GetAnimations().empty())
	{
		CGameInstance::Get().Add_Instance(m_pComModelInstance,m_pModelAnimator,*GetTransform().GetCombinedWorldMatrix());
	
		return;
	}


	CGameInstance::Get().AddRenderObject(RENDERGROUP::NONBLEND,this);
}

HRESULT CTestModel::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
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
			if(!m_pComModelInstance->GetModel()->GetAnimations().empty())
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

HRESULT CTestModel::Render_Instanced(ID3D11DeviceContext* pContext,const E::RENDER_CTX& ctx,const E::MODEL_INSTANCE_BATCH& Batch)
{
	ZoneScopedN("Render TestModel");

	if (!pContext)
		return E_INVALIDARG;

	const uint32_t iInstanceCount = Batch.Instances.size();

	if (iInstanceCount == 0)
		return S_OK;



	if (!m_pAnimComputeShader ||!m_pAnimComputeShader->GetComputeShader())
	{
		return E_FAIL;
	}

	// -------------------------------------------------
	// Compute Shader
	// -------------------------------------------------

	if (FAILED(Update_InstanceBuffer(pContext,Batch.Instances)))
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

	pContext->CSSetShader(m_pAnimComputeShader->GetComputeShader().Get(),nullptr,0);

	/*
	 * 한 Thread Group = 한 인스턴스라는 전제.
	 */
	pContext->Dispatch(iInstanceCount,1,1);

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

	const auto& vs =m_pResVertexInstancedShader;

	const auto& ps =m_pResPixelShader;

	if (!vs || !ps)
		return E_FAIL;

	pContext->IASetInputLayout(vs->GetInputLayout().Get());

	pContext->VSSetShader(vs->GetVertexShader().Get(),nullptr,0);

	pContext->PSSetShader(ps->GetPixelShader().Get(),nullptr,0);

	auto pModel =CGameInstance::Get().GetResourceFirst<CResModel>(Batch.Key.modelGroup,Batch.Key.modelTag);

	const uint32_t iNumMeshes =pModel->Get_NumMeshes();

	for (uint32_t iMeshIndex = 0;iMeshIndex < iNumMeshes;++iMeshIndex)
	{
		const auto& viBuffer =pModel->GetMeshes()[iMeshIndex];

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

		pContext->IASetVertexBuffers(0,1,vertexBuffers,strides,offsets);

		pContext->IASetIndexBuffer(viBuffer->GetIndexBuffer().Get(),viBuffer->GetIndexFormat(),0);

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

		pContext->DrawIndexedInstanced(viBuffer->GetNumIndices(),iInstanceCount,0,0,0);
	}



	if (FAILED(Unbind_AnimationVS(pContext)))
	{
		return E_FAIL;
	}

	return S_OK;
}
HRESULT CTestModel::Update_InstanceBuffer(ID3D11DeviceContext* pContext,const std::vector<GPU_ANIM_INSTANCE_DATA>& Instances)
{

	m_iCurrentInstanceCount =static_cast<uint32_t>(Instances.size());

	if (Instances.empty())
		return S_OK;

	constexpr uint32_t MAX_INSTANCE_COUNT = 512;

	if (m_iCurrentInstanceCount > MAX_INSTANCE_COUNT)
		return E_FAIL;

	auto pStructuredBuffer =CGameInstance::Get().GetResourceFirst<CResStructuredBuffer>(TAG_RES_GRP_PERMANENT_BUFFER,"SBUFFER_ANIMAITON");

	if (!pStructuredBuffer)
		return E_FAIL;

	ID3D11Buffer* pBuffer =pStructuredBuffer->GetBuffer().Get();

	if (!pBuffer)
		return E_FAIL;

	/*
	 * 이전 Batch에서 VS/CS에 연결되어 있을 수 있으므로
	 * Map 전에 SRV를 해제한다.
	 */
	ID3D11ShaderResourceView* pNullSRV = nullptr;

	pContext->CSSetShaderResources(6,1,&pNullSRV);

	pContext->VSSetShaderResources(6,1,&pNullSRV);

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

HRESULT CTestModel::Bind_InstanceBuffer_CS(ID3D11DeviceContext* pContext)
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
HRESULT CTestModel::Bind_FinalBoneUAV_CS(ID3D11DeviceContext* pContext)
{
	auto pStructuredBuffer =CGameInstance::Get().GetResourceFirst<CResStructuredBuffer>(TAG_RES_GRP_PERMANENT_BUFFER,"SBUFFER_FINALBONEMATRIX");

	if (!pStructuredBuffer)
		return E_FAIL;

	ID3D11UnorderedAccessView* pUAV = pStructuredBuffer->GetUAV().Get();

	if (!pUAV)
		return E_FAIL;

	// 이전 Draw에서 FinalBone 버퍼가 VS의 SRV로
	// 연결되어 있었다면 먼저 연결 해제
	ID3D11ShaderResourceView* pNullSRV = nullptr;

	pContext->VSSetShaderResources(7,1,&pNullSRV);

	// CS의 u0 슬롯에 출력 UAV 연결
	pContext->CSSetUnorderedAccessViews(0,1,&pUAV,nullptr);

	return S_OK;
}
HRESULT CTestModel::Unbind_AnimationCompute(ID3D11DeviceContext* pContext)
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

	pContext->CSSetShaderResources(0,7,pNullSRVs);

	// CS u0 UAV 해제
	ID3D11UnorderedAccessView* pNullUAV = nullptr;

	pContext->CSSetUnorderedAccessViews(0,1,&pNullUAV,nullptr);

	// Compute Shader 자체도 해제
	pContext->CSSetShader(nullptr,nullptr,0);

	return S_OK;
}

HRESULT CTestModel::Bind_InstanceBuffer_VS(ID3D11DeviceContext* pContext)
{

	auto pStructuredBuffer = CGameInstance::Get().GetResourceFirst<CResStructuredBuffer>(TAG_RES_GRP_PERMANENT_BUFFER,"SBUFFER_ANIMAITON");

	if (!pStructuredBuffer)
		return E_FAIL;

	ID3D11ShaderResourceView* pSRV = pStructuredBuffer->GetSRV().Get();

	if (!pSRV)
		return E_FAIL;

	// VS의 t6 슬롯에 InstanceData 연결
	pContext->VSSetShaderResources(6,1,&pSRV);

	return S_OK;
}

HRESULT CTestModel::Bind_FinalBoneSRV_VS( ID3D11DeviceContext* pContext)
{

	auto pStructuredBuffer =CGameInstance::Get().GetResourceFirst<CResStructuredBuffer>(TAG_RES_GRP_PERMANENT_BUFFER,"SBUFFER_FINALBONEMATRIX");

	if (!pStructuredBuffer)
		return E_FAIL;

	ID3D11ShaderResourceView* pSRV = pStructuredBuffer->GetSRV().Get();

	if (!pSRV)
		return E_FAIL;

	// VS의 t7 슬롯에 Compute 결과 연결
	pContext->VSSetShaderResources(7,1,&pSRV);

	return S_OK;
}
HRESULT CTestModel::Unbind_AnimationVS(ID3D11DeviceContext* pContext)
{
	if (!pContext)
		return E_INVALIDARG;

	ID3D11ShaderResourceView* pNullSRVs[3]{};

	pContext->VSSetShaderResources(6,3,pNullSRVs);

	return S_OK;
}

E::UPtr<CTestModel> CTestModel::Create()
{
	auto pInstance = E::ToUPtr(new CTestModel{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CTestModel");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CTestModel::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CTestModel{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CTestModel");
		return nullptr;
	}

	return pInstance;
}

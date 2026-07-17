#include "pch.h"
#include "TestPartObject.h"
#include "Client_Resources.h"
#include "ComConstantBuffer.h"
#include "ComStaticModelInstance.h"
#include "ComSocket.h"
#include "AnimationObject.h"
#include "ComAnimator.h"
#include "Resources.h"
#include "GameInstance.h"
NS_USING(Client)



CTestPartObject::CTestPartObject()
	: CGameObject{}
{
}

CTestPartObject::~CTestPartObject()
{
}

void CTestPartObject::UpdateGUI()
{
	CGameObject::UpdateGUI();

	if (ImGui::Button("NEW CLEAR HEACK")) {
		m_bAttach = false;
	}

}

HRESULT CTestPartObject::InitializePrototype(void* pArg)
{

	m_pResVertexNonAnimShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_PartObject");
	if (FAILED(m_pResVertexNonAnimShader->Load()))
	{
		return E_FAIL;
	}
	m_pResPixelNonAnimShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_PartObject");
	if (FAILED(m_pResPixelNonAnimShader->Load()))
	{
		return E_FAIL;
	}


	return S_OK;
}

HRESULT CTestPartObject::Initialize(void* pArg)
{
	if (FAILED(CGameObject::Initialize(pArg)))
	{
		return E_FAIL;
	}

	CTestPartObject::DESC* pDesc = reinterpret_cast<CTestPartObject::DESC*>(pArg);
	auto& GroupTag = pDesc->sGroupTag;
	auto& ResTag = pDesc->sResTag;
	m_hOwner = pDesc->hOwner;
	m_iBoneIndex = pDesc->iBoneIndex;
	m_vBoneOffset = pDesc->vBoneOffset;
	GetTransform().SetPosition(m_vBoneOffset);
	//cbuffer CB_GPU_PART_ATTACHMENT : register(b9)
	//{
	//	uint gParentInstanceIndex;
	//	uint gParentBoneIndex;
	//	float2 gPartAttachmentPadding;
	//};


	{
		CComConstantBuffer::DESC Desc{};
		Desc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_OBJECT };
		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ConstantBuffer", "ComCBufferPerObject", &Desc, &m_pComCBufferPerObject)))
		{
			return E_FAIL;
		};
	}
	{
		CComConstantBuffer::DESC Desc{};
		Desc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_PART_ATTACHMENT };
		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ConstantBuffer", "ComCBufferATTACTHObject", &Desc, &m_pComCBufferPartObject)))
		{
			return E_FAIL;
		};
	}

	
	{
		CComStaticModelInstance::DESC Desc{};
		//Desc.sGroupTag = "TEST";
		//Desc.sResTag = "Static_Model_Resource";
		Desc.sGroupTag = GroupTag;
		Desc.sResTag = ResTag;

		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_StaticModelInstance", "ComCModelIntance", &Desc, &m_pComModelInstance)))
		{
			return E_FAIL;
		};
	}

	{
		CComSocket::DESC des{};
		des.m_pOwner = m_hOwner;
		des.sModelInstanceName = "ComCModelIntance";
		des.sAnimationName = "ComCModelAnimator";
		des.iBoneIndex = m_iBoneIndex;
		des.m_fOffset = {0.f,0.f,0.f,0.f};
		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_Socket", "ComSocket", &des, &m_pSocket)))
		{
			return E_FAIL;
		};

		m_sAnimGetID = des.sAnimationName;
		
	}



	return S_OK;
}

void CTestPartObject::PriorityUpdate(E::_float fTimeDelta)
{
}

void CTestPartObject::Update(E::_float fTimeDelta)
{

}

void CTestPartObject::LateUpdate(E::_float fTimeDelta)
{
	ZoneScopedN("LateUpdate TestPartObject");
	auto pAnim = CGameInstance::Get().GetGameObjectByHandle(m_hOwner)->GetComponent<CComAnimator>(m_sAnimGetID);
	auto CurAnim = pAnim->GetCurAnimState();
	
	_float4x4 Dummy;
	m_pSocket->Get_Socket_MatrixAtPose(CurAnim.iAnimIndex, CurAnim.fTrackPosition,Dummy);


	const auto* pOwner = CGameInstance::Get().GetGameObjectByHandle(m_hOwner);

	const _matrix ownerWorld = pOwner->GetTransform().GetLoadedCombinedWorldMatrix();

	const _matrix socketWorld = XMLoadFloat4x4(&Dummy) * ownerWorld;

	XMStoreFloat4x4(&socketWorldFloat, socketWorld);

	//CGameInstance::Get().GetDbgLineRender()->SetColor({ 0.f, 1.f, 0.f, 1.f });
	//CGameInstance::Get().GetDbgLineRender()->AddBox({ 0.001f , 0.001f , 0.001f }, XMLoadFloat4x4(&socketWorldFloat));

	E::GPU_PART_INSTANCE_DATA instanceData{};
	if (SUCCEEDED(BuildPartInstanceData(instanceData)))CGameInstance::Get().Add_Part_Instance(m_pComModelInstance, instanceData);



	GetTransform().Update();

	//CGameInstance::Get().AddRenderObject(RENDERGROUP::NONBLEND, this);

}

HRESULT CTestPartObject::Render(ID3D11DeviceContext* pContext,  const E::RENDER_CTX& ctx)
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

	{
		E::CB_PART_ATTACHMENT cbAttachmentObject{};
		cbAttachmentObject.m_preTransform = m_pComModelInstance->GetModel()->Get_PreTransformMatrix();
		cbAttachmentObject.gParentBoneIndex = m_iBoneIndex;
		cbAttachmentObject.gParentInstanceIndex = CGameInstance::Get().GetGameObjectByHandleT<CAnimationObject>(m_hOwner)->GetInstanceModelNum();


		if (FAILED(m_pComCBufferPartObject->MapDiscard(pContext, &cbAttachmentObject, sizeof(cbAttachmentObject))))
		{
			return E_FAIL;
		}
		pContext->VSSetConstantBuffers(9, 1, m_pComCBufferPartObject->GetAdressOfBuffer());
		pContext->PSSetConstantBuffers(9, 1, m_pComCBufferPartObject->GetAdressOfBuffer());
	}

	if (FAILED(BindParentAnimationBuffers(pContext)))
	{
		return E_FAIL;
	}

	const auto& vs = m_pResVertexNonAnimShader;
	const auto& ps = m_pResPixelNonAnimShader;

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
			m_pComModelInstance->Bind_Textures(pContext, i);
			m_pComModelInstance->Bind_Materials(pContext, { 1.f, 1.f, 1.f }, 0.f, { 1.f, 1.f, 1.f }, 0.f, 1.f);	// EmissiveColor -> EmissiveIntensity -> Alpha 순
		}

		pContext->DrawIndexed(viBuffer->GetNumIndices(),  0, 0);
	}


	return S_OK;
}

HRESULT  CTestPartObject::Render_Instanced(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx, const E::MODEL_INSTANCE_BATCH& Batch) 
{
	ZoneScopedN("Render PartObject");
	const uint32_t iInstanceCount = static_cast<uint32_t>(Batch.PartInstances.size());

	if (iInstanceCount == 0)
		return S_OK;
	if (FAILED(UpdatePartInstanceBuffer(pContext, Batch.PartInstances)))
		return E_FAIL;

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

	{
		E::CB_PART_ATTACHMENT cbAttachmentObject{};
		cbAttachmentObject.m_preTransform = m_pComModelInstance->GetModel()->Get_PreTransformMatrix();
		cbAttachmentObject.gParentBoneIndex = m_iBoneIndex;
		cbAttachmentObject.gParentInstanceIndex = CGameInstance::Get().GetGameObjectByHandleT<CAnimationObject>(m_hOwner)->GetInstanceModelNum();
		cbAttachmentObject.bPad = {0.f,0.f};
		
		if (FAILED(m_pComCBufferPartObject->MapDiscard(pContext, &cbAttachmentObject, sizeof(cbAttachmentObject))))
		{
			return E_FAIL;
		}
		pContext->VSSetConstantBuffers(9, 1, m_pComCBufferPartObject->GetAdressOfBuffer());
		pContext->PSSetConstantBuffers(9, 1, m_pComCBufferPartObject->GetAdressOfBuffer());
	}

	if (FAILED(BindParentAnimationBuffers(pContext)))
	{
		return E_FAIL;
	}

	const auto& vs = m_pResVertexNonAnimShader;
	const auto& ps = m_pResPixelNonAnimShader;

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
			m_pComModelInstance->Bind_Textures(pContext, i);
			m_pComModelInstance->Bind_Materials(pContext, { 1.f, 1.f, 1.f }, 0.f, { 1.f, 1.f, 1.f }, 0.f, 1.f);	// EmissiveColor -> EmissiveIntensity -> Alpha 순
		}

		pContext->DrawIndexedInstanced(viBuffer->GetNumIndices(), iInstanceCount, 0, 0, 0);
	}


	return S_OK;
}


HRESULT  CTestPartObject::BindParentAnimationBuffers(ID3D11DeviceContext* pContext)
{
	if (pContext == nullptr)
		return E_INVALIDARG;

	auto instanceBuffer = CGameInstance::Get().GetResourceFirst<CResStructuredBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "SBUFFER_ANIMAITON");
	auto finalBoneBuffer = CGameInstance::Get().GetResourceFirst<CResStructuredBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "SBUFFER_FINALBONEMATRIX");
	auto partInstanceBuffer = CGameInstance::Get().GetResourceFirst<CResStructuredBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "SBUFFER_PART_INSTANCE");

	if (!instanceBuffer || !finalBoneBuffer || !partInstanceBuffer)
		return E_FAIL;

	ID3D11ShaderResourceView* instanceSRV = instanceBuffer->GetSRV().Get();
	ID3D11ShaderResourceView* finalBoneSRV = finalBoneBuffer->GetSRV().Get();
	ID3D11ShaderResourceView* partInstanceSRV = partInstanceBuffer->GetSRV().Get();
	if (!instanceSRV || !finalBoneSRV || !partInstanceSRV)
		return E_FAIL;


	pContext->VSSetShaderResources(6, 1, &instanceSRV);
	pContext->VSSetShaderResources(7, 1, &finalBoneSRV);
	pContext->VSSetShaderResources(8, 1, &partInstanceSRV);
	return S_OK;
}

HRESULT CTestPartObject::BuildPartInstanceData(E::GPU_PART_INSTANCE_DATA& outData) const
{
	auto* pOwner = CGameInstance::Get().GetGameObjectByHandleT<CAnimationObject>(m_hOwner);
	if (!pOwner)
		return E_FAIL;
	if (m_bAttach) {

		outData.WorldMatrix = *GetTransform().GetCombinedWorldMatrix();
		outData.bAttach = m_bAttach;
	}
	else {
		outData.WorldMatrix = socketWorldFloat;
		outData.bAttach = m_bAttach;
	}
	outData.iParentInstanceIndex = pOwner->GetInstanceModelNum();
	outData.iParentBoneIndex = m_iBoneIndex;

	return S_OK;
}

HRESULT CTestPartObject::UpdatePartInstanceBuffer(ID3D11DeviceContext* pContext, const std::vector<E::GPU_PART_INSTANCE_DATA>& instances)
{
	if (instances.empty() || instances.size() > 512)
		return E_FAIL;
	auto buffer = CGameInstance::Get().GetResourceFirst<CResStructuredBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "SBUFFER_PART_INSTANCE");
	if (!buffer || !buffer->GetBuffer())
		return E_FAIL;
	D3D11_BOX box{};
	box.right = static_cast<UINT>(sizeof(E::GPU_PART_INSTANCE_DATA) * instances.size());
	box.bottom = 1;
	box.back = 1;
	pContext->UpdateSubresource(buffer->GetBuffer().Get(), 0, &box, instances.data(), 0, 0);
	return S_OK;
}

E::UPtr<CTestPartObject> CTestPartObject::Create()
{
	auto pInstance = E::ToUPtr(new CTestPartObject{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CTestPartObject");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CTestPartObject::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CTestPartObject{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CTestPartObject");
		return nullptr;
	}

	return pInstance;
}

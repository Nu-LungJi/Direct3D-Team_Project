#include "pch.h"
#include "TestModel.h"
#include "Client_Resources.h"
#include "ComConstantBuffer.h"
#include "ComModelInstance.h"
#include "ComAnimator.h"
#include "Resources.h"
#include "GameInstance.h"
NS_USING(Client)

CTestModel::CTestModel()
	: CGameObject{}
{
}

CTestModel::~CTestModel()
{
}

void CTestModel::UpdateGUI()
{
	CGameObject::UpdateGUI();

}

HRESULT CTestModel::InitializePrototype(void* pArg)
{

	m_pResVertexNonAnimShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelNonAnim");
	//m_pResVertexShader = CResVertexShader::Create("./ShaderFiles/Shader_VtxNorTex.hlsl");
	if (FAILED(m_pResVertexNonAnimShader->Load()))
	{
		return E_FAIL;
	}
	m_pResPixelNonAnimShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_TestModelNonAnim");
	//m_pResPixelShader = CResPixelShader::Create("./ShaderFiles/Shader_VtxNorTex.hlsl");
	if (FAILED(m_pResPixelNonAnimShader->Load()))
	{
		return E_FAIL;
	}

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


	m_pResSamplerState = CGameInstance::Get().GetResourceFirst<CResSamplerState>(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_SS_LINEAR_WRAP);
	if (!m_pResSamplerState)
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

	

	return S_OK;
}

void CTestModel::PriorityUpdate(E::_float fTimeDelta)
{
}

void CTestModel::Update(E::_float fTimeDelta)
{

	if(m_pComModelInstance->GetModel()->GetAnimations().size() != 0)
		m_pModelAnimator->Update(fTimeDelta);
}

void CTestModel::LateUpdate(E::_float fTimeDelta)
{
	GetTransform().Update();
	CGameInstance::Get().AddRenderObject(RENDERGROUP::NONBLEND, this);
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
	}
	const auto& vs = m_pResVertexShader;
		//!m_pComModelInstance->GetModel()->GetAnimations().empty()
		//? m_pResVertexShader
		//: m_pResVertexNonAnimShader;
	
	const auto& ps = m_pResPixelShader;
		//!m_pComModelInstance->GetModel()->GetAnimations().empty()
		//? m_pResPixelShader
		//: m_pResPixelNonAnimShader;


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
			SPtr<CResTexture2D> DiffuseTexture = E::CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_DIFFUSE");
			if (auto Resource = m_pComModelInstance->Get_MeshTexture(i, AI_TEXTURE_TYPE::aiTextureType_DIFFUSE, 0)) {
				DiffuseTexture = Resource;
			}
			pContext->PSSetShaderResources(0, 1, DiffuseTexture->GetSRV().GetAddressOf());

			SPtr<CResTexture2D> NormalTexture = E::CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_NORMAL");
			if (auto Resource = m_pComModelInstance->Get_MeshTexture(i, AI_TEXTURE_TYPE::aiTextureType_NORMALS, 0)) {
				NormalTexture = Resource;
			}
			pContext->PSSetShaderResources(1, 1, NormalTexture->GetSRV().GetAddressOf());

			SPtr<CResTexture2D> SMROTexture = E::CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_SMRO");
			if (auto Resource = m_pComModelInstance->Get_MeshTexture(i, AI_TEXTURE_TYPE::aiTextureType_METALNESS, 0)) {
				SMROTexture = Resource;
			}
			pContext->PSSetShaderResources(2, 1, SMROTexture->GetSRV().GetAddressOf());

			SPtr<CResTexture2D> EmissiveTexture = E::CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_EMISSIVE");
			if (auto Resource = m_pComModelInstance->Get_MeshTexture(i, AI_TEXTURE_TYPE::aiTextureType_EMISSIVE, 0)) {
				EmissiveTexture = Resource;
			}
			pContext->PSSetShaderResources(3, 1, EmissiveTexture->GetSRV().GetAddressOf());
		}
		{
			auto MaterialConstantBuffer = E::CGameInstance::Get().GetResourceFirst<E::CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "CB_MATERIAL");
			D3D11_MAPPED_SUBRESOURCE MRES;
			if (SUCCEEDED(pContext->Map(MaterialConstantBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MRES)))
			{
				CB_MATERIAL   CMMAT;
				CMMAT.AlbedoColor			= m_fAlbedoColor;

				CMMAT.NormalIntensity		= m_fNormalIntensity;
				CMMAT.RoughnessIntensity	= m_fRoughnessIntensity;
				CMMAT.MetallicIntensity		= m_fMetallicIntensity;
				CMMAT.AmbientIntensity		= m_fAmbientIntensity;
				CMMAT.SpecularIntensity		= m_fSpecularIntensity;

				CMMAT.EmissiveColor			= m_fEmissiveColor;
				CMMAT.EmissiveIntensity		= m_fEmissiveIntensity;

				memcpy(MRES.pData, &CMMAT, sizeof(CB_MATERIAL));
				pContext->Unmap(MaterialConstantBuffer->GetCBuffer().Get(), 0);
			}
			pContext->PSSetConstantBuffers(3, 1, MaterialConstantBuffer->GetCBuffer().GetAddressOf());
		}

		pContext->DrawIndexed(viBuffer->GetNumIndices(), 0, 0);
	}

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

#include "pch.h"
#include "DecalVolume.h"
#include "ComConstantBuffer.h"
#include "GameInstance.h"
#include "Resources.h"

NS_USING(Engine)
namespace
{
	constexpr UINT DECAL_CONSTANT_BUFFER_SLOT = 11;
}


CDecalVolume::CDecalVolume()
	: CGameObject{}
{
}

CDecalVolume::CDecalVolume(const CDecalVolume& Prototype)
	: CGameObject{ Prototype }
	, m_pCubeBuffer{ Prototype.m_pCubeBuffer }
	, m_pVertexShader{ Prototype.m_pVertexShader }
	, m_pPixelShader{ Prototype.m_pPixelShader }
	, m_pLinearClampSampler{ Prototype.m_pLinearClampSampler }
{
}

CDecalVolume::~CDecalVolume()
{
}

HRESULT CDecalVolume::InitializePrototype(void* pArg)
{
	auto& gameInstance = CGameInstance::Get();

	if (!gameInstance.GetResourceFirst<CResCubeColBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "VIBuffer_DecalVolume"))
	{
		auto buffer = gameInstance.AddResource(TAG_RES_GRP_PERMANENT_BUFFER, "VIBuffer_DecalVolume", CResCubeColBuffer::Create());
		if (!buffer || FAILED(buffer->Load()))
			return E_FAIL;
	}

	if (!gameInstance.GetResourceFirst<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_DecalVolume"))
	{
		auto shader = gameInstance.AddResourceT<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_DecalVolume", "./ShaderFiles/Decal/Shader_DecalVolume.hlsl");
		if (!shader || FAILED(shader->Load()))
			return E_FAIL;
	}

	if (!gameInstance.GetResourceFirst<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_DecalVolume"))
	{
		auto shader = gameInstance.AddResourceT<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_DecalVolume", "./ShaderFiles/Decal/Shader_DecalVolume.hlsl");
		if (!shader || FAILED(shader->Load()))
			return E_FAIL;
	}

	if (!gameInstance.GetResourceFirst<CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "CB_DecalVolume"))
	{
		auto buffer = gameInstance.AddResource(TAG_RES_GRP_PERMANENT_BUFFER, "CB_DecalVolume", CResCBuffer::Create());
		CResCBuffer::CBUFFER_DESC desc{ .byteWidth = sizeof(CB_DECAL_VOLUME) };
		if (!buffer || FAILED(buffer->Load(desc)))
			return E_FAIL;
	}

	m_pCubeBuffer = gameInstance.GetResourceFirst<CResCubeColBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "VIBuffer_DecalVolume");
	m_pVertexShader = gameInstance.GetResourceFirst<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_DecalVolume");
	m_pPixelShader = gameInstance.GetResourceFirst<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_DecalVolume");
	m_pLinearClampSampler = gameInstance.GetResourceFirst<CResSamplerState>(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_SS_LINEAR_CLAMP);

	if (!m_pCubeBuffer || !m_pVertexShader || !m_pPixelShader || !m_pLinearClampSampler)
		return E_FAIL;

	return S_OK;
}

HRESULT CDecalVolume::Initialize(void* pArg)
{
	if (nullptr == pArg)
		return E_FAIL;

	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	const auto* pDesc = static_cast<DECAL_VOLUME_DESC*>(pArg);

	GetTransform().SetPosition(pDesc->vPosition);
	GetTransform().SetRotationEuler(pDesc->vRotation);
	GetTransform().SetScale(pDesc->vScale);

	m_vAlbedoColor = pDesc->vAlbedoColor;
	m_vEmissiveColor = pDesc->vEmissiveColor;
	m_fEmissiveIntensity = std::max(0.f, pDesc->fEmissiveIntensity);
	m_fOpacity = std::clamp(pDesc->fOpacity, 0.f, 1.f);
	m_fNormalThreshold = std::clamp(pDesc->fNormalThreshold, 0.f, 0.999f);
	m_fEdgeSoftness = std::clamp(pDesc->fEdgeSoftness, 0.001f, 0.49f);

	CComConstantBuffer::DESC objectBufferDesc{};
	objectBufferDesc.cBufferId = {
		TAG_RES_GRP_PERMANENT_BUFFER,
		TAG_RES_CBUFFER_OBJECT
	};
	if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ConstantBuffer", "ComCBufferPerObject", &objectBufferDesc, &m_pComCBufferPerObject)))
		return E_FAIL;

	CComConstantBuffer::DESC decalBufferDesc{};
	decalBufferDesc.cBufferId = {
		TAG_RES_GRP_PERMANENT_BUFFER,
		"CB_DecalVolume"
	};
	if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ConstantBuffer", "ComCBufferDecal", &decalBufferDesc, &m_pComCBufferDecal)))
		return E_FAIL;

	if (pDesc->sTextureGroup.hash != 0 && pDesc->sMaskTextureTag.hash != 0)
		SetMaskTexture(pDesc->sTextureGroup, pDesc->sMaskTextureTag);

	if (!m_pMaskTexture)
	{
		SetMaskTexture("DEFAULT_TEXTURE", "TEX_DEFAULT_WHITE");
	}

	if (!m_pMaskTexture)
		return E_FAIL;

	return S_OK;
}

HRESULT CDecalVolume::SetMaskTexture(const StringID& textureGroup, const StringID& textureTag)
{
	auto texture = CGameInstance::Get().GetResourceFirst<CResTexture2D>(textureGroup, textureTag);
	if (!texture)
		return E_FAIL;

	m_pMaskTexture = std::move(texture);
	m_sTextureGroup = textureGroup;
	m_sMaskTextureTag = textureTag;
	return S_OK;
}

const _string& CDecalVolume::GetMaskTexturePath() const
{
	static const _string emptyPath{};
	return m_pMaskTexture ? m_pMaskTexture->GetPath() : emptyPath;
}

void CDecalVolume::LateUpdate(_float fTimeDelta)
{
	__super::LateUpdate(fTimeDelta);
	GetTransform().Update();

	CGameInstance::Get().AddRenderObject(RENDERGROUP::DECAL, this);
}

HRESULT CDecalVolume::Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx)
{
	if (!pContext || !m_pComCBufferPerObject || !m_pComCBufferDecal ||
		!m_pCubeBuffer || !m_pVertexShader || !m_pPixelShader ||
		!m_pLinearClampSampler || !m_pMaskTexture)
		return E_FAIL;

	const _matrix world = GetTransform().GetLoadedCombinedWorldMatrix();

	CB_PER_OBJECT perObject{};
	XMStoreFloat4x4(&perObject.matWorld, world);
	XMStoreFloat4x4(&perObject.matWVP, world * ctx.matViewProj);

	if (FAILED(m_pComCBufferPerObject->MapDiscard(pContext, &perObject, sizeof(perObject))))
		return E_FAIL;

	CB_DECAL_VOLUME decalBuffer{};
	XMStoreFloat4x4(&decalBuffer.matInvWorld, XMMatrixInverse(nullptr, world));
	decalBuffer.vAlbedoColor = m_vAlbedoColor;
	decalBuffer.vEmissiveColorIntensity = {
		m_vEmissiveColor.x,
		m_vEmissiveColor.y,
		m_vEmissiveColor.z,
		m_fEmissiveIntensity
	};
	decalBuffer.vParams = {
		m_fOpacity,
		m_fNormalThreshold,
		m_fEdgeSoftness,
		0.f
	};

	if (FAILED(m_pComCBufferDecal->MapDiscard(pContext, &decalBuffer, sizeof(decalBuffer))))
		return E_FAIL;

	pContext->IASetInputLayout(m_pVertexShader->GetInputLayout().Get());
	pContext->VSSetShader(m_pVertexShader->GetVertexShader().Get(), nullptr, 0);
	pContext->PSSetShader(m_pPixelShader->GetPixelShader().Get(), nullptr, 0);

	ID3D11Buffer* vertexBuffer = m_pCubeBuffer->GetVertexBuffer().Get();
	const UINT stride = m_pCubeBuffer->GetVertexStride();
	const UINT offset = 0;
	pContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
	pContext->IASetIndexBuffer(m_pCubeBuffer->GetIndexBuffer().Get(), m_pCubeBuffer->GetIndexFormat(), 0);
	pContext->IASetPrimitiveTopology(m_pCubeBuffer->GetPrimitiveType());

	ID3D11Buffer* perObjectBuffer = m_pComCBufferPerObject->GetBuffer();
	ID3D11Buffer* decalConstantBuffer = m_pComCBufferDecal->GetBuffer();
	pContext->VSSetConstantBuffers(ETOUI(B_SLOTNUMBER::PER_OBJECT), 1, &perObjectBuffer);
	pContext->PSSetConstantBuffers(ETOUI(B_SLOTNUMBER::PER_OBJECT), 1, &perObjectBuffer);
	pContext->PSSetConstantBuffers(DECAL_CONSTANT_BUFFER_SLOT, 1, &decalConstantBuffer);

	ID3D11ShaderResourceView* maskSRV = m_pMaskTexture->GetSRV().Get();
	pContext->PSSetShaderResources(2, 1, &maskSRV);
	ID3D11SamplerState* sampler = m_pLinearClampSampler->GetSamplerState().Get();
	pContext->PSSetSamplers(1, 1, &sampler);

	pContext->DrawIndexed(m_pCubeBuffer->GetNumIndices(), 0, 0);

	ID3D11ShaderResourceView* nullSRV = nullptr;
	ID3D11Buffer* nullConstantBuffer = nullptr;
	pContext->PSSetConstantBuffers(DECAL_CONSTANT_BUFFER_SLOT, 1, &nullConstantBuffer);

	pContext->PSSetShaderResources(2, 1, &nullSRV);

	return S_OK;
}

void CDecalVolume::UpdateGUI()
{
	__super::UpdateGUI();
	ImGui::ColorEdit4("Decal Albedo", reinterpret_cast<float*>(&m_vAlbedoColor));
	ImGui::ColorEdit3("Decal Emissive", reinterpret_cast<float*>(&m_vEmissiveColor));
	ImGui::DragFloat("Emissive Intensity", &m_fEmissiveIntensity, 0.1f, 0.f, 100.f);
	ImGui::SliderFloat("Decal Opacity", &m_fOpacity, 0.f, 1.f);
	ImGui::SliderFloat("Normal Threshold", &m_fNormalThreshold, 0.f, 0.999f);
	ImGui::SliderFloat("Edge Softness", &m_fEdgeSoftness, 0.001f, 0.49f);
}

UPtr<CDecalVolume> CDecalVolume::Create()
{
	auto pInstance = ToUPtr(new CDecalVolume{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Create: CDecalVolume");
		return nullptr;
	}
	return pInstance;
}

UPtr<CPrototype> CDecalVolume::Clone(void* pArg)
{
	auto pInstance = ToUPtr(new CDecalVolume{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Clone: CDecalVolume");
		return nullptr;
	}
	return pInstance;
}

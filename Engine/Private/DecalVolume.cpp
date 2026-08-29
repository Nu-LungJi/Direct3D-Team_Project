#include "pch.h"
#include "DecalVolume.h"
#include "ComConstantBuffer.h"
#include "GameInstance.h"
#include "Resources.h"

NS_USING(Engine)

namespace
{
	constexpr UINT DECAL_CONSTANT_BUFFER_SLOT = 11;
	const std::vector<CDecalMaterial::PARAMETER_DESC> EMPTY_PARAMETERS{};
	const StringID EMPTY_STRING_ID{};
	const _string EMPTY_PATH{};
}

CDecalVolume::CDecalVolume()
	: CGameObject{}
{
}

CDecalVolume::CDecalVolume(const CDecalVolume& prototype)
	: CGameObject{ prototype }
	, m_pCubeBuffer{ prototype.m_pCubeBuffer }
	, m_pVertexShader{ prototype.m_pVertexShader }
	, m_pLinearWrapSampler{ prototype.m_pLinearWrapSampler }
	, m_pLinearClampSampler{ prototype.m_pLinearClampSampler }
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
		auto shader = gameInstance.AddResourceT<CResVertexShader>(
			TAG_RES_GRP_PERMANENT_SHADER,
			"VS_DecalVolume",
			"./ShaderFiles/Decal/Shader_DecalVolume.hlsl");
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
	m_pLinearWrapSampler = gameInstance.GetResourceFirst<CResSamplerState>(
		TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_SS_LINEAR_WRAP);
	m_pLinearClampSampler = gameInstance.GetResourceFirst<CResSamplerState>(
		TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_SS_LINEAR_CLAMP);

	if (!m_pCubeBuffer || !m_pVertexShader || !m_pLinearWrapSampler || !m_pLinearClampSampler)
		return E_FAIL;

	return S_OK;
}

HRESULT CDecalVolume::Initialize(void* pArg)
{
	if (!pArg || FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	const auto* desc = static_cast<DECAL_VOLUME_DESC*>(pArg);
	GetTransform().SetPosition(desc->vPosition);
	GetTransform().SetRotationEuler(desc->vRotation);
	GetTransform().SetScale(desc->vScale);

	m_fOpacity = std::clamp(desc->fOpacity, 0.f, 1.f);
	m_fNormalThreshold = std::clamp(desc->fNormalThreshold, 0.f, 0.999f);
	m_fEdgeSoftness = std::clamp(desc->fEdgeSoftness, 0.001f, 0.49f);

	CComConstantBuffer::DESC objectBufferDesc{};
	objectBufferDesc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_OBJECT };
	if (FAILED(AddComponentFromProto(
		"PERMANENT", "Prototype_Component_ConstantBuffer", "ComCBufferPerObject",
		&objectBufferDesc, &m_pComCBufferPerObject)))
		return E_FAIL;

	CComConstantBuffer::DESC decalBufferDesc{};
	decalBufferDesc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, "CB_DecalVolume" };
	if (FAILED(AddComponentFromProto(
		"PERMANENT", "Prototype_Component_ConstantBuffer", "ComCBufferDecal",
		&decalBufferDesc, &m_pComCBufferDecal)))
		return E_FAIL;

	if (FAILED(SetMaterial(desc->sMaterialPath.empty() ? DEFAULT_MATERIAL_PATH : desc->sMaterialPath)))
		return E_FAIL;

	if (desc->sTextureGroup.hash != 0 && desc->sMaskTextureTag.hash != 0)
	{
		if (FAILED(SetMaskTexture(desc->sTextureGroup, desc->sMaskTextureTag)))
			return E_FAIL;
	}
	else
	{
		const bool materialHasMask = std::any_of(
			m_Material->GetTextures().begin(),
			m_Material->GetTextures().end(),
			[](const CDecalMaterial::TEXTURE_DESC& texture)
			{
				return texture.slot == CDecalMaterial::TEXTURE_SLOT_BEGIN;
			});
		if (!materialHasMask)
			SetMaskTexture("DEFAULT_TEXTURE", "TEX_DEFAULT_WHITE");
	}

	return S_OK;
}

HRESULT CDecalVolume::SetMaterial(const _string& materialPath)
{
	auto material = CDecalMaterial::LoadShared(materialPath);
	if (!material)
		return E_FAIL;

	m_Material = std::move(material);
	m_MaterialPath = m_Material->GetPath();
	m_MaterialParameters = m_Material->GetDefaultParameters();
	return S_OK;
}

const std::vector<CDecalMaterial::PARAMETER_DESC>& CDecalVolume::GetMaterialParameters() const
{
	return m_Material ? m_Material->GetParameters() : EMPTY_PARAMETERS;
}

_float* CDecalVolume::GetMaterialParameterData(const _string& name)
{
	if (!m_Material)
		return nullptr;
	const auto* parameter = m_Material->FindParameter(name);
	return parameter ? m_MaterialParameters.data() + parameter->offset : nullptr;
}

const _float* CDecalVolume::GetMaterialParameterData(const _string& name) const
{
	return const_cast<CDecalVolume*>(this)->GetMaterialParameterData(name);
}

HRESULT CDecalVolume::SetMaterialParameter(const _string& name, const _float* values, size_t count)
{
	if (!values || !m_Material)
		return E_FAIL;
	const auto* parameter = m_Material->FindParameter(name);
	if (!parameter || count < parameter->count)
		return E_FAIL;
	std::copy_n(values, parameter->count, m_MaterialParameters.data() + parameter->offset);
	return S_OK;
}

HRESULT CDecalVolume::SetTextureOverride(UINT slot, const StringID& textureGroup, const StringID& textureTag)
{
	if (slot < CDecalMaterial::TEXTURE_SLOT_BEGIN || slot > CDecalMaterial::TEXTURE_SLOT_END)
		return E_INVALIDARG;

	auto texture = CGameInstance::Get().GetResourceFirst<CResTexture2D>(textureGroup, textureTag);
	if (!texture)
		return E_FAIL;

	m_TextureOverrides[slot] = std::move(texture);
	m_TextureOverrideGroups[slot] = textureGroup;
	m_TextureOverrideTags[slot] = textureTag;
	return S_OK;
}

void CDecalVolume::ClearTextureOverride(UINT slot)
{
	if (slot >= m_TextureOverrides.size())
		return;
	m_TextureOverrides[slot].reset();
	m_TextureOverrideGroups[slot] = {};
	m_TextureOverrideTags[slot] = {};
}

const StringID& CDecalVolume::GetTextureOverrideGroup(UINT slot) const
{
	return slot < m_TextureOverrideGroups.size() ? m_TextureOverrideGroups[slot] : EMPTY_STRING_ID;
}

const StringID& CDecalVolume::GetTextureOverrideTag(UINT slot) const
{
	return slot < m_TextureOverrideTags.size() ? m_TextureOverrideTags[slot] : EMPTY_STRING_ID;
}

const _string& CDecalVolume::GetTextureOverridePath(UINT slot) const
{
	if (slot >= m_TextureOverrides.size() || !m_TextureOverrides[slot])
		return EMPTY_PATH;
	return m_TextureOverrides[slot]->GetPath();
}

void CDecalVolume::LateUpdate(_float fTimeDelta)
{
	__super::LateUpdate(fTimeDelta);
	m_fTime = std::fmod(m_fTime + std::max(0.f, fTimeDelta), 10000.f);
	GetTransform().Update();
	CGameInstance::Get().AddRenderObject(RENDERGROUP::DECAL, this);
}

HRESULT CDecalVolume::Render(ID3D11DeviceContext* context, const RENDER_CTX& ctx)
{
	if (!context || !m_pComCBufferPerObject || !m_pComCBufferDecal ||
		!m_pCubeBuffer || !m_pVertexShader || !m_pLinearWrapSampler ||
		!m_pLinearClampSampler || !m_Material)
		return E_FAIL;

	const _matrix world = GetTransform().GetLoadedCombinedWorldMatrix();

	CB_PER_OBJECT perObject{};
	XMStoreFloat4x4(&perObject.matWorld, world);
	XMStoreFloat4x4(&perObject.matWVP, world * ctx.matViewProj);
	if (FAILED(m_pComCBufferPerObject->MapDiscard(context, &perObject, sizeof(perObject))))
		return E_FAIL;

	CB_DECAL_VOLUME decalBuffer{};
	XMStoreFloat4x4(&decalBuffer.matInvWorld, XMMatrixInverse(nullptr, world));
	decalBuffer.vProjectionParams = {
		m_fOpacity,
		m_fNormalThreshold,
		m_fEdgeSoftness,
		m_fTime
	};
	std::memcpy(
		decalBuffer.vMaterialParams.data(),
		m_MaterialParameters.data(),
		sizeof(m_MaterialParameters));
	if (FAILED(m_pComCBufferDecal->MapDiscard(context, &decalBuffer, sizeof(decalBuffer))))
		return E_FAIL;

	context->IASetInputLayout(m_pVertexShader->GetInputLayout().Get());
	context->VSSetShader(m_pVertexShader->GetVertexShader().Get(), nullptr, 0);
	if (FAILED(m_Material->Bind(context)))
		return E_FAIL;

	ID3D11Buffer* vertexBuffer = m_pCubeBuffer->GetVertexBuffer().Get();
	const UINT stride = m_pCubeBuffer->GetVertexStride();
	const UINT offset = 0;
	context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
	context->IASetIndexBuffer(m_pCubeBuffer->GetIndexBuffer().Get(), m_pCubeBuffer->GetIndexFormat(), 0);
	context->IASetPrimitiveTopology(m_pCubeBuffer->GetPrimitiveType());

	ID3D11Buffer* perObjectBuffer = m_pComCBufferPerObject->GetBuffer();
	ID3D11Buffer* decalConstantBuffer = m_pComCBufferDecal->GetBuffer();
	context->VSSetConstantBuffers(ETOUI(B_SLOTNUMBER::PER_OBJECT), 1, &perObjectBuffer);
	context->PSSetConstantBuffers(ETOUI(B_SLOTNUMBER::PER_OBJECT), 1, &perObjectBuffer);
	context->PSSetConstantBuffers(DECAL_CONSTANT_BUFFER_SLOT, 1, &decalConstantBuffer);

	for (UINT slot = CDecalMaterial::TEXTURE_SLOT_BEGIN; slot <= CDecalMaterial::TEXTURE_SLOT_END; ++slot)
	{
		if (!m_TextureOverrides[slot])
			continue;
		ID3D11ShaderResourceView* srv = m_TextureOverrides[slot]->GetSRV().Get();
		context->PSSetShaderResources(slot, 1, &srv);
	}

	ID3D11SamplerState* wrapSampler = m_pLinearWrapSampler->GetSamplerState().Get();
	ID3D11SamplerState* clampSampler = m_pLinearClampSampler->GetSamplerState().Get();
	context->PSSetSamplers(0, 1, &wrapSampler);
	context->PSSetSamplers(1, 1, &clampSampler);

	context->DrawIndexed(m_pCubeBuffer->GetNumIndices(), 0, 0);

	m_Material->Unbind(context);
	ID3D11ShaderResourceView* nullSRVs[CDecalMaterial::TEXTURE_SLOT_END - CDecalMaterial::TEXTURE_SLOT_BEGIN + 1]{};
	context->PSSetShaderResources(
		CDecalMaterial::TEXTURE_SLOT_BEGIN,
		static_cast<UINT>(std::size(nullSRVs)),
		nullSRVs);
	ID3D11Buffer* nullConstantBuffer = nullptr;
	context->PSSetConstantBuffers(DECAL_CONSTANT_BUFFER_SLOT, 1, &nullConstantBuffer);
	return S_OK;
}

void CDecalVolume::UpdateGUI()
{
	__super::UpdateGUI();
	ImGui::Text("Material: %s", m_MaterialPath.c_str());
	ImGui::SliderFloat("Decal Opacity", &m_fOpacity, 0.f, 1.f);
	ImGui::SliderFloat("Normal Threshold", &m_fNormalThreshold, 0.f, 0.999f);
	ImGui::SliderFloat("Edge Softness", &m_fEdgeSoftness, 0.001f, 0.49f);

	if (!m_Material)
		return;

	ImGui::Separator();
	ImGui::TextDisabled("Material Parameters");
	for (const auto& parameter : m_Material->GetParameters())
	{
		_float* value = m_MaterialParameters.data() + parameter.offset;
		switch (parameter.type)
		{
		case DECAL_PARAMETER_TYPE::COLOR3:
			ImGui::ColorEdit3(parameter.name.c_str(), value);
			break;
		case DECAL_PARAMETER_TYPE::COLOR4:
			ImGui::ColorEdit4(parameter.name.c_str(), value);
			break;
		case DECAL_PARAMETER_TYPE::FLOAT2:
			ImGui::DragFloat2(parameter.name.c_str(), value, parameter.speed, parameter.minValue, parameter.maxValue);
			break;
		case DECAL_PARAMETER_TYPE::FLOAT3:
			ImGui::DragFloat3(parameter.name.c_str(), value, parameter.speed, parameter.minValue, parameter.maxValue);
			break;
		case DECAL_PARAMETER_TYPE::FLOAT4:
			ImGui::DragFloat4(parameter.name.c_str(), value, parameter.speed, parameter.minValue, parameter.maxValue);
			break;
		default:
			ImGui::DragFloat(parameter.name.c_str(), value, parameter.speed, parameter.minValue, parameter.maxValue);
			break;
		}
	}
}

UPtr<CDecalVolume> CDecalVolume::Create()
{
	auto instance = ToUPtr(new CDecalVolume{});
	if (FAILED(instance->InitializePrototype()))
	{
		MSG_BOX("Failed to Create: CDecalVolume");
		return nullptr;
	}
	return instance;
}

UPtr<CPrototype> CDecalVolume::Clone(void* pArg)
{
	auto instance = ToUPtr(new CDecalVolume{ *this });
	if (FAILED(instance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Clone: CDecalVolume");
		return nullptr;
	}
	return instance;
}


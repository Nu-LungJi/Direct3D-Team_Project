#include "pch.h"
#include "BoostTrailInstancedUI.h"

#include "GameInstance.h"
#include "Level_Defines.h"
#include "Resources.h"

NS_USING(Client)

namespace
{
	constexpr _float FLIPBOOK_PADDING_PIXELS = 2.f;
}

CBoostTrailInstancedUI::CBoostTrailInstancedUI() = default;

HRESULT CBoostTrailInstancedUI::InitializeRenderer(
	size_t maxInstanceCount,
	int weight,
	const std::string& textureTag,
	uint32_t columns,
	uint32_t rows,
	uint32_t atlasSize,
	_bool useTextureAlpha)
{
	if (maxInstanceCount == 0 ||
		maxInstanceCount > static_cast<size_t>(UINT_MAX) ||
		textureTag.empty() || columns == 0 || rows == 0 || atlasSize == 0)
	{
		return E_INVALIDARG;
	}

	E::CUIObject::UIOBJECT_DESC desc{};
	desc.fAlpha = 1.f;
	desc.ResWeight = static_cast<uint32_t>(std::max(0, weight));
	desc.Name = "SpellMiniGame_TrailInstanced_" + textureTag;
	desc.ResTag = textureTag;
	if (FAILED(E::CUIObject::Initialize(&desc)))
		return E_FAIL;

	m_iMaxInstanceCount = maxInstanceCount;
	m_sTextureTag = textureTag;
	m_iColumns = columns;
	m_iRows = rows;
	m_iAtlasSize = atlasSize;
	m_bUseTextureAlpha = useTextureAlpha;
	m_Instances.reserve(maxInstanceCount);
	m_GPUInstances.reserve(maxInstanceCount);

	D3D11_BUFFER_DESC bufferDesc{};
	bufferDesc.ByteWidth = static_cast<UINT>(
		sizeof(GPU_INSTANCE) * maxInstanceCount);
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.StructureByteStride = sizeof(GPU_INSTANCE);

	const auto device = E::CGameInstance::Get().GetGraphicDevice();
	if (!device || FAILED(device->CreateBuffer(
		&bufferDesc,
		nullptr,
		m_pBoostTrailInstanceBuffer.GetAddressOf())))
	{
		return E_FAIL;
	}

	SetInputLcok(true);
	return S_OK;
}

void CBoostTrailInstancedUI::PriorityUpdate(_float)
{
}

void CBoostTrailInstancedUI::Update(_float)
{
}

void CBoostTrailInstancedUI::LateUpdate(_float fTimeDelta)
{
	if (m_Instances.empty())
		return;

	E::CUIObject::LateUpdate(fTimeDelta);
}

HRESULT CBoostTrailInstancedUI::Render(
	ID3D11DeviceContext* pContext,
	const E::RENDER_CTX& ctx)
{
	if (!pContext || m_Instances.empty() ||
		!m_pBoostTrailInstanceBuffer)
	{
		return S_OK;
	}

	const _float2 clientSize =
		E::CGameInstance::Get().GetClientScreenSize();
	const _float padding =
		FLIPBOOK_PADDING_PIXELS / static_cast<_float>(m_iAtlasSize);
	const _float2 uvSize = {
		1.f / static_cast<_float>(m_iColumns) - padding * 2.f,
		1.f / static_cast<_float>(m_iRows) - padding * 2.f
	};

	m_GPUInstances.clear();
	for (const INSTANCE_DESC& instance : m_Instances)
	{
		GPU_INSTANCE gpuInstance{};
		const _float x = instance.Position.x - clientSize.x * 0.5f;
		const _float y = -instance.Position.y + clientSize.y * 0.5f;
		const _matrix world =
			XMMatrixScaling(instance.Size.x, instance.Size.y, 1.f) *
			XMMatrixRotationZ(XMConvertToRadians(instance.Rotation)) *
			XMMatrixTranslation(x, y, 0.f);
		XMStoreFloat4x4(&gpuInstance.World, world);
		gpuInstance.Color = instance.Color;

		const uint32_t frame = instance.Frame %
			(m_iColumns * m_iRows);
		gpuInstance.UVOffset = {
			static_cast<_float>(frame % m_iColumns) /
				static_cast<_float>(m_iColumns) + padding,
			static_cast<_float>(frame / m_iColumns) /
				static_cast<_float>(m_iRows) + padding
		};
		gpuInstance.UVSize = uvSize;
		m_GPUInstances.push_back(gpuInstance);
	}

	D3D11_MAPPED_SUBRESOURCE mapped{};
	if (FAILED(pContext->Map(
		m_pBoostTrailInstanceBuffer.Get(),
		0,
		D3D11_MAP_WRITE_DISCARD,
		0,
		&mapped)))
	{
		return E_FAIL;
	}
	std::memcpy(
		mapped.pData,
		m_GPUInstances.data(),
		sizeof(GPU_INSTANCE) * m_GPUInstances.size());
	pContext->Unmap(m_pBoostTrailInstanceBuffer.Get(), 0);

	const auto& viBuffer = E::CGameInstance::Get().
		GetResourceFirst<E::CResQuadTexBuffer>(
			TAG_RES_GRP_PERMANENT_BUFFER,
			"VIBuffer_QuadTex");
	const auto& vs = E::CGameInstance::Get().
		GetResourceFirst<E::CResVertexShader>(
			TAG_RES_GRP_PERMANENT_SHADER,
			"VS_BoostTrailInstancedUI");
	const auto& ps = E::CGameInstance::Get().
		GetResourceFirst<E::CResPixelShader>(
			TAG_RES_GRP_PERMANENT_SHADER,
			m_bUseTextureAlpha ?
			"PS_ChaserTrailInstancedUI" :
			"PS_BoostTrailInstancedUI");

	pContext->IASetInputLayout(vs->GetInputLayout().Get());
	pContext->VSSetShader(vs->GetVertexShader().Get(), nullptr, 0);
	pContext->PSSetShader(ps->GetPixelShader().Get(), nullptr, 0);

	ID3D11Buffer* vertexBuffers[] = {
		viBuffer->GetVertexBuffer().Get(),
		m_pBoostTrailInstanceBuffer.Get()
	};
	const UINT strides[] = {
		viBuffer->GetVertexStride(),
		static_cast<UINT>(sizeof(GPU_INSTANCE))
	};
	const UINT offsets[] = { 0, 0 };
	pContext->IASetVertexBuffers(0, 2, vertexBuffers, strides, offsets);
	pContext->IASetIndexBuffer(
		viBuffer->GetIndexBuffer().Get(),
		viBuffer->GetIndexFormat(),
		0);
	pContext->IASetPrimitiveTopology(viBuffer->GetPrimitiveType());

	const auto perObject = E::CGameInstance::Get().
		GetResourceFirst<E::CResCBuffer>(
			TAG_RES_GRP_PERMANENT_BUFFER,
			"CB_PerObject");
	D3D11_MAPPED_SUBRESOURCE objectMapped{};
	if (SUCCEEDED(pContext->Map(
		perObject->GetCBuffer().Get(),
		0,
		D3D11_MAP_WRITE_DISCARD,
		0,
		&objectMapped)))
	{
		E::CB_PER_OBJECT cb{};
		XMStoreFloat4x4(&cb.matWVP, ctx.matProj);
		std::memcpy(objectMapped.pData, &cb, sizeof(cb));
		pContext->Unmap(perObject->GetCBuffer().Get(), 0);
	}
	pContext->VSSetConstantBuffers(
		ETOUI(B_SLOTNUMBER::PER_OBJECT),
		1,
		perObject->GetCBuffer().GetAddressOf());

	const std::string currentLevel = _string("LEVEL_") +
		MagicEnumToStringView(static_cast<LEVEL>(
			E::CGameInstance::Get().GetCurrentLevelID())).data();
	const auto& texture = E::CGameInstance::GetConst().
		GetResourceFirst<E::CResTexture2D>(
			currentLevel,
			m_sTextureTag);
	pContext->PSSetShaderResources(
		0,
		1,
		texture->GetSRV().GetAddressOf());

	const auto& sampler = E::CGameInstance::GetConst().
		GetResourceFirst<E::CResSamplerState>(
			TAG_RES_GRP_PERMANENT_STATE,
			TAG_RES_STATE_SS_LINEAR_WRAP);
	pContext->PSSetSamplers(
		0,
		1,
		sampler->GetSamplerState().GetAddressOf());

	const auto& blendState = E::CGameInstance::Get().
		GetResourceFirst<E::CResBlendState>(
			TAG_RES_GRP_PERMANENT_STATE,
			m_bUseTextureAlpha ?
			"BS_ALPHA_BLEND" :
			"BS_ADDITIVE");
	pContext->OMSetBlendState(
		blendState->GetBlendState().Get(),
		nullptr,
		0xffffffff);

	pContext->DrawIndexedInstanced(
		static_cast<UINT>(viBuffer->GetNumIndices()),
		static_cast<UINT>(m_GPUInstances.size()),
		0,
		0,
		0);

	const auto& alphaBlend = E::CGameInstance::Get().
		GetResourceFirst<E::CResBlendState>(
			TAG_RES_GRP_PERMANENT_STATE,
			"BS_ALPHA_BLEND");
	pContext->OMSetBlendState(
		alphaBlend->GetBlendState().Get(),
		nullptr,
		0xffffffff);
	return S_OK;
}

void CBoostTrailInstancedUI::ClearInstances()
{
	m_Instances.clear();
}

void CBoostTrailInstancedUI::AddInstance(const INSTANCE_DESC& desc)
{
	if (m_Instances.size() < m_iMaxInstanceCount)
		m_Instances.push_back(desc);
}

E::UPtr<CBoostTrailInstancedUI> CBoostTrailInstancedUI::Create(
	size_t maxInstanceCount,
	int weight,
	const std::string& textureTag,
	uint32_t columns,
	uint32_t rows,
	uint32_t atlasSize,
	_bool useTextureAlpha)
{
	auto instance = E::UPtr<CBoostTrailInstancedUI>(
		new CBoostTrailInstancedUI());
	if (FAILED(instance->InitializeRenderer(
		maxInstanceCount,
		weight,
		textureTag,
		columns,
		rows,
		atlasSize,
		useTextureAlpha)))
		return nullptr;
	return instance;
}

E::UPtr<E::CPrototype> CBoostTrailInstancedUI::Clone(void*)
{
	return Create(
		m_iMaxInstanceCount,
		GetWeight(),
		m_sTextureTag,
		m_iColumns,
		m_iRows,
		m_iAtlasSize,
		m_bUseTextureAlpha);
}

void CBoostTrailInstancedUI::Free()
{
	m_pBoostTrailInstanceBuffer.Reset();
	m_Instances.clear();
	m_GPUInstances.clear();
	E::CUIObject::Free();
}

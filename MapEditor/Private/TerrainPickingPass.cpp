#include "pch.h"
#include "TerrainPickingPass.h"

#include "GameInstance.h"
#include "ResPixelShader.h"
#include "ResVertexShader.h"
#include "Terrain.h"

NS_USING(Client)

namespace
{
	constexpr char TERRAIN_PICKING_SHADER_GROUP[] = "MAP_EDITOR_SHADER";
	constexpr char TERRAIN_PICKING_VS_TAG[] = "VS_TERRAIN_PICKING";
	constexpr char TERRAIN_PICKING_PS_TAG[] = "PS_TERRAIN_PICKING";

	struct alignas(16) CB_TERRAIN_PICKING
	{
		E::_float4x4 matWorld{};
		E::_float4x4 matWVP{};
	};
}

HRESULT CTerrainPickingPass::Initialize()
{
	m_pDevice = E::CGameInstance::Get().GetGraphicDevice();
	m_pContext = E::CGameInstance::Get().GetGraphicDeviceContext();
	if (!m_pDevice || !m_pContext ||
		FAILED(m_pDevice->CreateDeferredContext(0, m_pPickingContext.GetAddressOf())))
		return E_FAIL;

	const E::_float2 clientSize = E::CGameInstance::Get().GetClientScreenSize();
	if (clientSize.x < 1.f || clientSize.y < 1.f)
		return E_FAIL;
	m_iTargetWidth = static_cast<UINT>(clientSize.x);
	m_iTargetHeight = static_cast<UINT>(clientSize.y);

	D3D11_TEXTURE2D_DESC positionDesc{};
	positionDesc.Width = m_iTargetWidth;
	positionDesc.Height = m_iTargetHeight;
	positionDesc.MipLevels = 1;
	positionDesc.ArraySize = 1;
	positionDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	positionDesc.SampleDesc.Count = 1;
	positionDesc.Usage = D3D11_USAGE_DEFAULT;
	positionDesc.BindFlags = D3D11_BIND_RENDER_TARGET;
	if (FAILED(m_pDevice->CreateTexture2D(&positionDesc, nullptr, m_pPositionTexture.GetAddressOf())) ||
		FAILED(m_pDevice->CreateRenderTargetView(m_pPositionTexture.Get(), nullptr, m_pPositionRTV.GetAddressOf())))
		return E_FAIL;

	D3D11_TEXTURE2D_DESC depthDesc{};
	depthDesc.Width = m_iTargetWidth;
	depthDesc.Height = m_iTargetHeight;
	depthDesc.MipLevels = 1;
	depthDesc.ArraySize = 1;
	depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthDesc.SampleDesc.Count = 1;
	depthDesc.Usage = D3D11_USAGE_DEFAULT;
	depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	if (FAILED(m_pDevice->CreateTexture2D(&depthDesc, nullptr, m_pDepthTexture.GetAddressOf())) ||
		FAILED(m_pDevice->CreateDepthStencilView(m_pDepthTexture.Get(), nullptr, m_pDepthDSV.GetAddressOf())))
		return E_FAIL;

	D3D11_RASTERIZER_DESC rasterizerDesc{};
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.CullMode = D3D11_CULL_BACK;
	rasterizerDesc.DepthClipEnable = TRUE;
	// Scissor Rect 적용
	rasterizerDesc.ScissorEnable = TRUE;
	if (FAILED(m_pDevice->CreateRasterizerState(&rasterizerDesc, m_pScissorRasterizerState.GetAddressOf())))
		return E_FAIL;

	// 1 * 1 스테이징 텍스쳐 3개 생성
	D3D11_TEXTURE2D_DESC readbackDesc{};
	readbackDesc.Width = 1;
	readbackDesc.Height = 1;
	readbackDesc.MipLevels = 1;
	readbackDesc.ArraySize = 1;
	readbackDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	readbackDesc.SampleDesc.Count = 1;
	readbackDesc.Usage = D3D11_USAGE_STAGING;
	readbackDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	for (auto& slot : m_ReadbackSlots)
	{
		if (FAILED(m_pDevice->CreateTexture2D(&readbackDesc, nullptr, slot.texture.GetAddressOf())))
			return E_FAIL;
	}

	D3D11_BUFFER_DESC cbufferDesc{};
	cbufferDesc.ByteWidth = sizeof(CB_TERRAIN_PICKING);
	cbufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	if (FAILED(m_pDevice->CreateBuffer(&cbufferDesc, nullptr, m_pPickingCBuffer.GetAddressOf())))
		return E_FAIL;

	m_pPickingVS = E::CGameInstance::Get().GetResourceFirst<E::CResVertexShader>(TERRAIN_PICKING_SHADER_GROUP, TERRAIN_PICKING_VS_TAG);
	m_pPickingPS = E::CGameInstance::Get().GetResourceFirst<E::CResPixelShader>(TERRAIN_PICKING_SHADER_GROUP, TERRAIN_PICKING_PS_TAG);

	if (!m_pPickingVS || !m_pPickingPS || FAILED(m_pPickingVS->Load()) || FAILED(m_pPickingPS->Load()))
		return E_FAIL;

	return S_OK;
}

std::optional<E::_float3> CTerrainPickingPass::Pick(
	const E::CTerrain& terrain, uint32_t mouseX, uint32_t mouseY)
{
	if (!m_pPickingContext || !m_pContext || mouseX >= m_iTargetWidth || mouseY >= m_iTargetHeight)
		return std::nullopt;

	for (auto& slot : m_ReadbackSlots)
	{
		if (!slot.pending) 
			continue;

		// DO_NOT_WAIT : GPU 작업이 끝나지 않았다면 CPU를 기다리게 하지 않고 즉시 다음 값을 반환
		D3D11_MAPPED_SUBRESOURCE mapped{};
		const HRESULT mapResult = m_pContext->Map(slot.texture.Get(), 0, D3D11_MAP_READ, D3D11_MAP_FLAG_DO_NOT_WAIT, &mapped);

		if (mapResult == DXGI_ERROR_WAS_STILL_DRAWING) 
			continue;

		if (FAILED(mapResult))
		{
			slot.pending = false;
			continue;
		}

		const E::_float4 value = *static_cast<const E::_float4*>(mapped.pData);
		m_pContext->Unmap(slot.texture.Get(), 0);
		m_LastReadbackResult = value.w >= 0.5f ? std::optional<E::_float3>{ E::_float3{ value.x, value.y, value.z } } : std::nullopt;
		m_bHasReadbackResult = true;
		slot.pending = false;
	}

	auto& writeSlot = m_ReadbackSlots[m_iNextReadbackSlot];
	if (!writeSlot.pending && SUCCEEDED(RenderTerrainPosition(terrain, m_pPickingContext.Get(), mouseX, mouseY)))
	{
		D3D11_BOX sourceBox{};
		sourceBox.left = mouseX;
		sourceBox.right = mouseX + 1;
		sourceBox.top = mouseY;
		sourceBox.bottom = mouseY + 1;
		sourceBox.front = 0;
		sourceBox.back = 1;
		m_pPickingContext->CopySubresourceRegion(writeSlot.texture.Get(), 0, 0, 0, 0, m_pPositionTexture.Get(), 0, &sourceBox);
		ComPtr<ID3D11CommandList> commandList{};
		if (SUCCEEDED(m_pPickingContext->FinishCommandList(FALSE, commandList.GetAddressOf())))
		{
			m_pContext->ExecuteCommandList(commandList.Get(), TRUE);
			writeSlot.pending = true;
			m_iNextReadbackSlot = (m_iNextReadbackSlot + 1) % m_ReadbackSlots.size();
		}
	}

	return m_bHasReadbackResult ? m_LastReadbackResult : std::nullopt;
}

HRESULT CTerrainPickingPass::RenderTerrainPosition(const E::CTerrain& terrain, ID3D11DeviceContext* context, uint32_t mouseX, uint32_t mouseY)
{
	auto* camera = E::CGameInstance::Get().GetActiveCamera();
	if (!camera || !context)
		return E_FAIL;

	ID3D11RenderTargetView* rtv = m_pPositionRTV.Get();
	context->OMSetRenderTargets(1, &rtv, m_pDepthDSV.Get());
	const float clearColor[4]{};
	context->ClearRenderTargetView(m_pPositionRTV.Get(), clearColor);
	context->ClearDepthStencilView(m_pDepthDSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

	D3D11_VIEWPORT viewport{};
	viewport.Width = static_cast<float>(m_iTargetWidth);
	viewport.Height = static_cast<float>(m_iTargetHeight);
	viewport.MinDepth = 0.f;
	viewport.MaxDepth = 1.f;
	context->RSSetViewports(1, &viewport);
	// -----------------------------마우스 주변만 Rasterize-----------------------------
	context->RSSetState(m_pScissorRasterizerState.Get());
	constexpr LONG pickingRadius = 1;
	const D3D11_RECT scissorRect
	{
		std::max<LONG>(0, static_cast<LONG>(mouseX) - pickingRadius),
		std::max<LONG>(0, static_cast<LONG>(mouseY) - pickingRadius),
		std::min<LONG>(static_cast<LONG>(m_iTargetWidth), static_cast<LONG>(mouseX) + pickingRadius + 1),
		std::min<LONG>(static_cast<LONG>(m_iTargetHeight), static_cast<LONG>(mouseY) + pickingRadius + 1)
	};
	context->RSSetScissorRects(1, &scissorRect);
	// --------------------------------------------------------------------------------
	context->OMSetBlendState(nullptr, nullptr, 0xffffffff);
	context->OMSetDepthStencilState(nullptr, 0);
	context->GSSetShader(nullptr, nullptr, 0);
	context->HSSetShader(nullptr, nullptr, 0);
	context->DSSetShader(nullptr, nullptr, 0);
	context->IASetInputLayout(m_pPickingVS->GetInputLayout().Get());
	context->VSSetShader(m_pPickingVS->GetVertexShader().Get(), nullptr, 0);
	context->PSSetShader(m_pPickingPS->GetPixelShader().Get(), nullptr, 0);

	CB_TERRAIN_PICKING cbuffer{};
	cbuffer.matWorld = *terrain.GetTransform().GetCombinedWorldMatrix();
	XMStoreFloat4x4(&cbuffer.matWVP,terrain.GetTransform().GetLoadedCombinedWorldMatrix() * camera->GetView() * camera->GetProj());

	D3D11_MAPPED_SUBRESOURCE mapped{};
	if (FAILED(context->Map(m_pPickingCBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
		return E_FAIL;
	memcpy(mapped.pData, &cbuffer, sizeof(cbuffer));
	context->Unmap(m_pPickingCBuffer.Get(), 0);

	ID3D11Buffer* constantBuffer = m_pPickingCBuffer.Get();
	context->VSSetConstantBuffers(0, 1, &constantBuffer);

	for (const auto* chunk : terrain.GetVisibleChunks())
	{
		chunk->Bind(context);
		chunk->Draw(context);
	}
	return S_OK;
}

E::UPtr<CTerrainPickingPass> CTerrainPickingPass::Create()
{
	auto instance = E::ToUPtr(new CTerrainPickingPass{});
	if (FAILED(instance->Initialize()))
		return nullptr;
	return instance;
}

#include "pch.h"
#include "MapPickingPass.h"

#include "GameInstance.h"
#include "MapMeshObject.h"
#include "ResPixelShader.h"
#include "ResStaticModel.h"
#include "ResStaticModelMesh.h"
#include "ResVertexShader.h"

NS_USING(Client)

namespace
{
	constexpr char PICKING_SHADER_GROUP[] = "MAP_EDITOR_SHADER";
	constexpr char PICKING_VS_TAG[] = "VS_MAP_PICKING";
	constexpr char PICKING_PS_TAG[] = "PS_MAP_PICKING";

	struct alignas(16) CB_MAP_PICKING
	{
		E::_float4x4 matWVP{};
		uint32_t pickID = 0;
		uint32_t padding[3]{};
	};
	static_assert(sizeof(CB_MAP_PICKING) % 16 == 0);
}

HRESULT CMapPickingPass::Initialize()
{
	m_pPickingRTV.Reset();
	m_pPickingTexture.Reset();
	m_pDepthDSV.Reset();
	m_pDepthTexture.Reset();
	m_ReadbackTexture.Reset();
	m_pPickingCBuffer.Reset();
	m_pPickingContext.Reset();
	m_pRsState.Reset();

	m_pDevice = E::CGameInstance::Get().GetGraphicDevice();
	m_pContext = E::CGameInstance::Get().GetGraphicDeviceContext();
	if (!m_pDevice || !m_pContext)
		return E_FAIL;

	if (FAILED(m_pDevice->CreateDeferredContext(0, m_pPickingContext.GetAddressOf())))
		return E_FAIL;

	const E::_float2 clientSize = E::CGameInstance::Get().GetClientScreenSize();
	if (clientSize.x < 1.f || clientSize.y < 1.f)
		return E_FAIL;

	m_iTargetWidth = static_cast<UINT>(clientSize.x);
	m_iTargetHeight = static_cast<UINT>(clientSize.y);

	D3D11_TEXTURE2D_DESC pickingDesc{};
	pickingDesc.Width = m_iTargetWidth;
	pickingDesc.Height = m_iTargetHeight;
	pickingDesc.MipLevels = 1;
	pickingDesc.ArraySize = 1;
	pickingDesc.Format = DXGI_FORMAT_R32_UINT;
	pickingDesc.SampleDesc.Count = 1;
	pickingDesc.Usage = D3D11_USAGE_DEFAULT;
	pickingDesc.BindFlags = D3D11_BIND_RENDER_TARGET;
	if (FAILED(m_pDevice->CreateTexture2D(
		&pickingDesc, nullptr, m_pPickingTexture.GetAddressOf())))
	{
		return E_FAIL;
	}
	if (FAILED(m_pDevice->CreateRenderTargetView(
		m_pPickingTexture.Get(), nullptr, m_pPickingRTV.GetAddressOf())))
	{
		return E_FAIL;
	}

	D3D11_TEXTURE2D_DESC depthDesc{};
	depthDesc.Width = m_iTargetWidth;
	depthDesc.Height = m_iTargetHeight;
	depthDesc.MipLevels = 1;
	depthDesc.ArraySize = 1;
	depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthDesc.SampleDesc.Count = 1;
	depthDesc.Usage = D3D11_USAGE_DEFAULT;
	depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	if (FAILED(m_pDevice->CreateTexture2D(
		&depthDesc, nullptr, m_pDepthTexture.GetAddressOf())))
	{
		return E_FAIL;
	}
	if (FAILED(m_pDevice->CreateDepthStencilView(
		m_pDepthTexture.Get(), nullptr, m_pDepthDSV.GetAddressOf())))
	{
		return E_FAIL;
	}

	D3D11_TEXTURE2D_DESC readbackDesc{};
	readbackDesc.Width = 1;
	readbackDesc.Height = 1;
	readbackDesc.MipLevels = 1;
	readbackDesc.ArraySize = 1;
	readbackDesc.Format = DXGI_FORMAT_R32_UINT;
	readbackDesc.SampleDesc.Count = 1;
	readbackDesc.Usage = D3D11_USAGE_STAGING;
	readbackDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	if (FAILED(m_pDevice->CreateTexture2D(
		&readbackDesc, nullptr, m_ReadbackTexture.GetAddressOf())))
	{
		return E_FAIL;
	}

	D3D11_BUFFER_DESC cbufferDesc{};
	cbufferDesc.ByteWidth = sizeof(CB_MAP_PICKING);
	cbufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	if (FAILED(m_pDevice->CreateBuffer(
		&cbufferDesc, nullptr, m_pPickingCBuffer.GetAddressOf())))
	{
		return E_FAIL;
	}

	D3D11_RASTERIZER_DESC rsDesc{};
	ZeroMemory(&rsDesc, sizeof(rsDesc));
	rsDesc.FillMode = D3D11_FILL_SOLID;
	rsDesc.CullMode = D3D11_CULL_NONE;
	rsDesc.FrontCounterClockwise = FALSE;
	rsDesc.DepthClipEnable = TRUE;

	if (FAILED(m_pDevice->CreateRasterizerState(&rsDesc, m_pRsState.GetAddressOf())))
		return E_FAIL;

	m_pPickingVS = E::CGameInstance::Get().GetResourceFirst<E::CResVertexShader>(
		PICKING_SHADER_GROUP, PICKING_VS_TAG);
	m_pPickingPS = E::CGameInstance::Get().GetResourceFirst<E::CResPixelShader>(
		PICKING_SHADER_GROUP, PICKING_PS_TAG);
	if (!m_pPickingVS || !m_pPickingPS ||
		FAILED(m_pPickingVS->Load()) || FAILED(m_pPickingPS->Load()))
	{
		return E_FAIL;
	}


	return S_OK;
}

std::optional<E::CHandle> CMapPickingPass::Pick(uint32_t mouseX, uint32_t mouseY)
{
	if (!m_pPickingContext || !m_pContext || !m_pPickingTexture || !m_ReadbackTexture ||
		mouseX >= m_iTargetWidth || mouseY >= m_iTargetHeight)
	{
		return std::nullopt;
	}

	auto* camera = E::CGameInstance::Get().GetActiveCamera();
	if (camera == nullptr)
		return std::nullopt;

	const E::_matrix identity = XMMatrixIdentity();
	const E::_vector nearPoint = XMVector3Unproject(
		XMVectorSet(static_cast<float>(mouseX), static_cast<float>(mouseY), 0.f, 1.f),
		0.f, 0.f, static_cast<float>(m_iTargetWidth), static_cast<float>(m_iTargetHeight),
		0.f, 1.f, camera->GetProj(), camera->GetView(), identity);
	const E::_vector farPoint = XMVector3Unproject(
		XMVectorSet(static_cast<float>(mouseX), static_cast<float>(mouseY), 1.f, 1.f),
		0.f, 0.f, static_cast<float>(m_iTargetWidth), static_cast<float>(m_iTargetHeight),
		0.f, 1.f, camera->GetProj(), camera->GetView(), identity);

	m_PickTable = E::CGameInstance::Get().CollectMapMeshPickCandidates(
		nearPoint, XMVector3Normalize(farPoint - nearPoint));

	if (FAILED(RenderMapMeshObjectID(m_pPickingContext.Get())))
		return std::nullopt;

	D3D11_BOX sourceBox{};
	sourceBox.left = mouseX;
	sourceBox.right = mouseX + 1;
	sourceBox.top = mouseY;
	sourceBox.bottom = mouseY + 1;
	sourceBox.front = 0;
	sourceBox.back = 1;
	m_pPickingContext->CopySubresourceRegion(
		m_ReadbackTexture.Get(), 0, 0, 0, 0,
		m_pPickingTexture.Get(), 0, &sourceBox);

	ComPtr<ID3D11CommandList> commandList;
	if (FAILED(m_pPickingContext->FinishCommandList(FALSE, commandList.GetAddressOf())))
		return std::nullopt;

	// RestoreContextState=TRUE keeps the renderer's immediate-context state intact.
	m_pContext->ExecuteCommandList(commandList.Get(), TRUE);

	D3D11_MAPPED_SUBRESOURCE mapped{};
	if (FAILED(m_pContext->Map(
		m_ReadbackTexture.Get(), 0, D3D11_MAP_READ, 0, &mapped)))
	{
		return std::nullopt;
	}

	const uint32_t pickID = *static_cast<const uint32_t*>(mapped.pData);
	m_pContext->Unmap(m_ReadbackTexture.Get(), 0);

	if (pickID == 0 || pickID > m_PickTable.size())
		return std::nullopt;

	return m_PickTable[pickID - 1];
}

HRESULT CMapPickingPass::RenderMapMeshObjectID(ID3D11DeviceContext* context)
{
	if (context == nullptr || !m_pPickingRTV || !m_pDepthDSV ||
		!m_pPickingVS || !m_pPickingPS || !m_pPickingCBuffer)
	{
		return E_FAIL;
	}

	auto* camera = E::CGameInstance::Get().GetActiveCamera();
	if (camera == nullptr)
		return E_FAIL;

	ID3D11RenderTargetView* rtv = m_pPickingRTV.Get();
	context->OMSetRenderTargets(1, &rtv, m_pDepthDSV.Get());

	const float clearColor[4]{};
	context->ClearRenderTargetView(m_pPickingRTV.Get(), clearColor);
	context->ClearDepthStencilView(
		m_pDepthDSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

	D3D11_VIEWPORT viewport{};
	viewport.Width = static_cast<float>(m_iTargetWidth);
	viewport.Height = static_cast<float>(m_iTargetHeight);
	viewport.MinDepth = 0.f;
	viewport.MaxDepth = 1.f;
	context->RSSetViewports(1, &viewport);


	context->RSSetState(m_pRsState.Get());
	context->OMSetBlendState(nullptr, nullptr, 0xffffffff);
	context->OMSetDepthStencilState(nullptr, 0);
	context->GSSetShader(nullptr, nullptr, 0);
	context->HSSetShader(nullptr, nullptr, 0);
	context->DSSetShader(nullptr, nullptr, 0);
	context->IASetInputLayout(m_pPickingVS->GetInputLayout().Get());
	context->VSSetShader(m_pPickingVS->GetVertexShader().Get(), nullptr, 0);
	context->PSSetShader(m_pPickingPS->GetPixelShader().Get(), nullptr, 0);

	ID3D11Buffer* constantBuffer = m_pPickingCBuffer.Get();
	context->VSSetConstantBuffers(0, 1, &constantBuffer);

	const E::_matrix viewProjection = camera->GetView() * camera->GetProj();
	for (size_t objectIndex = 0; objectIndex < m_PickTable.size(); ++objectIndex)
	{
		auto* object = E::CGameInstance::Get().GetGameObjectByHandleT<E::CMapMeshObject>(
			m_PickTable[objectIndex]);
		if (object == nullptr)
			continue;

		auto model = E::CGameInstance::Get().GetResourceFirst<E::CResStaticModel>(
			object->GetModelResourceGroup(), object->GetModelResourceTag());
		if (!model)
			continue;

		object->GetTransform().Update();
		CB_MAP_PICKING cbuffer{};
		XMStoreFloat4x4(&cbuffer.matWVP,
			object->GetTransform().GetLoadedCombinedWorldMatrix() * viewProjection);
		cbuffer.pickID = static_cast<uint32_t>(objectIndex + 1);

		D3D11_MAPPED_SUBRESOURCE mapped{};
		if (FAILED(context->Map(
			m_pPickingCBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
		{
			return E_FAIL;
		}
		memcpy(mapped.pData, &cbuffer, sizeof(cbuffer));
		context->Unmap(m_pPickingCBuffer.Get(), 0);

		for (const auto& mesh : model->GetMeshes())
		{
			if (!mesh)
				continue;

			ID3D11Buffer* vertexBuffer = mesh->GetVertexBuffer().Get();
			const UINT stride = mesh->GetVertexStride();
			const UINT offset = 0;
			context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
			context->IASetIndexBuffer(
				mesh->GetIndexBuffer().Get(), mesh->GetIndexFormat(), 0);
			context->IASetPrimitiveTopology(mesh->GetPrimitiveType());
			context->DrawIndexed(mesh->GetNumIndices(), 0, 0);
		}
	}

	return S_OK;
}

E::UPtr<CMapPickingPass> CMapPickingPass::Create()
{
	auto instance = E::ToUPtr(new CMapPickingPass{});
	if (FAILED(instance->Initialize()))
		return nullptr;
	return instance;
}

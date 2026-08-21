#include "pch.h"
#include "MapNaviPosPick.h"

#include "GameInstance.h"
#include "ResPixelShader.h"
#include "ResVertexShader.h"
#include "MapMeshObject.h"
#include "ResStaticModel.h"
#include "ResStaticModelMesh.h"
NS_USING(Client)

namespace
{
	constexpr char PICKING_SHADER_GROUP[] =
		"MAP_EDITOR_SHADER";

	constexpr char PICKING_VS_TAG[] =
		"VS_MAP_MESH_POSITION_PICKING";

	constexpr char PICKING_PS_TAG[] =
		"PS_MAP_MESH_POSITION_PICKING";

	struct alignas(16) CB_MAP_NAVI_POS_PICKING
	{
		E::_float4x4 matWorld{};
		E::_float4x4 matWVP{};
	};

	static_assert(
		sizeof(CB_MAP_NAVI_POS_PICKING) % 16 == 0);
}

HRESULT CMapNaviPosPickPass::Initialize()
{
	m_pPositionTexture.Reset();
	m_pPositionRTV.Reset();

	m_pDepthTexture.Reset();
	m_pDepthDSV.Reset();

	m_pReadbackTexture.Reset();
	m_pPickingCBuffer.Reset();

	m_pRasterizerState.Reset();
	m_pPickingContext.Reset();

	m_pDevice =
		E::CGameInstance::Get()
		.GetGraphicDevice();

	m_pContext =
		E::CGameInstance::Get()
		.GetGraphicDeviceContext();

	if (!m_pDevice ||
		!m_pContext)
	{
		return E_FAIL;
	}

	if (FAILED(
		m_pDevice->CreateDeferredContext(
			0,
			m_pPickingContext.GetAddressOf())))
	{
		return E_FAIL;
	}

	const E::_float2 vClientSize =
		E::CGameInstance::Get()
		.GetClientScreenSize();

	if (vClientSize.x < 1.f ||
		vClientSize.y < 1.f)
	{
		return E_FAIL;
	}

	m_iTargetWidth = static_cast<UINT>(vClientSize.x);
	m_iTargetHeight = static_cast<UINT>(vClientSize.y);

	// 월드 위치를 출력받는 렌더타깃
	D3D11_TEXTURE2D_DESC PositionDesc{};

	PositionDesc.Width = m_iTargetWidth;
	PositionDesc.Height = m_iTargetHeight;

	PositionDesc.MipLevels = 1;
	PositionDesc.ArraySize = 1;

	PositionDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;

	PositionDesc.SampleDesc.Count = 1;
	PositionDesc.Usage = D3D11_USAGE_DEFAULT;

	PositionDesc.BindFlags = D3D11_BIND_RENDER_TARGET;

	if (FAILED(
		m_pDevice->CreateTexture2D(
			&PositionDesc,
			nullptr,
			m_pPositionTexture.GetAddressOf())))
	{
		return E_FAIL;
	}

	if (FAILED(
		m_pDevice->CreateRenderTargetView(
			m_pPositionTexture.Get(),
			nullptr,
			m_pPositionRTV.GetAddressOf())))
	{
		return E_FAIL;
	}

	// 가장 앞에 보이는 메시 표면을 판정하기 위한 Depth
	D3D11_TEXTURE2D_DESC DepthDesc{};

	DepthDesc.Width =
		m_iTargetWidth;

	DepthDesc.Height =
		m_iTargetHeight;

	DepthDesc.MipLevels = 1;
	DepthDesc.ArraySize = 1;

	DepthDesc.Format =
		DXGI_FORMAT_D24_UNORM_S8_UINT;

	DepthDesc.SampleDesc.Count = 1;
	DepthDesc.Usage = D3D11_USAGE_DEFAULT;

	DepthDesc.BindFlags =
		D3D11_BIND_DEPTH_STENCIL;

	if (FAILED(
		m_pDevice->CreateTexture2D(
			&DepthDesc,
			nullptr,
			m_pDepthTexture.GetAddressOf())))
	{
		return E_FAIL;
	}

	if (FAILED(
		m_pDevice->CreateDepthStencilView(
			m_pDepthTexture.Get(),
			nullptr,
			m_pDepthDSV.GetAddressOf())))
	{
		return E_FAIL;
	}

	// 클릭한 한 픽셀만 CPU에서 읽을 Staging Texture
	D3D11_TEXTURE2D_DESC ReadbackDesc{};

	ReadbackDesc.Width = 1;
	ReadbackDesc.Height = 1;

	ReadbackDesc.MipLevels = 1;
	ReadbackDesc.ArraySize = 1;

	ReadbackDesc.Format =
		DXGI_FORMAT_R32G32B32A32_FLOAT;

	ReadbackDesc.SampleDesc.Count = 1;
	ReadbackDesc.Usage = D3D11_USAGE_STAGING;

	ReadbackDesc.CPUAccessFlags =
		D3D11_CPU_ACCESS_READ;

	if (FAILED(
		m_pDevice->CreateTexture2D(
			&ReadbackDesc,
			nullptr,
			m_pReadbackTexture.GetAddressOf())))
	{
		return E_FAIL;
	}

	D3D11_BUFFER_DESC CBufferDesc{};

	CBufferDesc.ByteWidth =
		sizeof(CB_MAP_NAVI_POS_PICKING);

	CBufferDesc.Usage =
		D3D11_USAGE_DYNAMIC;

	CBufferDesc.BindFlags =
		D3D11_BIND_CONSTANT_BUFFER;

	CBufferDesc.CPUAccessFlags =
		D3D11_CPU_ACCESS_WRITE;

	if (FAILED(
		m_pDevice->CreateBuffer(
			&CBufferDesc,
			nullptr,
			m_pPickingCBuffer.GetAddressOf())))
	{
		return E_FAIL;
	}

	D3D11_RASTERIZER_DESC RasterizerDesc{};

	RasterizerDesc.FillMode =
		D3D11_FILL_SOLID;

	RasterizerDesc.CullMode =
		D3D11_CULL_NONE;

	RasterizerDesc.DepthClipEnable =
		TRUE;

	if (FAILED(
		m_pDevice->CreateRasterizerState(
			&RasterizerDesc,
			m_pRasterizerState.GetAddressOf())))
	{
		return E_FAIL;
	}

	m_pPickingVS =
		E::CGameInstance::Get()
		.GetResourceFirst<E::CResVertexShader>(
			PICKING_SHADER_GROUP,
			PICKING_VS_TAG);

	m_pPickingPS =
		E::CGameInstance::Get()
		.GetResourceFirst<E::CResPixelShader>(
			PICKING_SHADER_GROUP,
			PICKING_PS_TAG);

	if (!m_pPickingVS ||
		!m_pPickingPS ||
		FAILED(m_pPickingVS->Load()) ||
		FAILED(m_pPickingPS->Load()))
	{
		return E_FAIL;
	}

	return S_OK;
}
HRESULT CMapNaviPosPickPass::RenderMapMeshPosition(
	ID3D11DeviceContext* pContext)
{
	if (!pContext ||
		!m_pPositionRTV ||
		!m_pDepthDSV ||
		!m_pPickingVS ||
		!m_pPickingPS ||
		!m_pPickingCBuffer)
	{
		return E_FAIL;
	}

	auto* pCamera =
		E::CGameInstance::Get()
		.GetActiveCamera();

	if (!pCamera)
		return E_FAIL;

	ID3D11RenderTargetView* pRTV =
		m_pPositionRTV.Get();

	pContext->OMSetRenderTargets(
		1,
		&pRTV,
		m_pDepthDSV.Get());

	// Alpha 0은 아무 메시도 피킹되지 않았다는 의미
	constexpr float vClearColor[4] =
	{
		0.f,
		0.f,
		0.f,
		0.f
	};

	pContext->ClearRenderTargetView(
		m_pPositionRTV.Get(),
		vClearColor);

	pContext->ClearDepthStencilView(
		m_pDepthDSV.Get(),
		D3D11_CLEAR_DEPTH |
		D3D11_CLEAR_STENCIL,
		1.f,
		0);

	D3D11_VIEWPORT Viewport{};

	Viewport.Width = static_cast<float>(m_iTargetWidth);
	Viewport.Height = static_cast<float>(m_iTargetHeight);

	Viewport.MinDepth = 0.f;
	Viewport.MaxDepth = 1.f;

	pContext->RSSetViewports(1,&Viewport);
	pContext->RSSetState(m_pRasterizerState.Get());
	pContext->OMSetBlendState(nullptr,nullptr,0xffffffff);
	pContext->OMSetDepthStencilState(nullptr,0);
	pContext->GSSetShader(nullptr,nullptr,0);
	pContext->HSSetShader(nullptr,nullptr,0);
	pContext->DSSetShader(nullptr,nullptr,0);
	pContext->IASetInputLayout(m_pPickingVS->GetInputLayout().Get());
	pContext->VSSetShader(m_pPickingVS->GetVertexShader().Get(),nullptr,0);
	pContext->PSSetShader(m_pPickingPS->GetPixelShader().Get(),nullptr,0);

	ID3D11Buffer* pConstantBuffer = m_pPickingCBuffer.Get();

	pContext->VSSetConstantBuffers(0, 1,&pConstantBuffer);

	const E::_matrix ViewProjection = pCamera->GetView() * pCamera->GetProj();

	for (const E::CHandle& Handle : m_PickTable)
	{
		auto* pObject =	E::CGameInstance::Get().GetGameObjectByHandleT<E::CMapMeshObject>(Handle);

		if (!pObject)
			continue;

		auto pModel =
			E::CGameInstance::Get()
			.GetResourceFirst<
			E::CResStaticModel>(
				pObject
				->GetModelResourceGroup(),
				pObject
				->GetModelResourceTag());

		if (!pModel)
			continue;

		pObject->GetTransform().Update();

		const E::_matrix World =
			pObject->GetTransform()
			.GetLoadedCombinedWorldMatrix();

		CB_MAP_NAVI_POS_PICKING CBuffer{};

		XMStoreFloat4x4(
			&CBuffer.matWorld,
			World);

		XMStoreFloat4x4(
			&CBuffer.matWVP,
			World *
			ViewProjection);

		D3D11_MAPPED_SUBRESOURCE Mapped{};

		if (FAILED(
			pContext->Map(
				m_pPickingCBuffer.Get(),
				0,
				D3D11_MAP_WRITE_DISCARD,
				0,
				&Mapped)))
		{
			return E_FAIL;
		}

		memcpy(Mapped.pData, &CBuffer,sizeof(CBuffer));

		pContext->Unmap(m_pPickingCBuffer.Get(),0);

		for (const auto& pMesh :pModel->GetMeshes())
		{
			if (!pMesh)
				continue;

			ID3D11Buffer* pVertexBuffer = pMesh->GetVertexBuffer().Get();

			const UINT iStride = pMesh->GetVertexStride();

			constexpr UINT iOffset = 0;

			pContext->IASetVertexBuffers(
				0,
				1,
				&pVertexBuffer,
				&iStride,
				&iOffset);

			pContext->IASetIndexBuffer(
				pMesh->GetIndexBuffer().Get(),
				pMesh->GetIndexFormat(),
				0);

			pContext->IASetPrimitiveTopology(pMesh->GetPrimitiveType());

			pContext->DrawIndexed(pMesh->GetNumIndices(),
				0,
				0);
		}
	}

	return S_OK;
}
std::optional<E::_float3>CMapNaviPosPickPass::Pick(uint32_t iMouseX, uint32_t iMouseY)
{
	if (!m_pPickingContext ||
		!m_pContext ||
		!m_pPositionTexture ||
		!m_pReadbackTexture ||
		iMouseX >= m_iTargetWidth ||
		iMouseY >= m_iTargetHeight)
	{
		return std::nullopt;
	}

	auto* pCamera =
		E::CGameInstance::Get()
		.GetActiveCamera();

	if (!pCamera)
		return std::nullopt;

	const E::_matrix Identity =
		XMMatrixIdentity();

	const E::_vector vNearPoint =
		XMVector3Unproject(
			XMVectorSet(
				static_cast<float>(iMouseX),
				static_cast<float>(iMouseY),
				0.f,
				1.f),
			0.f,
			0.f,
			static_cast<float>(m_iTargetWidth),
			static_cast<float>(m_iTargetHeight),
			0.f,
			1.f,
			pCamera->GetProj(),
			pCamera->GetView(),
			Identity);

	const E::_vector vFarPoint =
		XMVector3Unproject(
			XMVectorSet(
				static_cast<float>(iMouseX),
				static_cast<float>(iMouseY),
				1.f,
				1.f),
			0.f,
			0.f,
			static_cast<float>(m_iTargetWidth),
			static_cast<float>(m_iTargetHeight),
			0.f,
			1.f,
			pCamera->GetProj(),
			pCamera->GetView(),
			Identity);

	const E::_vector vRayDirection =
		XMVector3Normalize(
			vFarPoint -
			vNearPoint);

	m_PickTable =
		E::CGameInstance::Get()
		.CollectMapMeshPickCandidates(
			vNearPoint,
			vRayDirection);

	if (m_PickTable.empty())
		return std::nullopt;

	if (FAILED(
		RenderMapMeshPosition(
			m_pPickingContext.Get())))
	{
		return std::nullopt;
	}

	D3D11_BOX SourceBox{};

	SourceBox.left =
		iMouseX;

	SourceBox.right =
		iMouseX + 1;

	SourceBox.top =
		iMouseY;

	SourceBox.bottom =
		iMouseY + 1;

	SourceBox.front = 0;
	SourceBox.back = 1;

	m_pPickingContext
		->CopySubresourceRegion(
			m_pReadbackTexture.Get(),
			0,
			0,
			0,
			0,
			m_pPositionTexture.Get(),
			0,
			&SourceBox);

	ComPtr<ID3D11CommandList>
		pCommandList{};

	if (FAILED(
		m_pPickingContext
		->FinishCommandList(
			FALSE,
			pCommandList.GetAddressOf())))
	{
		return std::nullopt;
	}

	m_pContext->ExecuteCommandList(
		pCommandList.Get(),
		TRUE);

	D3D11_MAPPED_SUBRESOURCE Mapped{};

	if (FAILED(
		m_pContext->Map(
			m_pReadbackTexture.Get(),
			0,
			D3D11_MAP_READ,
			0,
			&Mapped)))
	{
		return std::nullopt;
	}

	const E::_float4 vResult =
		*static_cast<const E::_float4*>(
			Mapped.pData);

	m_pContext->Unmap(
		m_pReadbackTexture.Get(),
		0);

	// Clear Color의 Alpha는 0,
	// 메시가 그려진 픽셀의 Alpha는 1
	if (vResult.w < 0.5f)
		return std::nullopt;

	return E::_float3
	{
		vResult.x,
		vResult.y,
		vResult.z
	};
}
E::UPtr<CMapNaviPosPickPass>CMapNaviPosPickPass::Create()
{
	auto pInstance = E::ToUPtr(new CMapNaviPosPickPass{});

	if (FAILED(pInstance->Initialize()))
		return nullptr;

	return pInstance;
}

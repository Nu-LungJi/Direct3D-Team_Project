#include "pch.h"

#ifdef _DEBUG

#include "MapChunkDebugRenderer.h"
#include "CameraObject.h"
#include "OctreeNode.h"

NS_USING(Engine)

HRESULT CMapChunkDebugRenderer::Initialize()
{
	auto& gameInstance = CGameInstance::Get();
	auto device = gameInstance.GetGraphicDevice();
	auto deviceContext = gameInstance.GetGraphicDeviceContext();
	if (!device || !deviceContext)
		return E_FAIL;

	m_Batch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(deviceContext.Get());
	m_Effect = std::make_unique<DirectX::BasicEffect>(device.Get());
	m_Effect->SetVertexColorEnabled(true);

	const void* shaderByteCode{};
	size_t byteCodeLength{};
	m_Effect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

	return device->CreateInputLayout(
		DirectX::VertexPositionColor::InputElements,
		DirectX::VertexPositionColor::InputElementCount,
		shaderByteCode,
		byteCodeLength,
		m_InputLayout.GetAddressOf());
}

HRESULT CMapChunkDebugRenderer::Render(const CHUNK_MAP& chunks)
{
	if (!m_IsEnabled)
		return E_FAIL;

	const auto* camera = CGameInstance::Get().GetActiveCamera();
	if (!camera || !m_Batch || !m_Effect || !m_InputLayout)
		return E_FAIL;

	const auto view = camera->GetView();
	const auto projection = camera->GetProj();
	const auto world = XMMatrixIdentity();

	for (const auto& chunkEntry : chunks)
	{
		const CMapChunk& chunk = chunkEntry.second;
		if (chunk.IsLoaded() && chunk.GetOctree())
		{
			std::vector<OCTREE_DEBUG_BOUNDS> octreeBounds;
			chunk.GetOctree()->CollectDebugBounds(octreeBounds);
			for (const auto& node : octreeBounds)
				DrawBox(node.bounds, Colors::Red, view, projection, world);
		}

		const FXMVECTOR chunkColor = chunk.IsLoaded() ? Colors::Lime : Colors::Red;
		DrawBox(chunk.GetBounds(), chunkColor, view, projection, world);
	}

	return S_OK;
}

void CMapChunkDebugRenderer::DrawBox(const BoundingBox& bounds,
	FXMVECTOR color,
	CXMMATRIX view,
	CXMMATRIX projection,
	CXMMATRIX world)
{
	m_Effect->SetView(view);
	m_Effect->SetProjection(projection);
	m_Effect->SetWorld(world);

	auto deviceContext = CGameInstance::Get().GetGraphicDeviceContext();
	deviceContext->IASetInputLayout(m_InputLayout.Get());
	m_Effect->Apply(deviceContext.Get());

	m_Batch->Begin();
	DX::Draw(m_Batch.get(), bounds, color);
	m_Batch->End();
}

#endif

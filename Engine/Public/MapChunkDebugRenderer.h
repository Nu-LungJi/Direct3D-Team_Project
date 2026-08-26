#pragma once

#ifdef _DEBUG

#include "Engine_Base.h"
#include "MapChunk.h"
#include "DebugDraw.h"
#include <directxtk/Effects.h>

NS_BEGIN(Engine)

class ENGINE_DLL CMapChunkDebugRenderer final
{
public:
	using CHUNK_MAP =
		std::unordered_map<MAPCHUNK_COORD, CMapChunk, tagMapChunkCoordHash>;

public:
	HRESULT Initialize();
	HRESULT Render(const CHUNK_MAP& chunks);

	void SetEnabled(_bool enabled) { m_IsEnabled = enabled; }
	_bool IsEnabled() const { return m_IsEnabled; }

private:
	void DrawBox(const BoundingBox& bounds,
		FXMVECTOR color,
		CXMMATRIX view,
		CXMMATRIX projection,
		CXMMATRIX world);

private:
	std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> m_Batch;
	std::unique_ptr<DirectX::BasicEffect> m_Effect;
	ComPtr<ID3D11InputLayout> m_InputLayout;
	_bool m_IsEnabled{};
};

NS_END

#endif

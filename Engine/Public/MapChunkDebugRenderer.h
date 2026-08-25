#pragma once

#ifdef _DEBUG

#include "Engine_Base.h"
#include "MapChunk.h"
#include "DebugDraw.h"
#include <directxtk/Effects.h>

NS_BEGIN(Engine)

// 로드 상태와 옥트리 경계를 화면에 표시하는 디버그 전용 렌더러다.
// 디버그 렌더링에 필요한 D3D 리소스와 활성 상태를 직접 소유한다.
class ENGINE_DLL CMapChunkDebugRenderer final
{
public:
	using CHUNK_MAP =
		std::unordered_map<MAPCHUNK_COORD, CMapChunk, tagMapChunkCoordHash>;

public:
	// 디버그 선 렌더링에 사용할 Direct3D 리소스를 한 번 생성한다.
	HRESULT Initialize();
	// 현재 청크와 로드된 청크의 옥트리 경계를 그린다.
	HRESULT Render(const CHUNK_MAP& chunks);

	// MapManager의 기존 디버그 표시 설정을 렌더러 내부에 보관한다.
	void SetEnabled(_bool enabled) { m_IsEnabled = enabled; }
	_bool IsEnabled() const { return m_IsEnabled; }

private:
	// 하나의 바운딩 박스를 현재 카메라 행렬로 그린다.
	void DrawBox(const BoundingBox& bounds,
		FXMVECTOR color,
		CXMMATRIX view,
		CXMMATRIX projection,
		CXMMATRIX world);

private:
	// 디버그 선을 모아서 그리는 DirectXTK 배치 객체다.
	std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> m_Batch;
	// 월드·뷰·투영 행렬과 정점 색상을 적용하는 DirectXTK 이펙트다.
	std::unique_ptr<DirectX::BasicEffect> m_Effect;
	// VertexPositionColor 정점 형식을 BasicEffect 입력에 연결한다.
	ComPtr<ID3D11InputLayout> m_InputLayout;
	// false이면 Render가 기존 동작과 동일하게 그리기를 수행하지 않는다.
	_bool m_IsEnabled{};
};

NS_END

#endif

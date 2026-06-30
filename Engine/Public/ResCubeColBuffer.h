#pragma once

#include "ResCubeBuffer.h"

NS_BEGIN(Engine)

class ENGINE_DLL CResCubeColBuffer final : public CResCubeBuffer
{
public:
	DECLARE_DERIVED_TYPE(CResCubeColBuffer, CResCubeBuffer)

private:
	explicit CResCubeColBuffer(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
	~CResCubeColBuffer() override;

public:
	//void SetVertexColors(const E::_float4& topLeft,
	//	const E::_float4& topRight,
	//	const E::_float4& bottomRight,
	//	const E::_float4& bottomLeft)
	//{
	//	m_colors[0] = topLeft;
	//	m_colors[1] = topRight;
	//	m_colors[2] = bottomRight;
	//	m_colors[3] = bottomLeft;
	//}


private:
	//E::_float4 m_colors[4] = {
	//	{1,0,0,1}, {0,1,0,1}, {0,0,1,1}, {1,1,1,1} // 기본값
	//};

public:
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {})  override;

public:
	static SPtr<CResCubeColBuffer> Create();
};

NS_END
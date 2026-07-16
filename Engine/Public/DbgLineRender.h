#pragma once

#include "Engine_Defines.h"
#include "IRenderable.h"

NS_BEGIN(Engine)
enum class DBG_LINE_DEPTH_MODE : uint8_t
{
    DISABLED,
    ENABLED
};

class CResVertexShader;
class CResDynamicVIBuffer;
class CResPixelShader;
class CResDepthStencilState;
class ENGINE_DLL CDbgLineRender  final : public CEngineBase, public IRenderable
{
private:
	explicit CDbgLineRender(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
	~CDbgLineRender() override;

public:
    void FrameEnd();

public:
    void SetColor(const _float4& vColor = { 0.f, 0.f, 0.f, 1.f });
    const _float4& GetColor() const { return m_vColor; }
    void SetDepthMode(DBG_LINE_DEPTH_MODE eMode) { m_eDepthMode = eMode; }
    DBG_LINE_DEPTH_MODE GetDepthMode() const { return m_eDepthMode; }
    void SetDepthTest(_bool bEnable) { SetDepthMode(bEnable ? DBG_LINE_DEPTH_MODE::ENABLED : DBG_LINE_DEPTH_MODE::DISABLED); }
public:
    void AddLine(
        const _float3& p0,
        const _float3& p1);
    void AddLine(
        const _float3& p0,
        const _float3& p1,
        const _float4& col);

    void AddBox(
        const _float3& halfExtent,
        FXMMATRIX world = XMMatrixIdentity());

    void AddSphere(
        float radius,
        FXMMATRIX world = XMMatrixIdentity());

    void AddCapsule(
        float radius,
        float halfHeight,
        FXMMATRIX world);

    void AddCylinder(
        float radius,
        float halfHeight,
        FXMMATRIX world = XMMatrixIdentity());

    void AddCone(
        float radius,
        float height,
        FXMMATRIX world = XMMatrixIdentity());

    void AddFrustum(
        float fovY,
        float aspect,
        float nearZ,
        float farZ,
        FXMMATRIX world);

    void AddRay(
        const _float3& origin,
        const _float3& direction,
        float length);

    void AddArrow(
        const _float3& origin,
        const _float3& direction,
        float length,
        float headLength = 0.2f,
        float headAngleDeg = 25.f);

    void AddGrid(
        uint32_t halfCount,
        float cellSize,
        FXMMATRIX world = XMMatrixIdentity());

    void AddQuad(
        float width,
        float height,
        FXMMATRIX world);

    void AddTriangle(
        const _float3& p0,
        const _float3& p1,
        const _float3& p2);

    void AddAxis(
        float length,
        FXMMATRIX world = XMMatrixIdentity());

    void AddCircle(
        float radius,
        FXMMATRIX world,
        uint32_t slice = 32);

    void AddCross(
        const _float3& position,
        float size = 0.1f);

    void AddTriangleMesh(
        const _float3* vertices,
        uint32_t vertexCount,
        const uint32_t* indices,
        uint32_t triangleCount,
        FXMMATRIX world = XMMatrixIdentity());

    void AddConvexHull(
        const _float3* vertices,
        uint32_t vertexCount,
        const uint32_t* indices,
        uint32_t triangleCount,
        FXMMATRIX world = XMMatrixIdentity());

    void AddBuiltedVertices(const std::vector<VTX_COL>& vecVertices);
    void AddPackedLineVertices(const void* pVertexData, size_t iVertexCount);


public:
    HRESULT Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx) override;
    bool HasRenderPass(RENDERPASS ePass) const override { return ePass == RENDERPASS::DEFAULT; };

private:
	HRESULT Initialize();
	_bool CanAddVertices(size_t iVertexCount) const;
	std::vector<VTX_DBG_LINE>& GetCurrentVertices();
	const std::vector<VTX_DBG_LINE>& GetCurrentVertices() const;

private:
	ComPtr<ID3D11Device> m_pDevice{};
	ComPtr<ID3D11DeviceContext> m_pContext{};

    _bool m_bRender{ true };
    _float4 m_vColor{0.f, 0.f, 0.f, 1.f};
    uint32_t m_iPackedColor{ 0xFF000000u };
    DBG_LINE_DEPTH_MODE m_eDepthMode{ DBG_LINE_DEPTH_MODE::DISABLED };

    const uint32_t m_iVertexCnt{ 1000000 };

    SPtr<CResDynamicVIBuffer> m_pDbgBuffer{};
    SPtr<CResVertexShader> m_pDbgVShader{};
    SPtr<CResPixelShader> m_pDbgPShader{};
    SPtr<CResDepthStencilState> m_pDepthState{};
    SPtr<CResDepthStencilState> m_pNoDepthState{};
    std::vector<VTX_DBG_LINE> m_DepthVertices{};
    std::vector<VTX_DBG_LINE> m_NoDepthVertices{};

public:
	static UPtr<CDbgLineRender> Create(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
};

NS_END

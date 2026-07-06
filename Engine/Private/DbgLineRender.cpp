#include "pch.h"
#include "DbgLineRender.h"
#include "GameInstance.h"
#include "Resources.h"

NS_USING(Engine)

CDbgLineRender::CDbgLineRender(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext)
    : m_pDevice { pDevice }
    , m_pContext{ pContext }
{
}

CDbgLineRender::~CDbgLineRender()
{
}

void CDbgLineRender::AddLine(const _float3& p0, const _float3& p1)
{
    if (m_Vertices.size() >= m_iVertexCnt) return;

    m_Vertices.push_back({ p0, m_vColor });
    m_Vertices.push_back({ p1, m_vColor });
}

void CDbgLineRender::AddLine(const _float3& p0, const _float3& p1, const _float4& col)
{
    if (m_Vertices.size() >= m_iVertexCnt) return;

    m_Vertices.push_back({ p0, col });
    m_Vertices.push_back({ p1, col });
}

void CDbgLineRender::AddBox(const _float3& halfExtent, FXMMATRIX world)
{
    if (m_Vertices.size() >= m_iVertexCnt) return;

    static constexpr _float3 local[8] =
    {
        {-1.f, -1.f, -1.f},
        { 1.f, -1.f, -1.f},
        { 1.f,  1.f, -1.f},
        {-1.f,  1.f, -1.f},

        {-1.f, -1.f,  1.f},
        { 1.f, -1.f,  1.f},
        { 1.f,  1.f,  1.f},
        {-1.f,  1.f,  1.f},
    };

    _float3 vertex[8];

    for (uint32_t i = 0; i < 8; ++i)
    {
        XMVECTOR p = XMVectorSet(
            local[i].x * halfExtent.x,
            local[i].y * halfExtent.y,
            local[i].z * halfExtent.z,
            1.f);

        p = XMVector3TransformCoord(p, world);
        XMStoreFloat3(&vertex[i], p);
    }

    static constexpr uint32_t edge[12][2] =
    {
        {0,1}, {1,2}, {2,3}, {3,0},
        {4,5}, {5,6}, {6,7}, {7,4},
        {0,4}, {1,5}, {2,6}, {3,7},
    };

    for (const auto& e : edge)
        AddLine(vertex[e[0]], vertex[e[1]]);
}

void CDbgLineRender::AddSphere(float radius, FXMMATRIX world)
{
    if (m_Vertices.size() >= m_iVertexCnt) return;

    constexpr uint32_t SliceCount = 24;
    constexpr uint32_t StackCount = 12;

    constexpr float PI = XM_PI;
    constexpr float TWO_PI = XM_2PI;

    // 위도
    for (uint32_t stack = 1; stack < StackCount; ++stack)
    {
        float phi = PI * stack / StackCount;
        float y = cosf(phi) * radius;
        float r = sinf(phi) * radius;

        for (uint32_t slice = 0; slice < SliceCount; ++slice)
        {
            float theta0 = TWO_PI * slice / SliceCount;
            float theta1 = TWO_PI * (slice + 1) / SliceCount;

            XMVECTOR p0 = XMVectorSet(
                r * cosf(theta0),
                y,
                r * sinf(theta0),
                1.f);

            XMVECTOR p1 = XMVectorSet(
                r * cosf(theta1),
                y,
                r * sinf(theta1),
                1.f);

            _float3 v0, v1;

            XMStoreFloat3(&v0, XMVector3TransformCoord(p0, world));
            XMStoreFloat3(&v1, XMVector3TransformCoord(p1, world));

            AddLine(v0, v1);
        }
    }

    // 경도
    for (uint32_t slice = 0; slice < SliceCount; ++slice)
    {
        float theta = TWO_PI * slice / SliceCount;

        for (uint32_t stack = 0; stack < StackCount; ++stack)
        {
            float phi0 = PI * stack / StackCount;
            float phi1 = PI * (stack + 1) / StackCount;

            XMVECTOR p0 = XMVectorSet(
                radius * sinf(phi0) * cosf(theta),
                radius * cosf(phi0),
                radius * sinf(phi0) * sinf(theta),
                1.f);

            XMVECTOR p1 = XMVectorSet(
                radius * sinf(phi1) * cosf(theta),
                radius * cosf(phi1),
                radius * sinf(phi1) * sinf(theta),
                1.f);

            _float3 v0, v1;

            XMStoreFloat3(&v0, XMVector3TransformCoord(p0, world));
            XMStoreFloat3(&v1, XMVector3TransformCoord(p1, world));

            AddLine(v0, v1);
        }
    }
}

void CDbgLineRender::AddCapsule(
    float radius,
    float halfHeight,
    FXMMATRIX world = XMMatrixIdentity())
{
    if (m_Vertices.size() >= m_iVertexCnt) return;

    constexpr uint32_t SliceCount = 24;
    constexpr uint32_t ArcCount = 12;

    constexpr float PI = XM_PI;
    constexpr float HALF_PI = XM_PIDIV2;
    constexpr float TWO_PI = XM_2PI;

    // ==========================
    // 원통부
    // ==========================
    for (uint32_t i = 0; i < SliceCount; ++i)
    {
        float t0 = TWO_PI * i / SliceCount;
        float t1 = TWO_PI * (i + 1) / SliceCount;

        _float3 top0{
            radius * cosf(t0),
             halfHeight,
            radius * sinf(t0)
        };

        _float3 top1{
            radius * cosf(t1),
             halfHeight,
            radius * sinf(t1)
        };

        _float3 bottom0{
            radius * cosf(t0),
            -halfHeight,
            radius * sinf(t0)
        };

        _float3 bottom1{
            radius * cosf(t1),
            -halfHeight,
            radius * sinf(t1)
        };

        auto Transform = [&](const _float3& p)
            {
                XMFLOAT3 out;
                XMStoreFloat3(&out,
                    XMVector3TransformCoord(
                        XMLoadFloat3(&p),
                        world));
                return out;
            };

        AddLine(Transform(top0), Transform(top1));
        AddLine(Transform(bottom0), Transform(bottom1));
        AddLine(Transform(top0), Transform(bottom0));
    }

    // ==========================
    // 반구(XZ 단면)
    // ==========================
    for (uint32_t j = 0; j < ArcCount; ++j)
    {
        float a0 = HALF_PI * j / ArcCount;
        float a1 = HALF_PI * (j + 1) / ArcCount;

        for (int sign : { -1, 1 })
        {
            for (uint32_t i = 0; i < SliceCount; ++i)
            {
                float theta = TWO_PI * i / SliceCount;

                auto MakePoint = [&](float a)
                    {
                        float r = radius * cosf(a);

                        return _float3{
                            r * cosf(theta),
                            sign * (halfHeight + radius * sinf(a)),
                            r * sinf(theta)
                        };
                    };

                auto p0 = MakePoint(a0);
                auto p1 = MakePoint(a1);

                XMFLOAT3 v0, v1;

                XMStoreFloat3(&v0,
                    XMVector3TransformCoord(XMLoadFloat3(&p0), world));

                XMStoreFloat3(&v1,
                    XMVector3TransformCoord(XMLoadFloat3(&p1), world));

                AddLine(v0, v1);
            }
        }
    }

    // ==========================
    // 세 방향 아크
    // ==========================
    constexpr float rot[3] =
    {
        0.f,
        XM_PIDIV2,
        XM_PIDIV4
    };

    for (float yaw : rot)
    {
        XMMATRIX rotY = XMMatrixRotationY(yaw);

        for (int sign : { -1, 1 })
        {
            for (uint32_t i = 0; i < ArcCount; ++i)
            {
                float a0 = PI * i / ArcCount;
                float a1 = PI * (i + 1) / ArcCount;

                XMVECTOR p0 = XMVectorSet(
                    radius * cosf(a0),
                    sign * halfHeight + radius * sinf(a0),
                    0.f,
                    1.f);

                XMVECTOR p1 = XMVectorSet(
                    radius * cosf(a1),
                    sign * halfHeight + radius * sinf(a1),
                    0.f,
                    1.f);

                p0 = XMVector3TransformCoord(p0, rotY * world);
                p1 = XMVector3TransformCoord(p1, rotY * world);

                XMFLOAT3 v0, v1;
                XMStoreFloat3(&v0, p0);
                XMStoreFloat3(&v1, p1);

                AddLine(v0, v1);
            }
        }
    }
}

void CDbgLineRender::AddCylinder(float radius, float halfHeight, FXMMATRIX world)
{
    if (m_Vertices.size() >= m_iVertexCnt) return;

    constexpr uint32_t SliceCount = 24;
    constexpr float TWO_PI = XM_2PI;

    _float3 top[SliceCount];
    _float3 bottom[SliceCount];

    // 원의 정점 생성
    for (uint32_t i = 0; i < SliceCount; ++i)
    {
        float theta = TWO_PI * i / SliceCount;

        XMVECTOR vTop = XMVectorSet(
            radius * cosf(theta),
            halfHeight,
            radius * sinf(theta),
            1.f);

        XMVECTOR vBottom = XMVectorSet(
            radius * cosf(theta),
            -halfHeight,
            radius * sinf(theta),
            1.f);

        XMStoreFloat3(&top[i], XMVector3TransformCoord(vTop, world));
        XMStoreFloat3(&bottom[i], XMVector3TransformCoord(vBottom, world));
    }

    // 위/아래 원
    for (uint32_t i = 0; i < SliceCount; ++i)
    {
        uint32_t next = (i + 1) % SliceCount;

        AddLine(top[i], top[next]);
        AddLine(bottom[i], bottom[next]);
    }

    // 세로선(4개만)
    constexpr uint32_t vertical[4] = { 0, 6, 12, 18 };

    for (uint32_t idx : vertical)
        AddLine(top[idx], bottom[idx]);
}

void CDbgLineRender::AddCone(float radius, float height, FXMMATRIX world)
{
    if (m_Vertices.size() >= m_iVertexCnt) return;

    constexpr uint32_t Slice = 24;

    const _float3 apex = { 0.f, height, 0.f };
    const _float3 center = { 0.f, 0.f, 0.f };

    const float step = XM_2PI / Slice;

    // 밑면 원
    for (uint32_t i = 0; i < Slice; ++i)
    {
        float a0 = step * i;
        float a1 = step * (i + 1);

        XMVECTOR p0 = XMVector3TransformCoord(
            XMVectorSet(cosf(a0) * radius, 0.f, sinf(a0) * radius, 1.f),
            world);

        XMVECTOR p1 = XMVector3TransformCoord(
            XMVectorSet(cosf(a1) * radius, 0.f, sinf(a1) * radius, 1.f),
            world);

        _float3 v0, v1;
        XMStoreFloat3(&v0, p0);
        XMStoreFloat3(&v1, p1);

        AddLine(v0, v1);
    }

    // 옆선
    constexpr uint32_t SideCount = 8;

    XMVECTOR apexPos = XMVector3TransformCoord(
        XMLoadFloat3(&apex),
        world);

    _float3 apexWorld;
    XMStoreFloat3(&apexWorld, apexPos);

    for (uint32_t i = 0; i < SideCount; ++i)
    {
        float angle = XM_2PI * i / SideCount;

        XMVECTOR base = XMVector3TransformCoord(
            XMVectorSet(cosf(angle) * radius, 0.f, sinf(angle) * radius, 1.f),
            world);

        _float3 baseWorld;
        XMStoreFloat3(&baseWorld, base);

        AddLine(apexWorld, baseWorld);
    }

    // 중심축(선택 사항)
    XMVECTOR baseCenter = XMVector3TransformCoord(
        XMLoadFloat3(&center),
        world);

    _float3 baseCenterWorld;
    XMStoreFloat3(&baseCenterWorld, baseCenter);

    AddLine(baseCenterWorld, apexWorld);
}

void CDbgLineRender::AddFrustum(float fovY, float aspect, float nearZ, float farZ, FXMMATRIX world)
{
    if (m_Vertices.size() >= m_iVertexCnt) return;

    const float tanHalf = tanf(fovY * 0.5f);

    const float nearH = tanHalf * nearZ;
    const float nearW = nearH * aspect;

    const float farH = tanHalf * farZ;
    const float farW = farH * aspect;

    XMFLOAT3 v[8] =
    {
        {-nearW, +nearH, nearZ}, // 0
        {+nearW, +nearH, nearZ}, // 1
        {+nearW, -nearH, nearZ}, // 2
        {-nearW, -nearH, nearZ}, // 3

        {-farW, +farH, farZ},    // 4
        {+farW, +farH, farZ},    // 5
        {+farW, -farH, farZ},    // 6
        {-farW, -farH, farZ},    // 7
    };

    for (auto& p : v)
    {
        XMVECTOR pos = XMVector3TransformCoord(XMLoadFloat3(&p), world);
        XMStoreFloat3(&p, pos);
    }

    // Near
    AddLine(v[0], v[1]);
    AddLine(v[1], v[2]);
    AddLine(v[2], v[3]);
    AddLine(v[3], v[0]);

    // Far
    AddLine(v[4], v[5]);
    AddLine(v[5], v[6]);
    AddLine(v[6], v[7]);
    AddLine(v[7], v[4]);

    // Connect
    AddLine(v[0], v[4]);
    AddLine(v[1], v[5]);
    AddLine(v[2], v[6]);
    AddLine(v[3], v[7]);
}

void CDbgLineRender::AddRay(const _float3& origin, const _float3& direction, float length)
{
    if (m_Vertices.size() >= m_iVertexCnt) return;

    XMVECTOR o = XMLoadFloat3(&origin);
    XMVECTOR d = XMVector3Normalize(XMLoadFloat3(&direction));

    XMFLOAT3 end;
    XMStoreFloat3(&end, o + d * length);

    AddLine(origin, end);
}

void CDbgLineRender::AddArrow(const _float3& origin, const _float3& direction, float length, float headLength, float headAngleDeg)
{
    if (m_Vertices.size() >= m_iVertexCnt) return;

    XMVECTOR dir = XMVector3Normalize(XMLoadFloat3(&direction));
    XMVECTOR start = XMLoadFloat3(&origin);
    XMVECTOR tip = start + dir * length;

    XMFLOAT3 p0, p1;
    XMStoreFloat3(&p0, start);
    XMStoreFloat3(&p1, tip);

    // 몸통
    AddLine(p0, p1);

    const float angle = XMConvertToRadians(headAngleDeg);

    // dir와 평행하지 않은 Up 선택
    XMVECTOR up = XMVectorSet(0, 1, 0, 0);
    if (fabsf(XMVectorGetX(XMVector3Dot(up, dir))) > 0.99f)
        up = XMVectorSet(1, 0, 0, 0);

    XMVECTOR right = XMVector3Normalize(XMVector3Cross(up, dir));
    up = XMVector3Normalize(XMVector3Cross(dir, right));

    XMVECTOR back = -dir;

    XMVECTOR head1 =
        XMVector3Normalize(back * cosf(angle) + right * sinf(angle));

    XMVECTOR head2 =
        XMVector3Normalize(back * cosf(angle) - right * sinf(angle));

    XMFLOAT3 h1, h2;

    XMStoreFloat3(&h1, tip + head1 * headLength);
    XMStoreFloat3(&h2, tip + head2 * headLength);

    AddLine(p1, h1);
    AddLine(p1, h2);
}

void CDbgLineRender::AddGrid(uint32_t halfCount, float cellSize, FXMMATRIX world)
{
    if (m_Vertices.size() >= m_iVertexCnt) return;

    const float extent = static_cast<float>(halfCount) * cellSize;

    for (int i = -static_cast<int>(halfCount); i <= static_cast<int>(halfCount); ++i)
    {
        const float d = i * cellSize;

        // X축 방향 선
        {
            XMVECTOR p0 = XMVectorSet(-extent, 0.f, d, 1.f);
            XMVECTOR p1 = XMVectorSet(extent, 0.f, d, 1.f);

            p0 = XMVector3TransformCoord(p0, world);
            p1 = XMVector3TransformCoord(p1, world);

            XMFLOAT3 v0, v1;
            XMStoreFloat3(&v0, p0);
            XMStoreFloat3(&v1, p1);

            AddLine(v0, v1);
        }

        // Z축 방향 선
        {
            XMVECTOR p0 = XMVectorSet(d, 0.f, -extent, 1.f);
            XMVECTOR p1 = XMVectorSet(d, 0.f, extent, 1.f);

            p0 = XMVector3TransformCoord(p0, world);
            p1 = XMVector3TransformCoord(p1, world);

            XMFLOAT3 v0, v1;
            XMStoreFloat3(&v0, p0);
            XMStoreFloat3(&v1, p1);

            AddLine(v0, v1);
        }
    }
}

void CDbgLineRender::AddQuad(float width, float height, FXMMATRIX world)
{
    if (m_Vertices.size() >= m_iVertexCnt) return;

    const float hx = width * 0.5f;
    const float hy = height * 0.5f;

    XMVECTOR corners[4] =
    {
        XMVectorSet(-hx, -hy, 0.f, 1.f),
        XMVectorSet(hx, -hy, 0.f, 1.f),
        XMVectorSet(hx,  hy, 0.f, 1.f),
        XMVectorSet(-hx,  hy, 0.f, 1.f),
    };

    XMFLOAT3 p[4];

    for (int i = 0; i < 4; ++i)
    {
        corners[i] = XMVector3TransformCoord(corners[i], world);
        XMStoreFloat3(&p[i], corners[i]);
    }

    AddLine(p[0], p[1]);
    AddLine(p[1], p[2]);
    AddLine(p[2], p[3]);
    AddLine(p[3], p[0]);
}

void CDbgLineRender::AddTriangle(const _float3& p0, const _float3& p1, const _float3& p2)
{
    if (m_Vertices.size() >= m_iVertexCnt) return;

    AddLine(p0, p1);
    AddLine(p1, p2);
    AddLine(p2, p0);
}

void CDbgLineRender::AddAxis(
    float length,
    FXMMATRIX world)
{
    if (m_Vertices.size() >= m_iVertexCnt) return;

    XMVECTOR origin = XMVector3TransformCoord(
        XMVectorZero(), world);

    XMVECTOR x = XMVector3TransformCoord(
        XMVectorSet(length, 0.f, 0.f, 1.f), world);

    XMVECTOR y = XMVector3TransformCoord(
        XMVectorSet(0.f, length, 0.f, 1.f), world);

    XMVECTOR z = XMVector3TransformCoord(
        XMVectorSet(0.f, 0.f, length, 1.f), world);

    XMFLOAT3 o, px, py, pz;
    XMStoreFloat3(&o, origin);
    XMStoreFloat3(&px, x);
    XMStoreFloat3(&py, y);
    XMStoreFloat3(&pz, z);

    const auto prev = m_vColor;

    SetColor({0.5f, 0.f, 0.f, 1.f});
    AddArrow(o, { 1.f, 0.f, 0.f }, length);

    SetColor({ 0.f, 0.5f, 0.f, 1.f });
    AddArrow(o, { 0.f, 1.f, 0.f }, length);

    SetColor({ 0.f, 0.f, 0.5f, 1.f });
    AddArrow(o, { 0.f, 0.f, 1.f }, length);

    SetColor(prev);
}

void CDbgLineRender::AddCircle(
    float radius,
    FXMMATRIX world,
    uint32_t slice)
{
    if (m_Vertices.size() >= m_iVertexCnt) return;

    if (slice < 3)
        return;

    constexpr float PI = XM_PI;
    const float delta = XM_2PI / static_cast<float>(slice);

    XMFLOAT3 prev;

    for (uint32_t i = 0; i <= slice; ++i)
    {
        float theta = delta * i;

        XMVECTOR p = XMVectorSet(
            cosf(theta) * radius,
            sinf(theta) * radius,
            0.f,
            1.f);

        p = XMVector3TransformCoord(p, world);

        XMFLOAT3 curr;
        XMStoreFloat3(&curr, p);

        if (i != 0)
            AddLine(prev, curr);

        prev = curr;
    }
}

void CDbgLineRender::AddCross(
    const _float3& p,
    float size)
{
    if (m_Vertices.size() >= m_iVertexCnt) return;

    AddLine(
        { p.x - size, p.y, p.z },
        { p.x + size, p.y, p.z });

    AddLine(
        { p.x, p.y - size, p.z },
        { p.x, p.y + size, p.z });

    AddLine(
        { p.x, p.y, p.z - size },
        { p.x, p.y, p.z + size });
}

void CDbgLineRender::AddTriangleMesh(const _float3* vertices, uint32_t vertexCount, const uint32_t* indices, uint32_t triangleCount, FXMMATRIX world)
{
    if (m_Vertices.size() >= m_iVertexCnt) return;

    for (uint32_t i = 0; i < triangleCount; ++i)
    {
        uint32_t i0 = indices[i * 3 + 0];
        uint32_t i1 = indices[i * 3 + 1];
        uint32_t i2 = indices[i * 3 + 2];

        AddTriangle(vertices[i0], vertices[i1], vertices[i2]);
    }
}

void CDbgLineRender::AddConvexHull(const _float3* vertices, uint32_t vertexCount, const uint32_t* indices, uint32_t triangleCount, FXMMATRIX world)
{
    if (m_Vertices.size() >= m_iVertexCnt) return;

    if (!vertices || !indices)
        return;

    for (uint32_t i = 0; i < triangleCount; ++i)
    {
        const uint32_t i0 = indices[i * 3 + 0];
        const uint32_t i1 = indices[i * 3 + 1];
        const uint32_t i2 = indices[i * 3 + 2];

        if (i0 >= vertexCount ||
            i1 >= vertexCount ||
            i2 >= vertexCount)
            continue;

        XMVECTOR v0 = XMVector3TransformCoord(
            XMLoadFloat3(&vertices[i0]), world);

        XMVECTOR v1 = XMVector3TransformCoord(
            XMLoadFloat3(&vertices[i1]), world);

        XMVECTOR v2 = XMVector3TransformCoord(
            XMLoadFloat3(&vertices[i2]), world);

        XMFLOAT3 p0, p1, p2;
        XMStoreFloat3(&p0, v0);
        XMStoreFloat3(&p1, v1);
        XMStoreFloat3(&p2, v2);

        AddTriangle(p0, p1, p2);
    }
}

void CDbgLineRender::AddBuiltedVertices(const std::vector<VTX_COL>& vecVertices)
{
    if (m_Vertices.size() >= m_iVertexCnt + vecVertices.size()) return;



    m_Vertices.insert(m_Vertices.end(), vecVertices.begin(), vecVertices.end());
}



HRESULT CDbgLineRender::Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx)
{
    if (!m_bRender)
    {
        return S_OK;
    }

    const auto& vs = m_pDbgVShader;
    const auto& ps = m_pDbgPShader;
    const auto& viBuffer = m_pDbgBuffer;

    m_pContext->VSSetShader(vs->GetVertexShader().Get(), nullptr, 0);
    m_pContext->PSSetShader(ps->GetPixelShader().Get(), nullptr, 0);

    ID3D11Buffer* vertexBuffers[] = {
        viBuffer->GetVertexBuffer().Get()
    };
    uint32_t strides[] = {
        viBuffer->GetVertexStride()
    };
    uint32_t offsets[] = {
        0
    };
    m_pContext->IASetInputLayout(vs->GetInputLayout().Get());
    m_pContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
    //m_pContext->IASetIndexBuffer(viBuffer->GetIndexBuffer().Get(), viBuffer->GetIndexFormat(), 0);
    m_pContext->IASetPrimitiveTopology(viBuffer->GetPrimitiveType());



    {
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        if (SUCCEEDED(m_pContext->Map(viBuffer->GetVertexBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource))) {
            memcpy(mappedResource.pData, m_Vertices.data(), sizeof(VTX_COL) * m_Vertices.size());
            m_pContext->Unmap(viBuffer->GetVertexBuffer().Get(), 0);
        }
    }

    m_pContext->Draw((uint32_t)m_Vertices.size(), 0);



    return S_OK;
}

void CDbgLineRender::FrameEnd()
{
    m_Vertices.clear();
}

HRESULT CDbgLineRender::Initialize()
{
    if (auto res = E::CGameInstance::Get()
        .AddResourceT<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_DbgLineRender", "./ShaderFiles/DbgLineRender/DbgLineRender.hlsl"))
    {
        if (FAILED(res->Load()))
        {
            return E_FAIL;
        }
        m_pDbgVShader = res;
    }
    if (auto res = E::CGameInstance::Get()
        .AddResourceT<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_DbgLineRender", "./ShaderFiles/DbgLineRender/DbgLineRender.hlsl"))
    {
        if (FAILED(res->Load()))
        {
            return E_FAIL;
        }
        m_pDbgPShader = res;
    }
    
    if (auto res = CGameInstance::Get()
        .AddResourceT(TAG_RES_GRP_PERMANENT_BUFFER, "DVI_ColliderDbg", CResDynamicVIBuffer::Create()))
    {
        CResDynamicVIBuffer::DESC desc{};
        desc.ePrimitiveType = D3D_PRIMITIVE_TOPOLOGY_LINELIST;

        desc.iVertexStride = sizeof(VTX_COL);
        desc.iNumVertices = m_iVertexCnt;
        desc.vertexDesc = {
            .ByteWidth = desc.iVertexStride * desc.iNumVertices,
            .Usage = D3D11_USAGE_DYNAMIC,
            .BindFlags = D3D11_BIND_VERTEX_BUFFER,
            .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
        };

        if (FAILED(res->Load(desc)))
        {
            return E_FAIL;
        }

        m_pDbgBuffer = res;
    }
    m_Vertices.reserve(m_iVertexCnt);
    return S_OK;
}


UPtr<CDbgLineRender> CDbgLineRender::Create(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext)
{
    auto pInstance = ToUPtr(new CDbgLineRender{ pDevice, pContext });
    if (FAILED(pInstance->Initialize()))
    {
        MSG_BOX("Failed to Created : CRenderer");
        return nullptr;
    }
    return pInstance;
}

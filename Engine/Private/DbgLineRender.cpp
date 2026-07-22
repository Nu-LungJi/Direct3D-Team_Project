#include "pch.h"
#include "DbgLineRender.h"
#include "GameInstance.h"
#include "Resources.h"

NS_USING(Engine)

namespace
{
    uint32_t PackDbgLineColor(const _float4& color)
    {
        const auto PackChannel = [](_float value)
        {
            return static_cast<uint32_t>(std::clamp(value, 0.f, 1.f) * 255.f + 0.5f);
        };

        return PackChannel(color.x)
            | (PackChannel(color.y) << 8)
            | (PackChannel(color.z) << 16)
            | (PackChannel(color.w) << 24);
    }
}

CDbgLineRender::CDbgLineRender(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext)
    : m_pDevice { pDevice }
    , m_pContext{ pContext }
{
}

CDbgLineRender::~CDbgLineRender()
{
}

void CDbgLineRender::SetColor(const _float4& vColor)
{
    m_vColor = vColor;
    m_iPackedColor = PackDbgLineColor(vColor);
}

_bool CDbgLineRender::CanAddVertices(size_t iVertexCount) const
{
    const size_t iCurrentCount = std::min<size_t>(
        m_DepthVertices.size() + m_NoDepthVertices.size(), m_iVertexCnt);
    return iVertexCount <= m_iVertexCnt - iCurrentCount;
}

std::vector<VTX_DBG_LINE>& CDbgLineRender::GetCurrentVertices()
{
    return m_eDepthMode == DBG_LINE_DEPTH_MODE::ENABLED
        ? m_DepthVertices
        : m_NoDepthVertices;
}

const std::vector<VTX_DBG_LINE>& CDbgLineRender::GetCurrentVertices() const
{
    return m_eDepthMode == DBG_LINE_DEPTH_MODE::ENABLED
        ? m_DepthVertices
        : m_NoDepthVertices;
}

void CDbgLineRender::AddLine(const _float3& p0, const _float3& p1)
{
    if (!CanAddVertices(2)) return;

    auto& vertices = GetCurrentVertices();
    vertices.push_back({ p0, m_iPackedColor });
    vertices.push_back({ p1, m_iPackedColor });
}

void CDbgLineRender::AddLine(const _float3& p0, const _float3& p1, const _float4& col)
{
    if (!CanAddVertices(2)) return;

    auto& vertices = GetCurrentVertices();
    const uint32_t color = PackDbgLineColor(col);
    vertices.push_back({ p0, color });
    vertices.push_back({ p1, color });
}

void CDbgLineRender::AddBox(const _float3& halfExtent, FXMMATRIX world)
{
    if (!CanAddVertices(2)) return;

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
    if (!CanAddVertices(2)) return;

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
    if (!CanAddVertices(2)) return;

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
    if (!CanAddVertices(2)) return;

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

void CDbgLineRender::AddWedge(FXMMATRIX world)
{
    if (!CanAddVertices(18)) return;

    _float3 vertices[6] =
    {
        { -0.5f, -0.5f, -0.5f }, { 0.5f, -0.5f, -0.5f },
        { -0.5f, -0.5f,  0.5f }, { 0.5f, -0.5f,  0.5f },
        { -0.5f,  0.5f,  0.5f }, { 0.5f,  0.5f,  0.5f }
    };
    for (auto& vertex : vertices)
        XMStoreFloat3(&vertex, XMVector3TransformCoord(XMLoadFloat3(&vertex), world));

    constexpr uint32_t edges[][2] =
    {
        { 0, 1 }, { 1, 3 }, { 3, 2 }, { 2, 0 },
        { 2, 4 }, { 4, 5 }, { 5, 3 },
        { 0, 4 }, { 1, 5 }
    };
    for (const auto& edge : edges)
        AddLine(vertices[edge[0]], vertices[edge[1]]);
}

void CDbgLineRender::AddCone(float radius, float height, FXMMATRIX world)
{
    if (!CanAddVertices(2)) return;

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
    if (!CanAddVertices(2)) return;

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
    if (!CanAddVertices(2)) return;

    XMVECTOR o = XMLoadFloat3(&origin);
    XMVECTOR d = XMVector3Normalize(XMLoadFloat3(&direction));

    XMFLOAT3 end;
    XMStoreFloat3(&end, o + d * length);

    AddLine(origin, end);
}

void CDbgLineRender::AddArrow(const _float3& origin, const _float3& direction, float length, float headLength, float headAngleDeg)
{
    if (!CanAddVertices(2)) return;

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
    if (!CanAddVertices(2)) return;

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
    if (!CanAddVertices(2)) return;

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
    if (!CanAddVertices(2)) return;

    AddLine(p0, p1);
    AddLine(p1, p2);
    AddLine(p2, p0);
}

void CDbgLineRender::AddAxis(float length, FXMMATRIX world)
{
	if (!CanAddVertices(2))
		return;

	XMVECTOR origin = XMVector3TransformCoord(XMVectorZero(), world);

	XMVECTOR x = XMVector3TransformCoord(
		XMVectorSet(length, 0.f, 0.f, 1.f), world);

	XMVECTOR y = XMVector3TransformCoord(
		XMVectorSet(0.f, length, 0.f, 1.f), world);

	XMVECTOR z = XMVector3TransformCoord(
		XMVectorSet(0.f, 0.f, length, 1.f), world);

	XMFLOAT3 o;
	XMStoreFloat3(&o, origin);

	XMVECTOR xDir = XMVector3Normalize(x - origin);
	XMVECTOR yDir = XMVector3Normalize(y - origin);
	XMVECTOR zDir = XMVector3Normalize(z - origin);

	XMFLOAT3 fx, fy, fz;
	XMStoreFloat3(&fx, xDir);
	XMStoreFloat3(&fy, yDir);
	XMStoreFloat3(&fz, zDir);

	const auto prev = m_vColor;

	SetColor({ 0.5f, 0.f, 0.f, 1.f });
	AddArrow(o, fx, length, 0.001f);

	SetColor({ 0.f, 0.5f, 0.f, 1.f });
	AddArrow(o, fy, length, 0.001f);

	SetColor({ 0.f, 0.f, 0.5f, 1.f });
	AddArrow(o, fz, length, 0.001f);

	SetColor(prev);
}

void CDbgLineRender::AddCircle(
    float radius,
    FXMMATRIX world,
    uint32_t slice)
{
    if (!CanAddVertices(2)) return;

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
    if (!CanAddVertices(2)) return;

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
    if (!CanAddVertices(2)) return;

    if (!vertices || !indices)
        return;

    for (uint32_t i = 0; i < triangleCount; ++i)
    {
        uint32_t i0 = indices[i * 3 + 0];
        uint32_t i1 = indices[i * 3 + 1];
        uint32_t i2 = indices[i * 3 + 2];

        if (i0 >= vertexCount || i1 >= vertexCount || i2 >= vertexCount)
            continue;

        XMVECTOR v0 = XMVector3TransformCoord(XMLoadFloat3(&vertices[i0]), world);
        XMVECTOR v1 = XMVector3TransformCoord(XMLoadFloat3(&vertices[i1]), world);
        XMVECTOR v2 = XMVector3TransformCoord(XMLoadFloat3(&vertices[i2]), world);

        _float3 p0, p1, p2;
        XMStoreFloat3(&p0, v0);
        XMStoreFloat3(&p1, v1);
        XMStoreFloat3(&p2, v2);

        AddTriangle(p0, p1, p2);
    }
}

void CDbgLineRender::AddConvexHull(const _float3* vertices, uint32_t vertexCount, const uint32_t* indices, uint32_t triangleCount, FXMMATRIX world)
{
    if (!CanAddVertices(2)) return;

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
    if (!CanAddVertices(vecVertices.size())) return;

    auto& vertices = GetCurrentVertices();
    const size_t iOldSize = vertices.size();
    vertices.resize(iOldSize + vecVertices.size());

    for (size_t i = 0; i < vecVertices.size(); ++i)
    {
        vertices[iOldSize + i] = {
            vecVertices[i].pos,
            PackDbgLineColor(vecVertices[i].color)
        };
    }
}

void CDbgLineRender::AddPackedLineVertices(const void* pVertexData, size_t iVertexCount)
{
    if (!pVertexData || !CanAddVertices(iVertexCount))
        return;

    auto& vertices = GetCurrentVertices();
    const size_t iOldSize = vertices.size();
    vertices.resize(iOldSize + iVertexCount);

    memcpy(
        vertices.data() + iOldSize,
        pVertexData,
        sizeof(VTX_DBG_LINE) * iVertexCount);
}



HRESULT CDbgLineRender::Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx)
{
    const uint32_t depthVertexCount = static_cast<uint32_t>(m_DepthVertices.size());
    const uint32_t noDepthVertexCount = static_cast<uint32_t>(m_NoDepthVertices.size());
    if (!m_bRender || (depthVertexCount == 0 && noDepthVertexCount == 0))
    {
        return S_OK;
    }

    if (!pContext || !m_pDbgBuffer || !m_pDbgVShader || !m_pDbgPShader)
        return E_FAIL;

    const auto& vs = m_pDbgVShader;
    const auto& ps = m_pDbgPShader;
    const auto& viBuffer = m_pDbgBuffer;

    pContext->VSSetShader(vs->GetVertexShader().Get(), nullptr, 0);
    pContext->PSSetShader(ps->GetPixelShader().Get(), nullptr, 0);

    ID3D11Buffer* vertexBuffers[] = {
        viBuffer->GetVertexBuffer().Get()
    };
    uint32_t strides[] = {
        viBuffer->GetVertexStride()
    };
    uint32_t offsets[] = {
        0
    };
    pContext->IASetInputLayout(vs->GetInputLayout().Get());
    pContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
    //m_pContext->IASetIndexBuffer(viBuffer->GetIndexBuffer().Get(), viBuffer->GetIndexFormat(), 0);
    pContext->IASetPrimitiveTopology(viBuffer->GetPrimitiveType());


    {
        D3D11_MAPPED_SUBRESOURCE mappedResource{};
		if (FAILED(pContext->Map(
			viBuffer->GetVertexBuffer().Get(),
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&mappedResource)))
            return E_FAIL;

		auto* pDst = static_cast<VTX_DBG_LINE*>(mappedResource.pData);
		if (depthVertexCount > 0)
			memcpy(pDst, m_DepthVertices.data(), sizeof(VTX_DBG_LINE) * depthVertexCount);

		if (noDepthVertexCount > 0)
			memcpy(pDst + depthVertexCount, m_NoDepthVertices.data(), sizeof(VTX_DBG_LINE) * noDepthVertexCount);

		pContext->Unmap(viBuffer->GetVertexBuffer().Get(), 0);
    }

    if (!m_pDepthState)
    {
        m_pDepthState = CGameInstance::Get().GetResourceFirst<CResDepthStencilState>(
            TAG_RES_GRP_PERMANENT_STATE, "DS_DBG_LINE_DEPTH_ON");
        if (!m_pDepthState)
            return E_FAIL;
    }

    if (!m_pNoDepthState)
    {
        m_pNoDepthState = CGameInstance::Get().GetResourceFirst<CResDepthStencilState>(
            TAG_RES_GRP_PERMANENT_STATE, "DS_NO_DEPTHSTENCIL");
        if (!m_pNoDepthState)
            return E_FAIL;
    }

	ComPtr<ID3D11DepthStencilState> previousDepthState{};
    UINT previousStencilRef = 0;
	pContext->OMGetDepthStencilState(previousDepthState.GetAddressOf(), &previousStencilRef);

	if (depthVertexCount > 0)
	{
		pContext->OMSetDepthStencilState(m_pDepthState->GetDepthStencilState().Get(), 0);
		pContext->Draw(depthVertexCount, 0);
	}

	if (noDepthVertexCount > 0)
	{
		pContext->OMSetDepthStencilState(m_pNoDepthState->GetDepthStencilState().Get(), 0);
		pContext->Draw(noDepthVertexCount, depthVertexCount);
	}
	pContext->OMSetDepthStencilState(previousDepthState.Get(), previousStencilRef);

    return S_OK;
}

void CDbgLineRender::FrameEnd()
{
    m_DepthVertices.clear();
    m_NoDepthVertices.clear();
    m_eDepthMode = DBG_LINE_DEPTH_MODE::DISABLED;
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

        desc.iVertexStride = sizeof(VTX_DBG_LINE);
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
    m_DepthVertices.reserve(m_iVertexCnt / 2);
    m_NoDepthVertices.reserve(m_iVertexCnt / 2);
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

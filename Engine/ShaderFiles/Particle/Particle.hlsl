#include "../ShaderDefines.hlsl"

struct VS_IN
{
    float3 pos : POSITION;
    float2 texCoord : TEXCOORD0;
    float2 uvSize : TEXCOORD1;
    float2 size : PSIZE;
    float rotation : ROTATION;
    float4 color : COLOR0;
    uint texIndex : TEXINDEX;
    uint frameIndex : FRAMEINDEX;
    uint light : LIGHT;
    uint flag : FLAG;
};

struct VS_OUT
{
    float3 posW : POSITION;

    float2 texCoord : TEXCOORD0;
    float2 uvSize : TEXCOORD1;

    float2 size : TEXCOORD2;

    float rotation : TEXCOORD3;

    float4 color : COLOR0;

    uint texIndex : TEXINDEX;
    uint frameIndex : FRAMEINDEX;
    uint light : LIGHT;
    uint flag : FLAG;
};

struct GS_OUT
{
    float4 posH : SV_Position;

    float2 texCoord : TEXCOORD0;

    float4 color : COLOR0;
    
    uint light : LIGHT;

    uint texIndex : TEXINDEX;
};

struct PS_OUT
{
    float4 target0 : SV_Target0;
};

Texture2DArray gItemTexture16_16Array : register(t6);
Texture2DArray gEntityTexture64_32Array : register(t7);
Texture2DArray gEntityTexture64_64Array : register(t8);
Texture2DArray gBlockTextureArray : register(t9);
Texture2DArray g128_128TextureArray : register(t10);

SamplerState gSamPointWrap : register(s0);

VS_OUT VSMain(VS_IN vin)
{
    VS_OUT vout;

    vout.posW = vin.pos;

    vout.texCoord = vin.texCoord;
    vout.uvSize = vin.uvSize;

    vout.size = vin.size;

    vout.rotation = vin.rotation;

    vout.color = vin.color;

    vout.texIndex = vin.texIndex;
    vout.frameIndex = vin.frameIndex;
    vout.flag = vin.flag;
    vout.light = vin.light;

    return vout;
}

[maxvertexcount(4)]
void GSMain(
    point VS_OUT input[1],
    inout TriangleStream<GS_OUT> stream)
{
    VS_OUT particle = input[0];

    float3 center = particle.posW;

    //
    // camera billboard basis
    // 카메라의 월드 행렬의 라이트 업이 필요한데
    // 회전부분은 전치해서 읽으면 카메라의 역행렬의 회전 라업룩을 읽을수 있어서
    // 전치했다치고 읽는것
    float3 camRight =
        normalize(float3(
            g_matView._11,
            g_matView._21,
            g_matView._31));

    float3 camUp =
        normalize(float3(
            g_matView._12,
            g_matView._22,
            g_matView._32));
    //float3 camRight = normalize(float3(g_matView._11, g_matView._12, g_matView._13));
    //float3 camUp = normalize(float3(g_matView._21, g_matView._22, g_matView._23));
    //
    // quad corners
    //
    // TriangleStrip 순서 (Z자 모양)
    // 1---3
    // |  /|
    // | / |
    // |/  |
    // 0---2

    float2 corners[4] =
    {
        float2(-0.5f, -0.5f), // 0 좌하
        float2(-0.5f, 0.5f), // 1 좌상
        float2(0.5f, -0.5f), // 2 우하
        float2(0.5f, 0.5f), // 3 우상
    };

    //
    // uv corners
    //
    float2 uvCorners[4] =
    {
        float2(0.f, 1.f), // 0 좌하
        float2(0.f, 0.f), // 1 좌상
        float2(1.f, 1.f), // 2 우하
        float2(1.f, 0.f), // 3 우상
    };

    float s = sin(particle.rotation);
    float c = cos(particle.rotation);

    GS_OUT gout;

    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        float2 local =
            corners[i] * particle.size;

        //
        // rotation
        // 2d회전행렬 적용
        float2 rotated;

        rotated.x =
            local.x * c - local.y * s;

        rotated.y =
            local.x * s + local.y * c;

        //
        // billboard world pos
        //
        float3 worldPos =
            center
            + camRight * rotated.x
            + camUp * rotated.y;

        //
        // clip space
        //
        //float4 posV = mul(float4(worldPos, 1.f), g_matView);
        //gout.posH = mul(posV, g_matProj);
        
        gout.posH = mul(float4(worldPos, 1.f), g_matViewProj);
        
        //
        // uv
        //
        gout.texCoord =
            particle.texCoord
            + uvCorners[i] * particle.uvSize;

        gout.color =
            particle.color;

        gout.texIndex =
            particle.texIndex;
        
        gout.light = particle.light;

        stream.Append(gout);
    }

    stream.RestartStrip();
}

PS_OUT PSMain(GS_OUT pin)
{
    uint groupId =
        GetTexArrayGroup(pin.texIndex);

    uint sliceIndex =
        GetTexSliceIndex(pin.texIndex);

    float4 albedo = 1.f;

    if (groupId == 7)
    {
        albedo =
            gEntityTexture64_32Array.Sample(
                gSamPointWrap,
                float3(pin.texCoord, sliceIndex));
    }
    else if (groupId == 8)
    {
        albedo =
            gEntityTexture64_64Array.Sample(
                gSamPointWrap,
                float3(pin.texCoord, sliceIndex));
    }
    else if (groupId == 6)
    {
        albedo =
            gItemTexture16_16Array.Sample(
                gSamPointWrap,
                float3(pin.texCoord, sliceIndex));
    }
    else if (groupId == 9)
    {
        albedo =
            gBlockTextureArray.Sample(
                gSamPointWrap,
                float3(pin.texCoord, sliceIndex));
    }
    else if (groupId == 10)
    {
        albedo =
            g128_128TextureArray.Sample(
                gSamPointWrap,
                float3(pin.texCoord, sliceIndex));
    }

        clip(albedo.a - 0.01f);
    
    uint blockLightRaw = pin.light & 0xF; // 하위 4비트
    uint skyLightRaw = (pin.light >> 4) & 0xF; // 상위 4비트

    // 2. 0.0f ~ 1.0f 범위의 float 비율로 변환
    float blockLight = blockLightRaw / 15.0f;
    float skyLight = (skyLightRaw / 15.0f) * g_fDayFactor; // 시간에 따른 가중치 반영

    // 3. 둘 중 큰 값을 최종 라이트 강도로 선택
    float finalLight = max(blockLight, skyLight);

    // 4. 동굴 깊은 곳이나 한밤중에도 최소한의 형체는 보이도록 최저 밝기 보정 (예: 8%)
    finalLight = max(finalLight, 0.08f);

    PS_OUT pout;

    pout.target0 =
        albedo * pin.color * float4(finalLight, finalLight, finalLight, 1.0f);

    return pout;
}
#include "../../Engine/ShaderFiles/ShaderDefines.hlsl"

#define BEHAVIOR_NONE       0
#define BEHAVIOR_DISTORTION (1u << 1)
#define BEHAVIOR_BILLBOARD (1u << 2)
#define BEHAVIOR_GRAVITY (1u << 3)
struct SPAWN_DATA
{
    float3 position;
    float pad0;
    float3 velocity;
    float life;
    float size;
    float endSize;
    float2 pad1;
    float4 rotation;
    float4 color;
    float4 originalEmissive;
    float4 emissive;
    float4  endEmissive;
    float spawnDelay;
    uint ownerID;
    uint iBehaviorType;
    float pad2;
    uint loop;
    float3 originalPosition; // 원래 스폰 위치
};


struct ParticleData
{
    float3 position;
    float pad1;
    float3 velocity;
    float life;
    float maxLife;
    float size;
    float startSize;
    float endSize;
    float4 rotation;
    uint alive;
    uint loop;
    float2 pad2; // 추가 필요: loop→color (8바이트)
    float4 color;
    float4 originalEmissive;
    float4 emissive;
    float4 endEmissive;
    uint frameIndex;
    uint ownerID;
    uint iBehaviorType ;
    float pad3;
    float3 originalPosition; // 원래 스폰 위치
    float pad4;
};
float3 RotateXYZ(float3 pos, float4 rotation)
{
    // rotation.x = pitch (X축 회전), rotation.y = yaw (Y축), rotation.z = roll (Z축)
    // 단위: 라디안

    float sx = sin(rotation.x);
    float cx = cos(rotation.x);
    float sy = sin(rotation.y);
    float cy = cos(rotation.y);
    float sz = sin(rotation.z);
    float cz = cos(rotation.z);

    // X축 회전
    float3 p = pos;
    p = float3(
        p.x,
        p.y * cx - p.z * sx,
        p.y * sx + p.z * cx
    );

    // Y축 회전
    p = float3(
        p.x * cy + p.z * sy,
        p.y,
        -p.x * sy + p.z * cy
    );

    // Z축 회전
    p = float3(
        p.x * cz - p.y * sz,
        p.x * sz + p.y * cz,
        p.z
    );

    return p;
}

float3 Compute_WorldNormal(Texture2D _NormalTex, float2 _TexCoord, float3 _InNormal, float3 _InTangent)
{
    float3 LocalNormal = _NormalTex.Sample(LinearWrap, _TexCoord).rgb;
    LocalNormal = normalize(LocalNormal * 2.f - 1.f);

    float3 N = normalize(_InNormal);
    float3 T = normalize(_InTangent);
    T = normalize(T - dot(T, N) * N);
    float3 B = normalize(cross(N, T));

    return normalize(LocalNormal.x * T + LocalNormal.y * B + LocalNormal.z * N);
}

bool Compute_DynamicLight(DynamicLight _Light, float3 _WorldPosition, inout float3 L, inout float3 Radiance)
{
    [branch]
    if (_Light.LightType == LIGHT_DIRECTIONAL)
    {
        L = normalize(-_Light.LightDirection);
        Radiance = _Light.LightColor * _Light.LightIntensity;
        return true;
    }
    else if (_Light.LightType == LIGHT_POINT)
    {
        float3 LightVector = _Light.Position - _WorldPosition;
        float Distance = length(LightVector);
        if (Distance > _Light.LightRange)
            return false;

        float Attenuation = 1.f / max(Distance * Distance, 0.0001f);
        float DistanceByRange = Distance / _Light.LightRange;
        float Window = clamp(1.f - pow(DistanceByRange, 4.f), 0.f, 1.f);

        L = normalize(LightVector);
        Radiance = _Light.LightColor * _Light.LightIntensity * (Attenuation * Window * Window);
        return true;
    }
    else if (_Light.LightType == LIGHT_SPOTLIGHT)
    {
        float3 LightVector = _Light.Position - _WorldPosition;
        float Distance = length(LightVector);
        if (Distance > _Light.LightRange)
            return false;

        float Attenuation = 1.f / max(Distance * Distance, 0.0001f);
        float DistanceByRange = Distance / _Light.LightRange;
        float Window = clamp(1.f - pow(DistanceByRange, 4.f), 0.f, 1.f);
        float DistanceFade = Attenuation * Window * Window;

        L = normalize(LightVector);

        float CosAngle = dot(-L, normalize(_Light.LightDirection));
        float Num = CosAngle - _Light.OuterAttanuation;
        float DeNum = _Light.InnerAttanuation - _Light.OuterAttanuation;
        float ConeFade = clamp(Num / max(0.000001f, DeNum), 0.f, 1.f);

        Radiance = _Light.LightColor * _Light.LightIntensity * (DistanceFade * ConeFade * ConeFade);
        return true;
    }
    return false;
}

float DistributionGGX(float3 N, float3 H, float _Roughness)
{
    float R = _Roughness * _Roughness;
    float R2 = R * R;
    float NDH = max(0.f, dot(N, H));
    float NDH2 = NDH * NDH;
    float Num = R2;
    float Denom = (NDH2 * (R2 - 1.0) + 1.0);
    Denom = PI * Denom * Denom;
    return Num / max(0.000001f, Denom);
}

float VisibilitySmithJointGGX(float NdotV, float NdotL, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float lambdaV = NdotL * sqrt(max((-NdotV * a2 + NdotV) * NdotV + a2, 0.001f));
    float lambdaL = NdotV * sqrt(max((-NdotL * a2 + NdotL) * NdotL + a2, 0.001f));
    float Denom = lambdaV + lambdaL;
    return Denom > 0.0f ? 0.5f / Denom : 0.0f;
}

float3 FresnelSchlick(float CTH, float3 MBR)
{
    float ClampCTH = clamp(CTH, 0.0f, 1.0f);
    return MBR + (1.0 - MBR) * pow(clamp(1.0 - ClampCTH, 0.0, 1.0), 5.0);
}

float3x3 Make_TBNMatrix(float3 _Normal, float3 _Tangent)
{
    float3 Normal = normalize(_Normal);
    float3 Tangent = normalize(_Tangent);
    Tangent = normalize(Tangent - dot(Tangent, Normal) * Normal);
    float3 BiNormal = normalize(cross(Normal, Tangent));
    return float3x3(Tangent, BiNormal, Normal);
}


float3 ReconstructWorldPos(float2 uv, float depth)
{
    float x = uv.x * 2.0f - 1.0f;
    float y = (1.0f - uv.y) * 2.0f - 1.0f;
    float z = depth;

    float4 ndcPos = float4(x, y, z, 1.0f);
    
    float4 worldPos = mul(ndcPos, g_matInvViewProj);
    return worldPos.xyz / worldPos.w;
}

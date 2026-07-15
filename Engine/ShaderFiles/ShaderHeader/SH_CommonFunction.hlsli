#include "../ShaderDefines.hlsl"

float4   Convert_WorldPosByDepth(float _Depth, float2 _TexCoord)
{
    // Depth = NDC   -> (InvProj) -> WorldSpace(InvView)
    float4 NDCWorldPos;
    
    // ViewSpace
    NDCWorldPos.x = _TexCoord.x * +2.f - 1.f;
    NDCWorldPos.y = _TexCoord.y * -2.f + 1.f;
    NDCWorldPos.z = _Depth;
    NDCWorldPos.w = 1.f;
    
    float4 WorldPos = mul(NDCWorldPos, g_matInvViewProj);
    
    return float4(WorldPos.xyz / WorldPos.w, 1.f);
}

float3x3 Make_TBNMatrix(float3 _Normal, float3 _Tangent)
{
    float3 Normal = normalize(_Normal);
    float3 Tangent = normalize(_Tangent);

    Tangent = normalize(Tangent - dot(Tangent, Normal) * Normal);
    
    float3 BiNormal = normalize(cross(Normal, Tangent));
    
    return float3x3(Tangent, BiNormal, Normal);
}
float3   Compute_WorldNormal(Texture2D _NormalTex, float2 _TexCoord, float4 _InNormal, float4 _InTangent)
{
    float3 LocalNormal = _NormalTex.Sample(LinearWrap, _TexCoord).rgb;
    LocalNormal = normalize(LocalNormal * 2.f - 1.f);
    float3x3 TBN = Make_TBNMatrix(_InNormal.xyz, _InTangent.xyz);

    float3 N = normalize(_InNormal.xyz);
    float3 T = normalize(_InTangent.xyz);
    
    T = normalize(T - dot(T, N) * N);
    float3 B = normalize(cross(N, T));
    
    float3 worldNormal = LocalNormal.x * T + LocalNormal.y * B + LocalNormal.z * N;

    return normalize(worldNormal);
}


bool Compute_DynamicLight(float3 _WorldPosition, out float3 L, out float3 Radiance)
{
    // Directional Light PBR
    [flatten]
    if (LightType == LIGHT_DIRECTIONAL)
    {
        L = normalize(-LightDirection.xyz);
        Radiance = LightColor * LightIntensity;
    }
    // Point Light PBR
    else if (LightType == LIGHT_POINT)
    {
        float MinimumDistance = 1.f;
        
        float3 LightVector = LightPosition - _WorldPosition;
        float Distance = length(LightVector);
        
        [flatten]
        if (Distance > LightRange)
            return false;

        float Attenuation = 1.f / max(Distance * Distance, 0.0001f);
        float DistanceByRange = Distance / LightRange;
        float Window = clamp(1.f - pow(DistanceByRange, 4.f), 0.f, 1.f);
        
        L = normalize(LightVector);
        Radiance = LightColor * LightIntensity * (Attenuation * Window * Window);
    }
    // SpotLight Light PBR
    else if (LightType == LIGHT_SPOTLIGHT)
    {
        float MinimumDistance = 1.f;
        
        float3 LightVector = LightPosition - _WorldPosition;
        float Distance = length(LightVector);
        [flatten]
        if (Distance > LightRange)
            return false;
        
        // Decrease By Distance
        float Attenuation = 1.f / max(Distance * Distance, 0.0001f);
        float DistanceByRange = Distance / LightRange;
        float Window = clamp(1.f - pow(DistanceByRange, 4.f), 0.f, 1.f);
        float DistanceFade = Attenuation * Window * Window;
        
        L = normalize(LightVector);
        
         // Decrease By SpotLight Cone
        float CosAngle = dot(-L, normalize(LightDirection));
        float Num = CosAngle - OuterAttanuation;
        float DeNum = InnerAttanuation - OuterAttanuation;
        float ConeFade = clamp(Num / max(0.000001f, DeNum), 0.f, 1.f);
        
        Radiance = LightColor * LightIntensity * (DistanceFade * ConeFade * ConeFade);
    }
    else
    {
        return false;
    }
    
    return true;
}

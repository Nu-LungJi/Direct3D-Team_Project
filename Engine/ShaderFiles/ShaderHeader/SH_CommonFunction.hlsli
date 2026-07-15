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


bool Compute_DynamicLight(float3 _WorldPosition, DynamicLight Light, out float3 L, out float3 Radiance)
{

    [flatten]
    if (Light.LightType == LIGHT_DIRECTIONAL)   // Directional Light PBR
    {
        L = normalize(-Light.LightDirection.xyz);
        Radiance = Light.LightColor * Light.LightIntensity;
    }
    else if (Light.LightType == LIGHT_POINT)    // Point Light PBR
    {
        float MinimumDistance = 1.f;
    
        float3 LightVector = Light.Position - _WorldPosition;
        float Distance = length(LightVector);
    
        [flatten]
        if (Distance > Light.LightRange)
            return false;
    
        // Decrease By Distance
        float Attenuation = 1.f / max(Distance * Distance, 0.0001f);
        float DistanceByRange = Distance / Light.LightRange;
        float Window = clamp(1.f - pow(DistanceByRange, 4.f), 0.f, 1.f);
    
        L = normalize(LightVector);
        Radiance = Light.LightColor * Light.LightIntensity * (Attenuation * Window * Window);
    }
    else if (Light.LightType == LIGHT_SPOTLIGHT)    // SpotLight Light PBR
    {
        float MinimumDistance = 1.f;
    
        float3 LightVector = Light.Position - _WorldPosition;
        float Distance = length(LightVector);
    
        [flatten]
        if (Distance > Light.LightRange)
            return false;
    
        // Decrease By Distance
        //float Attenuation = 1.f / max(Distance * Distance, 0.0001f);
        float Attenuation = 1.f / max(Distance, 0.0001f);
        float DistanceByRange = Distance / Light.LightRange;
        float Window = clamp(1.f - pow(DistanceByRange, 4.f), 0.f, 1.f);
        float DistanceFade = Attenuation * Window * Window;
    
        L = normalize(LightVector);
    
        // Decrease By SpotLight Cone
        float CosAngle = dot(-L, normalize(Light.LightDirection));
        float Num = CosAngle - Light.OuterAttanuation;
        float DeNum = Light.InnerAttanuation - Light.OuterAttanuation;
        float ConeFade = clamp(Num / max(0.000001f, DeNum), 0.f, 1.f);
    
        Radiance = Light.LightColor * Light.LightIntensity * DistanceFade; // * ConeFade * ConeFade);
    }
    
    return true;
}

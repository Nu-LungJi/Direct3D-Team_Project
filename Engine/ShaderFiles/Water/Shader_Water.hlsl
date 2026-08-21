#include "../ShaderDefines.hlsl"

// ==========================================
// Constant Buffers & Registers
// ==========================================

cbuffer CB_Water : register(b11)
{
	float4 g_vWaterColor;
	float4 g_vShallowColor;
	float4 g_vDeepColor;
	float4 g_vReflectionColor;
	float2 g_vScrollSpeed1;
	float2 g_vScrollSpeed2;
	float g_fTime;
	float g_fWaveIntensity;
	float g_fUVScale;
	float g_fSecondaryNormalScale;
};

// ==========================================
// Textures & Samplers
// ==========================================

Texture2D g_NormalTex0 : register(t0);
Texture2D g_NormalTex1 : register(t1);
SamplerState g_LinearSampler : register(s0);

// ==========================================
// I/O Structures
// ==========================================

struct VS_INPUT
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float2 uv : TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 position : SV_POSITION;
	float3 worldPos : TEXCOORD0;
	float3 viewDir : TEXCOORD1;
};

// ==========================================
// Vertex Shader
// ==========================================

VS_OUTPUT VSMain(VS_INPUT input)
{
	VS_OUTPUT output = (VS_OUTPUT) 0;

	float4 worldPos = mul(float4(input.position, 1.0f), g_matWorld);
	output.position = mul(float4(input.position, 1.0f), g_matWVP);
	output.worldPos = worldPos.xyz;
	float3 cameraPos = g_matInvView._41_42_43;
	output.viewDir = normalize(cameraPos - worldPos.xyz);

	return output;
}

// ==========================================
// Pixel Shader
// ==========================================

float4 PSMain(VS_OUTPUT input) : SV_Target
{
    // 시간에 따른 노멀 맵 UV 스크롤 계산
	// 월드 좌표를 사용하므로 수면 크기와 카메라 추적에 관계없이 파도 크기가 일정하다.
	float2 worldUV = input.worldPos.xz * g_fUVScale;
	float2 uv1 = worldUV + (g_vScrollSpeed1 * g_fTime);
	float2 uv2 = worldUV * g_fSecondaryNormalScale + (g_vScrollSpeed2 * g_fTime);

    // 두 장의 노멀 맵 샘플링 및 [0, 1] -> [-1, 1] 변환
	float3 normal1 = g_NormalTex0.Sample(g_LinearSampler, uv1).xyz * 2.0f - 1.0f;
	float3 normal2 = g_NormalTex1.Sample(g_LinearSampler, uv2).xyz * 2.0f - 1.0f;
    
    // 두 노멀을 합성하고 강도 조절
	float2 slope = (normal1.xy + normal2.xy) * g_fWaveIntensity;
	float normalY = max(0.05f, normal1.z * normal2.z);
	float3 waterNormal = normalize(float3(slope.x, normalY, slope.y));

    // 프레넬 효과 (Schlick 근사식: 시선이 수면에 누울수록 반사율이 높아짐)
	float3 viewDir = normalize(input.viewDir);
	float NdotV = saturate(dot(waterNormal, viewDir));
	float fresnel = pow(1.0f - NdotV, 4.0f);
	fresnel = lerp(0.1f, 0.9f, fresnel); // 최소/최대 반사율 클램프

    // 깊이/색상 틴트 및 반사 색상 합성
	float horizonFactor = saturate(1.0f - abs(viewDir.y));
	float3 baseWaterColor = lerp(g_vShallowColor.rgb, g_vDeepColor.rgb, 0.65f + horizonFactor * 0.2f);
	baseWaterColor = lerp(baseWaterColor, g_vWaterColor.rgb, 0.35f);
	float3 finalColor = lerp(baseWaterColor, g_vReflectionColor.rgb, fresnel);

	// 프레넬만으로는 정면에서 잔물결이 잘 보이지 않으므로 경사 대비와 태양 하이라이트를 더한다.
	float rippleContrast = saturate(length(slope) * 0.65f);
	finalColor *= lerp(0.82f, 1.12f, rippleContrast);
	float3 lightDir = normalize(float3(-0.45f, 0.8f, -0.35f));
	float specular = pow(saturate(dot(reflect(-lightDir, waterNormal), viewDir)), 48.0f);
	finalColor += g_vReflectionColor.rgb * specular * 0.45f;

	return float4(finalColor, g_vWaterColor.a);
}

#include "../ShaderDefines.hlsl"

// ==========================================
// Constant Buffers & Registers
// ==========================================

cbuffer CB_Water : register(b11)
{
	float4 g_vWaterColor; // 물 기본 색상 (RGB) 및 알파 (A)
	float2 g_vScrollSpeed1; // 첫 번째 노멀 맵 스크롤 속도
	float2 g_vScrollSpeed2; // 두 번째 노멀 맵 스크롤 속도
	float g_fTime; // 누적 시간
	float g_fWaveIntensity; // 파도 노멀 왜곡 강도
	float2 g_Padding;
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
	float2 uv : TEXCOORD1;
	float3 viewDir : TEXCOORD2;
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
	output.uv = input.uv;

    // 카메라 위치 계산 (상수 버퍼나 뷰 행렬 역행렬 활용 가능, 여기서는 예시로 대략적인 시선 벡터 계산)
    // 엔진 내부에 카메라 포지션 상수 버퍼가 있다면 해당 값을 사용하는 것이 가장 정확합니다.
	float3 cameraPos = g_matInvView._41_42_43;; // 필요시 엔진 카메라 위치로 대체
	output.viewDir = normalize(cameraPos - worldPos.xyz);

	return output;
}

// ==========================================
// Pixel Shader
// ==========================================

float4 PSMain(VS_OUTPUT input) : SV_Target
{
    // 시간에 따른 노멀 맵 UV 스크롤 계산
	float2 uv1 = input.uv * 10.0f + (g_vScrollSpeed1 * g_fTime);
	float2 uv2 = input.uv * 20.0f + (g_vScrollSpeed2 * g_fTime);

    // 두 장의 노멀 맵 샘플링 및 [0, 1] -> [-1, 1] 변환
	float3 normal1 = g_NormalTex0.Sample(g_LinearSampler, uv1).xyz * 2.0f - 1.0f;
	float3 normal2 = g_NormalTex1.Sample(g_LinearSampler, uv2).xyz * 2.0f - 1.0f;
    
    // 두 노멀을 합성하고 강도 조절
	float3 waterNormal = normalize(normal1 + normal2);
	waterNormal.xy *= g_fWaveIntensity;
	waterNormal = normalize(waterNormal);

    // 프레넬 효과 (Schlick 근사식: 시선이 수면에 누울수록 반사율이 높아짐)
	float3 viewDir = normalize(input.viewDir);
	float NdotV = saturate(dot(waterNormal, viewDir));
	float fresnel = pow(1.0f - NdotV, 4.0f);
	fresnel = lerp(0.1f, 0.9f, fresnel); // 최소/최대 반사율 클램프

    // 깊이/색상 틴트 및 반사 색상 합성
	float3 deepWaterColor = float3(0.02f, 0.1f, 0.2f); // 깊은 곳 색상
	float3 shallowColor = float3(0.1f, 0.4f, 0.5f); // 얕은 곳 색상
	float3 reflectionColor = float3(0.7f, 0.8f, 0.9f); // 하늘/주변 반사 색상

    // 기본 물 색상과 프레넬 반사 합성
	float3 baseWaterColor = lerp(shallowColor, deepWaterColor, 0.5f);
	float3 finalColor = lerp(baseWaterColor, reflectionColor, fresnel);

	return float4(finalColor, g_vWaterColor.a);
}

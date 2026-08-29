#include "../ShaderHeader/SH_CommonFunction.hlsli"

RWTexture2D<float4> OUTPUT : register(u0);

Texture2D<float4>	SceneColorTexture	: register(t0);
Texture2D<float4>	SceneDepthTexture	: register(t1);
Texture2D<float4>	DecalSurfaceTexture : register(t2);
Texture2D<float4>	SSRSurfaceTexture	: register(t3);
Texture2D<float4>	SceneNormalTexture	: register(t4);
Texture2D<float4>	PlanarEffectTexture	: register(t5);

static const float	MIN_RAY_DISTANCE	= 0.25f;
static const float	MAX_DISTANCE		= 48.f;
static const float	DEPTH_THICKNESS		= 0.15f;
static const uint	STEP_COUNT			= 192;
static const float	STEP_LENGTH			= MAX_DISTANCE / STEP_COUNT;

static const float SSR_INTENSITY = 1.6f; // 플레이어/일반 모델
static const float SSR_COLOR_GAIN = 1.15f;
static const float PLANAR_EFFECT_INTENSITY = 0.7f; // 이펙트 과반사 억제

cbuffer CB_PLANAR_REFLECTION : register(b8)
{
	float4x4	g_mPlanarReflectionViewProj;
	float		g_fPlanarReflectionEnabled;
	float3		g_fPlanarPadding;
};

bool RayMarch_Reflection(float3 _RayOrigin, float3 _ReflectDir, inout float3 _RayHitColor, inout float2 _RayTexCoord, uint _ScreenWidth, uint _ScreenHeight)
{
	bool	RayHit = false;
	float2	RayHitTexCoord	= 0.f;
	
	for (uint Step = 1; Step <= STEP_COUNT; ++Step) {
		
		float rayTravel = Step * STEP_LENGTH;

		if (rayTravel < MIN_RAY_DISTANCE)
			continue;
		float3	RayPosition = _RayOrigin + _ReflectDir * (Step * STEP_LENGTH);
		
		float4	ClipPos = mul(float4(RayPosition, 1.f), g_matViewProj);
		if (ClipPos.w <= 0.f)	break;

		float3	NDC = ClipPos.xyz / ClipPos.w;
		float2	RayTexCoord = float2(NDC.x * +0.5f + 0.5f, NDC.y * -0.5f + 0.5f);

		if (any(RayTexCoord <= 0.f)  || any(RayTexCoord >= 1.f))	break;
		
		int2 RayPixel = int2(RayTexCoord * float2(_ScreenWidth, _ScreenHeight));
		RayPixel = clamp(RayPixel, int2(0, 0), int2(_ScreenWidth - 1, _ScreenHeight - 1));
		
		float hitPuddleWeight = DecalSurfaceTexture.Load(int3(RayPixel, 0)).a;
		float hitRwbFloorWeight = SSRSurfaceTexture.Load(int3(RayPixel, 0)).r;

		if (max(hitPuddleWeight, hitRwbFloorWeight) > 0.001f)
			continue;

		float SceneDepth = SceneDepthTexture.Load(int3(RayPixel, 0)).r;
		if (SceneDepth >= 0.99999f)
			continue;

		float3 sceneWorld = Convert_WorldPosByDepth(SceneDepth, RayTexCoord).xyz;
		
		float rayDistance = length(RayPosition - g_vCamPos);
		float sceneDistance = length(sceneWorld - g_vCamPos);
		float depthDelta = rayDistance - sceneDistance;
		
		if (depthDelta >= -DEPTH_THICKNESS && depthDelta <= STEP_LENGTH + DEPTH_THICKNESS)
		{
			_RayTexCoord = RayTexCoord;
			_RayHitColor = SceneColorTexture.Load(int3(RayPixel, 0)).rgb;
			return true;
		}
	} 
	
	return RayHit;
}

float3 SamplePlanarEffectBlurred(float2 uv)
{
	uint width, height;
	PlanarEffectTexture.GetDimensions(width, height);

	float2 texel = 1.f / float2(width, height);

	float3 color = PlanarEffectTexture.SampleLevel(
        LinearClamp, uv, 0).rgb * 0.40f;

	color += PlanarEffectTexture.SampleLevel(
        LinearClamp, uv + float2(texel.x, 0.f), 0).rgb * 0.10f;
	color += PlanarEffectTexture.SampleLevel(
        LinearClamp, uv - float2(texel.x, 0.f), 0).rgb * 0.10f;
	color += PlanarEffectTexture.SampleLevel(
        LinearClamp, uv + float2(0.f, texel.y), 0).rgb * 0.10f;
	color += PlanarEffectTexture.SampleLevel(
        LinearClamp, uv - float2(0.f, texel.y), 0).rgb * 0.10f;

	color += PlanarEffectTexture.SampleLevel(
        LinearClamp, uv + texel, 0).rgb * 0.05f;
	color += PlanarEffectTexture.SampleLevel(
        LinearClamp, uv - texel, 0).rgb * 0.05f;
	color += PlanarEffectTexture.SampleLevel(
        LinearClamp, uv + float2(texel.x, -texel.y), 0).rgb * 0.05f;
	color += PlanarEffectTexture.SampleLevel(
        LinearClamp, uv + float2(-texel.x, texel.y), 0).rgb * 0.05f;

	return color;
}

float3 Capture_PlanarReflection(float4 _DepthWorld, float _FloorWeight) {
	float3 PlanarEffectColor = 0.f;
	
	if (_FloorWeight > 0.001f && g_fPlanarReflectionEnabled > 0.5f) {
		float4 PlanarClip = mul(float4(_DepthWorld.xyz, 1.f), g_mPlanarReflectionViewProj);
		
		if (PlanarClip.w > 0.f) {
			float2 PlanarTexCoord = float2(
				 PlanarClip.x / PlanarClip.w * 0.5f + 0.5f,
				-PlanarClip.y / PlanarClip.w * 0.5f + 0.5f
			);
			
			if (all(PlanarTexCoord >= 0.f) && all(PlanarTexCoord <= 1.f)) {
				float4 PlanarEffect = PlanarEffectTexture.SampleLevel(LinearClamp, PlanarTexCoord, 0);
				PlanarEffectColor = SamplePlanarEffectBlurred(PlanarTexCoord);
			}
		}
	}
	
	return PlanarEffectColor;
}



[numthreads(8, 8, 1)]
void CSMain(uint3 ID : SV_DispatchThreadID) {
	uint ScreenWidth, ScreenHeight;
	SceneColorTexture.GetDimensions(ScreenWidth, ScreenHeight);
	
	[branch]
	if (ID.x >= ScreenWidth || ID.y >= ScreenHeight) return;
	
	int3	PixelCoord = int3(ID.xy, 0);
	float2	TexCoord = (float2(ID.xy) + 0.5f) / float2(ScreenWidth, ScreenHeight);
	
	float4	SceneColor	= SceneColorTexture.Load(PixelCoord);
	
	float4	DecalSurfaceColor = DecalSurfaceTexture.Load(PixelCoord);
	float	DecalWeight = saturate(DecalSurfaceColor.a);
	
	float DepthTex = SceneDepthTexture.Load(PixelCoord).r;
	
	if (DepthTex >= 0.999999f)
	{
		OUTPUT[ID.xy] = SceneColor;
		return;
	}
	
	float4 decalSurface = DecalSurfaceTexture.Load(PixelCoord);
	float decalWeight = saturate(decalSurface.a);

	float4 ssrSurface = SSRSurfaceTexture.Load(PixelCoord);
	float floorWeight = saturate(ssrSurface.r);

	float reflectionMask = max(decalWeight, floorWeight);

	if (reflectionMask <= 0.001f)
	{
		OUTPUT[ID.xy] = SceneColor;
		return;
	}
	float3 surfaceNormal;
	float roughness;
	
	if (decalWeight > 0.001f)
	{
		float2 packedNormal = saturate(decalSurface.gb / max(decalWeight, 0.001f));
		
		surfaceNormal = DecodeOctNormal(packedNormal);
		roughness = saturate(decalSurface.r / max(decalWeight, 0.001f));
	}
	else
	{
		surfaceNormal = normalize(SceneNormalTexture.Load(PixelCoord).xyz * 2.f - 1.f);
		roughness = clamp(ssrSurface.g, 0.03f, 0.12f);
	}
	
	if (decalWeight > 0.001f)
	{
		surfaceNormal = float3(0.f, 1.f, 0.f);
	}
	
	float4 DepthWorld	 = Convert_WorldPosByDepth(DepthTex, TexCoord);

	float3 PlanarEffectColor = Capture_PlanarReflection(DepthWorld, floorWeight);

	float2 PackedNormal  = saturate(DecalSurfaceColor.gb / max(DecalWeight, 0.001f));
	float3 PuddleNormal  = DecodeOctNormal(PackedNormal);

	float3 ViewDirection = normalize(g_vCamPos - DepthWorld.xyz);
	float3 ReflectDir = normalize(reflect(-ViewDirection, surfaceNormal));
	float3 RayOrigin = DepthWorld.xyz + surfaceNormal * 0.15f;

	float3 RayHitColor	 = 0.f;
	float2 RayTexCoord	 = 0.f;

	bool SSRHit = RayMarch_Reflection(RayOrigin, ReflectDir, RayHitColor, RayTexCoord, ScreenWidth, ScreenHeight);
	
	float NDV = saturate(dot(surfaceNormal, ViewDirection));
	float Fresnel = 0.12f + 0.88f * pow(1.f - NDV, 2.5f);
	float BaseReflection = saturate(max(Fresnel, 0.06f) * (1.f - roughness) * 1.5f);
	
	float EdgeFade = 0.f;
	
	if (SSRHit) {
		EdgeFade = saturate(min(min(RayTexCoord.x, RayTexCoord.y), min(1.f - RayTexCoord.x, 1.f - RayTexCoord.y)) * 12.f);
	}
	float SSRAmount = SSRHit ? reflectionMask * BaseReflection * EdgeFade * SSR_INTENSITY : 0.f;
	
	float PlanarAmount = floorWeight * BaseReflection * PLANAR_EFFECT_INTENSITY;
	float3 FinalColor = lerp(SceneColor.rgb, RayHitColor * SSR_COLOR_GAIN, SSRAmount);
	
	FinalColor += PlanarEffectColor * PlanarAmount;
   
	OUTPUT[ID.xy] = float4(FinalColor * 0.7f, SceneColor.a);
	return; 
}

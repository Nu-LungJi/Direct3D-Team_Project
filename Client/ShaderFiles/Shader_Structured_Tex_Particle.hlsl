#include "../../Engine/ShaderFiles/Particle/Particle_Common_Struct_Func.hlsl"

cbuffer CB_PER_PARTICLE : register(b11)
{
	float g_fTimeDelta;
	uint g_iNumInstances;
	uint g_iFlipbookRows;
	uint g_iFlipbookColumns;
	uint g_iTotalFrames;
	float g_fTime;
	float2 g_fPadding;
};

StructuredBuffer<ParticleData> g_RenderBuffer : register(t0);
Texture2D g_DiffuseTexture : register(t1);
Texture2D g_NormalTexture : register(t2);
Texture2D g_DistortionTexture : register(t3);
Texture2D g_NoiseTexture : register(t4);
Texture2D g_AnyTexture : register(t8);
Texture2D g_BackgroundTex : register(t7);

struct VS_OUT
{
	float4 vPosition : SV_POSITION;
	float2 vTexcoord : TEXCOORD0;
	float4 vColor : TEXCOORD1;
	float4 vEmissive : TEXCOORD2;
	float4 vEndEmissive : TEXCOORD3;
	uint iBehaviorType : TEXCOORD4;
	float4 vScreenPos : TEXCOORD5;
	float3 vNormal : TEXCOORD6;
	float3 vTangent : TEXCOORD7;
	float3 vWorldPos : TEXCOORD8;
	float life : TEXCOORD9;
	float maxLife : TEXCOORD10;
};

VS_OUT VSMain(uint vID : SV_VertexID, uint instID : SV_InstanceID)
{
	VS_OUT Out = (VS_OUT) 0;

	ParticleData p = g_RenderBuffer[instID];

	if (!p.alive)
	{
		Out.vColor = 0;
	}

	float2 baseUV = float2(vID % 2, 1 - (vID / 2));
	float2 finalUV = baseUV;

	if (g_iTotalFrames > 1 && g_iFlipbookColumns > 0 && g_iFlipbookRows > 0)
	{
		uint frame = min(p.frameIndex, g_iTotalFrames - 1);
		uint col = frame % g_iFlipbookColumns;
		uint row = frame / g_iFlipbookColumns;
		float2 uvSize = float2(1.0f / g_iFlipbookColumns, 1.0f / g_iFlipbookRows);
		float2 uvOffset = float2(col, row) * uvSize;

		finalUV = uvOffset + baseUV * uvSize;
	}

	Out.vTexcoord = finalUV;

	float3 camRight = g_matInvView[0].xyz;
	float3 camUp = g_matInvView[1].xyz;
	float3 camFwd = g_matInvView[2].xyz;

	float3 local = float3((baseUV - 0.5f) * p.size, 0);

	float4 vWorldPos;
	if ((p.iBehaviorType & BEHAVIOR_BILLBOARD) != 0)
	{
		float sinRot = sin(p.rotation.z);
		float cosRot = cos(p.rotation.z);
		float2 rotatedLocal = float2(local.x * cosRot - local.y * sinRot, local.x * sinRot + local.y * cosRot);
		float3 worldPos = p.position + camRight * rotatedLocal.x + camUp * rotatedLocal.y;

		vWorldPos = float4(worldPos, 1.f);
		Out.vNormal = -camFwd;
		Out.vTangent = camRight * cosRot + camUp * sinRot;
	}
	else
	{
		float3 rotatedLocal = RotateXYZ(local, p.rotation);

		float3 worldPos = p.position + rotatedLocal;
		vWorldPos = float4(worldPos, 1.0f);

		Out.vNormal = RotateXYZ(float3(0, 0, -1), p.rotation);
		Out.vTangent = RotateXYZ(float3(1, 0, 0), p.rotation);
	}

	float4 vViewPos = mul(vWorldPos, g_matView);
	Out.vPosition = mul(vViewPos, g_matProj);
	Out.vScreenPos = Out.vPosition;
	Out.vWorldPos = vWorldPos.xyz;
	Out.vColor = p.color;
	Out.vEmissive = p.emissive;
	Out.vEndEmissive = p.endEmissive;
	Out.iBehaviorType = p.iBehaviorType;
	Out.life = p.life;
	Out.maxLife = p.maxLife;
	return Out;
}

struct PS_OUT
{
	float4 vDiffuse : SV_TARGET0;
};

PS_OUT PSMain(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;
    
	float4 vTextureColor = g_DiffuseTexture.Sample(LinearWrap, In.vTexcoord);
	if (all(vTextureColor.a <= 0.03f))
		discard;
	if (all(vTextureColor.rgb <= 0.1f))
		discard;
	float ratio = saturate(1.0f - (In.life / max(In.maxLife, 0.0001f)));
	float4 lerpedEmissive = lerp(In.vEmissive, In.vEndEmissive, ratio);
	if ((In.iBehaviorType & BEHAVIOR_DISTORTION) != 0)
	{
		clip(vTextureColor.a - 0.02f);
		clip(In.vColor.a - 0.02f);

		float2 screenUV = In.vScreenPos.xy / In.vScreenPos.w;
		screenUV.x = screenUV.x * 0.5f + 0.5f;
		screenUV.y = -screenUV.y * 0.5f + 0.5f;

		float4 vDistortionColor = g_DistortionTexture.Sample(LinearWrap, In.vTexcoord);
		float2 distortion = vDistortionColor.rg * 2.0f - 1.0f;

		float fEdgeMask = smoothstep(0.0f, 0.3f, vTextureColor.a) *
                          (1.0f - smoothstep(0.3f, 0.9f, vTextureColor.a));

		float distortionStrength = 0.01f * In.vColor.a * fEdgeMask;

		distortion *= distortionStrength;
		float4 distortedBackground = g_BackgroundTex.Sample(LinearClamp, screenUV + distortion);
		float4 vFinalColor = vTextureColor * In.vColor;
		float3 finalRGB = lerp(distortedBackground.rgb, vFinalColor.rgb, vFinalColor.a);
		finalRGB += lerpedEmissive.rgb * lerpedEmissive.a;

		Out.vDiffuse = float4(finalRGB, 1.0f);
		return Out;
	}

	float4 vFinalColor = vTextureColor * In.vColor;
	clip(vFinalColor.a - 0.02f);



	float3 instEmissive = lerpedEmissive.rgb * lerpedEmissive.a;
	float3 FinalColor = vFinalColor.rgb + instEmissive;

	Out.vDiffuse = float4(FinalColor, vFinalColor.a);
	
	return Out;
}

PS_OUT RemoveBlack(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;

	float ratio = saturate(1.0f - (In.life / max(In.maxLife, 0.0001f)));

	float4 texColor = g_DiffuseTexture.Sample(LinearWrap, In.vTexcoord);

	if (all(texColor.rgb < 0.1f))
		discard;
	float4 lerpedEmissive = lerp(In.vEmissive, In.vEndEmissive, ratio);

	float3 finalRGB = texColor.rgb * In.vColor.rgb + lerpedEmissive.rgb * lerpedEmissive.a;
	
	Out.vDiffuse = float4(finalRGB, texColor.a * In.vColor.a);
	return Out;
}

PS_OUT PSFlameRing(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;

	
	float4 texColor = g_DiffuseTexture.Sample(LinearWrap, In.vTexcoord);
	float ratio = saturate(1.0f - (In.life / max(In.maxLife, 0.0001f)));
	float4 lerpedEmissive = lerp(In.vEmissive, In.vEndEmissive, ratio);
	
	float3 finalRGB = texColor.rgb * In.vColor.rgb + lerpedEmissive.rgb * lerpedEmissive.a;
	
	
	float2 centerUV = In.vTexcoord * 2.f - 1.f;
	float radius = length(centerUV);
	float angle = atan2(centerUV.y, centerUV.x) / 6.2831853f + 0.5f;

	float outerRadius = 0.82f;
	float ringWidth = 0.025f;

	float outerRing = 1.f - smoothstep(ringWidth, ringWidth + 0.015f, abs(radius - outerRadius));

	float2 noiseUV1 = float2(angle * 6.f - g_fDeltaTime * 0.35f, radius * 3.f - g_fDeltaTime * 0.8f);
	float2 noiseUV2 = float2(angle * 13.f + g_fDeltaTime * 0.2f, radius * 5.f + g_fDeltaTime * 0.45f);

	float noise1 = g_NoiseTexture.Sample(LinearWrap, noiseUV1).r;
	float noise2 = g_NoiseTexture.Sample(LinearWrap, noiseUV2).r;
	float noise = saturate(noise1 * 0.7f + noise2 * 0.3f);

	float flameMinDepth = 0.06f;
	float flameMaxDepth = 1.f;
	float flameDepth = lerp(flameMinDepth, flameMaxDepth, noise);

	float innerRadius = outerRadius - flameDepth;

	float flameMask = smoothstep(innerRadius, innerRadius + 0.04f, radius);
	flameMask *= 1.f - smoothstep(outerRadius, outerRadius + 0.02f, radius);

	float edgeProgress = saturate((radius - innerRadius) / max(flameDepth, 0.0001f));
	float flameDetail = pow(edgeProgress, 1.5f);
	flameMask *= flameDetail;

	float pulse = 0.85f + sin(g_fDeltaTime * 8.f + angle * 25.f) * 0.15f;
	flameMask *= pulse;

	float finalMask = saturate(max(outerRing, flameMask));

	clip(finalMask - 0.01f);


	//float3 flameColor = lerp(innerColor, outerColor, edgeProgress);
	//flameColor += outerRing * float3(1.f, 1.f, 1.f);
	
	Out.vDiffuse = float4(finalRGB * finalMask, finalMask * In.vColor.a);

	return Out;
}

PS_OUT PSDragonBreath(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;

	float lifeRatio = saturate(1.f - In.life / max(In.maxLife, 0.0001f));
	float4 tex = g_DiffuseTexture.Sample(LinearWrap, In.vTexcoord);
	float mask = max(max(tex.r, tex.g), tex.b);

	clip(mask - 0.02f);

	float bodyMask = smoothstep(0.02f, 0.5f, mask);
	float emissiveMask = pow(saturate(mask), 1.5f);
	float4 emissive = lerp(In.vEmissive, In.vEndEmissive, lifeRatio);

	float3 bodyColor = In.vColor.rgb;
	float3 emissiveColor = emissive.rgb * emissive.a * emissiveMask;
	float3 finalColor = bodyColor + emissiveColor;
	float finalAlpha = saturate(bodyMask * In.vColor.a);

	Out.vDiffuse = float4(finalColor, finalAlpha);
	return Out;
}
PS_OUT PSRanrokSmoke(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;

	float lifeRatio = saturate(1.f - In.life / max(In.maxLife, 0.0001f));
	float fadeIn = smoothstep(0.f, 0.08f, lifeRatio);
	float fadeOut = 1.f - smoothstep(0.3f, 1.f, lifeRatio);

	float2 noiseUV1 = In.vTexcoord * 1.4f + float2(g_fTime * 0.035f, -g_fTime * 0.08f);
	float2 noiseUV2 = In.vTexcoord * 2.7f + float2(-g_fTime * 0.06f, -g_fTime * 0.13f);

	float2 noise1 = g_DistortionTexture.Sample(LinearWrap, noiseUV1).rg * 2.f - 1.f;
	float2 noise2 = g_DistortionTexture.Sample(LinearWrap, noiseUV2).gb * 2.f - 1.f;
	float2 flowOffset = noise1 * 0.045f + noise2 * 0.018f;

	float2 smokeUV = In.vTexcoord;
	smokeUV.x += sin(In.vTexcoord.y * 7.f + g_fTime * 1.3f) * 0.025f;
	smokeUV += flowOffset * saturate(0.4f + lifeRatio);

	float4 textureColor = g_DiffuseTexture.Sample(LinearWrap, smokeUV);
	float textureLuminance = dot(textureColor.rgb, float3(0.299f, 0.587f, 0.114f));
	float smokeMask = max(textureColor.a, textureLuminance);

	float dissolveNoise = g_DistortionTexture.Sample(LinearWrap, noiseUV2 + flowOffset).b;
	float dissolveAmount = smoothstep(0.45f, 1.f, lifeRatio);
	float dissolveMask = smoothstep(dissolveAmount - 0.18f, dissolveAmount + 0.18f, dissolveNoise);

	float edgeMask = 1.f - smoothstep(0.f, 0.18f, abs(dissolveNoise - dissolveAmount));
	float finalAlpha = smokeMask * dissolveMask * fadeIn * fadeOut * In.vColor.a;

	clip(finalAlpha - 0.01f);

	float4 lerpedEmissive = lerp(In.vEmissive, In.vEndEmissive, lifeRatio);
	float3 smokeColor = textureColor.rgb * In.vColor.rgb;
	float3 emissiveColor = lerpedEmissive.rgb * lerpedEmissive.a * edgeMask * finalAlpha * 0.15f;

	if ((In.iBehaviorType & BEHAVIOR_DISTORTION) != 0)
	{
		float2 screenUV = In.vScreenPos.xy / In.vScreenPos.w;
		screenUV = screenUV * float2(0.5f, -0.5f) + 0.5f;

		float2 distortion = flowOffset * 0.35f * finalAlpha;
		float3 distortedBackground = g_BackgroundTex.Sample(LinearClamp, screenUV + distortion).rgb;
		float3 finalColor = lerp(distortedBackground, smokeColor, finalAlpha) + emissiveColor;

		Out.vDiffuse = float4(finalColor, finalAlpha);
		return Out;
	}

	Out.vDiffuse = float4(smokeColor + emissiveColor, finalAlpha);
	return Out;
}
PS_OUT PSRanrokSmokeWispy(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;

	float ageRatio = saturate(1.f - In.life / max(In.maxLife, 0.0001f));
	float fadeIn = smoothstep(0.f, 0.18f, ageRatio);
	float fadeOut = 1.f - smoothstep(0.35f, 0.95f, ageRatio);
	float lifeFade = fadeIn * fadeOut;

	float2 uv = In.vTexcoord;
	float2 centeredUV = uv * 2.f - 1.f;

	float smokeBallMask = g_AnyTexture.Sample(LinearClamp, uv).r;
	float circleMask = 1.f - smoothstep(0.65f, 1.f, length(centeredUV));

	float2 distortionUV1 = uv + ageRatio * float2(0.4f, 0.2f);
	float2 distortion1 = g_NoiseTexture.Sample(LinearWrap, distortionUV1).rg * 2.f - 1.f;
	float distortionEdge = smoothstep(0.15f, 0.85f, smokeBallMask);
	float distortionStrength1 = lerp(0.2f, -0.157143f, distortionEdge);
	float2 wispUV1 = uv * float2(0.5f, 0.5f) + ageRatio * float2(0.03f, 0.01f);
	wispUV1 += distortion1 * distortionStrength1;

	float3 wispTexture1 = g_DiffuseTexture.Sample(LinearWrap, wispUV1).rgb;
	float wispMask1 = dot(wispTexture1, float3(0.299f, 0.587f, 0.114f));
	wispMask1 = pow(saturate(wispMask1), 0.55f);

	float smokeShapeMask = pow(saturate(smokeBallMask), 0.45f);
	float wispDensityMask = smoothstep(0.02f, 0.35f, wispMask1);
	float bodyMask = smokeShapeMask * lerp(0.65f, 1.f, wispDensityMask);

	float2 distortionUV2 = uv * 0.9f + ageRatio * float2(0.1f, 0.05f);
	float2 distortion2 = g_NoiseTexture.Sample(LinearWrap, distortionUV2).rg * 2.f - 1.f;
	float2 wispUV2 = uv * float2(0.45f, 0.45f) + ageRatio * float2(-0.02f, -0.01f);
	wispUV2 += distortion2 * 0.06f;

	float3 wispTexture2 = g_DiffuseTexture.Sample(LinearWrap, wispUV2).rgb;
	float wispMask2 = dot(wispTexture2, float3(0.299f, 0.587f, 0.114f));
	float thinWispMask = smoothstep(0.45f, 0.82f, wispMask2);
	thinWispMask *= circleMask;

	float4 emissive = lerp(In.vEmissive, In.vEndEmissive, ageRatio);
	float3 blackSmoke = lerp(float3(0.018f, 0.004f, 0.022f), In.vColor.rgb, 0.65f);
	float3 smokeBody = blackSmoke;
	float3 glowingWisp = emissive.rgb * emissive.a * thinWispMask * 0.15f;
	float3 finalColor = smokeBody + glowingWisp;

	float shapeMask = thinWispMask;
	clip(shapeMask - 0.04f);

	float finalAlpha = In.vColor.a * lifeFade;
	Out.vDiffuse = float4(finalColor, finalAlpha);
	return Out;
}

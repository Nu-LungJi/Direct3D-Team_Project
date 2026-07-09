
const static float  PI = 3.14159265359f;
const static int    MAX_LIGHT_COUNT = 8;

const static float3 AlbedoColor = { 1.f, 1.f, 1.f };

const static float  NormalIntensity      = 1.f;
const static float  RoughnessIntensity   = 1.f;
const static float  MetallicIntensity    = 1.f;
const static float  AmbientIntensity     = 1.f;
const static float  SpecularIntensity    = 1.f;

#define MAX_LIGHT_COUNT     8

#define LIGHT_DIRECTIONAL   0
#define LIGHT_POINT         1
#define LIGHT_SPOTLIGHT     2

#define MAX_REFLECTION_LOD  4.f

struct DirectionalLight
{
    float4 ambient;
    float4 diffuse;
    float4 specular;
    float3 direction;
    float pad;
};

struct PointLight
{
    float4 ambient;
    float4 diffuse;
    float4 specular;
    
    float3 pos;
    float range;
    
    float3 att;
    float _pad;
};

struct SpotLight
{
    float4 ambient;
    float4 diffuse;
    float4 specular;
    
    float3 pos;
    float range;
    
    float3 direction;
    float spot;
    
    float3 att;
    float _pad;
};

struct DynamicLight
{
    uint    LightType;      // Directional, Point, SpotLight
    
    float3  LightDirection;
    float3  LightColor;
    float   LightIntensity;
    float   LightRange;
    
    float3  Position;
    
    float   InnerAttanuation;
    float   OuterAttanuation;
    
    float2  LightPadding;
};
struct Material
{
    float4 ambient;
    float4 diffuse;
    float4 specular;
    float4 reflect;
};


// 1. 오브젝트당 n회 갱신 (슬롯 b0)
cbuffer CB_PER_OBJECT : register(b0)
{
    matrix g_matWorld;
    matrix g_matWVP;
    float4 g_objectColor;
    uint g_objectLight;
    float3 _g_object_pad;
};

// 2. 프레임당 1회 갱신 (슬롯 b1)
cbuffer CB_PER_PASS : register(b1)
{
    //DirectionalLight gDirLights;
    
    matrix g_matView; // _float4x4와 1:1 대응
    matrix g_matProj;
    matrix g_matViewProj;
    matrix g_matInvView;
    matrix g_matInvViewProj;
    float3 g_vCamPos;
    float g_fDayFactor; //
    matrix g_matSkyRotation;
    matrix g_matStarRotation;
    matrix g_matShadowLightViewProj;
    float3 g_vShadowLightDir;

};

cbuffer CB_BONES : register(b2)
{
     matrix g_BoneMatrices[512];
};

cbuffer CB_MATERIAL : register(b3)
{
    float3  EmissiveColor;
    float   EmissiveIntensity;
    
    float   ObjectAlpha;
    float3  ObjectPadding;
}

cbuffer CB_LIGHT_BUFFER : register(b4)
{
    DynamicLight AffectedLight[MAX_LIGHT_COUNT];
    int          g_iLightCount;
    float3       g_fLightPadding;
}

cbuffer CB_PER_UI : register(b7)
{
    float2 g_ui_texCoord;
    float2 g_ui_uvSize;
    float4 g_ui_color; 
    //uint g_ui_texIndex; 
    //float2 g_ui_borderUV; 
    //float _pad_perui; 
    //float2 g_ui_borderPx; 
    //float2 g_ui_rectSizePx;
};
cbuffer PostProcessBuffer : register(b8)
{
    float BloomIntensity; // 블룸 강도
    
    float DistortionIntensity; // 왜곡 강도
    float ChromaticIntensity; // 색수차 강도
    float VignetteIntensity; // 비네팅 강도
    float VignetteSmoothness; // 비네팅
    
    float3 Padding;
};

SamplerState LinearWrap                 : register(s0);
SamplerState LinearClamp                : register(s1);
SamplerState PointWrap                  : register(s2);
SamplerState PointClamp                 : register(s3);
SamplerState PointWrapNoMip             : register(s4);
SamplerState AnisotropicWrap            : register(s5);

SamplerComparisonState  ShadowSampler   : register(s6);

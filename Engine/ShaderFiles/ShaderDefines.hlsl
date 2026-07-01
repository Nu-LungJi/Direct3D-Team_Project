
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


cbuffer CB_PER_UI : register(b7)
{
    float2 g_ui_texCoord;
    float2 g_ui_uvSize; 
    float4 g_ui_color; 
    uint g_ui_texIndex; 
    float2 g_ui_borderUV; 
    float _pad_perui; 
    float2 g_ui_borderPx; 
    float2 g_ui_rectSizePx;
};

Texture2D gShadowMap : register(t4);
SamplerComparisonState gShadowSampler : register(s4);
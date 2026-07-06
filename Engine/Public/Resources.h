#pragma once

#include "ResDynamicBuffer.h"
#include "ResCBuffer.h"
#include "ResCubeColBuffer.h"
#include "ResDynamicVIBuffer.h"
#include "ResQuadColBuffer.h"
#include "ResQuadFullscreenTexBuffer.h"
#include "ResQuadTexBuffer.h"
#include "ResStructuredBuffer.h"
#include "ComConstantBuffer.h"
#include "ResFmodSound.h"

#include "ResJson.h"

#include "ResComputeShader.h"
#include "ResGeometryShader.h"
#include "ResGeoShaderStreamOut.h"
#include "ResPixelShader.h"
#include "ResTessDomainShader.h"
#include "ResTessHullShader.h"
#include "ResVertexShader.h"

#include "ResBlendState.h"
#include "ResDepthStencilState.h"
#include "ResRasterizerState.h"
#include "ResSamplerState.h"

#include "ResTexture2D.h"
#include "ResDynamicTexture2D.h"
#include "ResOffscreenTexture.h"
#include "ResTexture2DArray.h"
#include "ResTextureCubeMap.h"

#include "ResViewPort.h"

#include "ResFont.h"
#include "ResFontCustom.h"

//#include "ResTestModel.h"
//#include "ResTestModelAnim.h"
//#include "ResTestModelMesh.h"
//#include "ResTestModelChanel.h"
//#include "ResTestModelMaterial.h"
//#include "ResTestModelBone.h"


#include "ResModel.h"
#include "ResStaticModel.h"
#include "ResModelAnim.h"
#include "ResModelMesh.h"
#include "ResStaticModelMesh.h"
#include "ResModelChanel.h"
#include "ResModelMaterial.h"
#include "ResModelBone.h"


static const char* TAG_RES_GRP_PERMANENT_BUFFER = "PERMANENT_BUFFER";
static const char* TAG_RES_GRP_PERMANENT_SHADER = "PERMANENT_SHADER";
static const char* TAG_RES_GRP_PERMANENT_TEXTURE = "PERMANENT_TEXTURE";
static const char* TAG_RES_GRP_PERMANENT_STATE = "PERMANENT_STATE";
static const char* TAG_RES_GRP_PERMANENT_VP = "PERMANENT_VP";


// RS_: RasterazerState
// BS_: BlendState
// DS_: DepthStencilState
// SS_: SamplerState
constexpr static const char* TAG_RES_STATE_RS_SOLID_BACKCULL = "RS_SOLID_BACKCULL";
constexpr static const char* TAG_RES_STATE_RS_SOLID_FRONTCULL = "RS_SOLID_FRONTCULL";
constexpr static const char* TAG_RES_STATE_RS_SOLID_NOCULL = "RS_SOLID_NOCULL";
constexpr static const char* TAG_RES_STATE_RS_WIREFRAME_NOCULL = "RS_WIREFRAME_NOCULL";

constexpr static const char* TAG_RES_STATE_SS_LINEAR_WRAP = "SS_LinearWrap";
constexpr static const char* TAG_RES_STATE_SS_POINT_WRAP = "SS_PointWrap";
constexpr static const char* TAG_RES_STATE_SS_POINT_WRAP_NOMIP = "SS_PointWrapNoMip";
constexpr static const char* TAG_RES_STATE_SS_SAHDOW = "SS_SHADOW";

constexpr static const char* TAG_RES_CBUFFER_PASS = "CB_PerPass";
constexpr static const char* TAG_RES_CBUFFER_OBJECT = "CB_PerObject";
constexpr static const char* TAG_RES_CBUFFER_MATERIAL = "CB_PerMaterial";
constexpr static const char* TAG_RES_CBUFFER_PARTICLE = "CB_PerParticle";
constexpr static const char* TAG_RES_CBUFFER_SPAWN_PARTICLE = "CB_SpawnParticle";
constexpr static const char* TAG_RES_CBUFFER_INIT_PARTICLE = "CB_InitGPUParticle";



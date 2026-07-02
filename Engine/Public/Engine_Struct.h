#pragma once

namespace Engine
{
	typedef struct tagEngineDesc
	{
		HWND hWnd;
		HINSTANCE hInstance;
		WINMODE eWinMode;
		uint32_t iWinSizeX, iWinSizeY;
		uint32_t		iNumLevels;
	} ENGINE_DESC;


	typedef struct tagRenderContext
	{
		RENDERPASS pass;
		_vector eye{};
		_matrix matView{};
		_matrix matProj{};
		_matrix matViewProj{};
	} RENDER_CTX;

	typedef struct tagWorkerTask
	{
		_string sTaskName;
		_Func func;
	} WORKER_TASK;

	typedef struct tagMaterial
	{
		_float4 ambient{};
		_float4 diffuse{};
		_float4 specular{};
		_float4 reflect{};
	} MATERIAL;

	typedef struct tagDirectionalLight
	{
		_float4 ambient{};
		_float4 diffuse{};
		_float4 specular{};
		_float3 direction{};
		_float _pad{};
	} DIRECTIONAL_LIGHT;

	typedef struct tagPointLight
	{
		_float4 ambient{};
		_float4 diffuse{};
		_float4 specular{};
		_float3 pos{};
		_float range{};
		_float3 att{};//감쇠
		_float _pad{};
	} POINT_LIGHT;

	typedef struct tagSpotLight
	{
		_float4 ambient{};
		_float4 diffuse{};
		_float4 specular{};
		_float3 pos{};
		_float range{};
		_float3 direction{};
		_float spot{};
		_float3 att{};//감쇠
		_float _pad{};
	} SPOT_LIGHT;

	typedef struct tagPostProcess
	{
		_float DistortionIntensity;  // 왜곡 강도
		_float ChromaticIntensity;   // 색수차 강도
		_float VignetteIntensity;    // 비네팅 강도
		_float VignetteSmoothness;   // 비네팅
	} POSTPROCESS;
	typedef struct tagUiDesc
	{
		_string name;
		_float2 Pos;
		_float2 Scale;
	} UI_DESC;
	typedef struct tagKeyFrame
	{
		XMFLOAT3	vScale;
		XMFLOAT4	vRotation;
		XMFLOAT3	vTranslation;
		float		fTrackPosition;
	}KEYFRAME;

	
	typedef struct tagimguinode
	{
		int32_t		iID;
		_string		Name;
		_float2		vPos, vSize;
		_float		fValue;
		_float4		vColor;
		int32_t		iStartCnt, iEndCnt;

		tagimguinode(int id, const char* name, const _float2& pos, float value, const _float4& color, int inputs_count, int outputs_count) { iID = id; Name =  name; vPos = pos; fValue = value; vColor = color; iStartCnt = inputs_count; iEndCnt = outputs_count; }

		_float2 GetStartSlotPos(int slot_no) const  { return _float2(vPos.x, vPos.y + vSize.y * ((float)slot_no + 1) / ((float)iStartCnt + 1)); }
		_float2 GetEndSlotPos(int slot_no) const { return _float2(vPos.x + vSize.x, vPos.y + vSize.y * ((float)slot_no + 1) / ((float)iEndCnt+ 1)); }
	}GUINODE;
	
	typedef struct tagimguinodelink
	{
		int32_t		iStartIdx, iStartSlot, iEndIdx, iEndSlot;

		tagimguinodelink(int32_t input_idx, int32_t input_slot, int32_t output_idx, int32_t output_slot )
		{
			iStartIdx = input_idx; iStartSlot = input_slot; iEndIdx = output_idx; iEndSlot = output_slot;
		}

	}GUINODE_LINK;

	typedef struct tagimguiCurrentNode
	{
		GUINODE* pCurrentNode{ nullptr };
		int32_t  iSelectedSlot{ -1 }, iD{ -1 };
		_float2  vSlotPos;
		_bool	 bSelected = false;

		NODETYPE eType = NODETYPE::END;
	}GUICURRENT_NODE;

}
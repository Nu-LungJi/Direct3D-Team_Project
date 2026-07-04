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

	

	///////BeHavior 아
	typedef struct tagactionnode
	{
		_string NodeName{};
		_float2  vPos{}, vSize{};
		_float   fSpeed{};
	}ACTION_NODE;
	typedef struct tagdestnode
	{
		tagdestnode() = default;
		tagdestnode(_string Name, BEHAVIOR eBType, int32_t iNode)
		{
			DestName = Name; eType = eBType; iDestNode = iNode;
		}
		_string  DestName{};
		int32_t iDestNode{ -1 };

		BEHAVIOR eType{ BEHAVIOR::END };

		void Reset() { DestName = "";  eType = BEHAVIOR::END; iDestNode = -1; }

	}DEST_NODE;
	typedef struct tagimguinode
	{
		uint32_t	iID{};
		_string		Name{};
		_float2		vPos{}, vSize{};
		_float		fValue{};
		_float4		vColor{};
		BEHAVIOR    eMyType{};
		tagimguinode() = default;
		tagimguinode(BEHAVIOR eType, int32_t id, const _char* name, const _float2& pos, float value, const _float4& color) { eMyType = eType; iID = id; Name = name; vPos = pos; fValue = value; vColor = color;}
		_float2 GetStartSlotPos() const  { return _float2(vPos.x + vSize.x, vPos.y + vSize.y);}
		_float2 GetEndSlotPos(int slot_no  ,int32_t iMaxCnt) const { return  _float2(vPos.x, vPos.y + vSize.y * ((float)slot_no + 1) / ((float)iMaxCnt )); }
		DEST_NODE Get_DestInfo() {
			DEST_NODE Dst{};
			Dst.DestName = Name;
			Dst.eType = eMyType;
			Dst.iDestNode = iID;
			return Dst;
		}
	}GUINODE;

	typedef struct tagimguinodelink
	{//이거다 이거
		int32_t					iStartIdx{ -1 };
		DEST_NODE				ParentNode;
		std::vector<DEST_NODE>  SlotEnd{};
		
		tagimguinodelink() = default;
		tagimguinodelink(int32_t iEnd)
		{
			SlotEnd.resize(iEnd);
		}
	}GUINODE_LINK;

	typedef struct tagimguiCurrentNode
	{
		tagimguiCurrentNode() = default;
		tagimguiCurrentNode(GUINODE* pNode, GUINODE_LINK* pLink, int32_t iSlot)
		{
			pCurrentNode = pNode; pCurrentLink = pLink;  iSelectedSlot = iSlot;

		}
		GUINODE* pCurrentNode{ nullptr };
		GUINODE_LINK* pCurrentLink{ nullptr };
		int32_t  iSelectedSlot{ -1 }, iD{ -1 };
		_float2  vSlotPos;
		_bool	 bSelected = false;
		
		NODETYPE eType = NODETYPE::END;
	}GUICURRENT_NODE;
	///////NodeEditor용
}
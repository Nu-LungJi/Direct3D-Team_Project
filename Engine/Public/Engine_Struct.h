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

	typedef struct tagDynamicLight {
		uint32_t LightType;			// <= Engine_Enum ~ LIGHT_TYPE 활용하기

		XMFLOAT4X4		g_LightViewProj;
		XMFLOAT4X4		g_InvViewProj;

		_float3  LightDirection;
		_float3  LightColor;
		_float   LightIntensity;
		_float   LightRange;

		_float3  Position;

		_float   InnerAttanuation;
		_float   OuterAttanuation;

		_float2   LightPadding;
	} DYNAMIC_LIGHT;

	typedef struct tagPostProcess
	{
		_float BloomIntensity;		 // 블룸 강도

		_float DistortionIntensity;  // 왜곡 강도
		_float ChromaticIntensity;   // 색수차 강도
		_float VignetteIntensity;    // 비네팅 강도
		_float VignetteSmoothness;   // 비네팅

		_float3 PostProcessPadding;
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

	

	///////BeHavior//////
	typedef struct tagactionvalue
	{
		tagactionvalue() = default;
		tagactionvalue(int32_t iAnim) { iAnim = iAnimIndex; }
		int32_t  iAnimIndex{ -1 };
		_float   fSpeed{}, fTime{1.f}, fTick{};
	
	}ACTION_VALUE;
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
		XMFLOAT2	vPos{},vSize{};
		float		fValue{};
		XMFLOAT4	vColor{};
		_bool		bAbort{ false };
		BEHAVIOR    eMyType{};
		tagimguinode()=default ;
		tagimguinode(BEHAVIOR eType, int32_t id, const _char* name, XMFLOAT2 pos, float value, XMFLOAT4 color) { eMyType = eType; iID = id; Name = name; vPos = pos; fValue = value; vColor = color;}
		XMFLOAT2 GetStartSlotPos()  { return XMFLOAT2(vPos.x + vSize.x*0.5f, vPos.y ) ;}
		XMFLOAT2 GetEndSlotPos(int slot_no, int32_t iMaxCnt) const {
			return XMFLOAT2(vPos.x + vSize.x * ((float)slot_no + 1) / ((float)iMaxCnt), vPos.y + vSize.y);
		}
		DEST_NODE Get_DestInfo() {
			DEST_NODE Dst{};
			Dst.DestName = Name;
			Dst.eType = eMyType;
			Dst.iDestNode = iID;
			return Dst;
		}

	}GUINODE;

	typedef struct tagimguinodelink
	{
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

	typedef struct tagParticleSpawnData
	{
		_float3 position;
		_float3 velocity;
		_float  life;
		_float  size;
		_float4 color;
		_float4 emissive;
		_float spawnDelay;
	}PARTICLE_SPAWN_DATA;

	typedef struct tagParticleEmitRequest
	{
		uint32_t count;
		_bool    bLoop;
		_float   fSpawnInterval;
	} PARTICLE_EMIT_REQUEST;
	
	typedef struct tagBeamVertex
	{
		_float3 vPosition;
		_float2 vUV;
		_float4 vColor;
		_float4 vEmissive;
	}BEAM_VERTEX;

	typedef struct ChunkHeader
	{
		uint32_t type;
		uint32_t size;
	}CHUCKHEADER;

	typedef struct MODEL_FILE_HEADER
	{
		bool bHasBone;
		bool bHasAnimation;

		uint32_t MeshCount;
		uint32_t MaterialCount;
		uint32_t AnimationCount;
		uint32_t BoneCount;

	}MODEL_FILE_HEADER;

	typedef struct tagParticleSpecies {
		
	}PARTICLE_SPECIES;

	// 여러 청크를 관리할 때 key로 사용할 ChunkCoord
	typedef struct tagMapChunkCoord
	{
		int64_t x = 0;
		int64_t y = 0;
		int64_t z = 0;

		bool operator==(const tagMapChunkCoord& rhs) const
		{
			return x == rhs.x && y == rhs.y && z == rhs.z;
		}
	}MAPCHUNK_COORD;

	//----------------------------MapMeshObject ?몄뒪?댁떛------------------------
	typedef struct tagMapMeshInstanceData
	{
		_float4x4 world;
	} MAPMESH_INSTANCE_DATA;

	typedef struct tagMapMeshBatchKey
	{
		std::string modelGroup;
		std::string modelTag;

		bool operator==(const tagMapMeshBatchKey& rhs) const
		{
			return modelGroup == rhs.modelGroup && modelTag == rhs.modelTag;
		}
	} MAPMESH_BATCH_KEY;

	struct tagMapMeshBatchKeyHash
	{
		size_t operator()(const MAPMESH_BATCH_KEY& key) const noexcept
		{
			const size_t h1 = std::hash<std::string>{}(key.modelGroup);
			const size_t h2 = std::hash<std::string>{}(key.modelTag);
			return h1 ^ (h2 << 1);
		}
	};

	//class CResStaticModel;
	//typedef struct tagMapMeshBatch
	//{
	//	CResStaticModel* model = nullptr;
	//	std::vector<MAPMESH_INSTANCE_DATA> instances;
	//} MAPMESH_BATCH;
	//----------------------------MapMeshObject ?몄뒪?댁떛------------------------
}

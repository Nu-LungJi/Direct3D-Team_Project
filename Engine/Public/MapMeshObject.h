#pragma once
#include "GameObject.h"

NS_BEGIN(Engine)

class CComConstantBuffer;
class CComStaticModelInstance;
class CResDynamicBuffer;
class CResPixelShader;
class CResSamplerState;
class CResStaticModel;
class CResVertexShader;

class ENGINE_DLL CMapMeshObject : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CMapMeshObject, CGameObject)
	CMapMeshObject& operator=(const CMapMeshObject&) = delete;

protected:
	explicit CMapMeshObject();
	explicit CMapMeshObject(const CMapMeshObject& Prototype);
	~CMapMeshObject() override;

public:
	typedef struct tagMapMeshObjectDesc : public GAMEOBJECT_DESC
	{
		std::string modelGroupTag;
		std::string modelResTag;
		std::string protoGroupTag;
		std::string prototypeTag;
	} MAP_MESH_OBJECT_DESC;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void LateUpdate(_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx) override;
	void UpdateGUI() override;

public:
	const std::string& GetModelResourceGroup() const { return m_modelResourceGroup; }
	const std::string& GetModelResourceTag() const { return m_modelResourceTag; }
	HRESULT SetModelResource(const std::string& modelGroupTag, const std::string& modelResTag);

public:
	void SetRenderEnable(_bool enable) { m_bRenderEnable = enable; }

public:
	struct INSTANCING_STATS
	{
		_bool bEnabled = true;
		uint32_t iObjects = 0;
		uint32_t iInstances = 0;
		uint32_t iBatches = 0;
		uint32_t iDrawCalls = 0;
	};

	// 인스턴싱 On/Off , 드로우 콜 GUI
	static _bool IsInstancingEnabled() { return s_bInstancingEnabled; }
	static void SetInstancingEnabled(_bool bEnabled);
	static const INSTANCING_STATS& GetInstancingStats() { return s_LastStats; }

	static void ClearInstancingData(); // 매 프레임 인스턴싱 데이터 clear
	static void ReleaseInstancingResources(); // 종료할 때 인스턴싱 버퍼 해제

private:
	static HRESULT PushInstance(const SPtr<CResStaticModel>& pModel, const MAPMESH_INSTANCE_DATA& instanceData);
	static HRESULT EnsureInstanceBuffer(size_t instanceCount);
	static HRESULT RenderInstancedBatches(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx);

private:
	std::string m_modelResourceGroup{};
	std::string m_modelResourceTag{};
	CComStaticModelInstance* m_pComModelInstance{};
	CComConstantBuffer* m_pComCBufferPerObject{};
	SPtr<CResVertexShader> m_pResVertexShader{};
	SPtr<CResPixelShader> m_pResPixelShader{};
	SPtr<CResSamplerState> m_pResSamplerState{};

private:
	static std::unordered_map<SPtr<CResStaticModel>, std::vector<MAPMESH_INSTANCE_DATA>> s_InstanceBatches;
	static SPtr<CResDynamicBuffer> s_pInstanceBuffer;
	static size_t s_iInstanceCapacity;
	static std::optional<CHandle> s_hRenderRepresentative;// 대표로 렌더 콜 호출할 오브젝트

	// 드로우 콜 확인용
	static _bool s_bInstancingEnabled; // 인스턴싱 On/Off
	static INSTANCING_STATS s_FrameStats;
	static INSTANCING_STATS s_LastStats;

private:
	_bool m_bRenderEnable = false; // 렌더러에 들어갈 놈인가 (컬링 통과된 놈들)
public:
	static UPtr<CMapMeshObject> Create();
	UPtr<CPrototype> Clone(void* pArg) override;
};

NS_END

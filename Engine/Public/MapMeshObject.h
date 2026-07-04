#pragma once
#include "GameObject.h"

NS_BEGIN(Engine)

class CComConstantBuffer;
class CComModelInstance;
class CResPixelShader;
class CResSamplerState;
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

private:
	std::string m_modelResourceGroup{};
	std::string m_modelResourceTag{};
	CComModelInstance* m_pComModelInstance{};
	CComConstantBuffer* m_pComCBufferPerObject{};
	SPtr<CResVertexShader> m_pResVertexShader{};
	SPtr<CResPixelShader> m_pResPixelShader{};
	SPtr<CResSamplerState> m_pResSamplerState{};

public:
	static UPtr<CMapMeshObject> Create();
	UPtr<CPrototype> Clone(void* pArg) override;
};

NS_END

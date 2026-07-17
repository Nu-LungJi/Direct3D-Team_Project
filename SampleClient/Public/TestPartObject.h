
#pragma once
#include "GameObject.h"
#include "Client_Defines.h"
NS_BEGIN(Engine)
class CComConstantBuffer;
class CResTexture2D;
class CResVertexShader;
class CResPixelShader;
class CResSamplerState;
class CResModel;
class CComStaticModelInstance;
class CComSocket;
NS_END

NS_BEGIN(Client)
class CTestPartObject final : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CTestPartObject, CGameObject)

public:
	typedef struct tagPartObjectDesc : public CGameObject::GAMEOBJECT_DESC
	{
		// The animated model to follow and the bone name on that model.

		CHandle hOwner{};
		uint32_t iBoneIndex{};
		_float3 vBoneOffset{};
		StringID sGroupTag;
		StringID sResTag;

	
	}DESC;

private:
	CTestPartObject();
	~CTestPartObject() override;

public:
	void UpdateGUI() override;
public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext,  const E::RENDER_CTX& ctx) override;
	HRESULT Render_Instanced(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx, const E::MODEL_INSTANCE_BATCH& Batch) override;

public:
	HRESULT BindParentAnimationBuffers(ID3D11DeviceContext* pContext);
	HRESULT UpdatePartInstanceBuffer(ID3D11DeviceContext* pContext, const std::vector<E::GPU_PART_INSTANCE_DATA>& instances);
	HRESULT BuildPartInstanceData(E::GPU_PART_INSTANCE_DATA& outData) const;
private:
	CComStaticModelInstance* m_pComModelInstance{};
	// nonAnim
	SPtr<CResPixelShader> m_pResPixelNonAnimShader{};
	SPtr<CResVertexShader> m_pResVertexNonAnimShader{};

	CComConstantBuffer* m_pComCBufferPerObject{};
	CComConstantBuffer* m_pComCBufferPartObject{};

	CComSocket* m_pSocket;

	CHandle m_hOwner{};
	uint32_t m_iBoneIndex{};
	_float3 m_vBoneOffset{};

public:
	static E::UPtr<CTestPartObject> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END

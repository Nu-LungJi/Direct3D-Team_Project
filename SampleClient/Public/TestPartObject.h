
#pragma once
#include "AnimationObject.h"
#include "Client_Defines.h"
NS_BEGIN(Engine)
class CComConstantBuffer;
class CResTexture2D;
class CResVertexShader;
class CResPixelShader;
class CResSamplerState;
class CResModel;
class CComStaticModelInstance;
NS_END

NS_BEGIN(Client)
class CTestPartObject final : public CAnimationObject
{
public:
	DECLARE_DERIVED_TYPE(CTestPartObject, CAnimationObject)

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
private:
	CComStaticModelInstance* m_pComModelInstance{};
	// nonAnim
	SPtr<CResPixelShader> m_pResPixelNonAnimShader{};
	SPtr<CResVertexShader> m_pResVertexNonAnimShader{};

	CComConstantBuffer* m_pComCBufferPerObject{};
	CComConstantBuffer* m_pComCBufferPartObject{};
	CHandle m_hOwner{};
	uint32_t m_iBoneIndex{};
	_float3 m_vBoneOffset{};

public:
	static E::UPtr<CTestPartObject> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END

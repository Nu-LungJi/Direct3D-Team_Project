#pragma once
#include "Engine_Defines.h"
#include "ResCBuffer.h"
#include "GameObject.h"
#include "ComConstantBuffer.h"
#include "ResVertexShader.h"
#include "ResPixelShader.h"
#include "ResQuadTexBuffer.h"
#include "ResTexture2D.h"
#include "ResDynamicTexture2D.h"
#include "ResSamplerState.h"
#include "ComCollider.h"

NS_BEGIN(Engine)

class ENGINE_DLL CLight final : public CGameObject {
private:
	CLight();
	~CLight() override;
public:
	typedef struct tagLightDesc : public CGameObject::GAMEOBJECT_DESC
	{

	}DESC;
public:
	DECLARE_DERIVED_TYPE(CLight, CGameObject)

public:
	HRESULT			InitializePrototype(void* pArg) override;
	HRESULT			Initialize(void* pArg) override;
	void			PriorityUpdate(E::_float fTimeDelta) override;
	void			Update(E::_float fTimeDelta) override;
	void			LateUpdate(E::_float fTimeDelta) override;
	HRESULT			Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx);

public:
	VOID			Set_LightType(LIGHT_TYPE _LTYPE) { DynamicLight.LightType = ETOUI(_LTYPE); }
	LIGHT_TYPE		Get_LightType() { return static_cast<LIGHT_TYPE>(DynamicLight.LightType); }

	VOID			Set_LightDirection(XMFLOAT3 _Direction) { DynamicLight.LightDirection = _Direction; }
	XMFLOAT3		Get_LightDirection() { return DynamicLight.LightDirection; }

	VOID			Set_LightColor(XMFLOAT3 _Color) { DynamicLight.LightColor = _Color; }
	XMFLOAT3		Get_LightColor() { return DynamicLight.LightColor; }

	VOID			Set_LightIntensity(_float _Intensity) { DynamicLight.LightIntensity = _Intensity; }
	_float			Get_LightIntensity() { return DynamicLight.LightIntensity; }

	VOID			Set_LightRange(_float _Range) { DynamicLight.LightRange = _Range; }
	_float			Get_LightRange() { return DynamicLight.LightRange; }

	VOID			Set_LightPosition(XMFLOAT3 _Position) { m_pComTransform->SetPosition(_Position); }
	XMFLOAT3		Get_LightPosition() { return m_pComTransform->GetPosition(); }

	VOID			Set_LightInnerAttenuation(_float _Attenuation) { DynamicLight.InnerAttanuation = _Attenuation; }
	_float			Get_LightInnerAttenuation() { return DynamicLight.InnerAttanuation; }

	VOID			Set_LightOuterAttenuation(_float _Attenuation) { DynamicLight.OuterAttanuation= _Attenuation; }
	_float			Get_LightOuterAttenuation() { return DynamicLight.OuterAttanuation; }

	VOID			Set_LightViewProj(uint32_t _Index, XMMATRIX _LightViewProj) { XMStoreFloat4x4(&DynamicLight.g_LightViewProj[_Index], _LightViewProj);	}
	XMFLOAT4X4		Get_LightViewProj(uint32_t _Index)							{ return DynamicLight.g_LightViewProj[_Index];								}
	XMMATRIX		Get_LoadedLightViewProj(uint32_t _Index)					{ return XMLoadFloat4x4(&DynamicLight.g_LightViewProj[_Index]);				}

	VOID			Add_ShadowRenderGroup(ACTORTYPE _ATYPE, CGameObject* pRenderObject);

	VOID			Set_ShadowMapIndex(uint32_t _Index) { ShadowMapIndex = _Index; }
	
	VOID			Change_LightType(LIGHT_TYPE _LTYPE);

	_bool			Is_StaticDirty()				{ return DirtyFlag;  }
	VOID			Set_StaticDirty(_bool _Flag)	{ DirtyFlag = _Flag; }
public:
	ComPtr<ID3D11Texture2D>				Get_DynamicShadowTexture()	{ return m_pDynamicShadowTexture.Get();	}
	ComPtr<ID3D11DepthStencilView>		Get_DynamicShadowDSV()		{ return m_pDynamicShadowDSV.Get();		}
	ComPtr<ID3D11ShaderResourceView>	Get_DynamicShadowSRV()		{ return m_pDynamicShadowSRV.Get();		}

	ComPtr<ID3D11Texture2D>				Get_StaticShadowTexture()	{ return m_pStaticShadowTexture.Get();	}
	ComPtr<ID3D11DepthStencilView>		Get_StaticShadowDSV()		{ return m_pStaticShadowDSV.Get();		}
	ComPtr<ID3D11ShaderResourceView>	Get_StaticShadowSRV()		{ return m_pStaticShadowSRV.Get();		}

private:
	DYNAMIC_LIGHT						DynamicLight{};

	CComConstantBuffer*					m_pComCBufferPerObject{	};

	SPtr<CCollider>						m_pColliderSphere{};
	SPtr<CCollider>						m_pColliderFrustum{};

	ComPtr<ID3D11Texture2D>				m_pStaticShadowTexture	= { nullptr };
	ComPtr<ID3D11DepthStencilView>		m_pStaticShadowDSV		= { nullptr };
	ComPtr<ID3D11ShaderResourceView>	m_pStaticShadowSRV		= { nullptr };

	ComPtr<ID3D11Texture2D>				m_pDynamicShadowTexture = { nullptr };
	ComPtr<ID3D11DepthStencilView>		m_pDynamicShadowDSV		= { nullptr };
	ComPtr<ID3D11ShaderResourceView>	m_pDynamicShadowSRV		= { nullptr };

	ComPtr<ID3D11Texture2D>				m_pFinalShadowTexture	= { nullptr };
	ComPtr<ID3D11DepthStencilView>		m_pFinalShadowDSV		= { nullptr };
	ComPtr<ID3D11ShaderResourceView>	m_pFinalShadowSRV		= { nullptr };

	std::vector<CGameObject*>			m_pRenderable_StaticObjectList{};
	std::vector<CGameObject*>			m_pRenderable_DynamicObjectList{};

	XMFLOAT4X4							LightView{};
	XMFLOAT4X4							LightProj{};
	XMFLOAT4X4							LightViewProj{};
	XMFLOAT4X4							InvViewProj{};

	_bool								DirtyFlag = { true };
	uint32_t							ShadowMapIndex{};

public:
	VOID		UpdateGUI() override;

	_bool		Check_ObjectInArea();
	VOID		Update_Collider();

	HRESULT		Capture_ShadowMap(ID3D11DeviceContext* pContext);

	VOID		Render_StaticShadow(ID3D11DeviceContext* pContext);
	VOID		Render_DynamicShadow(ID3D11DeviceContext* pContext);

	VOID		Bind_ShadowMapTarget(ID3D11DeviceContext* pContext, _bool _DrawStaticShadow);

	XMFLOAT4X4	Get_LightViewProj() { return LightViewProj; }

public:
	static UPtr<CLight> Create();
	UPtr<CPrototype>	Clone(void* pArg) override;
};
NS_END


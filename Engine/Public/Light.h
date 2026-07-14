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
	HRESULT			Initialize_ShadowMap();

public:
	VOID			Set_LightType(LIGHT_TYPE _LTYPE)				{ m_LightType = _LTYPE;	}
	LIGHT_TYPE		Get_LightType()									{ return m_LightType;	}

	VOID			Set_LightDirection(XMFLOAT3 _Direction)			{ m_fLightDirection = _Direction;		}
	XMFLOAT3		Get_LightDirection()							{ return m_fLightDirection;				}

	VOID			Set_LightColor(XMFLOAT3 _Color)					{ m_fLightColor = _Color;				}
	XMFLOAT3		Get_LightColor()								{ return m_fLightColor;					}

	VOID			Set_LightIntensity(_float _Intensity)			{ m_fLightIntensity = _Intensity;		}
	_float			Get_LightIntensity()							{ return m_fLightIntensity;				}

	VOID			Set_LightRange(_float _Range)					{ m_fLightRange = _Range;				}
	_float			Get_LightRange()								{ return m_fLightRange;					}

	VOID			Set_LightPosition(XMFLOAT3 _Position)			{ m_pComTransform->SetPosition(_Position);	}
	XMFLOAT3		Get_LightPosition()								{ return m_pComTransform->GetPosition();	}

	VOID			Set_LightInnerAttenuation(_float _Attenuation)	{ m_fInnerAttanuation = _Attenuation;	}
	_float			Get_LightInnerAttenuation()						{ return m_fInnerAttanuation;			}

	VOID			Set_LightOuterAttenuation(_float _Attenuation)	{ m_fOuterAttanuation = _Attenuation;	}
	_float			Get_LightOuterAttenuation()						{ return m_fOuterAttanuation;			}

	VOID			Add_ShadowRenderGroup(ACTORTYPE _ATYPE, CGameObject* pRenderObject);

	VOID			Set_ShadowMapIndex(uint32_t _Index) { ShadowMapIndex = _Index; }

private:
	LIGHT_TYPE		m_LightType{LIGHT_TYPE::DIRECTIONAL};

	_float3			m_fLightDirection {1.f, -1.f, 1.f};
	_float3			m_fLightColor{1.f, 1.f, 1.f};
	_float			m_fLightIntensity{10.f};
	_float			m_fLightRange{5.f};

	_float			m_fInnerAttanuation = { 20.f };
	_float			m_fOuterAttanuation = { 30.f };

	SPtr<CResSamplerState>	m_pResSamplerState		{	};

	CComConstantBuffer*		m_pComCBufferPerObject	{	};
	
	SPtr<CCollider>			m_pColliderSphere{};
	SPtr<CCollider>			m_pColliderFrustum{};

	_bool	StaticShadow_DirtyFlag = { true };

	SPtr<CResDynamicTexture2D>	m_pResDynTexStaticShadowMap{};
	SPtr<CResDynamicTexture2D>	m_pResDynTexDynamicShadowMap{};

	std::vector<CGameObject*>	m_pRenderable_StaticObjectList{};
	std::vector<CGameObject*>	m_pRenderable_DynamicObjectList{};

	XMFLOAT4X4	LightView{};
	XMFLOAT4X4	LightProj{};
	XMFLOAT4X4	LightViewProj{};
	XMFLOAT4X4	InvViewProj{};

	uint32_t		ShadowMapIndex{};

public:
	VOID	UpdateGUI() override;

	_bool	Check_ObjectInArea();
	VOID	Update_Collider();

	HRESULT Capture_ShadowMap(ID3D11DeviceContext* pContext);

	VOID	Render_StaticShadow (ID3D11DeviceContext* pContext);
	VOID	Render_DynamicShadow(ID3D11DeviceContext* pContext);

	const SPtr<CResDynamicTexture2D>& Get_DynamicShadowMap()	{ return m_pResDynTexDynamicShadowMap; }
	const SPtr<CResDynamicTexture2D>& Get_StaticShadowMap()		{ return m_pResDynTexStaticShadowMap;  }

	VOID	Bind_ShadowMapTarget(ID3D11DeviceContext* pContext, _bool _DrawStaticShadow);

	XMFLOAT4X4	Get_LightViewProj() { return LightViewProj; }

public:
	HRESULT		InitializePrototype(void* pArg) override;
	HRESULT		Initialize(void* pArg) override;
	void		PriorityUpdate(E::_float fTimeDelta) override;
	void		Update(E::_float fTimeDelta) override;
	void		LateUpdate(E::_float fTimeDelta) override;
	HRESULT		Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx);

public:
	static UPtr<CLight> Create();
	UPtr<CPrototype>	Clone(void* pArg) override;
};
NS_END


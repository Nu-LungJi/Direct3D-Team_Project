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

#define	SCREENX 1280
#define SCREENY	720

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
	HRESULT			InitializePrototype(VOID* pArg) override;
	HRESULT			Initialize(VOID* pArg) override;
	VOID			PriorityUpdate(E::_float _DT) override;
	VOID			Update(E::_float _DT) override;
	VOID			LateUpdate(E::_float _DT) override;
	HRESULT			Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx);

	VOID			Update_ObjectConstantBuffer(ID3D11DeviceContext* pContext);
	VOID			Update_EffectLight(const _float& _DT);

public:
	const DYNAMIC_LIGHT& Get_LightData() { return m_pDynamicLight; }

	VOID			Set_LightType(LIGHT_TYPE _LTYPE) { m_pDynamicLight.LightType = ETOUI(_LTYPE); m_bDirtyFlag = true; }
	LIGHT_TYPE		Get_LightType() { return static_cast<LIGHT_TYPE>(m_pDynamicLight.LightType); }

	VOID			Set_LightDirection(XMFLOAT3 _Direction) { m_pDynamicLight.LightDirection = _Direction; }
	XMFLOAT3		Get_LightDirection()					{ return m_pDynamicLight.LightDirection; }

	VOID			Set_LightColor(XMFLOAT3 _Color) { m_pDynamicLight.LightColor = _Color;}
	XMFLOAT3		Get_LightColor() { return m_pDynamicLight.LightColor; }

	VOID			Set_LightIntensity(_float _Intensity) { m_pDynamicLight.LightIntensity = _Intensity; }
	_float			Get_LightIntensity() { return m_pDynamicLight.LightIntensity; }

	VOID			Set_LightRange(_float _Range);
	_float			Get_LightRange()				{ return m_pDynamicLight.LightRange; }

	VOID			Set_LightPosition(XMFLOAT3 _Position) { m_pComTransform->SetPosition(_Position); m_bDirtyFlag = true;}
	VOID			Set_LightPosition(XMVECTOR _Position) { m_pComTransform->SetPosition(_Position); m_bDirtyFlag = true;}
	XMFLOAT3		Get_LightPosition() { return m_pComTransform->GetPosition(); }

	VOID			Set_LightInnerAttenuation(_float _Attenuation) { m_pDynamicLight.InnerAttanuation = _Attenuation; }
	_float			Get_LightInnerAttenuation() { return m_pDynamicLight.InnerAttanuation; }

	VOID			Set_LightOuterAttenuation(_float _Attenuation) { m_pDynamicLight.OuterAttanuation= _Attenuation; }
	_float			Get_LightOuterAttenuation() { return m_pDynamicLight.OuterAttanuation; }

	VOID			Set_LightActivateState(_bool _State)	{ m_bActivate_State = _State;	}
	_bool			Get_LightActivateState()				{ return m_bActivate_State;	}

	VOID			Set_LightViewProj(uint32_t _Index, XMMATRIX _LightViewProj) { XMStoreFloat4x4(&m_pDynamicLight.g_LightViewProj[_Index], _LightViewProj);	}
	XMFLOAT4X4		Get_LightViewProj(uint32_t _Index)							{ return m_pDynamicLight.g_LightViewProj[_Index];								}
	XMMATRIX		Get_LoadedLightViewProj(uint32_t _Index)					{ return XMLoadFloat4x4(&m_pDynamicLight.g_LightViewProj[_Index]);				}
	
	VOID			Set_LightLifeTime(_float _LifeTime)		{ m_fLifeTime = _LifeTime;	}
	_float			Get_LightLifeTime()						{ return m_fLifeTime;		}

	VOID			Set_LightVelocity(XMFLOAT3 _Velocity)	{ m_fVelocity = _Velocity;	}
	XMFLOAT3		Get_LightVelocity()						{ return m_fVelocity;		}

	VOID			Set_PointLightOuterAttenuation(_float _Attenuation) { m_fPointLightOuterAttenuation = _Attenuation;	}
	_float			Get_PointLightOuterAttenuation()					{ return m_fPointLightOuterAttenuation;			}

	VOID			Set_PointLightInnerAttenuation(_float _Attenuation) { m_fPointLightInnerAttenuation = _Attenuation; }
	_float			Get_PointLightInnerAttenuation()					{ return m_fPointLightInnerAttenuation;			}

	VOID			AddShadowRenderGroup(ACTORTYPE _ATYPE, CGameObject* pRenderObject);
	
	HRESULT			Change_LightType(LIGHT_TYPE _LTYPE);

	const SPtr<CCollider>& Get_SphereCollider()		{ return m_pColliderSphere; }
	const SPtr<CCollider>& Get_FrustumCollider()	{ return m_pColliderFrustum; }

	_bool			Is_StaticDirty()				{ return m_bDirtyFlag;  }
	VOID			Set_StaticDirty(_bool _Flag)	{ m_bDirtyFlag = _Flag; }

	_bool			Is_EffectLight()				{ return m_bEffectLightFlag; }
	VOID			Set_EffectLight(_bool _Flag = true)	{ m_bEffectLightFlag = _Flag; }

	VOID			Set_ShadowSlotNumb(int32_t _Numb)	{ m_ShadowSlot = _Numb; }
	int32_t			Get_ShadowSlotNumb()				{ return m_ShadowSlot;  }

private:
	DYNAMIC_LIGHT						m_pDynamicLight{};

	CComConstantBuffer*					m_pComCBufferPerObject{	};
	CComConstantBuffer*					m_pComCBufferPerPass  {	};

	SPtr<CCollider>						m_pColliderSphere{};
	SPtr<CCollider>						m_pColliderFrustum{};

	std::vector<CGameObject*>			m_pRenderable_StaticObjectList{};
	std::vector<CGameObject*>			m_pRenderable_DynamicObjectList{};

	_bool								m_bDirtyFlag = { true };
	_bool								m_bActivate_State = { true };
	_bool								m_bEffectLightFlag = { false };

	_float								m_fLifeTime = { 0.f };
	XMFLOAT3							m_fVelocity = { 0.f, 0.f, 0.f };

	_float								m_fPointLightInnerAttenuation{};
	_float								m_fPointLightOuterAttenuation{};

	XMFLOAT4X4 LightView{}, LightProj{};

private:	// PointLight
	XMVECTOR DirectionVec[6];
	XMVECTOR BaseUpVec[6];

	int32_t m_ShadowSlot{};

public:
	VOID		UpdateGUI() override;

	_bool		Check_ObjectInArea();
	VOID		Update_Collider();

	HRESULT		Capture_ShadowMap(ID3D11DeviceContext* pContext, E::RENDER_CTX& ctx, const std::vector<IRenderable*>& _ObjectList);
	VOID		Reset_Light();
public:
	static UPtr<CLight> Create();
	UPtr<CPrototype>	Clone(void* pArg) override;
};
NS_END


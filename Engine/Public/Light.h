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
	HRESULT			InitializePrototype(void* pArg) override;
	HRESULT			Initialize(void* pArg) override;
	void			PriorityUpdate(E::_float fTimeDelta) override;
	void			Update(E::_float fTimeDelta) override;
	void			LateUpdate(E::_float fTimeDelta) override;
	HRESULT			Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx);

	void			Update_ObjectConstantBuffer(ID3D11DeviceContext* pContext);

public:
	const DYNAMIC_LIGHT& Get_LightData() { return m_pDynamicLight; }

	VOID			Set_LightType(LIGHT_TYPE _LTYPE) { m_pDynamicLight.LightType = ETOUI(_LTYPE); DirtyFlag = true; }
	LIGHT_TYPE		Get_LightType() { return static_cast<LIGHT_TYPE>(m_pDynamicLight.LightType); }

	VOID			Set_LightDirection(XMFLOAT3 _Direction) { m_pDynamicLight.LightDirection = _Direction; }
	XMFLOAT3		Get_LightDirection()					{ return m_pDynamicLight.LightDirection; }

	VOID			Set_LightColor(XMFLOAT3 _Color) { m_pDynamicLight.LightColor = _Color;}
	XMFLOAT3		Get_LightColor() { return m_pDynamicLight.LightColor; }

	VOID			Set_LightIntensity(_float _Intensity) { m_pDynamicLight.LightIntensity = _Intensity; }
	_float			Get_LightIntensity() { return m_pDynamicLight.LightIntensity; }

	VOID			Set_LightRange(_float _Range) { m_pDynamicLight.LightRange = _Range; DirtyFlag = true; }
	_float			Get_LightRange()				{ return m_pDynamicLight.LightRange; }

	VOID			Set_LightPosition(XMFLOAT3 _Position) { m_pComTransform->SetPosition(_Position); DirtyFlag = true;}
	XMFLOAT3		Get_LightPosition() { return m_pComTransform->GetPosition(); }

	VOID			Set_LightInnerAttenuation(_float _Attenuation) { m_pDynamicLight.InnerAttanuation = _Attenuation; }
	_float			Get_LightInnerAttenuation() { return m_pDynamicLight.InnerAttanuation; }

	VOID			Set_LightOuterAttenuation(_float _Attenuation) { m_pDynamicLight.OuterAttanuation= _Attenuation; }
	_float			Get_LightOuterAttenuation() { return m_pDynamicLight.OuterAttanuation; }

	VOID			Set_LightViewProj(uint32_t _Index, XMMATRIX _LightViewProj) { XMStoreFloat4x4(&m_pDynamicLight.g_LightViewProj[_Index], _LightViewProj);	}
	XMFLOAT4X4		Get_LightViewProj(uint32_t _Index)							{ return m_pDynamicLight.g_LightViewProj[_Index];								}
	XMMATRIX		Get_LoadedLightViewProj(uint32_t _Index)					{ return XMLoadFloat4x4(&m_pDynamicLight.g_LightViewProj[_Index]);				}
	
	VOID			Add_ShadowRenderGroup(ACTORTYPE _ATYPE, CGameObject* pRenderObject);
	
	HRESULT			Change_LightType(LIGHT_TYPE _LTYPE);

	const SPtr<CCollider>& Get_SphereCollider()		{ return m_pColliderSphere; }
	const SPtr<CCollider>& Get_FrustumCollider()	{ return m_pColliderFrustum; }

	_bool			Is_StaticDirty()				{ return DirtyFlag;  }
	VOID			Set_StaticDirty(_bool _Flag)	{ DirtyFlag = _Flag; }

public:
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

	_bool								DirtyFlag = { true };

	XMFLOAT4X4 LightView{}, LightProj{};

private:	// PointLight
	XMVECTOR DirectionVec[6];
	XMVECTOR BaseUpVec[6];

	int32_t m_ShadowSlot{};

private:
	VOID	Update_PointLight_ProjectionMatrix(_float _Range);

public:
	VOID		UpdateGUI() override;

	_bool		Check_ObjectInArea();
	VOID		Update_Collider();

	HRESULT		Capture_ShadowMap(ID3D11DeviceContext* pContext, const std::vector<CGameObject*>& _ObjectList);

public:
	static UPtr<CLight> Create();
	UPtr<CPrototype>	Clone(void* pArg) override;
};
NS_END


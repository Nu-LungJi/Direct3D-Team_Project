#pragma once
#include "Engine_Defines.h"
#include "ResCBuffer.h"
#include "GameObject.h"

NS_BEGIN(Engine)

class ENGINE_DLL CLight final : public CGameObject {

public:
	DECLARE_DERIVED_TYPE(CLight, CGameObject)

public:
	VOID			Set_LightType(LIGHT_TYPE _LTYPE)				{ m_LightType = _LTYPE; }
	LIGHT_TYPE		Get_LightType()									{ return m_LightType;	}

	VOID			Set_LightDirection(XMFLOAT3 _Direction)			{ m_fLightDirection = _Direction;		}
	XMFLOAT3		Get_LightDirection()							{ return m_fLightDirection;				}

	VOID			Set_LightColor(XMFLOAT3 _Color)					{ m_fLightColor = _Color;				}
	XMFLOAT3		Get_LightColor()								{ return m_fLightColor;					}

	VOID			Set_LightIntensity(_float _Intensity)			{ m_fLightIntensity = _Intensity;		}
	_float			Get_LightIntensity()							{ return m_fLightIntensity;				}

	VOID			Set_LightRange(_float _Range)					{ m_fLightRange = _Range;				}
	_float			Get_LightRange()								{ return m_fLightRange;					}

	VOID			Set_LightPosition(XMFLOAT3 _Position)			{ m_fPosition = _Position;				}
	XMFLOAT3		Get_LightPosition()								{ return m_fPosition;					}

	VOID			Set_LightInnerAttenuation(_float _Attenuation)	{ m_fInnerAttanuation = _Attenuation;	}
	_float			Get_LightInnerAttenuation()						{ return m_fInnerAttanuation;			}

	VOID			Set_LightOuterAttenuation(_float _Attenuation)	{ m_fOuterAttanuation = _Attenuation;	}
	_float			Get_LightOuterAttenuation()						{ return m_fOuterAttanuation;			}

private:
	LIGHT_TYPE		m_LightType;

	_float3			m_fLightDirection;
	_float3			m_fLightColor;
	_float			m_fLightIntensity;
	_float			m_fLightRange;

	_float3			m_fPosition;

	_float			m_fInnerAttanuation;
	_float			m_fOuterAttanuation;

public:
	static SPtr<CLight> Create();
};
NS_END
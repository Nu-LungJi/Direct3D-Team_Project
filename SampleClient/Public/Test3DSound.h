#pragma once
#include "GameObject.h"
#include "Client_Defines.h"
NS_BEGIN(Engine)
NS_END

NS_BEGIN(Client)
class CResTerrainVIBuffer;
class CTest3DSound final : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CTest3DSound, CGameObject)

public:
	typedef struct tagTerrainDesc : public CGameObject::GAMEOBJECT_DESC
	{
	}DESC;

private:
	CTest3DSound();
	~CTest3DSound() override;

public:
	HRESULT Initialize(void* pArg) override;
	void LateUpdate(E::_float fTimeDelta) override;

private:
	SOUND_ID m_soundID{ INVALID_SOUND_ID };

public:
	static E::UPtr<CTest3DSound> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;

private:
	void Free() override;
};

NS_END

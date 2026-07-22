#pragma once
#include "GameObject.h"
#include "Client_Defines.h"
NS_BEGIN(Engine)
class CComSound;
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
		std::string loopSoundPath{};
	}DESC;

private:
	CTest3DSound();
	~CTest3DSound() override;

public:
	void UpdateGUI() override;
	HRESULT Initialize(void* pArg) override;
	void LateUpdate(E::_float fTimeDelta) override;

private:
	E::CComSound* m_pComSound{};

public:
	static E::UPtr<CTest3DSound> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END

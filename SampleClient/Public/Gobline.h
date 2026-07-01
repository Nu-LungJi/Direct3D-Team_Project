#pragma once
#include "GameObject.h"
#include "Client_Defines.h"


NS_BEGIN(Client)
class CGobline : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CGobline, CGameObject)
public:
	typedef struct tagGoblnedesc : CGameObject::GAMEOBJECT_DESC
	{

	}GOBLINE_DESC;
private:
	CGobline();
	~CGobline() override;

public:
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) override;

public:
	static E::UPtr<CGobline> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END


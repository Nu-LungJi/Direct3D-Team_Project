#include "pch.h"
#include "AnimationObject.h"

#include "ComAnimator.h"
#include "ComModelInstance.h"
#include "GameInstance.h"

NS_USING(Engine)

CAnimationObject::CAnimationObject(): CGameObject{}
{
}

CAnimationObject::CAnimationObject(const CAnimationObject& Prototype): CGameObject{ Prototype }
{


}

CAnimationObject::~CAnimationObject()
{
}

HRESULT CAnimationObject::Initialize(
	void* pArg)
{
	if (FAILED(
		CGameObject::Initialize(pArg)))
	{
		return E_FAIL;
	}

	return S_OK;
}

void CAnimationObject::Update(_float fTimeDelta)
{
	CGameObject::Update(fTimeDelta);

}

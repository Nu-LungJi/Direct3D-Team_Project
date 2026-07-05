#include "BTMove.h"

NS_USING(Client)

CBTMove::CBTMove()
{

}

CBTMove::~CBTMove()
{
}
HRESULT CBTMove::Initalize(void* pArg)
{
	
	__super::Initalize(pArg);

	return S_OK;
}
HRESULT CBTMove::Priority_Update(_float fTimeDelta)
{
	return E_NOTIMPL;
}

HRESULT CBTMove::Update(_float fTimeDelta)
{
	return E_NOTIMPL;
}

HRESULT CBTMove::Late_Update(_float fTimeDelta)
{
	return E_NOTIMPL;
}

EVALUATE CBTMove::Evaluate()
{
	return EVALUATE();
}

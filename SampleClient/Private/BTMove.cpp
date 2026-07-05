#include "BTMove.h"

NS_USING(Client)

CBTMove::CBTMove()
{

}

CBTMove::CBTMove(const CBTMove& Prototype) : CBTActionNode(Prototype)
{
}

CBTMove::~CBTMove()
{
}
HRESULT CBTMove::InitializePrototype()
{

	return S_OK;
}
HRESULT CBTMove::Initalize(void* pArg)
{

	__super::Initalize(pArg);

	return S_OK;
}

EVALUATE CBTMove::Evaluate()
{
	return EVALUATE();
}
E::UPtr<CBTMove> CBTMove::Create()
{
	auto pInstance = E::ToUPtr(new CBTMove{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CBTMove");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CBTRoot> CBTMove::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTMove{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTRoot");
		return nullptr;
	}

	return pInstance;
}

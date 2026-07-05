#include "pch.h"
#include "BTAnimation.h"

NS_USING(Client)

CBTAnimation::CBTAnimation()
{

}

CBTAnimation::CBTAnimation(const CBTAnimation& Prototype) : CBTActionNode(Prototype)
{
}

CBTAnimation::~CBTAnimation()
{
}
HRESULT CBTAnimation::InitializePrototype()
{

	return S_OK;
}
HRESULT CBTAnimation::Initalize(void* pArg)
{
	__super::Initalize(pArg);
	return S_OK;
}

EVALUATE CBTAnimation::Evaluate()
{
	return EVALUATE();
}
E::UPtr<CBTAnimation> CBTAnimation::Create()
{
	auto pInstance = E::ToUPtr(new CBTAnimation{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CBTAnimation");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CBTRoot> CBTAnimation::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTAnimation{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTAnimation");
		return nullptr;
	}

	return pInstance;
}

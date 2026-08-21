#include "pch.h"
#include "AccioActivity_RampLarge.h"

NS_USING(Client)

CAccioActivity_RampLarge::CAccioActivity_RampLarge() = default;
CAccioActivity_RampLarge::CAccioActivity_RampLarge(const CAccioActivity_RampLarge& prototype)
	: CAccioActivityPartBase{ prototype }
{
}

StringID CAccioActivity_RampLarge::GetModelResourceTag() const
{
	return "Static_AccioActivity_RampLarge_Resource";
}

UPtr<CAccioActivity_RampLarge> CAccioActivity_RampLarge::Create()
{
	auto pInstance = ToUPtr(new CAccioActivity_RampLarge{});
	if (FAILED(pInstance->InitializePrototype())) return nullptr;
	return pInstance;
}

UPtr<CPrototype> CAccioActivity_RampLarge::Clone(void* pArg)
{
	auto pInstance = ToUPtr(new CAccioActivity_RampLarge{ *this });
	if (FAILED(pInstance->Initialize(pArg))) return nullptr;
	return pInstance;
}

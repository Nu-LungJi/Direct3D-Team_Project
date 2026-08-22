#include "pch.h"
#include "AccioActivity_LampSmall.h"

NS_USING(Client)

CAccioActivity_LampSmall::CAccioActivity_LampSmall() = default;
CAccioActivity_LampSmall::CAccioActivity_LampSmall(const CAccioActivity_LampSmall& prototype)
	: CAccioActivityPartBase{ prototype }
{
}

StringID CAccioActivity_LampSmall::GetModelResourceTag() const
{
	return "Static_AccioActivity_RampSmall_Resource";
}

UPtr<CAccioActivity_LampSmall> CAccioActivity_LampSmall::Create()
{
	auto pInstance = ToUPtr(new CAccioActivity_LampSmall{});
	if (FAILED(pInstance->InitializePrototype())) return nullptr;
	return pInstance;
}

UPtr<CPrototype> CAccioActivity_LampSmall::Clone(void* pArg)
{
	auto pInstance = ToUPtr(new CAccioActivity_LampSmall{ *this });
	if (FAILED(pInstance->Initialize(pArg))) return nullptr;
	return pInstance;
}

#include "pch.h"
#include "AccioActivity_BumperA.h"

NS_USING(Client)

CAccioActivity_BumperA::CAccioActivity_BumperA() = default;
CAccioActivity_BumperA::CAccioActivity_BumperA(const CAccioActivity_BumperA& prototype)
	: CAccioActivityPartBase{ prototype }
{
}

StringID CAccioActivity_BumperA::GetModelResourceTag() const
{
	return "Static_AccioActivity_BumperA_Resource";
}

UPtr<CAccioActivity_BumperA> CAccioActivity_BumperA::Create()
{
	auto pInstance = ToUPtr(new CAccioActivity_BumperA{});
	if (FAILED(pInstance->InitializePrototype())) return nullptr;
	return pInstance;
}

UPtr<CPrototype> CAccioActivity_BumperA::Clone(void* pArg)
{
	auto pInstance = ToUPtr(new CAccioActivity_BumperA{ *this });
	if (FAILED(pInstance->Initialize(pArg))) return nullptr;
	return pInstance;
}

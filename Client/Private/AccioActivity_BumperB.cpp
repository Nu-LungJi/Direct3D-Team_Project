#include "pch.h"
#include "AccioActivity_BumperB.h"

NS_USING(Client)

CAccioActivity_BumperB::CAccioActivity_BumperB() = default;
CAccioActivity_BumperB::CAccioActivity_BumperB(const CAccioActivity_BumperB& prototype)
	: CAccioActivityPartBase{ prototype }
{
}

StringID CAccioActivity_BumperB::GetModelResourceTag() const
{
	return "Static_AccioActivity_Bumper_Resource";
}

UPtr<CAccioActivity_BumperB> CAccioActivity_BumperB::Create()
{
	auto pInstance = ToUPtr(new CAccioActivity_BumperB{});
	if (FAILED(pInstance->InitializePrototype())) return nullptr;
	return pInstance;
}

UPtr<CPrototype> CAccioActivity_BumperB::Clone(void* pArg)
{
	auto pInstance = ToUPtr(new CAccioActivity_BumperB{ *this });
	if (FAILED(pInstance->Initialize(pArg))) return nullptr;
	return pInstance;
}

#include "pch.h"
#include "Prototype.h"

NS_USING(Engine)

CPrototype::CPrototype()
	: CEngineBase{}
{
}

CPrototype::CPrototype(const CPrototype& Prototype)
	: CEngineBase{ Prototype }
{
}

CPrototype::~CPrototype()
{
}

//CPrototype::CPrototype(const CPrototype& rhs)
//{
//}

HRESULT CPrototype::InitializePrototype(void* pArg)
{
	return S_OK;
}

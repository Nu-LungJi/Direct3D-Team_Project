#pragma once
#include "Engine_Defines.h"

NS_BEGIN(Engine)

class ENGINE_DLL CPrototype : public CEngineBase
{
public:
	DECLARE_DERIVED_TYPE(CPrototype, CEngineBase)

protected:
	CPrototype();
	CPrototype(const CPrototype& Prototype);
	~CPrototype() override;

protected:
	virtual HRESULT InitializePrototype(void* pArg = nullptr);

public:
	virtual UPtr<CPrototype> Clone(void* pArg) = 0;
};

NS_END

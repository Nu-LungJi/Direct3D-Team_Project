#pragma once
#include "Engine_Defines.h"

NS_BEGIN(Engine)
class ENGINE_DLL CBTRoot : public CEngineBase
{
public:
	DECLARE_DERIVED_TYPE(CBTRoot, CEngineBase)
protected:
	explicit CBTRoot();
	~CBTRoot() override;

};

NS_END
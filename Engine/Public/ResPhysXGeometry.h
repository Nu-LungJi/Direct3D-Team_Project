#pragma once
#include "Resource.h"

NS_BEGIN(Engine)

class ENGINE_DLL CResPhysXGeometry : public CResource
{
public:
	DECLARE_DERIVED_TYPE(CResPhysXGeometry, CResource)

protected:
	explicit CResPhysXGeometry(const _string& sPath);
	~CResPhysXGeometry() override;
};

NS_END

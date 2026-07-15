#pragma once
#include "Resource.h"
NS_BEGIN(Engine)

class ENGINE_DLL CResLua : public CResource
{
public:
	struct DESC {
	};
public:
	DECLARE_DERIVED_TYPE(CResLua, CResource)

protected:
	explicit CResLua(const _string& sPath);
	~CResLua() override;

public:

protected:

protected:
};

NS_END

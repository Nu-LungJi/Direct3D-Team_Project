#pragma once

#include "AccioActivityPartBase.h"

NS_BEGIN(Client)

class CAccioActivity_LampSmall final : public CAccioActivityPartBase
{
public:
	DECLARE_DERIVED_TYPE(CAccioActivity_LampSmall, CAccioActivityPartBase)

private:
	CAccioActivity_LampSmall();
	CAccioActivity_LampSmall(const CAccioActivity_LampSmall& prototype);
	~CAccioActivity_LampSmall() override = default;

public:
	static UPtr<CAccioActivity_LampSmall> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

protected:
	StringID GetModelResourceTag() const override;
};

NS_END

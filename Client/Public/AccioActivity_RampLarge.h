#pragma once

#include "AccioActivityPartBase.h"

NS_BEGIN(Client)

class CAccioActivity_RampLarge final : public CAccioActivityPartBase
{
public:
	DECLARE_DERIVED_TYPE(CAccioActivity_RampLarge, CAccioActivityPartBase)

private:
	CAccioActivity_RampLarge();
	CAccioActivity_RampLarge(const CAccioActivity_RampLarge& prototype);
	~CAccioActivity_RampLarge() override = default;

public:
	static UPtr<CAccioActivity_RampLarge> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

protected:
	StringID GetModelResourceTag() const override;
};

NS_END

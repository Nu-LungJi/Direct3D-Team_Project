#pragma once

#include "AccioActivityPartBase.h"

NS_BEGIN(Client)

class CAccioActivity_BumperB final : public CAccioActivityPartBase
{
public:
	DECLARE_DERIVED_TYPE(CAccioActivity_BumperB, CAccioActivityPartBase)

private:
	CAccioActivity_BumperB();
	CAccioActivity_BumperB(const CAccioActivity_BumperB& prototype);
	~CAccioActivity_BumperB() override = default;

public:
	static UPtr<CAccioActivity_BumperB> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

protected:
	StringID GetModelResourceTag() const override;
};

NS_END

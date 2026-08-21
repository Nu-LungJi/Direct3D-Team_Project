#pragma once

#include "AccioActivityPartBase.h"

NS_BEGIN(Client)

class CAccioActivity_BumperA final : public CAccioActivityPartBase
{
public:
	DECLARE_DERIVED_TYPE(CAccioActivity_BumperA, CAccioActivityPartBase)

private:
	CAccioActivity_BumperA();
	CAccioActivity_BumperA(const CAccioActivity_BumperA& prototype);
	~CAccioActivity_BumperA() override = default;

public:
	static UPtr<CAccioActivity_BumperA> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

protected:
	StringID GetModelResourceTag() const override;
};

NS_END

#pragma once

#include "GameObject.h"

NS_BEGIN(Engine)

class CComAnimator;
class CComModelInstance;

struct MODEL_INSTANCE_BATCH;

class ENGINE_DLL CAnimationObject abstract : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CAnimationObject,CGameObject)

	CAnimationObject & operator=( const CAnimationObject&) = delete;

protected:
	explicit CAnimationObject();
	explicit CAnimationObject(const CAnimationObject& Prototype);

	~CAnimationObject() override;

public:
	HRESULT Initialize(void* pArg) override;

	void Update(_float fTimeDelta) override;

public:
	uint32_t GetInstanceModelNum() { return m_iInstanceModelNum; }
	void     SetInstanceModelNum(uint32_t iInstacneNum) override { m_iInstanceModelNum = iInstacneNum; }

private:
	uint32_t m_iInstanceModelNum = 0.f;
	

};

NS_END

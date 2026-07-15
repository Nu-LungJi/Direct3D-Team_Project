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
	
	virtual HRESULT Render_Instanced(
		ID3D11DeviceContext* pContext,
		const RENDER_CTX& ctx,
		const MODEL_INSTANCE_BATCH& Batch) = 0;

};

NS_END

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

	CAnimationObject & operator=(const CAnimationObject&) = delete;
public:
	struct DESC : public CGameObject::GAMEOBJECT_DESC
	{

	};
protected:
	explicit CAnimationObject();
	explicit CAnimationObject(const CAnimationObject& Prototype);

	~CAnimationObject() override;

public:
	HRESULT Initialize(void* pArg) override;

	void Update(_float fTimeDelta) override;

public:
	void UpdateRenderVisibility(const BoundingBox& WorldBounds);

	_bool IsMainViewVisible() const { return m_bMainViewVisible; }
	_bool IsShadowVisible() const { return m_bShadowVisible; }
	_bool ShouldSubmitRenderInstance() const { return m_bMainViewVisible || m_bShadowVisible; }



public:
	uint32_t GetInstanceModelNum() { return m_iInstanceModelNum; }
	void     SetInstanceModelNum(uint32_t iInstacneNum) override { m_iInstanceModelNum = iInstacneNum; }

private:
	static _bool IntersectsClipVolume(const BoundingBox& WorldBounds, _fmatrix ViewProj);

private:
	uint32_t m_iInstanceModelNum = 0.f;

	_bool m_bMainViewVisible{ true };
	_bool m_bShadowVisible{ true };


};

NS_END

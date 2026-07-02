
#pragma once

#include "Component.h"

NS_BEGIN(Engine)
class CComModelInstance;
class CComAnimMontage;

class ENGINE_DLL CComAnimator : public CComponent
{
public:
	typedef struct tagDesc : CComponent::DESC
	{
		CComModelInstance* _pModelInstance;
	}DESC;
public:
	DECLARE_DERIVED_TYPE(CComAnimator, CComponent)


private:
	explicit CComAnimator();
	~CComAnimator() override;

private:
	HRESULT Initialize(void* pArg) override;

public:
	HRESULT Update(_float fTimeDelta);

	HRESULT Play_AnimationMontage(const std::string& strAnimMontageName);
	 

public:
	HRESULT AnimEditor_Play_AnimResource(_float fTimeDelta, uint32_t iModelAnimNum);
	HRESULT AnimEditor_Play_AnimMontage(_float fTimeDelta, const std::string& strAnimMontageName);




private:
	CComModelInstance* m_pModelInstance;
	std::unordered_map <std::string, CComAnimMontage*> m_mapAnimMontages;


public:
	static UPtr<CComAnimator> Create();
	UPtr<CPrototype> Clone(void* pArg) override;
};

NS_END
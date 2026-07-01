#pragma once
#include "Component.h"

NS_BEGIN(Engine)
class  ENGINE_DLL CComBeHavior : public CComponent
{
public:
	DECLARE_DERIVED_TYPE(CComBeHavior, CComponent)

	//CComBeHavior& operator=(const CComBeHavior&) = delete;
protected:
	explicit CComBeHavior();
	explicit CComBeHavior(const CComBeHavior& Prototype);
	 ~CComBeHavior() override;

private:
	virtual HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initalize(void* pArg);
	int32_t	Find_Secquence(const _string& strSecquenceName);

public:
	void				Update(_float fTimeDelta);

public:
	HRESULT				Add_Secqunce(const _string& strSecquenceName);
	HRESULT				Add_SecqunceToNode(const _string& strSequenceName, UPtr<class CBTRoot> pActionNode);

private:	
	UPtr<class CBTSelector>					m_Root;
	std::map<_string, uint32_t>				m_NodeHandles;

public:
	static UPtr<CComBeHavior>Create();
	virtual UPtr<CPrototype> Clone(void* pArg) override;
};

NS_END
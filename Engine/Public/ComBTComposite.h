#pragma once
#include "ComBTRoot.h"

//¼¿·ºÅÍ ½ÃÄö½º¿ëµµ
NS_BEGIN(Engine)

class  ComBTComposite : public ComBTRoot
{
public:
	DECLARE_DERIVED_TYPE(ComBTComposite, CEngineBase)
protected:
	explicit ComBTComposite() = default;
	virtual ~ComBTComposite() = default;

	virtual HRESULT InitializePrototype(void* pArg = nullptr);
	virtual HRESULT Initalize(void* pArg) { return S_OK; }
public:
	virtual EVALUATE		Evaluate() PURE;
	HRESULT	Add_Node(const StringID& svGroupTag, const StringID& svPrototypetag, void* pArg);
private:
	std::list<SPtr<ComBTRoot>> m_Actions;

};
NS_END
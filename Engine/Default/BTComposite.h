#pragma once
#include "BTRoot.h"

//¼¿·ºÅÍ ½ÃÄö½º¿ëµµ
NS_BEGIN(Engine)

class  CBTComposite : public CBTRoot
{
protected:
	explicit CBTComposite();
	virtual ~CBTComposite();

	virtual HRESULT InitializePrototype(void* pArg = nullptr);
	virtual HRESULT Initalize(void* pArg) { return S_OK; }

protected:
	HRESULT			Find_Node(const _string& tagSecqunce);
public:
	virtual EVALUATE		Evaluate() PURE;
private:
	std::vector<std::unique_ptr<CBTRoot>> m_Actions;
	std::map<_string, uint32_t>			  m_NodeHandles;
};
NS_END
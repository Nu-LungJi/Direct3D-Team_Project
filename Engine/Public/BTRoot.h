#pragma once
#include "Engine_Defines.h"

enum class EVALUATE { SUCCESS, FAILED, RUN };
//뿌리
NS_BEGIN(Engine)
class  CBTRoot : public CEngineBase
{
public:
	DECLARE_DERIVED_TYPE(CBTRoot, CEngineBase)
public:
	typedef struct tagbtroot
	{
		GUINODE		 m_GuiNode;
		GUINODE_LINK m_GuiLink;
		_string		 NodeName;
	}BTROOT_DESC;

protected:
	explicit CBTRoot();
	~CBTRoot() override;

	virtual HRESULT Initalize(void* pArg);
public:
	GUINODE& Get_GuiNodeInfo() { return m_GuiNode; }
	GUINODE_LINK& Get_GuiNodeLink() { return m_GuiLink; }
	virtual HRESULT	Priority_Update(_float fTimeDelta) { return S_OK; };
	virtual HRESULT	Update(_float fTimeDelta) { return S_OK; };
	virtual HRESULT	Late_Update(_float fTimeDelta) { return S_OK; };
public:
	virtual EVALUATE		Evaluate() PURE;
	const ACTION_NODE& Get_NodeInfo() { return m_NodeInfo; }
protected:
	ACTION_NODE							m_NodeInfo;
	GUINODE								m_GuiNode;
	GUINODE_LINK						m_GuiLink;

public:
	virtual UPtr<CBTRoot>Clone(void* pArg) PURE;
};

NS_END
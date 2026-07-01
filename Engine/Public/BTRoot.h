#pragma once
#include "Engine_Defines.h"

enum class EVALUATE { SUCCESS, FAILED, RUN };
//»Ñ¸®
NS_BEGIN(Engine)
class  CBTRoot : public CEngineBase
{
public:
	DECLARE_DERIVED_TYPE(CBTRoot, CEngineBase)

	typedef struct tagbtroot
	{
		_string NodeName;
	}BTROOT_DESC;

protected:
	explicit CBTRoot();
	 ~CBTRoot() override;
	
	virtual HRESULT Initalize(void* pArg);
public:

	virtual HRESULT	Priority_Update(_float fTimeDelta) PURE;
	virtual HRESULT	Update(_float fTimeDelta)		   PURE;
	virtual HRESULT	Late_Update(_float fTimeDelta)	   PURE;
public:
	virtual EVALUATE		Evaluate() PURE;
	const _string&		Get_NodeName() { return m_NodeName; }
protected:
	std::string							m_NodeName;

};

NS_END
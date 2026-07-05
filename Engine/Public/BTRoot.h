#pragma once
#include "Engine_Defines.h"
#include "GameInstance.h"
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
		CHandle		Handle;
		GUINODE		 m_GuiNode;
		GUINODE_LINK m_GuiLink;
		_string		 NodeName;
	}BTROOT_DESC;

protected:
	explicit CBTRoot();
	~CBTRoot() override;

	virtual HRESULT Initalize(void* pArg);
public:
	GUINODE&		Get_GuiNodeInfo() { return m_GuiNode; }
	GUINODE_LINK&	Get_GuiNodeLink() { return m_GuiLink; }
	CHandle&		Get_Handle() { return m_Handle; }

	virtual HRESULT	Priority_Update(_float fTimeDelta) { return S_OK; };
	virtual HRESULT	Update(_float fTimeDelta) { return S_OK; };
	virtual HRESULT	Late_Update(_float fTimeDelta) { return S_OK; };
public:
	virtual nlohmann::json				Save_Node()PURE;
	virtual HRESULT						Load_json(nlohmann::json& j) PURE;
public:
	virtual EVALUATE		Evaluate(_float fTimeDelta) PURE;

protected:
	GUINODE								m_GuiNode;
	GUINODE_LINK						m_GuiLink;
	CHandle								m_Handle;

public:
	template<typename T1> 
	class CComponent* Get_Component(const CHandle & Handle, const _string& name)
	{
		if (auto pObj = CGameInstance::Get().GetGameObjectByHandle(Handle))
		{
			if (auto pComBt = pObj->GetComponent<T1>(name))
			{
				m_Handle = Handle;
				return pComBt;
			}
		}
		return nullptr;
	}
public:
	virtual UPtr<CBTRoot>Clone(void* pArg) PURE;
};

NS_END

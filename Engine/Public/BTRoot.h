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
		NODEGROUP    eGroup;
		GUINODE		 m_GuiNode;
		GUINODE_LINK m_GuiLink;
		_string		 NodeName;
		_string      m_MasterName;
	}BTROOT_DESC;

protected:
	explicit CBTRoot();
	CBTRoot(const CBTRoot& rhs);
	~CBTRoot() override;

	virtual HRESULT InitalizePrototype(void* pArg = nullptr);
	virtual HRESULT Initalize(void* pArg);
public:
	GUINODE&		Get_GuiNodeInfo() { return m_GuiNode; }
	GUINODE_LINK&	Get_GuiNodeLink() { return m_GuiLink; }
	void			Set_Handle(CHandle Handle) { m_Handle = Handle; }
	CHandle&		Get_Handle() { return m_Handle; }
public:
	virtual nlohmann::json		Save_Node();
	virtual HRESULT				Load_json(const nlohmann::json& j);
public:
	virtual EVALUATE		Evaluate(_float fTimeDelta) PURE;
protected:
	GUINODE								m_GuiNode;
	GUINODE_LINK						m_GuiLink;
	CHandle								m_Handle;
	_string								m_MasterName;
	NODEGROUP							m_eGroup{};
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

#pragma once
#include "Component.h"
#include "BTRoot.h"
NS_BEGIN(Engine)
class  ENGINE_DLL CComBeHavior : public CComponent
{
public:		
	typedef struct tagbehaviordesc : public CComponent::DESC
	{

	}BEHAVIOR_DESC;
public:
	DECLARE_DERIVED_TYPE(CComBeHavior, CComponent)

	//CComBeHavior& operator=(const CComBeHavior&) = delete;
protected:
	explicit CComBeHavior();
	explicit CComBeHavior(const CComBeHavior& Prototype);
	 ~CComBeHavior() override;

private:
	virtual HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	
	void					Set_NodeInfo(CBTRoot* pNode);
public:
	void					Save_Data(const _string& filePath);
	HRESULT					Load_Data(const _string& filePath);
public:
	void					Update(_float fTimeDelta);					
	void					UpdateGUI()	override;

public:
	uint32_t&				Get_NodeID() { return m_iNodeID; }
	class CBTComposite*		Get_Selector();
	CBTRoot*				Find_Node(const uint32_t& iNode);

	void					RegistNode  (uint32_t iIndex, CBTRoot* pNode);
	void					UnRegistNode(uint32_t iindex);
	void					Set_JsonFileName(const _string& Name) { m_FileName = Name; }
private:	
	UPtr<class CBTComposite>				m_Root;
	std::map<uint32_t,CBTRoot*>				m_NodeMap;
	uint32_t								m_iNodeID{ 0 };
	_string									m_FileName{};
public:
	static UPtr<CComBeHavior>Create();
	virtual UPtr<CPrototype> Clone(void* pArg) override;
};

NS_END

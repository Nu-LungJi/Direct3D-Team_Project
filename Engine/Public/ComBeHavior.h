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
	
	int32_t					Recursive_Call_Node(class CBTRoot* pNode, int32_t* iIndex, const _string& NodeName);
	int32_t					Find_Node(const _string& NodeName);
public:
	void					Update(_float fTimeDelta);					
	void					UpdateGUI()		override;
public:
	uint32_t&				Get_NodeID() { return m_iNodeID; }
	HRESULT					Add_Node(void* pArg);

	//HRESULT					Add_Secqunce(const _string& strSecquenceName);
	//HRESULT					Add_SecqunceToNode(const _string& strSequenceName, UPtr<class CBTRoot> pActionNode);
	
	int32_t					Check_AllNode( const _string& NodeName);

	class CBTRoot*			Get_Node(const _string& NodeName);
	class CBTSelector*		Get_Selector();
private:	
	UPtr<class CBTSelector>						m_Root;
	std::map<_string, uint32_t>				m_NodeHandles;

	uint32_t								m_iNodeID{ 0 };
public:
	static UPtr<CComBeHavior>Create();
	virtual UPtr<CPrototype> Clone(void* pArg) override;
};

NS_END
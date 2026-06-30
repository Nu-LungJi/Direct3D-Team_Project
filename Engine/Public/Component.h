#pragma once
#include "Prototype.h"
#include "Handle.h"

NS_BEGIN(Engine)
class CGameObject;
class ENGINE_DLL CComponent : public CPrototype
{
public:
	DECLARE_DERIVED_TYPE(CComponent, CPrototype)
	CComponent& operator=(const CComponent&) = delete;

public:
	typedef struct tagComponentDesc: public CEngineBase
	{
		CGameObject* pGameObject{};
	}DESC;
	//public:
	//	void SetGameObject(CGameObject* pGameObject) { m_pGameObject = pGameObject; }
	//	CGameObject* GetGameObject() const { return m_pGameObject; }

protected:
	explicit CComponent();
	explicit CComponent(const CComponent& Prototype);
	~CComponent() override;

public:
	virtual void UpdateGUI();

protected:
	virtual HRESULT Initialize(void* pArg);

protected:
	//std::optional<CHandle> m_ObjectHandle{};
	CGameObject* m_pGameObject{};
public:
	//void SetGameObject(CGameObject* pObj) { m_pGameObject = pObj; }
	CGameObject* GetGameObject() const { return m_pGameObject; }

	//const std::optional<CHandle>& GetGameObjectHandle() const { return m_ObjectHandle; }
	//void SetGameObjectHandle(const CHandle& handle)
	//{
	//	assert(!m_ObjectHandle.has_value() && "이 오브젝트는 이미 핸들이 할당되어 있습니다!");

	//	if (!m_ObjectHandle.has_value())
	//	{
	//		m_ObjectHandle = handle;
	//	}
	//}

};

NS_END
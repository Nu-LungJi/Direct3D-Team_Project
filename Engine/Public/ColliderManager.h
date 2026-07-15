#pragma once
#include "Engine_Defines.h"

NS_BEGIN(Engine)
class CCollider;
class CColliderManager final: public CEngineBase
{
public:
	DECLARE_DERIVED_TYPE(CColliderManager, CEngineBase)

private:
	explicit CColliderManager();
	~CColliderManager() override;

public:
	void UpdateGUI();
	void Update();

	void FrameStart();
	void FrameEnd();


public:
	void AddColliderGroup(const StringID& groupTag, const CCollider*);
	const std::vector<const CCollider*>* GetColliderGroup(const StringID& groupTag) const;
	const std::unordered_map<StringID, std::vector<const CCollider*>>* GetColliders() const { return &m_Colliders; }
	const CCollider* GetColliderGroupFirst(const StringID& groupTag) const;
	_bool IntersectColl(const CCollider* pColl1, const CCollider* pColl2);

private:
	void ClearColliderGroup();

private:
	HRESULT Initialize();

private:
	std::unordered_map<StringID, std::vector<const CCollider*>> m_Colliders{};
	std::unordered_map<const CCollider*, _float4> m_DbgColor{};

	_bool m_bRender{ true };
	std::unordered_map<StringID, _bool> m_DbgRenders{};


public:
	static UPtr<CColliderManager> Create();
};

NS_END

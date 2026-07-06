#pragma once
#include "Prototype.h"
#include "MyTreeNode.h"
#include "Component.h"
#include "ComTransform.h"
#include "Handle.h"
#include "IRenderable.h"

NS_BEGIN(Engine)

class ENGINE_DLL CGameObject : public CPrototype,
								public IRenderable,
								public CMyTreeNode<CGameObject>
{
public:
	DECLARE_DERIVED_TYPE(CGameObject, CPrototype)
	// ENGINE_DLL 인애들은 반드시 명시적으로 복사 생성자, 복사 대입연산자 딜리트하거나 재정의해주어야함
	CGameObject& operator=(const CGameObject&) = delete;

public:
	typedef struct tagGameObjectDesc
	{
		CHandle __handle{};
		_string sObjectTag{};
	}GAMEOBJECT_DESC;

protected:
	explicit CGameObject();
	explicit CGameObject(const CGameObject& Prototype);
	~CGameObject();

public:
	virtual HRESULT Initialize(void* pArg);
	virtual void PriorityUpdate(_float fTimeDelta);
	virtual void Update(_float fTimeDelta);
	virtual void LateUpdate(_float fTimeDelta);
	virtual void UpdateGUI();

public:
	HRESULT Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx) override;
	bool HasRenderPass(RENDERPASS ePass) const override { return (m_RenderPassFlags & static_cast<uint32_t>(ePass)) != 0; };

protected:
	uint32_t m_RenderPassFlags = ETOUI(RENDERPASS::DEFAULT);

public:
	CComTransform& GetTransform() { return *m_pComTransform; }
	const CComTransform& GetTransform() const { return *m_pComTransform; }
protected:
	std::vector<std::pair<StringID, UPtr<CComponent>>> m_Components{};
	std::unordered_map<StringID, size_t> m_ComponentsLookup{};
	CComTransform* m_pComTransform{};

private:
	UPtr<CPrototype> CloneComponentProtoType(const StringID& svGroupTag, const StringID& svPrototypetag, void* pArg) const;


public:
	template<typename T>
	HRESULT AddComponentFromProto(const StringID& svGroupTag, const StringID& svPrototypetag, const StringID& svComponentTag,  void* pArg = nullptr, T** outPtr = nullptr)
	{
		auto pCom = CloneComponentProtoType(svGroupTag, svPrototypetag, pArg);
		if (!pCom)
		{
			return E_FAIL;
		}

		T* pCache = AddComponent(svComponentTag, static_uptr_cast<T>(std::move(pCom)));

		if (outPtr)
		{
			*outPtr = pCache;
		}

		return S_OK;
	}



	template<typename T>
	T* AddComponent(const StringID& tagComponent, UPtr<T> pComponent)
	{
		auto iter = m_ComponentsLookup.find(tagComponent);
		if (iter != m_ComponentsLookup.end())
		{
			return nullptr;
		}

		//pComponent->SetGameObject(this);

		T* pCache = pComponent.get();

		auto size = m_Components.size();
		std::pair<StringID, UPtr<CComponent>> a{ tagComponent, std::move(pComponent) };
		m_Components.push_back(std::move(a));
		m_ComponentsLookup.emplace(tagComponent, size);


		return pCache;
	}

	template<typename T>
	T* GetComponent(const StringID& tagComponent)
	{
		auto iter = m_ComponentsLookup.find(tagComponent);
		if (iter == m_ComponentsLookup.end())
		{
			return nullptr;
		}

		if (T::StaticType == m_Components[iter->second].second->GetType())
		{
			return static_cast<T*>(m_Components[iter->second].second.get());
		}

		return nullptr;
	};


protected:
	HRESULT DelComponent(const StringID& tagComponent);

public:
	_string_view GetObjectTag() const { return m_sObjectTag; }
	void SetObjectTag(_string_view sObjectTag) { m_sObjectTag = sObjectTag; }
protected:
	_string m_sObjectTag{};


private:
	CHandle m_ObjectHandle{};
public:
	const CHandle& GetHandle() const { return m_ObjectHandle; }

protected:
	void Free() override;

public:
	void SetPendingDestroy(_bool b = true);
	void SetPendingDestroyCascade(_bool b = true);
	_bool GetPendingDestroy() const { return m_bPendingDestroy; }
private:
	_bool m_bPendingDestroy{ false };
};

NS_END

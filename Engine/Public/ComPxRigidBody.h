#pragma once
#include "Component.h"
NS_BEGIN(physx)
class PxRigidActor;
NS_END


NS_BEGIN(Engine)

class ENGINE_DLL CComPxRigidBody : public CComponent
{
public:
	enum class TYPE { STATIC, DYNAMIC, KINEMATIC };
	struct DESC : public CComponent::DESC
	{
		TYPE eType = TYPE::DYNAMIC;
		float   fMass = 1.0f;
		XMFLOAT3 vPosition = { 0.f, 0.f, 0.f };
		XMFLOAT4 vRotation = { 0.f, 0.f, 0.f, 1.f }; // Quaternion
	};
public:
	DECLARE_DERIVED_TYPE(CComPxRigidBody, CComponent)

public:
	void UpdateGUI() override;

public:
	physx::PxRigidActor* GetActor() const { return m_pActor; }
	bool IsDynamic() const { return m_bIsDynamic; }
	float GetMass() const { return m_fMass; };
private:
	explicit CComPxRigidBody();
	~CComPxRigidBody() override;

private:
	HRESULT Initialize(void* pArg) override;

private:
	physx::PxRigidActor* m_pActor{};
	bool          m_bIsDynamic = true;
	float   m_fMass{};
	TYPE m_eType{};


public:
	static UPtr<CComPxRigidBody> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

private:
	void Free() override;
};

NS_END

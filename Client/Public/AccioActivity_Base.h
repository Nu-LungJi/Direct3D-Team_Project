#pragma once

#include "AccioActivityPartBase.h"

NS_BEGIN(Engine)
class CComPxBoxCollider;
class CComPxRigidBody;
NS_END

NS_BEGIN(Client)

class CAccioActivity_Base final : public CAccioActivityPartBase
{
public:
	DECLARE_DERIVED_TYPE(CAccioActivity_Base, CAccioActivityPartBase)

	struct DESC : public CAccioActivityPartBase::DESC
	{
		std::array<ACCIO_ACTIVITY_BOX_COLLIDER_DESC, 4> BoxColliders{
			ACCIO_ACTIVITY_BOX_COLLIDER_DESC{
				.vHalfExtents = { 11.75f, 1.f, 0.45f },
				.vLocalOffset = { 0.f, 3.1f, 29.15f } },
			ACCIO_ACTIVITY_BOX_COLLIDER_DESC{
				.vHalfExtents = { 0.3f, 1.f, 1.65f },
				.vLocalOffset = { -11.45f, 3.1f, 27.05f } },
			ACCIO_ACTIVITY_BOX_COLLIDER_DESC{
				.vHalfExtents = { 0.3f, 1.f, 1.65f },
				.vLocalOffset = { 11.4f, 3.1f, 27.05f } },
			ACCIO_ACTIVITY_BOX_COLLIDER_DESC{
				.vHalfExtents = { 11.2f, 0.9f, 22.6f },
				.vLocalOffset = { 0.f, 1.45f, 6.9f } }
		};
		PX_FILTER_DESC tPhysicsFilter{
			.iLayer = ETOUI(COLLISION_LAYER::WORLD_STATIC),
			.iSimulationMask = PX_ALL_LAYERS,
			.iQueryMask = PX_ALL_LAYERS
		};
	};

private:
	CAccioActivity_Base();
	CAccioActivity_Base(const CAccioActivity_Base& prototype);
	~CAccioActivity_Base() override = default;

public:
	static UPtr<CAccioActivity_Base> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

protected:
	StringID GetModelResourceTag() const override;

private:
	HRESULT InitializeBasePhysics(const DESC& desc);

private:
	CComPxRigidBody* m_pComPxRigidBody{};
	std::array<CComPxBoxCollider*, 4> m_pComPxBoxColliders{};
};

NS_END

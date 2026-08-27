#pragma once

#include "AccioActivityPartBase.h"

NS_BEGIN(Engine)
class CComPxBoxCollider;
class CComPxConvexCollider;
class CComPxRigidBody;
NS_END

NS_BEGIN(Client)

class CAccioActivity_Platform final : public CAccioActivityPartBase
{
public:
	DECLARE_DERIVED_TYPE(CAccioActivity_Platform, CAccioActivityPartBase)
	static constexpr uint32_t NPC_MOVE_AREA_SHAPE_SUB_INDEX = 100u;

	struct WEDGE_COLLIDER_DESC
	{
		_float3 vScale{ 1.f, 0.25f, 1.f };
		_float3 vLocalOffset{};
		_float3 vLocalRotation{};
	};

	struct DESC : public CAccioActivityPartBase::DESC
	{
		ACCIO_ACTIVITY_BOX_COLLIDER_DESC BoxCollider{
			.vHalfExtents = { 13.35f, 1.25f, 4.9f },
			.vLocalOffset = { 0.f, 1.5f, -27.9f }
		};
		WEDGE_COLLIDER_DESC WedgeCollider{
			.vScale = { 26.75f, 3.55f, 4.8f },
			.vLocalOffset = { 0.f, 0.95f, -35.25f }
		};
		ACCIO_ACTIVITY_BOX_COLLIDER_DESC NpcMoveAreaTrigger{
			.vHalfExtents = { 12.5f, 2.f, 4.25f },
			.vLocalOffset = { 0.f, 4.25f, -27.9f }
		};
		PX_FILTER_DESC tPhysicsFilter{
			.iLayer = ETOUI(COLLISION_LAYER::WORLD_STATIC),
			.iSimulationMask = PX_ALL_LAYERS,
			.iQueryMask = PX_ALL_LAYERS
		};
		PX_FILTER_DESC tNpcMoveAreaTriggerFilter{
			.iLayer = ETOUI(COLLISION_LAYER::TRIGGER),
			.iSimulationMask = ETOUI(COLLISION_LAYER::NPC_BODY),
			.iQueryMask = PX_ALL_LAYERS
		};
	};

private:
	CAccioActivity_Platform();
	CAccioActivity_Platform(const CAccioActivity_Platform& prototype);
	~CAccioActivity_Platform() override = default;

public:
	static UPtr<CAccioActivity_Platform> Create();
	UPtr<CPrototype> Clone(void* pArg) override;
	_bool GetNpcMoveAreaWorldOBB(BoundingOrientedBox& outArea) const;

protected:
	StringID GetModelResourceTag() const override;
	HRESULT InitializePlatformPhysics(const DESC& desc);

private:
	CComPxRigidBody* m_pComPxRigidBody{};
	CComPxBoxCollider* m_pComPxBoxCollider{};
	CComPxConvexCollider* m_pComPxWedgeCollider{};
	CComPxBoxCollider* m_pComPxNpcMoveAreaTrigger{};
};

NS_END

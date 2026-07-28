#pragma once
#include "ISerializable.h"
#include "SerializerInterface.h"

NS_BEGIN(Engine)

inline constexpr uint32_t PX_RAGDOLL_DATA_VERSION = 1u;

enum class PX_RAGDOLL_SHAPE_TYPE : uint8_t
{
	BOX,
	SPHERE,
	CAPSULE
};

enum class PX_RAGDOLL_D6_MOTION : uint8_t
{
	LOCKED,
	LIMITED,
	FREE
};

struct PX_RAGDOLL_SHAPE_DESC final : public ISerializable
{
	std::string sName{};
	PX_RAGDOLL_SHAPE_TYPE eType{ PX_RAGDOLL_SHAPE_TYPE::CAPSULE };

	// Ragdoll Actor 기준 Shape Local Pose.
	_float3 vLocalPosition{};
	_float4 vLocalRotation{ 0.f, 0.f, 0.f, 1.f };

	// Box는 Half Extents, Sphere/Capsule은 Radius와 Half Height를 사용한다.
	_float3 vHalfExtents{ 0.25f, 0.25f, 0.25f };
	_float fRadius{ 0.25f };
	_float fHalfHeight{ 0.25f };

	uint32_t iLayer{ PX_DEFAULT_LAYER };
	uint32_t iSimulationMask{ PX_ALL_LAYERS };
	uint32_t iQueryMask{ PX_ALL_LAYERS };
	_bool bSimulationEnabled{ true };
	_bool bQueryEnabled{ true };

	void Serialize(ISerializer& Serializer) const override
	{
		WRITE_ALL(
			Serializer,
			sName,
			eType,
			vLocalPosition,
			vLocalRotation,
			vHalfExtents,
			fRadius,
			fHalfHeight,
			iLayer,
			iSimulationMask,
			iQueryMask,
			bSimulationEnabled,
			bQueryEnabled);
	}

	void Deserialize(IDeserializer& Deserializer) override
	{
		READ_ALL(
			Deserializer,
			sName,
			eType,
			vLocalPosition,
			vLocalRotation,
			vHalfExtents,
			fRadius,
			fHalfHeight,
			iLayer,
			iSimulationMask,
			iQueryMask,
			bSimulationEnabled,
			bQueryEnabled);
	}
};

struct PX_RAGDOLL_BODY_DESC final : public ISerializable
{
	std::string sBodyName{};
	std::string sBoneName{};

	// Animation Bone Pose에서 PhysX Actor Pose로 이동하는 Local Offset.
	_float3 vBoneToActorPosition{};
	_float4 vBoneToActorRotation{ 0.f, 0.f, 0.f, 1.f };

	_float fMass{ 1.f };
	_float fLinearDamping{ 0.1f };
	_float fAngularDamping{ 0.5f };
	_float fMaxDepenetrationVelocity{ 10.f };
	_bool bGravityEnabled{ true };
	std::vector<PX_RAGDOLL_SHAPE_DESC> Shapes{};

	void Serialize(ISerializer& Serializer) const override
	{
		WRITE_ALL(
			Serializer,
			sBodyName,
			sBoneName,
			vBoneToActorPosition,
			vBoneToActorRotation,
			fMass,
			fLinearDamping,
			fAngularDamping,
			fMaxDepenetrationVelocity,
			bGravityEnabled,
			Shapes);
	}

	void Deserialize(IDeserializer& Deserializer) override
	{
		READ_ALL(
			Deserializer,
			sBodyName,
			sBoneName,
			vBoneToActorPosition,
			vBoneToActorRotation,
			fMass,
			fLinearDamping,
			fAngularDamping,
			fMaxDepenetrationVelocity,
			bGravityEnabled,
			Shapes);
	}
};

struct PX_RAGDOLL_D6_JOINT_DESC final : public ISerializable
{
	std::string sJointName{};
	std::string sParentBodyName{};
	std::string sChildBodyName{};

	// 각 Actor Local 공간의 동일한 관절 기준점과 기준축.
	_float3 vParentLocalPosition{};
	_float4 vParentLocalRotation{ 0.f, 0.f, 0.f, 1.f };
	_float3 vChildLocalPosition{};
	_float4 vChildLocalRotation{ 0.f, 0.f, 0.f, 1.f };

	PX_RAGDOLL_D6_MOTION eTwistMotion{ PX_RAGDOLL_D6_MOTION::LIMITED };
	PX_RAGDOLL_D6_MOTION eSwingYMotion{ PX_RAGDOLL_D6_MOTION::LIMITED };
	PX_RAGDOLL_D6_MOTION eSwingZMotion{ PX_RAGDOLL_D6_MOTION::LIMITED };

	_float fTwistLowerDegrees{ -45.f };
	_float fTwistUpperDegrees{ 45.f };
	_float fSwingYDegrees{ 45.f };
	_float fSwingZDegrees{ 45.f };
	_float fLimitStiffness{};
	_float fLimitDamping{};
	_float fLimitRestitution{};
	_float fLimitBounceThreshold{};

	_float fBreakForce{ std::numeric_limits<_float>::max() };
	_float fBreakTorque{ std::numeric_limits<_float>::max() };
	_float fInvMassScaleParent{ 1.f };
	_float fInvMassScaleChild{ 1.f };
	_float fInvInertiaScaleParent{ 1.f };
	_float fInvInertiaScaleChild{ 1.f };
	_bool bCollisionEnabled{};
	_bool bVisualizationEnabled{ true };
	_bool bEnabled{ true };

	void Serialize(ISerializer& Serializer) const override
	{
		WRITE_ALL(
			Serializer,
			sJointName,
			sParentBodyName,
			sChildBodyName,
			vParentLocalPosition,
			vParentLocalRotation,
			vChildLocalPosition,
			vChildLocalRotation,
			eTwistMotion,
			eSwingYMotion,
			eSwingZMotion,
			fTwistLowerDegrees,
			fTwistUpperDegrees,
			fSwingYDegrees,
			fSwingZDegrees,
			fLimitStiffness,
			fLimitDamping,
			fLimitRestitution,
			fLimitBounceThreshold,
			fBreakForce,
			fBreakTorque,
			fInvMassScaleParent,
			fInvMassScaleChild,
			fInvInertiaScaleParent,
			fInvInertiaScaleChild,
			bCollisionEnabled,
			bVisualizationEnabled,
			bEnabled);
	}

	void Deserialize(IDeserializer& Deserializer) override
	{
		READ_ALL(
			Deserializer,
			sJointName,
			sParentBodyName,
			sChildBodyName,
			vParentLocalPosition,
			vParentLocalRotation,
			vChildLocalPosition,
			vChildLocalRotation,
			eTwistMotion,
			eSwingYMotion,
			eSwingZMotion,
			fTwistLowerDegrees,
			fTwistUpperDegrees,
			fSwingYDegrees,
			fSwingZDegrees,
			fLimitStiffness,
			fLimitDamping,
			fLimitRestitution,
			fLimitBounceThreshold,
			fBreakForce,
			fBreakTorque,
			fInvMassScaleParent,
			fInvMassScaleChild,
			fInvInertiaScaleParent,
			fInvInertiaScaleChild,
			bCollisionEnabled,
			bVisualizationEnabled,
			bEnabled);
	}
};

struct PX_RAGDOLL_DESC final : public ISerializable
{
	uint32_t iVersion{ PX_RAGDOLL_DATA_VERSION };
	std::string sSkeletonTag{};
	std::vector<PX_RAGDOLL_BODY_DESC> Bodies{};
	std::vector<PX_RAGDOLL_D6_JOINT_DESC> Joints{};

	void Serialize(ISerializer& Serializer) const override
	{
		WRITE_ALL(Serializer, iVersion, sSkeletonTag, Bodies, Joints);
	}

	void Deserialize(IDeserializer& Deserializer) override
	{
		READ_ALL(Deserializer, iVersion, sSkeletonTag, Bodies, Joints);
	}
};

NS_END

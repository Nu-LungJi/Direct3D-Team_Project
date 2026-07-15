#pragma once
namespace Engine
{
	enum class ES_EngineProtoMajorType
	{
		PERMANENT,
		CAMERAS,
		PHYSX,
		UI,
	};

	enum class ES_EngineProtoComponent
	{
		Prototype_Component_Transform,
		Prototype_Component_ConstantBuffer,
		Prototype_Component_ModelInstance,
		Prototype_Component_StaticModelInstance,
		Prototype_Component_Animator
	};

	enum class ES_EngineProtoPhysXComponent
	{
		Prototype_Component_ComPxBoxCollider,
		Prototype_Component_ComPxCapsuleCollider,
		Prototype_Component_ComPxSphereCollider,
		Prototype_Component_ComPxTriMeshCollider,
		Prototype_Component_ComPxRigidBody
	};

	enum class ES_EngineProtoGameObject
	{
		Prototype_GameObject_FlyCamera,
		Prototype_GameObject_ShadowCamera,
		Prototype_GameObject_UICamera
	};



}

#include "pch.h"
#include "PhysXManager.h"
#include "PhysxManagerListener.h"
#include "GameInstance.h"
#include "PhysXCollisionProxyEditor.h"
#include "PhysXCookingEditor.h"
#include "RagdollEditorGUI.h"
#include "PhysXCollisionProxyObject.h"

#include <filesystem>

#pragma push_macro("new")
#undef new
#include "PxShape.h"
#include "PxPhysicsAPI.h"
#pragma pop_macro("new")


using namespace physx;
static physx::PxDefaultAllocator gDefaultAllocator;
static physx::PxDefaultErrorCallback gDefaultErrorCallback;

NS_USING(Engine)

namespace
{
	class CPxSceneQueryFilter final : public PxQueryFilterCallback
	{
	public:
		CPxSceneQueryFilter(const CPhysXManager& manager, const PX_QUERY_FILTER_DESC& tDesc, PxQueryHitType::Enum eHitType)
			: m_Manager{ manager }, m_tDesc{ tDesc }, m_eHitType{ eHitType }
		{
		}

		PxQueryHitType::Enum preFilter(
			const PxFilterData& tQueryData,
			const PxShape* pShape,
			const PxRigidActor* pActor,
			PxHitFlags&) override
		{
			if (!pShape || !pActor)
				return PxQueryHitType::eNONE;

			if (!m_tDesc.bIncludeTrigger && pShape->getFlags().isSet(PxShapeFlag::eTRIGGER_SHAPE))
				return PxQueryHitType::eNONE;

			if ((tQueryData.word0 & pShape->getQueryFilterData().word0) == 0)
				return PxQueryHitType::eNONE;

			if (!(m_tDesc.hIgnoreGameObject == CHandle{}))
			{
				if (const auto tShapeData = m_Manager.FindShapeUserData(pShape);
					tShapeData && tShapeData->hGameObject == m_tDesc.hIgnoreGameObject)
					return PxQueryHitType::eNONE;

				if (const auto tActorData = m_Manager.FindActorUserData(pActor);
					tActorData && tActorData->hGameObject == m_tDesc.hIgnoreGameObject)
					return PxQueryHitType::eNONE;
			}

			return m_eHitType;
		}

		PxQueryHitType::Enum postFilter(
			const PxFilterData&, const PxQueryHit&, const PxShape*, const PxRigidActor*) override
		{
			return m_eHitType;
		}

	private:
		const CPhysXManager& m_Manager;
		const PX_QUERY_FILTER_DESC& m_tDesc;
		PxQueryHitType::Enum m_eHitType{};
	};

	class CPxCCTInteractionFilter final : public PxControllerFilterCallback
	{
	public:
		bool filter(const PxController& controllerA, const PxController& controllerB) override
		{
			const PxRigidDynamic* pActorA = controllerA.getActor();
			const PxRigidDynamic* pActorB = controllerB.getActor();
			if (!pActorA || !pActorB)
				return false;

			PxShape* pShapeA{};
			PxShape* pShapeB{};
			if (pActorA->getShapes(&pShapeA, 1) != 1 || !pShapeA ||
				pActorB->getShapes(&pShapeB, 1) != 1 || !pShapeB)
			{
				return false;
			}

			const PxFilterData tFilterA = pShapeA->getQueryFilterData();
			const PxFilterData tFilterB = pShapeB->getQueryFilterData();
			return (tFilterA.word1 & tFilterB.word0) != 0 &&
				(tFilterB.word1 & tFilterA.word0) != 0;
		}
	};

	_bool BuildQueryFilterData(const PX_QUERY_FILTER_DESC& tDesc, _bool bMultiple, PxQueryFilterData& outFilterData)
	{
		outFilterData.data.word0 = tDesc.iQueryMask;
		outFilterData.flags = PxQueryFlag::ePREFILTER;

		if (tDesc.bQueryStatic)
			outFilterData.flags |= PxQueryFlag::eSTATIC;
		if (tDesc.bQueryDynamic)
			outFilterData.flags |= PxQueryFlag::eDYNAMIC;
		if (bMultiple)
			outFilterData.flags |= PxQueryFlag::eNO_BLOCK;

		return tDesc.bQueryStatic || tDesc.bQueryDynamic;
	}

	_bool BuildGeometry(const PX_QUERY_GEOMETRY_DESC& tDesc, PxGeometryHolder& outGeometry)
	{
		switch (tDesc.eType)
		{
		case PX_QUERY_GEOMETRY_TYPE::BOX:
			if (tDesc.vBoxHalfExtents.x <= 0.f || tDesc.vBoxHalfExtents.y <= 0.f || tDesc.vBoxHalfExtents.z <= 0.f)
				return false;
			outGeometry.storeAny(PxBoxGeometry{ tDesc.vBoxHalfExtents.x, tDesc.vBoxHalfExtents.y, tDesc.vBoxHalfExtents.z });
			return true;

		case PX_QUERY_GEOMETRY_TYPE::SPHERE:
			if (tDesc.fRadius <= 0.f)
				return false;
			outGeometry.storeAny(PxSphereGeometry{ tDesc.fRadius });
			return true;

		case PX_QUERY_GEOMETRY_TYPE::CAPSULE:
			if (tDesc.fRadius <= 0.f || tDesc.fCapsuleHalfHeight < 0.f)
				return false;
			outGeometry.storeAny(PxCapsuleGeometry{ tDesc.fRadius, tDesc.fCapsuleHalfHeight });
			return true;
		}

		return false;
	}

	_bool BuildPose(const PX_QUERY_POSE& tPose, PxTransform& outPose)
	{
		PxQuat tRotation{ tPose.vRotation.x, tPose.vRotation.y, tPose.vRotation.z, tPose.vRotation.w };
		if (!tRotation.isFinite() || tRotation.magnitudeSquared() <= 0.f)
			return false;

		tRotation.normalize();
		outPose = PxTransform{ PxVec3{ tPose.vPosition.x, tPose.vPosition.y, tPose.vPosition.z }, tRotation };
		return outPose.isValid();
	}

	_bool BuildDirection(const _float3& vDirection, PxVec3& outDirection)
	{
		outDirection = PxVec3{ vDirection.x, vDirection.y, vDirection.z };
		const PxReal fLengthSquared = outDirection.magnitudeSquared();
		if (!outDirection.isFinite() || fLengthSquared <= 0.f)
			return false;

		outDirection *= PxRecipSqrt(fLengthSquared);
		return true;
	}

	template<typename TResult>
	void FillQueryObjectResult(
		const CPhysXManager& manager,
		const PxRigidActor* pActor,
		const PxShape* pShape,
		TResult& outResult)
	{
		if (pShape)
		{
			if (const auto tShapeData = manager.FindShapeUserData(pShape))
			{
				outResult.hGameObject = tShapeData->hGameObject;
				outResult.eShapeType = tShapeData->eType;
				outResult.iShapeSubIndex = tShapeData->iSubIndex;
			}
		}

		if (outResult.hGameObject == CHandle{} && pActor)
		{
			if (const auto tActorData = manager.FindActorUserData(pActor))
				outResult.hGameObject = tActorData->hGameObject;
		}

		if (!(outResult.hGameObject == CHandle{}))
		{
			outResult.pGameObject = CGameInstance::Get().
				GetGameObjectByHandle(outResult.hGameObject);
		}
	}

	template<typename THit>
	PX_RAYCAST_RESULT MakeLocationHitResult(const CPhysXManager& manager, const THit& tHit)
	{
		PX_RAYCAST_RESULT tResult{};
		tResult.bHit = true;
		tResult.vHitpos = { tHit.position.x, tHit.position.y, tHit.position.z };
		tResult.vHitNormal = { tHit.normal.x, tHit.normal.y, tHit.normal.z };
		tResult.fDistance = tHit.distance;
		FillQueryObjectResult(manager, tHit.actor, tHit.shape, tResult);
		return tResult;
	}

	PX_OVERLAP_RESULT MakeOverlapResult(const CPhysXManager& manager, const PxOverlapHit& tHit)
	{
		PX_OVERLAP_RESULT tResult{};
		tResult.bHit = true;
		FillQueryObjectResult(manager, tHit.actor, tHit.shape, tResult);
		return tResult;
	}
}

CPhysXManager::CPhysXManager()
{
}

CPhysXManager::~CPhysXManager()
{
}

_bool CPhysXManager::RegisterActor(const physx::PxActor* pActor, const PX_ACTOR_USER_DATA& userData)
{
	if (!pActor)
		return false;

	std::unique_lock lock{ m_UserDataRegistryMutex };
	m_ActorUserDataRegistry.insert_or_assign(pActor, userData);
	return true;
}

void CPhysXManager::UnregisterActor(const physx::PxActor* pActor)
{
	if (!pActor)
		return;

	std::unique_lock lock{ m_UserDataRegistryMutex };
	m_ActorUserDataRegistry.erase(pActor);
}

std::optional<PX_ACTOR_USER_DATA> CPhysXManager::FindActorUserData(const physx::PxActor* pActor) const
{
	if (!pActor)
		return std::nullopt;

	std::shared_lock lock{ m_UserDataRegistryMutex };
	const auto iter = m_ActorUserDataRegistry.find(pActor);
	if (iter == m_ActorUserDataRegistry.end())
		return std::nullopt;

	return iter->second;
}

CGameObject* CPhysXManager::FindGameObject(const physx::PxActor* pActor) const
{
	const auto userData = FindActorUserData(pActor);
	if (!userData)
		return nullptr;

	return CGameInstance::Get().GetGameObjectByHandle(userData->hGameObject);
}

_bool CPhysXManager::RegisterShape(const physx::PxShape* pShape, const PX_SHAPE_USER_DATA& userData)
{
	if (!pShape)
		return false;

	std::unique_lock lock{ m_UserDataRegistryMutex };
	m_ShapeUserDataRegistry.insert_or_assign(pShape, userData);
	return true;
}

void CPhysXManager::UnregisterShape(const physx::PxShape* pShape)
{
	if (!pShape)
		return;

	std::unique_lock lock{ m_UserDataRegistryMutex };
	m_ShapeUserDataRegistry.erase(pShape);
}

std::optional<PX_SHAPE_USER_DATA> CPhysXManager::FindShapeUserData(const physx::PxShape* pShape) const
{
	if (!pShape)
		return std::nullopt;

	std::shared_lock lock{ m_UserDataRegistryMutex };
	const auto iter = m_ShapeUserDataRegistry.find(pShape);
	if (iter == m_ShapeUserDataRegistry.end())
		return std::nullopt;

	return iter->second;
}

void CPhysXManager::QueueCCTShapeHit(
	const CHandle& hOwner, const CHandle& hOther, const PX_CCT_HIT_DATA& tHit)
{
	if (m_pListener)
		m_pListener->QueueCCTShapeHit(hOwner, hOther, tHit);
}

void CPhysXManager::QueueCCTControllerHit(
	const CHandle& hOwner, const CHandle& hOther, const PX_CCT_HIT_DATA& tHit)
{
	if (m_pListener)
		m_pListener->QueueCCTControllerHit(hOwner, hOther, tHit);
}

void CPhysXManager::QueueCCTObstacleHit(
	const CHandle& hOwner, const PX_CCT_OBSTACLE_HIT_DATA& tHit)
{
	if (m_pListener)
		m_pListener->QueueCCTObstacleHit(hOwner, tHit);
}

void CPhysXManager::UpdateGUI()
{
    ImGui::Begin("CPhysXManager");
	ImGui::Text("Simulation: %s", m_bGpuSimulationEnabled ? "GPU" : "CPU");
	ImGui::Checkbox("CCT Interactions", &m_bCCTInteractionsEnabled);
    ImGui::Text("m_bDbgRender: %i", m_bDbgRender);
    if (ImGui::Button("DebugRender"))
    {
        m_bDbgRender = !m_bDbgRender;
    }
	if (ImGui::Button("Open Cooking Editor"))
	{
		if (m_pCookingEditor)
			m_pCookingEditor->Open();
	}
	if (ImGui::Button("Open Ragdoll Editor"))
	{
		if (m_pRagdollEditor)
			m_pRagdollEditor->Open();
	}
	if (ImGui::CollapsingHeader("Collision Layers"))
	{
		ImGui::Text("Registered: %zu", m_CollisionLayerNames.size());
		if (ImGui::BeginTable(
			"CollisionLayerTable",
			3,
			ImGuiTableFlags_Borders |
			ImGuiTableFlags_RowBg |
			ImGuiTableFlags_SizingStretchProp))
		{
			ImGui::TableSetupColumn("Bit", ImGuiTableColumnFlags_WidthFixed, 42.f);
			ImGui::TableSetupColumn("Name");
			ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 96.f);
			ImGui::TableHeadersRow();

			for (const auto& [iValue, sName] : m_CollisionLayerNames)
			{
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				if (iValue == 0)
					ImGui::TextUnformatted("-");
				else
				{
					uint32_t iBit{};
					for (uint32_t i = iValue; i > 1; i >>= 1)
						++iBit;
					ImGui::Text("%u", iBit);
				}

				ImGui::TableSetColumnIndex(1);
				ImGui::TextUnformatted(sName.c_str());
				ImGui::TableSetColumnIndex(2);
				ImGui::Text("0x%08X", iValue);
			}
			ImGui::EndTable();
		}
	}
    ImGui::End();

	if (m_pCollisionProxyEditor)
		m_pCollisionProxyEditor->UpdateGUI(0.f);
	if (m_pCookingEditor)
		m_pCookingEditor->UpdateGUI();
	if (m_pRagdollEditor)
		m_pRagdollEditor->UpdateGUI();
}

void CPhysXManager::SetCollisionLayerNames(std::vector<std::pair<uint32_t, std::string>> layerNames)
{
	layerNames.erase(
		std::remove_if(
			layerNames.begin(),
			layerNames.end(),
			[](const auto& entry)
			{
				const uint32_t iValue = entry.first;
				return entry.second.empty() ||
					(iValue != 0 && (iValue & (iValue - 1)) != 0);
			}),
		layerNames.end());
	std::sort(
		layerNames.begin(),
		layerNames.end(),
		[](const auto& lhs, const auto& rhs)
		{
			return lhs.first < rhs.first;
		});
	layerNames.erase(
		std::unique(
			layerNames.begin(),
			layerNames.end(),
			[](const auto& lhs, const auto& rhs)
			{
				return lhs.first == rhs.first;
			}),
		layerNames.end());

	m_CollisionLayerNames = std::move(layerNames);

	if (m_pRagdollEditor)
		m_pRagdollEditor->SetCollisionLayerNames(m_CollisionLayerNames);
	if (m_pCollisionProxyEditor)
		m_pCollisionProxyEditor->SetCollisionLayerNames(m_CollisionLayerNames);
}

_bool CPhysXManager::EditCollisionLayerGUI(
	const char* pLabel,
	uint32_t& iLayer) const
{
	if (!pLabel)
		return false;

	if (m_CollisionLayerNames.empty())
	{
		return ImGui::InputScalar(
			pLabel,
			ImGuiDataType_U32,
			&iLayer,
			nullptr,
			nullptr,
			"%08X",
			ImGuiInputTextFlags_CharsHexadecimal);
	}

	std::string sPreview = "Unregistered";
	for (const auto& [iValue, sName] : m_CollisionLayerNames)
	{
		if (iValue == iLayer)
		{
			sPreview = sName;
			break;
		}
	}

	_bool bChanged{};
	if (ImGui::BeginCombo(pLabel, sPreview.c_str()))
	{
		for (const auto& [iValue, sName] : m_CollisionLayerNames)
		{
			const _bool bSelected = iLayer == iValue;
			if (ImGui::Selectable(sName.c_str(), bSelected))
			{
				iLayer = iValue;
				bChanged = true;
			}
			if (bSelected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	ImGui::SameLine();
	ImGui::TextDisabled("0x%08X", iLayer);
	return bChanged;
}

_bool CPhysXManager::EditCollisionLayerMaskGUI(
	const char* pLabel,
	uint32_t& iMask) const
{
	if (!pLabel)
		return false;

	if (m_CollisionLayerNames.empty())
	{
		return ImGui::InputScalar(
			pLabel,
			ImGuiDataType_U32,
			&iMask,
			nullptr,
			nullptr,
			"%08X",
			ImGuiInputTextFlags_CharsHexadecimal);
	}

	uint32_t iRegisteredMask{};
	uint32_t iSelectedCount{};
	for (const auto& [iValue, sName] : m_CollisionLayerNames)
	{
		if (iValue == 0)
			continue;
		iRegisteredMask |= iValue;
		if ((iMask & iValue) != 0)
			++iSelectedCount;
	}

	std::string sPreview{};
	if (iMask == 0)
		sPreview = "None";
	else if (iMask == PX_ALL_LAYERS)
		sPreview = "All";
	else
	{
		sPreview = std::to_string(iSelectedCount) + " selected";
		if ((iMask & ~iRegisteredMask) != 0)
			sPreview += " + custom";
	}

	_bool bChanged{};
	if (ImGui::BeginCombo(pLabel, sPreview.c_str()))
	{
		if (ImGui::Button("None"))
		{
			iMask = 0;
			bChanged = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("All Registered"))
		{
			iMask = iRegisteredMask;
			bChanged = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("All Bits"))
		{
			iMask = PX_ALL_LAYERS;
			bChanged = true;
		}
		ImGui::Separator();

		for (const auto& [iValue, sName] : m_CollisionLayerNames)
		{
			if (iValue == 0)
				continue;

			_bool bSelected = (iMask & iValue) != 0;
			if (ImGui::Checkbox(sName.c_str(), &bSelected))
			{
				if (bSelected)
					iMask |= iValue;
				else
					iMask &= ~iValue;
				bChanged = true;
			}
		}
		ImGui::EndCombo();
	}
	ImGui::SameLine();
	ImGui::TextDisabled("0x%08X", iMask);
	return bChanged;
}

std::vector<CHandle> CPhysXManager::CreateCollisionProxyObjects(
	const PX_COLLISION_PROXY_FILE& data, std::string_view layerName)
{
	std::vector<CHandle> handles{};
	if (data.iVersion != 3 || layerName.empty())
		return handles;

	auto destroyCreatedObjects = [&]()
	{
		for (const CHandle& handle : handles)
		{
			if (auto* object = CGameInstance::Get().GetGameObjectByHandle(handle))
				object->SetPendingDestroyCascade();
		}
		handles.clear();
	};

	for (const auto& actor : data.actors)
	{
		if (!actor.bEnabled)
			continue;

		StringID prototypeGroup{ "PERMANENT" };
		StringID prototypeTag{ "Prototype_GameObject_PhysXCollisionProxy" };
		if (!actor.sPrototypeTag.empty())
		{
			prototypeGroup = StringID{ PX_COLLISION_PROXY_PROTOTYPE_GROUP };
			prototypeTag = StringID{ actor.sPrototypeTag };
		}

		PX_COLLISION_PROXY_FILE actorData{};
		actorData.actors.push_back(actor);
		CPhysXCollisionProxyObject::DESC desc{};
		desc.sObjectTag = actor.sName;
		desc.pCollisionData = &actorData;

		auto handle = CGameInstance::Get().AddGameObjectToLayer(
			prototypeGroup, prototypeTag, layerName, &desc);
		if (!handle)
		{
			DEBUG_LOG_STR(std::string("[PX][CollisionProxy] Failed to create actor: ") +
				actor.sName + " (type: " +
				(actor.sPrototypeTag.empty() ? "EngineDefault" : actor.sPrototypeTag) + ")\n");
			destroyCreatedObjects();
			return handles;
		}
		handles.push_back(*handle);
	}

	return handles;
}

std::vector<CHandle> CPhysXManager::CreateCollisionProxyObjectsFromFile(
	std::string collisionFileName, std::string_view layerName)
{
	PX_COLLISION_PROXY_FILE data{};
	const std::string path = MakePxCollisionFilePath(std::move(collisionFileName));
	if (!std::filesystem::exists(path) ||
		FAILED(CGameInstance::Get().JsonDeSerialize(path, data, "CollisionProxies")))
	{
		DEBUG_LOG_STR(std::string("[PX][CollisionProxy] Failed to load file: ") + path + "\n");
		return {};
	}
	return CreateCollisionProxyObjects(data, layerName);
}

//_bool CPhysXManager::RayCast(const _float3& vOrigin, const _float3& vNormalizedDir, _float fMaxDistance, PX_RAYCAST_RESULT& outResult) const
//{
//	PX_RAYCAST_DESC tDesc{};
//	tDesc.vOrigin = vOrigin;
//	tDesc.vDirection = vNormalizedDir;
//	tDesc.fMaxDistance = fMaxDistance;
//	return RayCast(tDesc, outResult);
//}
//
//_bool CPhysXManager::RayCastMultiple(const _float3& vOrigin, const _float3& vNormalizedDir, _float fMaxDistance, std::vector<PX_RAYCAST_RESULT>& outVecResult, uint32_t iMaxHit) const
//{
//	PX_RAYCAST_DESC tDesc{};
//	tDesc.vOrigin = vOrigin;
//	tDesc.vDirection = vNormalizedDir;
//	tDesc.fMaxDistance = fMaxDistance;
//	return RayCastMultiple(tDesc, outVecResult, iMaxHit);
//}

_bool CPhysXManager::RayCast(const PX_RAYCAST_DESC& tDesc, PX_RAYCAST_RESULT& outResult) const
{
	outResult = {};
	if (!m_pScene || tDesc.fMaxDistance <= 0.f)
		return false;

	PxVec3 vDirection{};
	PxQueryFilterData tFilterData{};
	if (!BuildDirection(tDesc.vDirection, vDirection) || !BuildQueryFilterData(tDesc.tFilter, false, tFilterData))
		return false;

	CPxSceneQueryFilter tFilter{ *this, tDesc.tFilter, PxQueryHitType::eBLOCK };
	PxRaycastBuffer tHitBuffer{};
	PxHitFlags tHitFlags{ PxHitFlag::eDEFAULT };
	if (tDesc.bHitMeshBothSides)
		tHitFlags |= PxHitFlag::eMESH_BOTH_SIDES;

	if (!m_pScene->raycast(
		PxVec3{ tDesc.vOrigin.x, tDesc.vOrigin.y, tDesc.vOrigin.z },
		vDirection,
		tDesc.fMaxDistance,
		tHitBuffer,
		tHitFlags,
		tFilterData,
		&tFilter) || !tHitBuffer.hasBlock)
		return false;

	outResult = MakeLocationHitResult(*this, tHitBuffer.block);
	return true;
}

_bool CPhysXManager::RayCastMultiple(
	const PX_RAYCAST_DESC& tDesc,
	std::vector<PX_RAYCAST_RESULT>& outVecResult,
	uint32_t iMaxHit,
	PX_QUERY_MULTIPLE_STATUS* pOutStatus) const
{
	outVecResult.clear();
	if (pOutStatus)
		*pOutStatus = {};
	if (!m_pScene || tDesc.fMaxDistance <= 0.f || iMaxHit == 0 ||
		iMaxHit == std::numeric_limits<uint32_t>::max())
		return false;

	PxVec3 vDirection{};
	PxQueryFilterData tFilterData{};
	if (!BuildDirection(tDesc.vDirection, vDirection) || !BuildQueryFilterData(tDesc.tFilter, true, tFilterData))
		return false;

	const uint32_t iBufferCapacity = iMaxHit + (pOutStatus ? 1u : 0u);
	std::vector<PxRaycastHit> Hits(iBufferCapacity);
	PxRaycastBuffer tHitBuffer{ Hits.data(), iBufferCapacity };
	CPxSceneQueryFilter tFilter{ *this, tDesc.tFilter, PxQueryHitType::eTOUCH };
	PxHitFlags tHitFlags{ PxHitFlag::eDEFAULT };
	if (tDesc.bHitMeshBothSides)
		tHitFlags |= PxHitFlag::eMESH_BOTH_SIDES;

	m_pScene->raycast(
		PxVec3{ tDesc.vOrigin.x, tDesc.vOrigin.y, tDesc.vOrigin.z },
		vDirection,
		tDesc.fMaxDistance,
		tHitBuffer,
		tHitFlags,
		tFilterData,
		&tFilter);

	const uint32_t iReturnedHitCount =
		std::min<uint32_t>(tHitBuffer.getNbTouches(), iMaxHit);
	outVecResult.reserve(iReturnedHitCount);
	for (uint32_t i = 0; i < iReturnedHitCount; ++i)
		outVecResult.push_back(MakeLocationHitResult(*this, tHitBuffer.getTouch(i)));
	if (pOutStatus)
	{
		pOutStatus->bTruncated = tHitBuffer.getNbTouches() > iMaxHit;
	}

	std::sort(outVecResult.begin(), outVecResult.end(), [](const PX_RAYCAST_RESULT& a, const PX_RAYCAST_RESULT& b)
	{
		return a.fDistance < b.fDistance;
	});
	return !outVecResult.empty();
}

_bool CPhysXManager::Sweep(const PX_SWEEP_DESC& tDesc, PX_SWEEP_RESULT& outResult) const
{
	outResult = {};
	if (!m_pScene || tDesc.fMaxDistance <= 0.f)
		return false;

	PxGeometryHolder tGeometry{};
	PxTransform tPose{ PxIdentity };
	PxVec3 vDirection{};
	PxQueryFilterData tFilterData{};
	if (!BuildGeometry(tDesc.tGeometry, tGeometry) || !BuildPose(tDesc.tPose, tPose) ||
		!BuildDirection(tDesc.vDirection, vDirection) || !BuildQueryFilterData(tDesc.tFilter, false, tFilterData))
		return false;

	CPxSceneQueryFilter tFilter{ *this, tDesc.tFilter, PxQueryHitType::eBLOCK };
	PxSweepBuffer tHitBuffer{};
	if (!m_pScene->sweep(
		tGeometry.any(), tPose, vDirection, tDesc.fMaxDistance, tHitBuffer,
		PxHitFlag::eDEFAULT, tFilterData, &tFilter) || !tHitBuffer.hasBlock)
		return false;

	outResult = MakeLocationHitResult(*this, tHitBuffer.block);
	return true;
}

_bool CPhysXManager::SweepMultiple(
	const PX_SWEEP_DESC& tDesc,
	std::vector<PX_SWEEP_RESULT>& outVecResult,
	uint32_t iMaxHit,
	PX_QUERY_MULTIPLE_STATUS* pOutStatus) const
{
	outVecResult.clear();
	if (pOutStatus)
		*pOutStatus = {};
	if (!m_pScene || tDesc.fMaxDistance <= 0.f || iMaxHit == 0 ||
		iMaxHit == std::numeric_limits<uint32_t>::max())
		return false;

	PxGeometryHolder tGeometry{};
	PxTransform tPose{ PxIdentity };
	PxVec3 vDirection{};
	PxQueryFilterData tFilterData{};
	if (!BuildGeometry(tDesc.tGeometry, tGeometry) || !BuildPose(tDesc.tPose, tPose) ||
		!BuildDirection(tDesc.vDirection, vDirection) || !BuildQueryFilterData(tDesc.tFilter, true, tFilterData))
		return false;

	const uint32_t iBufferCapacity = iMaxHit + (pOutStatus ? 1u : 0u);
	std::vector<PxSweepHit> Hits(iBufferCapacity);
	PxSweepBuffer tHitBuffer{ Hits.data(), iBufferCapacity };
	CPxSceneQueryFilter tFilter{ *this, tDesc.tFilter, PxQueryHitType::eTOUCH };
	m_pScene->sweep(
		tGeometry.any(), tPose, vDirection, tDesc.fMaxDistance, tHitBuffer,
		PxHitFlag::eDEFAULT, tFilterData, &tFilter);

	const uint32_t iReturnedHitCount =
		std::min<uint32_t>(tHitBuffer.getNbTouches(), iMaxHit);
	outVecResult.reserve(iReturnedHitCount);
	for (uint32_t i = 0; i < iReturnedHitCount; ++i)
		outVecResult.push_back(MakeLocationHitResult(*this, tHitBuffer.getTouch(i)));
	if (pOutStatus)
	{
		pOutStatus->bTruncated = tHitBuffer.getNbTouches() > iMaxHit;
	}

	std::sort(outVecResult.begin(), outVecResult.end(), [](const PX_SWEEP_RESULT& a, const PX_SWEEP_RESULT& b)
	{
		return a.fDistance < b.fDistance;
	});
	return !outVecResult.empty();
}

_bool CPhysXManager::Overlap(const PX_OVERLAP_DESC& tDesc, PX_OVERLAP_RESULT& outResult) const
{
	outResult = {};
	std::vector<PX_OVERLAP_RESULT> Results{};
	if (!OverlapMultiple(tDesc, Results, 1))
		return false;

	outResult = Results.front();
	return true;
}

_bool CPhysXManager::OverlapMultiple(
	const PX_OVERLAP_DESC& tDesc,
	std::vector<PX_OVERLAP_RESULT>& outVecResult,
	uint32_t iMaxHit,
	PX_QUERY_MULTIPLE_STATUS* pOutStatus) const
{
	outVecResult.clear();
	if (pOutStatus)
		*pOutStatus = {};
	if (!m_pScene || iMaxHit == 0 ||
		iMaxHit == std::numeric_limits<uint32_t>::max())
		return false;

	PxGeometryHolder tGeometry{};
	PxTransform tPose{ PxIdentity };
	PxQueryFilterData tFilterData{};
	if (!BuildGeometry(tDesc.tGeometry, tGeometry) || !BuildPose(tDesc.tPose, tPose) ||
		!BuildQueryFilterData(tDesc.tFilter, true, tFilterData))
		return false;

	const uint32_t iBufferCapacity = iMaxHit + (pOutStatus ? 1u : 0u);
	std::vector<PxOverlapHit> Hits(iBufferCapacity);
	PxOverlapBuffer tHitBuffer{ Hits.data(), iBufferCapacity };
	CPxSceneQueryFilter tFilter{ *this, tDesc.tFilter, PxQueryHitType::eTOUCH };
	m_pScene->overlap(tGeometry.any(), tPose, tHitBuffer, tFilterData, &tFilter);

	const uint32_t iReturnedHitCount =
		std::min<uint32_t>(tHitBuffer.getNbTouches(), iMaxHit);
	outVecResult.reserve(iReturnedHitCount);
	for (uint32_t i = 0; i < iReturnedHitCount; ++i)
		outVecResult.push_back(MakeOverlapResult(*this, tHitBuffer.getTouch(i)));
	if (pOutStatus)
	{
		pOutStatus->bTruncated = tHitBuffer.getNbTouches() > iMaxHit;
	}

	return !outVecResult.empty();
}



void CPhysXManager::UpdateDebugRender(_float fTimeDelta)
{
	if (m_bDbgRender)
	{
		const PxRenderBuffer& renderBuffer = m_pScene->getRenderBuffer();
		const PxU32 nbLines = renderBuffer.getNbLines();
		const PxDebugLine* lines = renderBuffer.getLines();

		static_assert(sizeof(PxVec3) == sizeof(_float3));
		static_assert(sizeof(PxDebugLine) == sizeof(VTX_DBG_LINE) * 2);
		static_assert(offsetof(PxDebugLine, pos0) == 0);
		static_assert(offsetof(PxDebugLine, color0) == 12);
		static_assert(offsetof(PxDebugLine, pos1) == 16);
		static_assert(offsetof(PxDebugLine, color1) == 28);
		if (nbLines > 0 && lines)
		{
			auto* pDbgLineRender = CGameInstance::Get().GetDbgLineRender();
			const auto ePreviousDepthMode = pDbgLineRender->GetDepthMode();

			pDbgLineRender->SetDepthTest(true);
			pDbgLineRender->AddPackedLineVertices(
				lines,
				static_cast<size_t>(nbLines) * 2);
			pDbgLineRender->SetDepthMode(ePreviousDepthMode);
		}
	}
	//if (m_bDbgRender)
	//{
	//	const PxRenderBuffer& renderBuffer = m_pScene->getRenderBuffer();
	//	const PxU32 nbLines = renderBuffer.getNbLines();
	//	const PxDebugLine* lines = renderBuffer.getLines();

	//	// TODO:순회가 아니라 memcpy방식으로 수정
	//	for (PxU32 i = 0; i < nbLines; i++) {
	//		const PxDebugLine& line = lines[i];
	//		CGameInstance::Get().GetDbgLineRender()
	//			->AddLine(
	//				_float3{ line.pos0.x, line.pos0.y, line.pos0.z },
	//				_float3{ line.pos1.x, line.pos1.y, line.pos1.z }, ColorIntToFloat4(line.color0));
	//	}
	//}
}

void CPhysXManager::Update(_float fTimeDeta)
{
    UpdateDebugRender(fTimeDeta);
}

//word0 = 자신의 Layer
//word1 = 자신이 허용하는 Simulation Mask
//word2 = 현재 미사용
//word3 = 현재 미사용
static physx::PxFilterFlags MyFilterShader(
    physx::PxFilterObjectAttributes attributes0, physx::PxFilterData filterData0,
    physx::PxFilterObjectAttributes attributes1, physx::PxFilterData filterData1,
    physx::PxPairFlags& pairFlags, const void* constantBlock, physx::PxU32 constantBlockSize)
{
	const bool bLayerMaskMatched =
		(filterData0.word0 & filterData1.word1) != 0 &&
		(filterData1.word0 & filterData0.word1) != 0;

	if (!bLayerMaskMatched)
		return physx::PxFilterFlag::eSUPPRESS;

	if (physx::PxFilterObjectIsTrigger(attributes0) ||
		physx::PxFilterObjectIsTrigger(attributes1))
	{
		pairFlags = physx::PxPairFlag::eTRIGGER_DEFAULT;
	}
	else
	{
		pairFlags =
			physx::PxPairFlag::eCONTACT_DEFAULT |
			physx::PxPairFlag::eNOTIFY_TOUCH_FOUND |
			physx::PxPairFlag::eNOTIFY_TOUCH_LOST |
			physx::PxPairFlag::eNOTIFY_CONTACT_POINTS;
	}

	return physx::PxFilterFlag::eDEFAULT;
}

HRESULT CPhysXManager::Initialize()
{
	m_pCollisionProxyEditor = CPhysXCollisionProxyEditor::Create();
	if (!m_pCollisionProxyEditor)
		return E_FAIL;
	m_pCookingEditor = CPhysXCookingEditor::Create();
	if (!m_pCookingEditor)
		return E_FAIL;
	m_pRagdollEditor = CRagdollEditorGUI::Create();
	if (!m_pRagdollEditor)
		return E_FAIL;

    m_pListener = CPhysxManagerListener::Create();
	if (!m_pListener)
	{
		MSG_BOX("Failed to create PhysX simulation event listener");
		return E_FAIL;
	}

    // Foundation 생성
    {
        m_pFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gDefaultAllocator, gDefaultErrorCallback);
        if (!m_pFoundation)
        {
            MSG_BOX("PxCreateFoundation FAILED");
            return E_FAIL;
        }
    }
    
    // 피직스 생성
    {
        bool recordMemoryAllocations = false;

        // 피직스 비주얼디버거 설정
#ifdef _DEBUG
        recordMemoryAllocations = true;
        m_pPvd = physx::PxCreatePvd(*m_pFoundation);
		if (m_pPvd)
		{
			physx::PxPvdTransport* pPvdTransport =
				physx::PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);

			if (pPvdTransport)
			{
				if (!m_pPvd->connect(*pPvdTransport, PxPvdInstrumentationFlag::eALL))
				{
					OutputDebugStringA("PhysX PVD connection failed. Continuing without PVD.\n");
					m_pPvd->release();
					m_pPvd = nullptr;
					pPvdTransport->release();
				}
			}
			else
			{
				OutputDebugStringA("PhysX PVD transport creation failed. Continuing without PVD.\n");
				m_pPvd->release();
				m_pPvd = nullptr;
			}
		}
		else
		{
			OutputDebugStringA("PhysX PVD creation failed. Continuing without PVD.\n");
		}
#endif // DEBUG

        // ToleranceScale: 현실의 물리 법칙을 얼마나 정밀하게 계산할 것인가
        physx::PxTolerancesScale scale{ PxTolerancesScale() };
        m_pPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *m_pFoundation, scale, recordMemoryAllocations, m_pPvd);
        if (!m_pPhysics)
        {
            MSG_BOX("PxCreatePhysics FAILED");
            return E_FAIL;
        }

        if (!PxInitExtensions(*m_pPhysics, m_pPvd)) {
            MSG_BOX("PxInitExtensions FAILED");
            return E_FAIL;
        }
    }


    //{
    //    PxCudaContextManagerDesc cudaDesc;
    //    PxCudaContextManager* cudaContext =
    //        PxCreateCudaContextManager(*m_pFoundation, cudaDesc, nullptr);

    //    if (!cudaContext || !cudaContext->contextIsValid())
    //    {
    //        MSG_BOX("CUDA FAIL")
    //        // GPU PhysX 불가
    //    }
    //}

    // Scene 생성
    {
        _float fGravitY = -9.81f;
        uint32_t iCpuDispatcherCnt = 4;

        m_pCpuDispatcher = physx::PxDefaultCpuDispatcherCreate(iCpuDispatcherCnt);
        if (!m_pCpuDispatcher)
        {
            MSG_BOX("PxDefaultCpuDispatcherCreate FAIL");
            return E_FAIL;
        }
		auto CreateScene = [&](const _bool bUseGpu) -> physx::PxScene*
		{
			physx::PxSceneDesc sceneDesc(m_pPhysics->getTolerancesScale());
			sceneDesc.gravity = physx::PxVec3(0.0f, fGravitY, 0.0f);
			sceneDesc.filterShader = MyFilterShader;
			sceneDesc.flags |= physx::PxSceneFlag::eENABLE_ACTIVE_ACTORS;
			sceneDesc.simulationEventCallback = m_pListener.get();
			sceneDesc.cpuDispatcher = m_pCpuDispatcher;

			if (bUseGpu)
			{
				sceneDesc.cudaContextManager = m_pCudaContextManager;
				sceneDesc.flags |= physx::PxSceneFlag::eENABLE_GPU_DYNAMICS;
				sceneDesc.broadPhaseType = physx::PxBroadPhaseType::eGPU;
			}

			return m_pPhysics->createScene(sceneDesc);
		};

		physx::PxCudaContextManagerDesc cudaDesc{};
		//m_pCudaContextManager = PxCreateCudaContextManager(*m_pFoundation, cudaDesc, nullptr);
		m_pCudaContextManager = nullptr;

		if (m_pCudaContextManager && m_pCudaContextManager->contextIsValid())
		{
			m_pScene = CreateScene(true);
			m_bGpuSimulationEnabled = m_pScene != nullptr;
		}

		if (!m_bGpuSimulationEnabled)
		{
			if (m_pCudaContextManager)
			{
				m_pCudaContextManager->release();
				m_pCudaContextManager = nullptr;
			}

			m_pScene = CreateScene(false);
		}

        if (!m_pScene)
        {
            MSG_BOX("createScene FAIL");
            return E_FAIL;
        }

		m_pControllerManager = PxCreateControllerManager(*m_pScene);
		if (!m_pControllerManager)
		{
			MSG_BOX("Failed to create PhysX Controller Manager");
			return E_FAIL;
		}

        // for debug
        {
            m_pScene->setVisualizationParameter(PxVisualizationParameter::eSCALE, 1.0f); // 전체 스케일
            m_pScene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_SHAPES, 1.0f); // 충돌체 그리기
            m_pScene->setVisualizationParameter(PxVisualizationParameter::eACTOR_AXES, 1.0f);
            //m_pScene->setVisualizationParameter(PxVisualizationParameter::eJOINT_LIMITS, 1.0f);
            //m_pScene->setVisualizationParameter(PxVisualizationParameter::eCONTACT_NORMAL, 1.0f);
            //m_pScene->setVisualizationParameter(PxVisualizationParameter::eCONTACT_FORCE, 1.0f);
            //m_pScene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_DYNAMIC, 1.0f);
        }
    }

    
	return S_OK;
}

void CPhysXManager::PrepareCCTInteractions(_float fFixedTimeDelta)
{
	if (!m_bCCTInteractionsEnabled || !m_pControllerManager || fFixedTimeDelta <= 0.f ||
		m_pControllerManager->getNbControllers() < 2)
	{
		return;
	}

	ZoneScopedN("CPhysXManager_PrepareCCTInteractions");
	CPxCCTInteractionFilter tFilter{};
	m_pControllerManager->computeInteractions(fFixedTimeDelta, &tFilter);
}

void CPhysXManager::StepSimulation(float fixedDeltaTime)
{
    {
        ZoneScopedN("CPhysXManager_simulate And fetchResults");
        m_pScene->simulate(fixedDeltaTime);
        m_pScene->fetchResults(true);
    }

    {
        ZoneScopedN("CPhysXManager_SyncPhysicsToComponents");
        SyncPhysicsToComponents();
    }

	{
		ZoneScopedN("CPhysXManager_DispatchPendingEvents");
		if (m_pListener)
			m_pListener->DispatchPendingEvents();
	}
}

void CPhysXManager::SyncPhysicsToComponents()
{
    physx::PxU32 nbActiveActors = 0;
    physx::PxActor** activeActors = m_pScene->getActiveActors(nbActiveActors);

    for (physx::PxU32 i = 0; i < nbActiveActors; ++i)
    {
		auto actor = static_cast<physx::PxRigidActor*>(activeActors[i]);
		if (!actor)
		{
			continue;
		}

		const auto tActorUserData =
			FindActorUserData(actor);
		if (!tActorUserData ||
			tActorUserData->eType !=
				PX_ACTOR_TYPE::RIGID_BODY)
		{
			// CCT, Ragdoll Bone, Articulation Link는 각 전용
			// 컴포넌트가 Transform/Bone 동기화를 담당한다.
			continue;
		}

		auto* pObj = FindGameObject(actor);
        if (!pObj)
        {
            continue;
        }

        {
            physx::PxTransform pose = actor->getGlobalPose();

            PX_SYNC_DATA data{};
            data.vPos = { pose.p.x, pose.p.y, pose.p.z };
            data.vQuat = { pose.q.x , pose.q.y, pose.q.z, pose.q.w };
            pObj->SyncActivePhysXData(data);
        }
    }
}

UPtr<CPhysXManager> CPhysXManager::Create()
{
    auto pInstance = ToUPtr(new CPhysXManager{ });
    if (FAILED(pInstance->Initialize()))
    {
        MSG_BOX("Failed to Created : CPhysXManager");
        return nullptr;
    }
    return pInstance;
}

void CPhysXManager::Free()
{
	m_pRagdollEditor.reset();
	m_pCookingEditor.reset();
	m_pCollisionProxyEditor.reset();

	{
		std::unique_lock lock{ m_UserDataRegistryMutex };
		m_ShapeUserDataRegistry.clear();
		m_ActorUserDataRegistry.clear();
	}

    // 해제는 생성의 역순
	if (m_pControllerManager)
	{
		// Manager가 릴리즈되면서 관리하던 모든 Controller 객체도 내부적으로 정리됩니다.
		m_pControllerManager->release();
		m_pControllerManager = nullptr;
	}

    if (m_pScene)
	{
		m_pScene->release();
		m_pScene = nullptr;
	}

	if (m_pCudaContextManager)
	{
		m_pCudaContextManager->release();
		m_pCudaContextManager = nullptr;
	}
	m_bGpuSimulationEnabled = false;

    if (m_pPhysics) m_pPhysics->release();
    if (m_pCpuDispatcher) m_pCpuDispatcher->release();

    PxCloseExtensions();

    if (m_pPvd) {
        auto transport = m_pPvd->getTransport();
        m_pPvd->release();
        if (transport) transport->release();
    }

    if (m_pFoundation) m_pFoundation->release();

    CEngineBase::Free();
}

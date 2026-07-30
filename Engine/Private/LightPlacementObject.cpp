#include "pch.h"
#include "LightPlacementObject.h"

#include "GameInstance.h"
#include "Light.h"

#include <filesystem>

NS_USING(Engine)

namespace Engine::LightPlacementObjectDetail
{
	inline constexpr uint32_t SUPPORTED_VERSION = 1;
	inline constexpr _float MIN_LIGHT_RANGE = 0.02f;
	inline constexpr _float MIN_DIRECTION_LENGTH_SQ = 1e-8f;

	_float3 NormalizeDirection(const _float3& direction)
	{
		const _vector loaded = XMLoadFloat3(&direction);
		if (XMVectorGetX(XMVector3LengthSq(loaded)) <=
			MIN_DIRECTION_LENGTH_SQ)
		{
			return { 0.f, -1.f, 0.f };
		}

		_float3 normalized{};
		XMStoreFloat3(
			&normalized,
			XMVector3Normalize(loaded));
		return normalized;
	}
}

HRESULT CLightPlacementObject::Initialize(void* pArg)
{
	auto* desc = static_cast<DESC*>(pArg);
	if (!desc ||
		(!desc->pLightData &&
			desc->sLightFileName.empty()) ||
		(desc->sPlacementGroup.empty() &&
			desc->sLightFileName.empty()))
	{
		return E_FAIL;
	}

	if (FAILED(CGameObject::Initialize(pArg)))
		return E_FAIL;

	m_sPlacementGroup =
		MakeLightPlacementGroupName(
			desc->sPlacementGroup.empty()
			? desc->sLightFileName
			: desc->sPlacementGroup);

	LIGHT_PLACEMENT_FILE data{};
	if (FAILED(LoadLightData(*desc, data)))
		return E_FAIL;

	const HRESULT result = SpawnLights(data);
	if (SUCCEEDED(result))
	{
		CGameInstance::Get().
			SetActivePlacementLightGroup(
				m_sPlacementGroup);
	}
	return result;
}

HRESULT CLightPlacementObject::LoadLightData(
	const DESC& desc,
	LIGHT_PLACEMENT_FILE& outData) const
{
	if (desc.pLightData)
	{
		outData = *desc.pLightData;
		return outData.iVersion ==
			LightPlacementObjectDetail::SUPPORTED_VERSION
			? S_OK
			: E_FAIL;
	}

	const std::string path =
		MakeLightPlacementFilePath(
			desc.sLightFileName);
	if (!std::filesystem::exists(path))
	{
		outData = {};
		outData.iVersion =
			LightPlacementObjectDetail::SUPPORTED_VERSION;

		DEBUG_LOG_STR(
			std::string{
				"[Light][Placement] File not found. "
				"Starting with empty placement: " } +
			path + "\n");
		return S_OK;
	}

	if (FAILED(CGameInstance::Get().JsonDeSerialize(
		path,
		outData,
		"LightPlacements")))
	{
		return E_FAIL;
	}

	if (outData.iVersion !=
		LightPlacementObjectDetail::SUPPORTED_VERSION)
	{
		DEBUG_LOG_STR(
			std::string{
				"[Light][Placement] Unsupported version: " } +
			std::to_string(outData.iVersion) + "\n");
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CLightPlacementObject::SpawnLights(
	const LIGHT_PLACEMENT_FILE& data)
{
	RemoveSpawnedLights();
	m_SpawnedLightHandles.reserve(data.lights.size());

	for (const LIGHT_PLACEMENT_ENTRY& source :
		data.lights)
	{
		LIGHT_PLACEMENT_ENTRY entry = source;
		entry.vDirection =
			LightPlacementObjectDetail::NormalizeDirection(
				entry.vDirection);
		entry.fIntensity =
			std::max(entry.fIntensity, 0.f);
		entry.fRange = std::max(
			entry.fRange,
			LightPlacementObjectDetail::MIN_LIGHT_RANGE);
		entry.fOuterAttenuation = std::clamp(
			entry.fOuterAttenuation,
			0.1f,
			75.f);
		entry.fInnerAttenuation = std::clamp(
			entry.fInnerAttenuation,
			0.f,
			entry.fOuterAttenuation);

		std::optional<CHandle> handle{};
		switch (entry.eType)
		{
		case LIGHT_TYPE::DIRECTIONAL:
			handle =
				CGameInstance::Get().
				Add_DirectionalLight(
					entry.vDirection,
					entry.vColor,
					entry.fIntensity);
			break;

		case LIGHT_TYPE::POINT:
			handle =
				CGameInstance::Get().
				Add_PointLight(
					entry.vPosition,
					entry.vColor,
					entry.fIntensity,
					entry.fRange);
			break;

		case LIGHT_TYPE::SPOTLIGHT:
			handle =
				CGameInstance::Get().
				Add_SpotLight(
					entry.vPosition,
					entry.vColor,
					entry.fIntensity,
					entry.fRange,
					entry.fInnerAttenuation,
					entry.fOuterAttenuation);
			break;

		default:
			RemoveSpawnedLights();
			return E_FAIL;
		}

		if (!handle)
		{
			RemoveSpawnedLights();
			return E_FAIL;
		}

		CLight* light = CGameInstance::Get().
			GetGameObjectByHandleT<CLight>(*handle);
		if (!light)
		{
			CGameInstance::Get().Remove_Light(*handle);
			RemoveSpawnedLights();
			return E_FAIL;
		}

		if (!entry.sName.empty())
			light->SetObjectTag(entry.sName);
		light->Set_LightPlacementGroup(
			m_sPlacementGroup);
		light->Set_LightAlias(entry.sAlias);
		light->Set_LightPosition(entry.vPosition);
		light->Set_LightDirection(entry.vDirection);
		light->Set_LightActivateState(entry.bActive);
		m_SpawnedLightHandles.push_back(*handle);
	}

	DEBUG_LOG_STR(
		std::string{
			"[Light][Placement] Spawned lights: " } +
		std::to_string(m_SpawnedLightHandles.size()) +
		"\n");
	return S_OK;
}

void CLightPlacementObject::RemoveSpawnedLights()
{
	for (const CHandle& handle :
		m_SpawnedLightHandles)
	{
		CGameInstance::Get().Remove_Light(handle);
	}
	m_SpawnedLightHandles.clear();
}

UPtr<CLightPlacementObject>
CLightPlacementObject::Create()
{
	auto instance =
		ToUPtr(new CLightPlacementObject{});
	if (FAILED(instance->InitializePrototype()))
		return nullptr;
	return instance;
}

UPtr<CPrototype>
CLightPlacementObject::Clone(void* pArg)
{
	auto instance =
		ToUPtr(new CLightPlacementObject{ *this });
	if (FAILED(instance->Initialize(pArg)))
		return nullptr;
	return instance;
}

void CLightPlacementObject::Free()
{
	if (!m_sPlacementGroup.empty())
	{
		CGameInstance::Get().
			Remove_PlacementLightGroup(
				m_sPlacementGroup);
		m_SpawnedLightHandles.clear();
	}
	else
	{
		RemoveSpawnedLights();
	}
	CGameObject::Free();
}

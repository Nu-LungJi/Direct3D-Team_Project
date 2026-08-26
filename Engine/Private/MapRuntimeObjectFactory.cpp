#include "pch.h"
#include "MapRuntimeObjectFactory.h"

#include "DecalVolume.h"
#include "MapMeshObject.h"
#include "ResTexture2D.h"

NS_USING(Engine)

MAP_MESH_OBJECT_FILE_DATA CMapRuntimeObjectFactory::MakeMapMeshObjectFileData(const CMapMeshObject& object, const std::string& layerName) const
{
	const auto& transform = object.GetTransform();

	MAP_MESH_OBJECT_FILE_DATA fileData{};
	fileData.objectTag = object.GetObjectTag();
	fileData.protoGroup = "PERMANENT";
	fileData.prototype = "Prototype_GameObject_MapMeshObject";
	fileData.modelGroup = object.GetModelResourceGroup();
	fileData.model = object.GetModelResourceTag();
	fileData.layer = layerName;
	fileData.position = transform.GetPosition();
	fileData.rotation = transform.GetQuaternion();
	fileData.scale = transform.GetScale();
	fileData.windDesc = object.GetWindDesc();

	return fileData;
}

std::optional<CHandle> CMapRuntimeObjectFactory::CreateMapMeshObject(const MAP_MESH_OBJECT_FILE_DATA& objectData, _bool updateTransformImmediately) const
{
	CMapMeshObject::MAP_MESH_OBJECT_DESC desc{};
	desc.sObjectTag = objectData.objectTag;
	desc.protoGroupTag = objectData.protoGroup;
	desc.prototypeTag = objectData.prototype;
	desc.modelGroupTag = objectData.modelGroup;
	desc.modelResTag = objectData.model;
	desc.windDesc = objectData.windDesc;

	auto handle = CGameInstance::Get().AddGameObjectToLayer(desc.protoGroupTag, desc.prototypeTag, objectData.layer, &desc);
	if (!handle)
		return std::nullopt;

	auto* object = CGameInstance::Get().GetGameObjectByHandle(*handle);
	if (!object)
		return std::nullopt;

	auto& transform = object->GetTransform();
	transform.SetPosition(objectData.position);
	transform.SetQuaternion(objectData.rotation);
	transform.SetScale(objectData.scale);

	if (updateTransformImmediately)
		transform.Update();

	return handle;
}

MAP_DECAL_FILE_DATA CMapRuntimeObjectFactory::MakeDecalFileData(const CDecalVolume& decal, const std::string& layerName) const
{
	const auto& transform = decal.GetTransform();
	const _bool hasMaskOverride = decal.GetMaskTextureGroup().hash != 0 && decal.GetMaskTextureTag().hash != 0;

	MAP_DECAL_FILE_DATA fileData{};
	fileData.objectTag = decal.GetObjectTag();
	fileData.layer = layerName;
	fileData.materialPath = decal.GetMaterialPath();
	fileData.textureGroup = hasMaskOverride ? decal.GetMaskTextureGroup().GetDbgStr() : "";
	fileData.textureTag = hasMaskOverride ? decal.GetMaskTextureTag().GetDbgStr() : "";
	fileData.texturePath = decal.GetMaskTexturePath();
	fileData.position = transform.GetPosition();
	fileData.rotation = transform.GetQuaternion();
	fileData.scale = transform.GetScale();
	fileData.opacity = decal.GetOpacity();
	fileData.normalThreshold = decal.GetNormalThreshold();
	fileData.edgeSoftness = decal.GetEdgeSoftness();
	fileData.hasMaterialParameters = true;

	for (const auto& parameter : decal.GetMaterialParameters())
	{
		const _float* values = decal.GetMaterialParameterData(parameter.name);
		if (!values)
			continue;

		MAP_DECAL_PARAMETER_DATA parameterData{};
		parameterData.name = parameter.name;
		parameterData.values.assign(values, values + parameter.count);
		fileData.materialParameters.push_back(std::move(parameterData));
	}

	for (UINT slot = CDecalMaterial::TEXTURE_SLOT_BEGIN; slot <= CDecalMaterial::TEXTURE_SLOT_END; ++slot)
	{
		const auto& group = decal.GetTextureOverrideGroup(slot);
		const auto& tag = decal.GetTextureOverrideTag(slot);
		if (group.hash == 0 || tag.hash == 0)
			continue;

		fileData.textureOverrides.push_back(
		{
			slot,
			group.GetDbgStr(),
			tag.GetDbgStr(),
			decal.GetTextureOverridePath(slot)
		});
	}

	return fileData;
}

std::optional<CHandle> CMapRuntimeObjectFactory::CreateDecal(const MAP_DECAL_FILE_DATA& decalData) const
{
	auto& gameInstance = CGameInstance::Get();
	const std::string textureGroup = decalData.textureGroup.empty() ? std::string(TAG_RES_GRP_MAP_DECAL_TEXTURE) : decalData.textureGroup;

	if (!decalData.textureTag.empty() && !gameInstance.GetResourceFirst<CResTexture2D>(textureGroup, decalData.textureTag))
	{
		if (decalData.texturePath.empty())
			return std::nullopt;

		auto texture = gameInstance.AddResourceT<CResTexture2D>(textureGroup, decalData.textureTag, CResTexture2D::Create(decalData.texturePath));
		if (!texture || FAILED(texture->Load()))
			return std::nullopt;
	}

	CDecalVolume::DECAL_VOLUME_DESC desc{};
	desc.sObjectTag = decalData.objectTag;
	desc.sMaterialPath = decalData.materialPath.empty() ? std::string(CDecalVolume::DEFAULT_MATERIAL_PATH) : decalData.materialPath;
	desc.fOpacity = decalData.opacity;
	desc.fNormalThreshold = decalData.normalThreshold;
	desc.fEdgeSoftness = decalData.edgeSoftness;
	if (!decalData.textureTag.empty())
	{
		desc.sTextureGroup = textureGroup;
		desc.sMaskTextureTag = decalData.textureTag;
	}

	auto handle = gameInstance.AddGameObjectToLayer(decalData.protoGroup, decalData.prototype, MAPDECALOBJECTLAYER, &desc);
	if (!handle)
		return std::nullopt;

	auto* decal = gameInstance.GetGameObjectByHandleT<CDecalVolume>(*handle);
	if (!decal)
		return std::nullopt;

	auto& transform = decal->GetTransform();
	transform.SetPosition(decalData.position);
	transform.SetQuaternion(decalData.rotation);
	transform.SetScale(decalData.scale);
	transform.Update();

	if (decalData.hasMaterialParameters)
	{
		for (const auto& parameter : decal->GetMaterialParameters())
		{
			const auto savedParameter = std::find_if(decalData.materialParameters.begin(), decalData.materialParameters.end(),
				[&parameter](const MAP_DECAL_PARAMETER_DATA& candidate)
				{
					return candidate.name == parameter.name;
				});
			if (savedParameter == decalData.materialParameters.end())
				continue;

			std::array<_float, 4> values{};
			const size_t copyCount = std::min<size_t>(parameter.count, savedParameter->values.size());
			std::copy_n(savedParameter->values.begin(), copyCount, values.begin());
			decal->SetMaterialParameter(parameter.name, values.data(), parameter.count);
		}
	}
	else
	{
		decal->SetMaterialParameter("Albedo", &decalData.legacyAlbedo.x, 4);
		decal->SetMaterialParameter("Emissive Color", &decalData.legacyEmissive.x, 3);
		decal->SetMaterialParameter("Emissive Intensity", &decalData.legacyEmissiveIntensity, 1);
	}

	for (const auto& textureData : decalData.textureOverrides)
	{
		if (textureData.slot < CDecalMaterial::TEXTURE_SLOT_BEGIN ||
			textureData.slot > CDecalMaterial::TEXTURE_SLOT_END ||
			textureData.tag.empty())
		{
			continue;
		}

		const std::string group = textureData.group.empty() ? std::string(TAG_RES_GRP_MAP_DECAL_TEXTURE) : textureData.group;
		if (!gameInstance.GetResourceFirst<CResTexture2D>(group, textureData.tag))
		{
			if (textureData.path.empty())
				continue;
			auto texture = gameInstance.AddResourceT<CResTexture2D>(group, textureData.tag, CResTexture2D::Create(textureData.path));
			if (!texture || FAILED(texture->Load()))
				continue;
		}
		decal->SetTextureOverride(textureData.slot, group, textureData.tag);
	}

	return handle;
}

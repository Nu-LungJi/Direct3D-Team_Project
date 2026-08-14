#pragma once
#include "GameObject.h"
#include "DecalMaterial.h"

NS_BEGIN(Engine)

class CComConstantBuffer;
class CResCubeColBuffer;
class CResSamplerState;
class CResTexture2D;
class CResVertexShader;

class ENGINE_DLL CDecalVolume final : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CDecalVolume, CGameObject)
	CDecalVolume& operator=(const CDecalVolume&) = delete;
	static constexpr const char* DEFAULT_MATERIAL_PATH = "./DecalMaterials/BasicDecal.json";

protected:
	explicit CDecalVolume();
	explicit CDecalVolume(const CDecalVolume& prototype);
	~CDecalVolume() override;

public:
	struct DECAL_VOLUME_DESC : public GAMEOBJECT_DESC
	{
		_float3 vPosition{};
		_float3 vRotation{};
		_float3 vScale{ 10.f, 2.f, 10.f };

		_float fOpacity{ 1.f };
		_float fNormalThreshold{ 0.4f };
		_float fEdgeSoftness{ 0.05f };
		_string sMaterialPath{ DEFAULT_MATERIAL_PATH };

		// Legacy/default mask override. Material texture slot t2 is replaced when set.
		StringID sTextureGroup{};
		StringID sMaskTextureTag{};
	};

private:
	struct CB_DECAL_VOLUME
	{
		_float4x4 matInvWorld{};
		_float4 vProjectionParams{}; // opacity, normal threshold, edge softness, time
		std::array<_float4, CDecalMaterial::PARAMETER_FLOAT_COUNT / 4> vMaterialParams{};
	};
	static_assert(sizeof(CB_DECAL_VOLUME) % 16 == 0);

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void LateUpdate(_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* context, const RENDER_CTX& ctx) override;
	void UpdateGUI() override;

public:
	HRESULT SetMaterial(const _string& materialPath);
	const _string& GetMaterialPath() const { return m_MaterialPath; }
	const SPtr<CDecalMaterial>& GetMaterial() const { return m_Material; }
	const std::vector<CDecalMaterial::PARAMETER_DESC>& GetMaterialParameters() const;
	_float* GetMaterialParameterData(const _string& name);
	const _float* GetMaterialParameterData(const _string& name) const;
	HRESULT SetMaterialParameter(const _string& name, const _float* values, size_t count);

	HRESULT SetTextureOverride(UINT slot, const StringID& textureGroup, const StringID& textureTag);
	void ClearTextureOverride(UINT slot);
	const StringID& GetTextureOverrideGroup(UINT slot) const;
	const StringID& GetTextureOverrideTag(UINT slot) const;
	const _string& GetTextureOverridePath(UINT slot) const;

	HRESULT SetMaskTexture(const StringID& textureGroup, const StringID& textureTag)
	{
		return SetTextureOverride(CDecalMaterial::TEXTURE_SLOT_BEGIN, textureGroup, textureTag);
	}
	const StringID& GetMaskTextureGroup() const { return GetTextureOverrideGroup(CDecalMaterial::TEXTURE_SLOT_BEGIN); }
	const StringID& GetMaskTextureTag() const { return GetTextureOverrideTag(CDecalMaterial::TEXTURE_SLOT_BEGIN); }
	const _string& GetMaskTexturePath() const { return GetTextureOverridePath(CDecalMaterial::TEXTURE_SLOT_BEGIN); }

	_float GetOpacity() const { return m_fOpacity; }
	_float GetNormalThreshold() const { return m_fNormalThreshold; }
	_float GetEdgeSoftness() const { return m_fEdgeSoftness; }
	void SetOpacity(_float value) { m_fOpacity = std::clamp(value, 0.f, 1.f); }
	void SetNormalThreshold(_float value) { m_fNormalThreshold = std::clamp(value, 0.f, 0.999f); }
	void SetEdgeSoftness(_float value) { m_fEdgeSoftness = std::clamp(value, 0.001f, 0.49f); }

private:
	CComConstantBuffer* m_pComCBufferPerObject{};
	CComConstantBuffer* m_pComCBufferDecal{};
	SPtr<CResCubeColBuffer> m_pCubeBuffer{};
	SPtr<CResVertexShader> m_pVertexShader{};
	SPtr<CResSamplerState> m_pLinearWrapSampler{};
	SPtr<CResSamplerState> m_pLinearClampSampler{};

	SPtr<CDecalMaterial> m_Material{};
	_string m_MaterialPath{ DEFAULT_MATERIAL_PATH };
	std::array<_float, CDecalMaterial::PARAMETER_FLOAT_COUNT> m_MaterialParameters{};
	std::array<SPtr<CResTexture2D>, CDecalMaterial::TEXTURE_SLOT_END + 1> m_TextureOverrides{};
	std::array<StringID, CDecalMaterial::TEXTURE_SLOT_END + 1> m_TextureOverrideGroups{};
	std::array<StringID, CDecalMaterial::TEXTURE_SLOT_END + 1> m_TextureOverrideTags{};

	_float m_fOpacity{ 1.f };
	_float m_fNormalThreshold{ 0.4f };
	_float m_fEdgeSoftness{ 0.05f };
	_float m_fTime{};

public:
	static UPtr<CDecalVolume> Create();
	UPtr<CPrototype> Clone(void* pArg) override;
};

NS_END


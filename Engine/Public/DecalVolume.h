#pragma once
#include "GameObject.h"

NS_BEGIN(Engine)

class CComConstantBuffer;
class CResCubeColBuffer;
class CResPixelShader;
class CResSamplerState;
class CResTexture2D;
class CResVertexShader;

class ENGINE_DLL CDecalVolume : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CDecalVolume, CGameObject)
	CDecalVolume& operator=(const CDecalVolume&) = delete;

protected:
	explicit CDecalVolume();
	explicit CDecalVolume(const CDecalVolume& Prototype);
	~CDecalVolume() override;

public:
	typedef struct tagDecalVolumeDesc : public GAMEOBJECT_DESC
	{
		_float3 vPosition{};
		_float3 vRotation{};
		_float3 vScale{ 10.f, 2.f, 10.f };

		_float4 vAlbedoColor{ 0.15f, 0.005f, 0.002f, 1.f };
		_float3 vEmissiveColor{ 1.f, 0.01f, 0.f };
		_float  fEmissiveIntensity{ 8.f };

		_float fOpacity{ 1.f };
		_float fNormalThreshold{ 0.4f };
		_float fEdgeSoftness{ 0.05f };

		StringID sTextureGroup{};
		StringID sMaskTextureTag{};
	} DECAL_VOLUME_DESC;

private:
	typedef struct tagDecalVolumeConstantBuffer
	{
		_float4x4 matInvWorld{};
		_float4 vAlbedoColor{};
		_float4 vEmissiveColorIntensity{};
		_float4 vParams{};
	} CB_DECAL_VOLUME;
	static_assert(sizeof(CB_DECAL_VOLUME) % 16 == 0);

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void LateUpdate(_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx) override;
	void UpdateGUI() override;

public:
	HRESULT SetMaskTexture(const StringID& textureGroup, const StringID& textureTag);
	const StringID& GetMaskTextureGroup() const { return m_sTextureGroup; }
	const StringID& GetMaskTextureTag() const { return m_sMaskTextureTag; }
	const _string& GetMaskTexturePath() const;

	const _float4& GetAlbedoColor() const { return m_vAlbedoColor; }
	const _float3& GetEmissiveColor() const { return m_vEmissiveColor; }
	_float GetEmissiveIntensity() const { return m_fEmissiveIntensity; }
	_float GetOpacity() const { return m_fOpacity; }
	_float GetNormalThreshold() const { return m_fNormalThreshold; }
	_float GetEdgeSoftness() const { return m_fEdgeSoftness; }

	void SetAlbedoColor(const _float4& value) { m_vAlbedoColor = value; }
	void SetEmissiveColor(const _float3& value) { m_vEmissiveColor = value; }
	void SetEmissiveIntensity(_float value) { m_fEmissiveIntensity = std::max(0.f, value); }
	void SetOpacity(_float value) { m_fOpacity = std::clamp(value, 0.f, 1.f); }
	void SetNormalThreshold(_float value) { m_fNormalThreshold = std::clamp(value, 0.f, 0.999f); }
	void SetEdgeSoftness(_float value) { m_fEdgeSoftness = std::clamp(value, 0.001f, 0.49f); }

private:
	CComConstantBuffer* m_pComCBufferPerObject{};
	CComConstantBuffer* m_pComCBufferDecal{};
	SPtr<CResCubeColBuffer> m_pCubeBuffer{};
	SPtr<CResVertexShader> m_pVertexShader{};
	SPtr<CResPixelShader> m_pPixelShader{};
	SPtr<CResSamplerState> m_pLinearClampSampler{};
	SPtr<CResTexture2D> m_pMaskTexture{};
	StringID m_sTextureGroup{};
	StringID m_sMaskTextureTag{};


	_float4 m_vAlbedoColor{ 0.15f, 0.005f, 0.002f, 1.f };
	_float3 m_vEmissiveColor{ 1.f, 0.01f, 0.f };
	_float m_fEmissiveIntensity{ 8.f };
	_float m_fOpacity{ 1.f };
	_float m_fNormalThreshold{ 0.4f };
	_float m_fEdgeSoftness{ 0.05f };

public:
	static UPtr<CDecalVolume> Create();
	UPtr<CPrototype> Clone(void* pArg) override;
};

NS_END

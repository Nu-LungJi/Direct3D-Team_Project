#pragma once

#include "UITex.h"
#include "Client_Defines.h"

NS_BEGIN(Engine)
class CComConstantBuffer;
class CButtonComponent;
NS_END

NS_BEGIN(Client)

class CMiniMap final : public E::CUITex
{
public:
	DECLARE_DERIVED_TYPE(CMiniMap, E::CUITex)

	enum class MINIMAP_MODE : uint32_t
	{
		WORLD_MAP,
		DUNGEON_FOG
	};

	struct MINIMAP_PROFILE
	{
		MINIMAP_MODE Mode{ MINIMAP_MODE::WORLD_MAP };
		std::string TextureTag{ "TEX_UI_T_MapMini_Sanctuary_03_D" };
		_float2 WorldMinXZ{};
		_float2 WorldMaxXZ{};
		_float2 UVMin{};
		_float2 UVMax{ 1.f, 1.f };
		_float MapScale{ 0.6f };
		_float SmokeIntensity{ 0.65f };
		_float SmokeSpeed{ 1.f };
		_float FogAlphaMultiplier{ 1.f };
		_float PlayerScrollScale{ 0.003f };
	};

private:
	CMiniMap();
	~CMiniMap() override;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) override;
	void SetMiniMapProfile(const MINIMAP_PROFILE& profile);

private:
	virtual void PlayEffect(uint32_t uiState);

private:
	CComConstantBuffer* m_pComCBufferPerUI = nullptr;
	CComConstantBuffer* m_pMinimapCBuffer = nullptr;
	CButtonComponent* m_pComCButton = nullptr;

	_float3 m_cameraLook{0.f, 0.f, 1.f};
	_float3 m_playerLook{1.f, 0.f, 1.f};

	_float2 tMapOffset{};
	_float tRotation{ 0.f};
	_float tScale{0.6f};
	_float m_fSmokeTime{};
	_float3 m_vPreviousPlayerPosition{};
	_bool m_bHasPreviousPlayerPosition{ false };
	MINIMAP_PROFILE m_MiniMapProfile{};
	uint32_t m_iConfiguredLevel{ static_cast<uint32_t>(-1) };

	_bool m_SearchPlayerIcon{false};
	CHandle m_hPlayerIcon{};

	static constexpr size_t MONSTER_MARKER_COUNT = 20;
	static constexpr _float MONSTER_MARKER_SIZE = 18.f;
	static constexpr _float MONSTER_DETECTION_RADIUS = 60.f;
	static constexpr _float MONSTER_SEARCH_INTERVAL = 0.1f;
	static constexpr _float MINIMAP_BORDER_PADDING = 4.3f;

	_bool m_bMonsterMarkerPoolInitialized{ false };
	_float m_fMonsterSearchAcc{ MONSTER_SEARCH_INTERVAL };
	std::vector<CHandle> m_vMonsterMarkerHandles;
	std::vector<CHandle> m_vNearbyMonsterHandles;

private:
	void SearchPlayerIcon();
	void SetPlayerIconRot(_float rot);
	void CalcDir();
	void InitializeMonsterMarkerPool();
	void RefreshNearbyMonsters(E::CGameObject* pPlayer);
	void UpdateMonsterMarkers(E::_float fTimeDelta, E::CGameObject* pPlayer);
	void HideMonsterMarkers();
	void SetMonsterMarkerVisible(E::CUIObject* pMarker, _bool bVisible);
	void ConfigureDefaultProfile();
	void UpdateWorldMapOffset(const _float3& playerPosition);
	void UpdateFogMovementOffset(const _float3& playerPosition);

public:
	static E::UPtr<CMiniMap> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END

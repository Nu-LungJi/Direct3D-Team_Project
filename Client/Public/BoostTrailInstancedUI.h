#pragma once

#include "Client_Defines.h"
#include "UIObject.h"

NS_BEGIN(Client)

// SpellMiniGame boost smoke renderer.
// It owns a dedicated dynamic instance buffer and does not share or modify
// the Engine particle instance buffer.
class CBoostTrailInstancedUI final : public E::CUIObject
{
public:
	DECLARE_DERIVED_TYPE(CBoostTrailInstancedUI, E::CUIObject)

	struct INSTANCE_DESC
	{
		_float2 Position{};
		_float2 Size{};
		_float Rotation{};
		_float4 Color{ 1.f, 1.f, 1.f, 1.f };
		uint32_t Frame{};
	};

private:
	struct GPU_INSTANCE
	{
		_float4x4 World{};
		_float4 Color{};
		_float2 UVOffset{};
		_float2 UVSize{};
	};

private:
	CBoostTrailInstancedUI();
	~CBoostTrailInstancedUI() override = default;

public:
	void PriorityUpdate(_float fTimeDelta) override;
	void Update(_float fTimeDelta) override;
	void LateUpdate(_float fTimeDelta) override;
	HRESULT Render(
		ID3D11DeviceContext* pContext,
		const E::RENDER_CTX& ctx) override;

	void ClearInstances();
	void AddInstance(const INSTANCE_DESC& desc);
	_bool HasInstances() const { return !m_Instances.empty(); }

public:
	static E::UPtr<CBoostTrailInstancedUI> Create(
		size_t maxInstanceCount,
		int weight,
		const std::string& textureTag = "TEX_VFX_T_Fireball_Stream_D",
		uint32_t columns = 6,
		uint32_t rows = 6,
		uint32_t atlasSize = 1024,
		_bool useTextureAlpha = false);
	E::UPtr<E::CPrototype> Clone(void* pArg) override;

private:
	HRESULT InitializeRenderer(
		size_t maxInstanceCount,
		int weight,
		const std::string& textureTag,
		uint32_t columns,
		uint32_t rows,
		uint32_t atlasSize,
		_bool useTextureAlpha);
	void Free() override;

private:
	size_t m_iMaxInstanceCount{};
	std::string m_sTextureTag{ "TEX_VFX_T_Fireball_Stream_D" };
	uint32_t m_iColumns{ 6 };
	uint32_t m_iRows{ 6 };
	uint32_t m_iAtlasSize{ 1024 };
	_bool m_bUseTextureAlpha{};
	std::vector<INSTANCE_DESC> m_Instances{};
	std::vector<GPU_INSTANCE> m_GPUInstances{};
	ComPtr<ID3D11Buffer> m_pBoostTrailInstanceBuffer{};
};

NS_END

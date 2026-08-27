#pragma once
#include "InteractiveNpc.h"

NS_BEGIN(Client)

// 대사와 시네마틱은 CInteractiveNpc의 흐름을 사용하고,
// OPEN_SHOP 대사 액션에서 지팡이 상점 UI를 여는 상점 전용 NPC.
class CShopNpc final : public CInteractiveNpc
{
public:
	struct DESC : public CInteractiveNpc::DESC
	{
		// false면 일반 2D 상점, true면 NPC 기준 월드 패널 상점을 연다.
		_bool WorldSpaceShop{};
		_float3 ShopPanelPositionOffset{ 0.f, 1.6f, 1.2f };
		_float3 ShopPanelRotationOffsetDegrees{};
	};

public:
	DECLARE_DERIVED_TYPE(CShopNpc, CInteractiveNpc)

private:
	CShopNpc() = default;
	CShopNpc(const CShopNpc& prototype);
	~CShopNpc() override = default;

public:
	HRESULT Initialize(void* pArg) override;

	static E::UPtr<CShopNpc> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;

protected:
	void OpenShop() override;

private:
	_bool m_bWorldSpaceShop{};
	_float3 m_vShopPanelPositionOffset{ 0.f, 1.6f, 1.2f };
	_float3 m_vShopPanelRotationOffsetDegrees{};
};

NS_END

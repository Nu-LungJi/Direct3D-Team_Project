#pragma once

#include "GameObject.h"

NS_BEGIN(Client)

class CMagicBridgeController final : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CMagicBridgeController, CGameObject)

	enum class STATE
	{
		IDLE,
		ACTIVATING,
		ACTIVE,
		DEACTIVATING
	};

	struct DESC : public CGameObject::GAMEOBJECT_DESC
	{
		_float3 vAnchorA{ 0.f, 3.f, 0.f };
		_float3 vAnchorB{ 10.f, 2.f, 0.f };
		_float fActivatedAnchorAY{ 1.f };
		_float fBlockSpacing{ 1.007f };
		uint32_t iWidthCount{ 1 };
		_float fWidthSpacing{ 1.007f };
		_float fMoveSpeed{ 2.f };
		_float fWaveDelay{ 1.f };
		_bool bEnablePhysics{ true };
		_bool bStartActivated{};
	};

private:
	struct BRIDGE_BLOCK
	{
		CHandle hBlock{};
		_float fInitialY{};
		_float fActivatedY{};
		_float fDelay{};
	};

private:
	CMagicBridgeController();
	CMagicBridgeController(const CMagicBridgeController& prototype);
	~CMagicBridgeController() override = default;

public:
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(_float fTimeDelta) override;
	void UpdateGUI() override;

	void Activate();
	void Deactivate();
	STATE GetState() const { return m_eState; }

public:
	static UPtr<CMagicBridgeController> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

private:
	HRESULT CreateBridge(const DESC& Desc);
	HRESULT CreateBridgeBlock(
		const _float3& vPosition,
		_float fActivatedY,
		_float fDelay,
		_bool bEnablePhysics);
	void BeginTransition(STATE eState);
	void SetAllTargets(_bool bActivated);
	void HoldCurrentHeights();
	void ClearBridgeBlocks();

private:
	std::vector<BRIDGE_BLOCK> m_Blocks{};
	STATE m_eState{ STATE::IDLE };
	_float m_fStateTime{};
	_float m_fMoveSpeed{ 2.f };
	_float m_fTransitionDuration{};
};

NS_END

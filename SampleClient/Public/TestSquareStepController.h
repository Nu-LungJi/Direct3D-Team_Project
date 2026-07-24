#pragma once

#include "GameObject.h"

NS_BEGIN(Client)

class CTestSquareStepController final : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CTestSquareStepController, CGameObject)

	enum class PLACEMENT
	{
		GRID,
		CIRCLE
	};

	struct DESC : public CGameObject::GAMEOBJECT_DESC
	{
		CHandle hTarget{};
		_float3 vOrigin{};
		PLACEMENT ePlacement{ PLACEMENT::GRID };
		uint32_t iCountX{ 10 };
		uint32_t iCountZ{ 10 };
		_float fSpacingX{ 1.007f };
		_float fSpacingZ{ 1.007f };
		_float fCircleRadius{ 13.f };
		_float fCircleSpacing{ 1.007f };
		_float fInfluenceRadius{ 3.f };
		_float fMaxRaiseHeight{ 1.f };
		_float fRaiseSpeed{ 2.f };
		_float fReturnSpeed{ 2.f };
		_float fFollowDelay{ 0.3f };
		_bool bEnablePhysics{ true };
	};

private:
	CTestSquareStepController();
	CTestSquareStepController(const CTestSquareStepController& prototype);
	~CTestSquareStepController() override = default;

public:
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(_float fTimeDelta) override;

	const std::vector<CHandle>& GetSquareStepHandles() const
	{
		return m_SquareStepHandles;
	}

public:
	static UPtr<CTestSquareStepController> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

private:
	HRESULT CreateGrid(const DESC& Desc);
	HRESULT CreateFilledCircle(const DESC& Desc);
	HRESULT CreateSquareStep(
		const _float3& vPosition, _bool bEnablePhysics);
	void ClearSquareSteps();

private:
	std::vector<CHandle> m_SquareStepHandles{};
	CHandle m_hTarget{};
	_float m_fInfluenceRadius{ 3.f };
	_float m_fMaxRaiseHeight{ 1.f };
	_float m_fRaiseSpeed{ 2.f };
	_float m_fReturnSpeed{ 2.f };
	_float m_fFollowDelay{ 0.3f };
	_float3 m_vFollowPosition{};
	_bool m_bFollowPositionInitialized{};
};

NS_END


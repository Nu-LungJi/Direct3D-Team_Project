#include "pch.h"
#include "ComLocomotion.h"

NS_USING(Engine)

CComLocomotion::CComLocomotion()
{
}

CComLocomotion::CComLocomotion(const CComLocomotion& rhs)
	: CComponent{ rhs }
{
}

CComLocomotion::~CComLocomotion()
{
}

HRESULT CComLocomotion::Initialize(void* pArg)
{
	if (FAILED(CComponent::Initialize(pArg)))
		return E_FAIL;

	return S_OK;
}

void CComLocomotion::SetMoveIntent(const _float3& vDirection, _float fSpeed)
{
	const _float fLengthSq =
		vDirection.x * vDirection.x +
		vDirection.z * vDirection.z;

	if (fLengthSq <= std::numeric_limits<_float>::epsilon() || fSpeed <= 0.f)
	{
		ClearMoveIntent();
		return;
	}

	const _float fInvLength = 1.f / std::sqrt(fLengthSq);
	m_tOutput.vMoveDirection = {
		vDirection.x * fInvLength,
		0.f,
		vDirection.z * fInvLength };
	m_tOutput.fMoveSpeed = fSpeed;
	m_tOutput.bMoveRequested = true;
}

void CComLocomotion::ClearMoveIntent()
{
	m_tOutput = {};
}

_bool CComLocomotion::ConsumeJumpRequest()
{
	const _bool bRequested = m_bJumpRequested;
	m_bJumpRequested = false;
	return bRequested;
}

void CComLocomotion::UpdateGUI()
{
	CComponent::UpdateGUI();
	ImGui::Text("Move Requested: %s", m_tOutput.bMoveRequested ? "true" : "false");
	ImGui::Text("Move Direction: %.3f, %.3f, %.3f",
		m_tOutput.vMoveDirection.x,
		m_tOutput.vMoveDirection.y,
		m_tOutput.vMoveDirection.z);
	ImGui::Text("Move Speed: %.3f", m_tOutput.fMoveSpeed);
	ImGui::Text("Jump Requested: %s", m_bJumpRequested ? "true" : "false");
}

UPtr<CComLocomotion> CComLocomotion::Create()
{
	auto pInstance = ToUPtr(new CComLocomotion{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CComLocomotion");
		return nullptr;
	}

	return pInstance;
}

UPtr<CPrototype> CComLocomotion::Clone(void* pArg)
{
	auto pInstance = ToUPtr(new CComLocomotion{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CComLocomotion");
		return nullptr;
	}

	return pInstance;
}

void CComLocomotion::Free()
{
	CComponent::Free();
}

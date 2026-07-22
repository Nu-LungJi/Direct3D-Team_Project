#include "pch.h"
#include "ComCharacterMoveIntent.h"

NS_USING(Engine)

CComCharacterMoveIntent::CComCharacterMoveIntent()
{
}

CComCharacterMoveIntent::CComCharacterMoveIntent(const CComCharacterMoveIntent& rhs)
	: CComponent{ rhs }
{
}

CComCharacterMoveIntent::~CComCharacterMoveIntent()
{
}

HRESULT CComCharacterMoveIntent::Initialize(void* pArg)
{
	if (FAILED(CComponent::Initialize(pArg)))
		return E_FAIL;

	return S_OK;
}

void CComCharacterMoveIntent::SetMoveIntent(const _float3& vDirection, _float fSpeed)
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

void CComCharacterMoveIntent::ClearMoveIntent()
{
	m_tOutput.vMoveDirection = {};
	m_tOutput.fMoveSpeed = 0.f;
	m_tOutput.bMoveRequested = false;
}

void CComCharacterMoveIntent::SetFacingIntent(const _float3& vDirection, _float fTurnSpeed)
{
	const _float fLengthSq =
		vDirection.x * vDirection.x +
		vDirection.z * vDirection.z;

	if (fLengthSq <= std::numeric_limits<_float>::epsilon() || fTurnSpeed <= 0.f)
	{
		ClearFacingIntent();
		return;
	}

	const _float fInvLength = 1.f / std::sqrt(fLengthSq);
	m_tOutput.vFacingDirection = {
		vDirection.x * fInvLength,
		0.f,
		vDirection.z * fInvLength };
	m_tOutput.fTurnSpeed = fTurnSpeed;
	m_tOutput.bFacingRequested = true;
	m_tOutput.bImmediateFacing = false;
}

void CComCharacterMoveIntent::SetFacingIntentImmediate(const _float3& vDirection)
{
	const _float fLengthSq =
		vDirection.x * vDirection.x +
		vDirection.z * vDirection.z;

	if (fLengthSq <= std::numeric_limits<_float>::epsilon())
	{
		ClearFacingIntent();
		return;
	}

	const _float fInvLength = 1.f / std::sqrt(fLengthSq);
	m_tOutput.vFacingDirection = {
		vDirection.x * fInvLength,
		0.f,
		vDirection.z * fInvLength };
	m_tOutput.fTurnSpeed = 0.f;
	m_tOutput.bFacingRequested = true;
	m_tOutput.bImmediateFacing = true;
}

void CComCharacterMoveIntent::ClearFacingIntent()
{
	m_tOutput.vFacingDirection = {};
	m_tOutput.fTurnSpeed = 0.f;
	m_tOutput.bFacingRequested = false;
	m_tOutput.bImmediateFacing = false;
}

_bool CComCharacterMoveIntent::ConsumeJumpRequest()
{
	const _bool bRequested = m_bJumpRequested;
	m_bJumpRequested = false;
	return bRequested;
}

void CComCharacterMoveIntent::RequestWarp(const _float3& vPosition)
{
	if (!std::isfinite(vPosition.x) ||
		!std::isfinite(vPosition.y) ||
		!std::isfinite(vPosition.z))
	{
		ClearWarpRequest();
		return;
	}

	m_vWarpPosition = vPosition;
	m_bWarpRequested = true;
}

_bool CComCharacterMoveIntent::ConsumeWarpRequest(_float3& vOutPosition)
{
	if (!m_bWarpRequested)
		return false;

	vOutPosition = m_vWarpPosition;
	ClearWarpRequest();
	return true;
}

void CComCharacterMoveIntent::ClearWarpRequest()
{
	m_vWarpPosition = {};
	m_bWarpRequested = false;
}

void CComCharacterMoveIntent::UpdateGUI()
{
	CComponent::UpdateGUI();
	ImGui::Text("Move Requested: %s", m_tOutput.bMoveRequested ? "true" : "false");
	ImGui::Text("Move Direction: %.3f, %.3f, %.3f",
		m_tOutput.vMoveDirection.x,
		m_tOutput.vMoveDirection.y,
		m_tOutput.vMoveDirection.z);
	ImGui::Text("Move Speed: %.3f", m_tOutput.fMoveSpeed);
	ImGui::Text("Facing Requested: %s", m_tOutput.bFacingRequested ? "true" : "false");
	ImGui::Text("Facing Direction: %.3f, %.3f, %.3f",
		m_tOutput.vFacingDirection.x,
		m_tOutput.vFacingDirection.y,
		m_tOutput.vFacingDirection.z);
	ImGui::Text("Turn Speed: %.3f deg/s", m_tOutput.fTurnSpeed);
	ImGui::Text("Immediate Facing: %s", m_tOutput.bImmediateFacing ? "true" : "false");
	ImGui::Text("Jump Requested: %s", m_bJumpRequested ? "true" : "false");
	ImGui::Text("Warp Requested: %s", m_bWarpRequested ? "true" : "false");
	ImGui::Text("Warp Position: %.3f, %.3f, %.3f",
		m_vWarpPosition.x,
		m_vWarpPosition.y,
		m_vWarpPosition.z);
}

UPtr<CComCharacterMoveIntent> CComCharacterMoveIntent::Create()
{
	auto pInstance = ToUPtr(new CComCharacterMoveIntent{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CComCharacterMoveIntent");
		return nullptr;
	}

	return pInstance;
}

UPtr<CPrototype> CComCharacterMoveIntent::Clone(void* pArg)
{
	auto pInstance = ToUPtr(new CComCharacterMoveIntent{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CComCharacterMoveIntent");
		return nullptr;
	}

	return pInstance;
}

void CComCharacterMoveIntent::Free()
{
	CComponent::Free();
}

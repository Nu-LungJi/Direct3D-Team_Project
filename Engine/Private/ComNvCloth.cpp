#include "pch.h"
#include "ComNvCloth.h"

#include "GameInstance.h"

NS_USING(Engine)

CComNvCloth::CComNvCloth()
{
}

CComNvCloth::CComNvCloth(
	const CComNvCloth& Prototype)
	: CComponent{ Prototype }
{
}

CComNvCloth::~CComNvCloth()
{
}

HRESULT CComNvCloth::Initialize(void* pArg)
{
	const auto* pDesc = static_cast<const DESC*>(pArg);
	if (!pDesc ||
		FAILED(CComponent::Initialize(pArg)))
	{
		return E_INVALIDARG;
	}

	NVCLOTH_FABRIC_HANDLE hFabric{};
	if (FAILED(CGameInstance::Get().CreateNvClothFabric(
		pDesc->tFabric,
		hFabric)))
	{
		return E_FAIL;
	}

	NVCLOTH_CLOTH_DESC tClothDesc =
		pDesc->tCloth;
	tClothDesc.hFabric = hFabric;
	tClothDesc.vecPositions =
		pDesc->tFabric.vecPositions;
	tClothDesc.vecInverseMasses =
		pDesc->tFabric.vecInverseMasses;

	NVCLOTH_CLOTH_HANDLE hCloth{};
	if (FAILED(CGameInstance::Get().CreateNvCloth(
		tClothDesc,
		hCloth)))
	{
		CGameInstance::Get().ReleaseNvClothFabric(
			hFabric);
		return E_FAIL;
	}

	m_hFabric = hFabric;
	m_hCloth = hCloth;
	m_iParticleCount =
		pDesc->tFabric.vecPositions.size();
	return S_OK;
}

_bool CComNvCloth::IsValid() const
{
	return static_cast<_bool>(m_hFabric) &&
		static_cast<_bool>(m_hCloth);
}

NVCLOTH_FABRIC_HANDLE
CComNvCloth::GetFabricHandle() const
{
	return m_hFabric;
}

NVCLOTH_CLOTH_HANDLE
CComNvCloth::GetClothHandle() const
{
	return m_hCloth;
}

size_t CComNvCloth::GetParticleCount() const
{
	return m_iParticleCount;
}

_bool CComNvCloth::GetParticles(
	std::vector<_float3>& OutParticles) const
{
	OutParticles.clear();
	if (!m_hCloth)
		return false;

	return CGameInstance::Get().GetNvClothParticles(
		m_hCloth,
		OutParticles);
}

_bool CComNvCloth::GetGpuParticleView(
	NVCLOTH_GPU_PARTICLE_VIEW& OutView) const
{
	OutView = {};
	if (!m_hCloth)
		return false;

	return CGameInstance::Get().
		GetNvClothGpuParticleView(
			m_hCloth,
			OutView);
}

_bool CComNvCloth::SetSimulationTransform(
	const _float3& vTranslation,
	const _float4& vRotation,
	_bool bTeleport)
{
	if (!m_hCloth)
		return false;

	return CGameInstance::Get().SetNvClothTransform(
		m_hCloth,
		vTranslation,
		vRotation,
		bTeleport);
}

_bool CComNvCloth::SetCollisions(
	const NVCLOTH_COLLISION_DESC& Desc)
{
	if (!m_hCloth)
		return false;

	return CGameInstance::Get().SetNvClothCollisions(
		m_hCloth,
		Desc);
}

void CComNvCloth::UpdateGUI()
{
	CComponent::UpdateGUI();
	ImGui::PushID(this);
	ImGui::Text(
		"Runtime: %s",
		IsValid() ? "valid" : "invalid");
	ImGui::Text(
		"Fabric Handle: %llu",
		static_cast<unsigned long long>(
			m_hFabric.iValue));
	ImGui::Text(
		"Cloth Handle: %llu",
		static_cast<unsigned long long>(
			m_hCloth.iValue));
	ImGui::Text(
		"Particles: %zu",
		m_iParticleCount);
	ImGui::PopID();
}

void CComNvCloth::ReleaseRuntime()
{
	if (m_hCloth)
	{
		CGameInstance::Get().ReleaseNvCloth(
			m_hCloth);
		m_hCloth = {};
	}

	if (m_hFabric)
	{
		CGameInstance::Get().ReleaseNvClothFabric(
			m_hFabric);
		m_hFabric = {};
	}

	m_iParticleCount = 0;
}

UPtr<CComNvCloth> CComNvCloth::Create()
{
	auto pInstance = ToUPtr(new CComNvCloth{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Create: CComNvCloth");
		return nullptr;
	}

	return pInstance;
}

UPtr<CPrototype> CComNvCloth::Clone(void* pArg)
{
	auto pInstance =
		ToUPtr(new CComNvCloth{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Clone: CComNvCloth");
		return nullptr;
	}

	return pInstance;
}

void CComNvCloth::Free()
{
	ReleaseRuntime();
	CComponent::Free();
}

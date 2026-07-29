#include "pch.h"
#include "Player_Magic_Bullet.h"
#include "Client_Resources.h"
#include "Trail_CPU.h"

NS_USING(Client)

CPlayer_Magic_Bullet::CPlayer_Magic_Bullet()
	: CGameObject{}
{
}

CPlayer_Magic_Bullet::~CPlayer_Magic_Bullet()
{
}

void CPlayer_Magic_Bullet::UpdateGUI()
{
	CGameObject::UpdateGUI();


}

HRESULT CPlayer_Magic_Bullet::InitializePrototype(void* pArg)
{
	return S_OK;
}

HRESULT CPlayer_Magic_Bullet::Initialize(void* pArg)
{
	if (nullptr == pArg)
		return E_FAIL;

	const auto pDesc = static_cast<const MAGIC_BULLET_DESC*>(pArg);
	if (FAILED(CGameObject::Initialize(pArg)))
		return E_FAIL;

	m_vStartPosition = pDesc->vStartPosition;
	m_vEndPosition = pDesc->vEndPosition;
	m_fSpeed = pDesc->fSpeed;

	BuildSpline(pDesc->fCurveHeight, pDesc->iSampleCount);
	if (m_Splines.empty())
		return E_FAIL;

	GetTransform().SetPosition(m_Splines.front());

	return S_OK;
}

void CPlayer_Magic_Bullet::PriorityUpdate(E::_float fTimeDelta)
{
}

void CPlayer_Magic_Bullet::Update(E::_float fTimeDelta)
{
	if (m_Splines.size() < 2 || m_iSplineIndex >= m_Splines.size() - 1)
		return;

	_float fRemainDistance = m_fSpeed * fTimeDelta;

	while (fRemainDistance > 0.f && m_iSplineIndex < m_Splines.size() - 1)
	{
		const _vector vCurrent = XMLoadFloat3(&m_Splines[m_iSplineIndex]);
		const _vector vNext = XMLoadFloat3(&m_Splines[m_iSplineIndex + 1]);
		const _float fSegmentLength = XMVectorGetX(XMVector3Length(vNext - vCurrent));
		const _float fSegmentRemain = fSegmentLength - m_fDistanceOnSegment;

		if (fSegmentLength <= 0.0001f)
		{
			++m_iSplineIndex;
			m_fDistanceOnSegment = 0.f;
			continue;
		}

		if (fRemainDistance >= fSegmentRemain)
		{
			fRemainDistance -= fSegmentRemain;
			++m_iSplineIndex;
			m_fDistanceOnSegment = 0.f;
			GetTransform().SetPosition(m_Splines[m_iSplineIndex]);
		}
		else
		{
			m_fDistanceOnSegment += fRemainDistance;
			const _float fRatio = m_fDistanceOnSegment / fSegmentLength;
			GetTransform().SetPosition(XMVectorLerp(vCurrent, vNext, fRatio));
			fRemainDistance = 0.f;
		}
	}
	{
		_float3 vstart, vend;

		vstart = _float3(m_pComTransform->GetPosition().x, m_pComTransform->GetPosition().y + 0.4f, m_pComTransform->GetPosition().z);
		vend = _float3(m_pComTransform->GetPosition().x, m_pComTransform->GetPosition().y - 0.4f, m_pComTransform->GetPosition().z);
		CGameInstance::Get().AddTrailPoint("PlayerAttackTrail_CPU", "PlayerAttackTrail_CPU", vstart, vend);
	}

	if (m_iSplineIndex >= m_Splines.size() - 1)
		SetPendingDestroy();
}

void CPlayer_Magic_Bullet::LateUpdate(E::_float fTimeDelta)
{
	GetTransform().Update();

	auto matrix = XMLoadFloat4x4(m_pComTransform->GetWorldMatrix());

	auto cachedCol = CGameInstance::Get().GetDbgLineRender()->GetColor();
	auto cachedDepth = CGameInstance::Get().GetDbgLineRender()->GetDepthMode();
	CGameInstance::Get().GetDbgLineRender()->SetColor({ 1.f, 0.f, 0.f, 1.f });
	CGameInstance::Get().GetDbgLineRender()->SetDepthTest(false);
	CGameInstance::Get().GetDbgLineRender()->AddSphere(0.1f, matrix);
	CGameInstance::Get().GetDbgLineRender()->SetColor(cachedCol);
	CGameInstance::Get().GetDbgLineRender()->SetDepthMode(cachedDepth);
}

HRESULT CPlayer_Magic_Bullet::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
	return S_OK;
}

void CPlayer_Magic_Bullet::BuildSpline(_float fCurveHeight, uint32_t iSampleCount)
{
	m_Splines.clear();
	m_iSplineIndex = 0;
	m_fDistanceOnSegment = 0.f;

	iSampleCount = std::max(iSampleCount, 3u);

	const _vector vStart = XMLoadFloat3(&m_vStartPosition);
	const _vector vEnd = XMLoadFloat3(&m_vEndPosition);

	_vector vForward = vEnd - vStart;
	const _float fDistance = XMVectorGetX(XMVector3Length(vForward));

	if (fDistance <= 0.0001f)
		return;

	vForward = XMVector3Normalize(vForward);

	// 진행 방향과 수직인 축 생성
	_vector vRight = XMVector3Cross(XMVectorSet(0.f, 1.f, 0.f, 0.f), vForward);

	if (XMVectorGetX(XMVector3LengthSq(vRight)) <= 0.0001f)
		vRight = XMVectorSet(1.f, 0.f, 0.f, 0.f);
	else
		vRight = XMVector3Normalize(vRight);

	const _vector vUp = XMVector3Normalize(XMVector3Cross(vForward, vRight));

	// 발사할 때 한 번만 랜덤 결정
	const _float fPhase = XMConvertToRadians((_float)(rand() % 360));

	const _float fFrequency = 1.5f + (_float)(rand() % 150) / 100.f;

	const _float fRightWeight = (rand() % 2 == 0) ? 1.f : -1.f;

	m_Splines.reserve(iSampleCount + 1);

	for (uint32_t i = 0; i <= iSampleCount; ++i)
	{
		const _float t = static_cast<_float>(i) / iSampleCount;

		// 기본 직선 위치
		_vector vPosition = XMVectorLerp(vStart, vEnd, t);

		// 시작점과 도착점에서 흔들림이 반드시 0이 되게 함
		const _float fEnvelope = std::sin(XM_PI * t);

		const _float fWave = std::sin(XM_2PI * fFrequency * t + fPhase);

		const _float fSecondWave = std::sin(XM_2PI * (fFrequency * 0.7f) * t + fPhase * 0.5f);

		vPosition += vRight * fWave * fCurveHeight * fEnvelope * fRightWeight;

		vPosition += vUp * fSecondWave * fCurveHeight * 0.5f * fEnvelope;

		_float3 vStoredPosition{};
		XMStoreFloat3(&vStoredPosition, vPosition);
		m_Splines.push_back(vStoredPosition);
	}
}

E::UPtr<CPlayer_Magic_Bullet> CPlayer_Magic_Bullet::Create()
{
	auto pInstance = E::ToUPtr(new CPlayer_Magic_Bullet{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CPlayer_Magic_Bullet");
		return nullptr;
	}
	return pInstance;
}

E::UPtr<E::CPrototype> CPlayer_Magic_Bullet::Clone(void* pArg)
{
	auto pInstance = E::ToUPtr(new CPlayer_Magic_Bullet{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CPlayer_Magic_Bullet");
		return nullptr;
	}

	return pInstance;
}

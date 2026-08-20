#include "pch.h"
#include "FlightRing.h"
#include "Client_Defines.h"

NS_USING(Client)

CFlightRing::CFlightRing()
{
}

CFlightRing::~CFlightRing()
{
}

void CFlightRing::UpdateGUI()
{
}

HRESULT CFlightRing::InitializePrototype(void* pArg)
{
	return S_OK;
}

HRESULT CFlightRing::Initialize(void* pArg)
{
	return S_OK;
}

void CFlightRing::LateUpdate(E::_float fTimeDelta)
{
	CGameInstance::Get().AddRenderObject(RENDERGROUP::NONBLEND, this);
}

HRESULT CFlightRing::Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx)
{

	return S_OK;
}

_bool CFlightRing::PassCheck(CHandle hPlayer)
{
	CGameObject* pPlayer = CGameInstance::Get().GetGameObjectByHandle(hPlayer);
	if (pPlayer == nullptr)
		return false;

	m_curPlayerPos = pPlayer->GetTransform().GetPosition();

	XMVECTOR vPrevPos = XMLoadFloat3(&m_prevPlayerPos);
	XMVECTOR vCurrPos = XMLoadFloat3(&m_curPlayerPos);

	// 두 벡터가 완전히 같은지 비교 (결과를 불리언으로 반환)
	if (XMVector3Equal(vPrevPos, vCurrPos))
		return false;

	// 링의 평면 정보 (중심점과 Look 벡터)
	_float3 ringPos = GetTransform().GetPosition();

	XMVECTOR vRingPos = XMLoadFloat3(&ringPos);
	XMVECTOR vRingNormal = GetTransform().GetState(STATE::LOOK);
	vRingNormal = XMVector3Normalize(vRingNormal);

	// 평면과의 거리 계산 (Plane Distance: Dot(Pos - RingPos, Normal))
	// XMVector3Dot의 결과는 x, y, z, w 모든 컴포넌트에 같은 값이 복제되므로 XMVectorGetX로 꺼냄
	XMVECTOR vPrevDistVec = XMVector3Dot(XMVectorSubtract(vPrevPos, vRingPos), vRingNormal);
	XMVECTOR vCurrDistVec = XMVector3Dot(XMVectorSubtract(vCurrPos, vRingPos), vRingNormal);

	float prevDist = XMVectorGetX(vPrevDistVec);
	float currDist = XMVectorGetX(vCurrDistVec);

	// 부호가 다르다면 평면을 가로지른 것
	if (prevDist * currDist < 0.f)
	{
		// 선분과 평면의 교점 계산
		float t = prevDist / (prevDist - currDist);
		XMVECTOR vIntersectPos = XMVectorAdd(vPrevPos, XMVectorScale(XMVectorSubtract(vCurrPos, vPrevPos), t));

		// 교점에서 링 중심까지의 거리 측정
		XMVECTOR vDistToCenterVec = XMVector3Length(XMVectorSubtract(vIntersectPos, vRingPos));
		float distanceToCenter = XMVectorGetX(vDistToCenterVec);


		if (distanceToCenter <= m_fRingRadius)
		{
			m_prevPlayerPos = m_curPlayerPos;
			m_bCheckComplete = true;
			return true; // 통과 성공
		}
	}

	m_prevPlayerPos = m_curPlayerPos;
	return false;
}

E::UPtr<CFlightRing> CFlightRing::Create()
{
	auto pInstance = E::ToUPtr(new CFlightRing{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CFlightRing");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CFlightRing::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CFlightRing{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CFlightRing");
		return nullptr;
	}

	return pInstance;
}

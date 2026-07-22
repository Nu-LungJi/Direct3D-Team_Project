#include "pch.h"
#include "UIObject.h"
#include "GameInstance.h"

NS_USING(Engine)

CUIObject::CUIObject()
{
}

CUIObject::~CUIObject()
{
}

void CUIObject::UpdateGUI()
{
	CGameObject::UpdateGUI();
}

HRESULT CUIObject::Initialize(void* pArg)
{
	auto		pDesc = static_cast<UIOBJECT_DESC*>(pArg);

	m_UIINFO.fX = pDesc->fX;
	m_UIINFO.fY = pDesc->fY;
	m_UIINFO.SizeX = pDesc->fSizeX;
	m_UIINFO.SizeY = pDesc->fSizeY;
	m_UIINFO.Alpha = pDesc->fAlpha;
	m_UIINFO.Weight = pDesc->ResWeight;
	m_UIINFO.Restag = pDesc->ResTag;
	m_UIINFO.Name = pDesc->Name;

	if (FAILED(CGameObject::Initialize(pArg)))
		return E_FAIL;

	CalcUICoord();

	m_CurrentState = UI_STATE::APPEAR;

	return S_OK;
}

void CUIObject::Update(_float fTimeDelta)
{
	// 부모가 있다면 local변수
	if (std::nullopt != m_pParent)
	{
		CUIObject* parentUI = E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(*m_pParent);
		UI_INFO& parentInfo = parentUI->GetUIInfo();

		m_ScaleRatio = parentUI->GetScaleRatio();

		// 1. 부모의 Pivot(회전 중심) 가져오기
		// (만약 에러가 난다면 CUIObject에 _float2 GetPivot() const { return m_vPivot; } 를 추가해 주세요)
		_float2 parentPivot = parentUI->GetPivot();

		// 2. 부모의 회전 중심점(Pivot)으로부터 자식이 얼만큼 떨어져 있는지(거리 벡터) 계산
		_float distanceX = m_UIINFO.LocalX - parentPivot.x;
		_float distanceY = m_UIINFO.LocalY - parentPivot.y;

		// 3. 부모의 각도 변환 및 🌟Y축 반전 보정🌟
		_float parentRad = XMConvertToRadians(parentInfo.Rot);
		_float cosR = cosf(parentRad);

		// CalcUICoord(Y 상단)와 Update(Y 하단)의 회전 방향을 일치시키기 위해 
		// 각도를 반대로(-parentRad) 주어 sin의 부호를 뒤집습니다!
		_float sinR = sinf(-parentRad);

		// 4. 피벗을 기준으로 거리 벡터를 회전
		_float rotatedX = (distanceX * cosR) - (distanceY * sinR);
		_float rotatedY = (distanceX * sinR) + (distanceY * cosR);

		// 5. 최종 위치 = 부모 기준위치 + 부모 피벗 오프셋 + 회전된 거리
		m_UIINFO.fX = parentInfo.fX + ((parentPivot.x + rotatedX) * m_ScaleRatio);
		m_UIINFO.fY = parentInfo.fY + ((parentPivot.y + rotatedY) * m_ScaleRatio);

		// 6. 자식의 최종 회전각과 알파값 갱신
		m_UIINFO.Rot = parentInfo.Rot + m_UIINFO.LocalRot;
		m_UIINFO.Alpha = parentInfo.Alpha * m_UIINFO.AlphaRatio;

		CalcUICoord();
	}
	else
	{
		m_vPivot = { 0.f, 0.f };
	}
}

void CUIObject::LateUpdate(_float fTimeDelta)
{
	if (!m_isActive)
		return;
}

void CUIObject::PlayEffect(uint32_t uiState)
{
}

void CUIObject::ClearEffectTweens()
{
	m_pComTween->ClearTweens();
}

void CUIObject::DeleteChild(CHandle childHandle)
{
	m_vChildren.erase(
		std::remove(m_vChildren.begin(), m_vChildren.end(), childHandle),
		m_vChildren.end());
}

void CUIObject::CalcUICoord()
{
	//auto clientSize = CGameInstance::Get().GetClientScreenSize();
	//auto clientWidth = clientSize.x;
	//auto clientHeight = clientSize.y;
	//if (m_pComTransform == nullptr)
	//	return;
	//GetTransform().SetScale(E::_float3{ m_UIINFO.SizeX * m_ScaleRatio, m_UIINFO.SizeY * m_ScaleRatio, 1.f });
	//auto x = m_UIINFO.fX - clientWidth * 0.5f;
	//auto y = -m_UIINFO.fY + clientHeight * 0.5f;
	//
	//GetTransform().SetPosition(XMVectorSet(x, y, GetTransform().GetPosition().z, 1.f));
	//
	//
	//GetTransform().SetRotation({0.f, 0.f, 1.f}, m_UIINFO.Rot);

	auto clientSize = CGameInstance::Get().GetClientScreenSize();
	auto clientWidth = clientSize.x;
	auto clientHeight = clientSize.y;

	if (m_pComTransform == nullptr) return;

	// 크기 세팅
	GetTransform().SetScale(E::_float3{ m_UIINFO.SizeX * m_ScaleRatio, m_UIINFO.SizeY * m_ScaleRatio, 1.f });

	// 1. 화면 중앙이 (0,0)인 직교 좌표계로 변환 (기존 코드)
	_float x = m_UIINFO.fX - clientWidth * 0.5f;
	_float y = -m_UIINFO.fY + clientHeight * 0.5f; // DirectX는 화면 위쪽이 +Y

	// 2. 피벗(Pivot) 오프셋 보정 연산
	// m_vPivot이 픽셀 단위의 거리(오프셋)라고 가정합니다.
	// 화면 좌표계에 맞춰 피벗의 Y값 부호도 반전시킵니다.
	_float pivotX = m_vPivot.x * m_ScaleRatio;
	_float pivotY = -m_vPivot.y * m_ScaleRatio;

	// 3. UI 자신의 회전각(Rot) 라디안 변환
	_float rad = XMConvertToRadians(m_UIINFO.Rot);
	_float cosR = cosf(rad);
	_float sinR = sinf(rad);

	// 4. 피벗 점을 기준으로 회전시켰을 때 이동해야 할 위치 계산
	_float rotatedPivotX = pivotX * cosR - pivotY * sinR;
	_float rotatedPivotY = pivotX * sinR + pivotY * cosR;

	// 5. 최종 앵커 위치 보정 
	// (기존 위치에 원래 피벗 거리를 더하고, 회전된 피벗 거리만큼 빼서 위치를 보정합니다)
	x = x + pivotX - rotatedPivotX;
	y = y + pivotY - rotatedPivotY;

	// 6. 보정된 위치와 회전값을 Transform에 꽂아주기
	GetTransform().SetPosition(XMVectorSet(x, y, GetTransform().GetPosition().z, 1.f));
	GetTransform().SetRotation({ 0.f, 0.f, 1.f }, m_UIINFO.Rot);
}

void CUIObject::SetChildPivot()
{
	for (auto cHandle : m_vChildren)
	{
		if (nullptr != E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(cHandle))
		{
			Engine::CUIObject* pUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(cHandle);
			pUI->SetPivot(m_vPivot);
		}
			
	}
}

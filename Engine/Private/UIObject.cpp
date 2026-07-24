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
		if (m_bWorldSpace)
		{
			// 1. 3D UI 크기 조절 (픽셀 -> 미터 단위)
			_float scaleFactor = 0.01f; // 에디터의 m_fWorldScaleFactor를 연동하시면 더 좋습니다.
			GetTransform().SetScale(E::_float3{ m_UIINFO.SizeX * scaleFactor, m_UIINFO.SizeY * scaleFactor, 1.f });

			// 2. 자식 UI일 경우: 부모 행렬 추적 및 3D 로컬 좌표 세팅
			if (std::nullopt != m_pParent)
			{
				CUIObject* parentUI = E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(*m_pParent);

				// [핵심 1] 엔진의 CComTransform 계층 구조 연결 (부모의 최종 행렬을 내 부모 행렬로 세팅)
				GetTransform().SetParentWorldMatrix(*parentUI->GetTransform().GetCombinedWorldMatrix());

				// [핵심 2] 2D 픽셀 오프셋을 3D 로컬 좌표로 변환
				// UI 스크린 좌표는 Y가 아래로 갈수록 증가하지만, 3D 월드는 Y가 위로 갈수록 증가하므로 Y축 부호를 반전(-LocalY)시킵니다.
				_float local3DX = m_UIINFO.LocalX * scaleFactor;
				_float local3DY = -m_UIINFO.LocalY * scaleFactor;

				
				_float local3DZ = -0.001f * m_UIINFO.WeightOffset;

				GetTransform().SetPosition(XMVectorSet(local3DX, local3DY, local3DZ, 1.f));
			}
		}
		else
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
			m_UIINFO.Weight = parentInfo.Weight + m_UIINFO.WeightOffset;

			CalcUICoord();
		}

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

	if (m_bWorldSpace)
		E::CGameInstance::Get().AddRenderObject(E::RENDERGROUP::BLEND, this);
	else
		E::CGameInstance::Get().AddRenderObject(E::RENDERGROUP::UI, this);

	GetTransform().Update();
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
	auto clientSize = CGameInstance::Get().GetClientScreenSize();
	auto clientWidth = clientSize.x;
	auto clientHeight = clientSize.y;

	if (m_pComTransform == nullptr) return;

	GetTransform().SetScale(E::_float3{ m_UIINFO.SizeX * m_ScaleRatio, m_UIINFO.SizeY * m_ScaleRatio, 1.f });

	_float x = m_UIINFO.fX - clientWidth * 0.5f;
	_float y = -m_UIINFO.fY + clientHeight * 0.5f;

	_float pivotX = m_vPivot.x * m_ScaleRatio;
	_float pivotY = -m_vPivot.y * m_ScaleRatio;

	_float rad = XMConvertToRadians(m_UIINFO.Rot);
	_float cosR = cosf(rad);
	_float sinR = sinf(rad);

	_float rotatedPivotX = pivotX * cosR - pivotY * sinR;
	_float rotatedPivotY = pivotX * sinR + pivotY * cosR;

	x = x + pivotX - rotatedPivotX;
	y = y + pivotY - rotatedPivotY;

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

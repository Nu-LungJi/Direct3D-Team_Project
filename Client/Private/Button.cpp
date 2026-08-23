#include "pch.h"
#include "Button.h"
#include "GameInstance.h"
#include "CameraObject.h"
#include "Resources.h"
#include "UIManager.h"
#include "Client_Defines.h"
#include "EffectUI.h"
#include "ButtonComponent.h"
#include "TweenComponent.h"
#include "UIObject.h"
#include "Level_Defines.h"
#include "UI_Enums.h"
#include "VideoObject.h"

NS_USING(Client)

CButton::CButton()
{

}

CButton::~CButton()
{
	if (m_UIINFO.UIType == ETOUI(UI_TYPE::BUTTON))
	{
		if(nullptr != SafeGetOBJ(m_SpellDesc))
			PlayScaleAlphaDownDelete(m_SpellDesc);
		if (nullptr != SafeGetOBJ(m_SpellPaper))
			PlayScaleAlphaDownDelete(m_SpellPaper);
	}
}

HRESULT CButton::InitializePrototype(void* pArg)
{
	return S_OK;
}

HRESULT CButton::Initialize(void* pArg)
{
	auto		pDesc = static_cast<CUIObject::UIOBJECT_DESC*>(pArg);

	if (FAILED(CUIObject::Initialize(pDesc)))
		return E_FAIL;

	{
		/* Buffer */
		CComConstantBuffer::DESC Desc{};
		Desc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, "CB_PerUI" };
		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ConstantBuffer", "ComCBufferPerUI", &Desc, &m_pComCBufferPerUI)))
		{
			return E_FAIL;
		};

		/* Component */
		CComponent::DESC CDesc{};
		Desc.pGameObject = this;

		if (FAILED(AddComponentFromProto(ES_EngineProtoMajorType::UI, "Prototype_Component_ButtonUI", "Com_Button", &CDesc, &m_pComCButton)))
		{
			return E_FAIL;
		};

		if (FAILED(AddComponentFromProto(ES_EngineProtoMajorType::UI, "Prototype_Component_Tween", "Com_Tween", &CDesc, &m_pComTween)))
		{
			return E_FAIL;
		};
	}

	m_UIINFO.UIType = ETOUI(UI_TYPE::BUTTON);
	m_UIINFO.AlphaRatio = 0.f;

	return S_OK;
}

void CButton::PriorityUpdate(E::_float fTimeDelta)
{
}

void CButton::Update(E::_float fTimeDelta)
{
	_float2 mousePos = E::CGameInstance::Get().GetMousePos();

	if (!m_isActive)
		return;

	CUIObject::Update(fTimeDelta);

	m_pComCButton->CheckPixelPerfectCollision(mousePos, true);

	if (m_bMouseTracking)
	{
		m_UIINFO.fX = mousePos.x;
		m_UIINFO.fY = mousePos.y;
		CalcUICoord();
	}

	if (m_pComTween != nullptr)
	{
		m_pComTween->Tick(fTimeDelta);
	}

	if (m_IsHover)
	{
		m_HoverTimer += fTimeDelta;
	}
}

void CButton::LateUpdate(E::_float fTimeDelta)
{
	if (!m_isActive)
		return;

	CUIObject::LateUpdate(fTimeDelta);

	//E::CGameInstance::Get().AddRenderObject(E::RENDERGROUP::UI, this);
	//GetTransform().Update();
}

HRESULT CButton::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
	std::string currentLevel = _string("LEVEL_") + MagicEnumToStringView(static_cast<LEVEL>(E::CGameInstance::Get().GetCurrentLevelID())).data();

	//VS_QuadTex
	const auto& vs = E::CGameInstance::Get().GetResourceFirst<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_QuadTexUI");
	const auto& ps = E::CGameInstance::Get().GetResourceFirst<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_QuadTexUI");
	const auto& viBuffer = E::CGameInstance::Get().GetResourceFirst<E::CResQuadTexBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "VIBuffer_QuadTex");

	pContext->IASetInputLayout(vs->GetInputLayout().Get());
	pContext->VSSetShader(vs->GetVertexShader().Get(), nullptr, 0);
	pContext->PSSetShader(ps->GetPixelShader().Get(), nullptr, 0);

	ID3D11Buffer* vertexBuffers[] = {
			viBuffer->GetVertexBuffer().Get()
	};
	uint32_t strides[] = {
		viBuffer->GetVertexStride()
	};
	uint32_t offsets[] = {
		0
	};
	pContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
	pContext->IASetIndexBuffer(viBuffer->GetIndexBuffer().Get(), viBuffer->GetIndexFormat(), 0);
	pContext->IASetPrimitiveTopology(viBuffer->GetPrimitiveType());

	{
		E::CB_PER_UI perUI{};
		perUI.texCoord = { 0.f, 0.f };
		perUI.uvSize = { 0.f, 0.f };
		perUI.color = { m_UIINFO.Color.x, m_UIINFO.Color.y, m_UIINFO.Color.z, m_UIINFO.Alpha };
		perUI.uvFlip = { m_UIINFO.FlipX ? 1.f : 0.f, m_UIINFO.FlipY ? 1.f : 0.f };

		if (FAILED(m_pComCBufferPerUI->MapDiscard(pContext, &perUI, sizeof(perUI))))
		{
			return E_FAIL;
		}
		pContext->VSSetConstantBuffers(ETOUI(B_SLOTNUMBER::UI), 1, m_pComCBufferPerUI->GetAdressOfBuffer());
		pContext->PSSetConstantBuffers(ETOUI(B_SLOTNUMBER::UI), 1, m_pComCBufferPerUI->GetAdressOfBuffer());
	}

	{
		//auto pUICam = E::CGameInstance::Get().GetActiveUICamera();
		{
			auto pCbPerObject = E::CGameInstance::Get().GetResourceFirst<E::CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "CB_PerObject");
			D3D11_MAPPED_SUBRESOURCE mappedSubResource;
			if (SUCCEEDED(pContext->Map(pCbPerObject->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubResource)))
			{

				E::CB_PER_OBJECT cbPerObject{};
				cbPerObject.matWorld = *GetTransform().GetWorldMatrix();
				XMStoreFloat4x4(&cbPerObject.matWVP, GetTransform().GetLoadedWorldMatrix() * ctx.matProj);

				memcpy(mappedSubResource.pData, &cbPerObject, sizeof(cbPerObject));
				pContext->Unmap(pCbPerObject->GetCBuffer().Get(), 0);
			}
			pContext->VSSetConstantBuffers(ETOUI(B_SLOTNUMBER::PER_OBJECT), 1, pCbPerObject->GetCBuffer().GetAddressOf());
			pContext->PSSetConstantBuffers(ETOUI(B_SLOTNUMBER::PER_OBJECT), 1, pCbPerObject->GetCBuffer().GetAddressOf());
		}
	}

	{
		const auto& srv = E::CGameInstance::GetConst().GetResourceFirst<E::CResTexture2D>(currentLevel, m_UIINFO.Restag);
		pContext->PSSetShaderResources(0, 1, srv->GetSRV().GetAddressOf());
	}

	pContext->DrawIndexed(viBuffer->GetNumIndices(), 0, 0);



	return S_OK;
}

void CButton::PlayEffect(uint32_t uiState)
{
	if (m_pComTween == nullptr)
		return;

	if (uiState & ETOUI(UI_STATE::APPEAR))
	{
		ClearEffectTweens();
		if (Appear) Appear(this);
	}

	if (uiState & ETOUI(UI_STATE::DISAPPEAR))
	{
		ClearEffectTweens();
		if (Disappear) Disappear(this);
	}

	if (m_bInputLocked)
		return;

	if (uiState & ETOUI(UI_STATE::ENTER))
	{
		if (OnHoverEnter) {
			ClearEffectTweens();
			OnHoverEnter(this);
		}

		if (m_Effect_Hovered_Handle != std::nullopt && 
			(nullptr != E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*m_Effect_Hovered_Handle)))
		{
			CUIObject* pHoverUI = E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(*m_Effect_Hovered_Handle);
			ClearHoveredEffect();
			pHoverUI->OnHoverEnter(pHoverUI);
		}

		if (m_UIINFO.UIType == ETOUI(UI_TYPE::BUTTON))
		{
			m_IsHover = true;
			std::vector<CHandle> vSpellDesc = GET_SINGLE(UIManager)->LoadPrefab(m_DescJsonName);
			m_SpellDesc = vSpellDesc[0];
			m_SpellPaper = vSpellDesc[1];

			static_cast<CVideoObject*>(SafeGetOBJ(SafeGetOBJ(m_SpellPaper)->GetChildren()[0]))->SetPath(m_VideoPath);
		}

		E::CGameInstance::Get().GetSoundManager()->Play2D("./Resources/SampleClient/Sound/UI/ButtonSelect.wav", SOUND_PLAY_DESC{
		.sBusID = SOUND_BUS::UI,
		.fVolume = 1.f,
		.fPitch = 1.f,
		.iPriority = 64,
		.bLoop = false
			});
	}

	if (uiState & ETOUI(UI_STATE::EXIT))
	{
		if (OnHoverExit) {
			ClearEffectTweens();
			OnHoverExit(this);
		}

		if (m_Effect_Hovered_Handle != std::nullopt &&
			(nullptr != E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*m_Effect_Hovered_Handle)))
		{
			CUIObject* pHoverUI = E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(*m_Effect_Hovered_Handle);
			ClearHoveredEffect();
			pHoverUI->OnHoverExit(pHoverUI);
		}
		
		if (m_UIINFO.UIType == ETOUI(UI_TYPE::BUTTON))
		{
			PlayScaleAlphaDownDelete(m_SpellDesc, 0.1f);
			PlayScaleAlphaDownDelete(m_SpellPaper);
		}
	}

	if (uiState & ETOUI(UI_STATE::CLICK))
	{
		
		if (OnClicked) {
			ClearEffectTweens();
			OnClicked(this);
		}
		if (OnClickedAction) OnClickedAction(m_UIINFO.Restag);


		if (m_Effect_Clicked_Handle != std::nullopt &&
			(nullptr != E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*m_Effect_Clicked_Handle)))
		{
			CEffectUI* pClickUI = E::CGameInstance::Get().GetGameObjectByHandleT<CEffectUI>(*m_Effect_Clicked_Handle);
			ClearClickEffect();
			pClickUI->OnClicked(pClickUI);
		}
	}
}

void CButton::ClearEffectTweens()
{
	m_pComTween->ClearTweens();
}

void CButton::ClearHoveredEffect()
{
	if (m_Effect_Hovered_Handle != std::nullopt &&
		(nullptr != E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*m_Effect_Hovered_Handle)))
	{
		CUIObject* pHoverUI = E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(*m_Effect_Hovered_Handle);

		pHoverUI->GetTweenCom()->ClearTweens();
	}
}

void CButton::ClearClickEffect()
{
	if (m_Effect_Clicked_Handle != std::nullopt &&
		(nullptr != E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*m_Effect_Clicked_Handle)))
	{
		CUIObject* pClickUI = E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(*m_Effect_Clicked_Handle);

		pClickUI->GetTweenCom()->ClearTweens();
	}
}

void CButton::PlayScaleAlphaDownDelete(CHandle pHandle, _float delay)
{
	CUIObject* pBtn = SafeGetOBJ(pHandle);
	auto pTween = pBtn->GetTweenCom();

	pBtn->SetInputLcok(true);

	_float scaleRatio = pBtn->GetScaleRatio();
	_float Alpah = pBtn->GetAlpha();

	pTween->PlayTween(scaleRatio, 0.f, 0.2f,
		[pBtn](float currentValue) {
			pBtn->SetScaleRatio(currentValue);
			pBtn->CalcUICoord();
		}, [pHandle]() {
			if (auto pObj = GetSafeUI(pHandle)) GET_SINGLE(UIManager)->DeleteUIRecursive(pHandle);
			}, EEaseType::EaseOutQuad, delay);

		pTween->PlayTween(Alpah, 0.f, 0.1f,
			[pBtn](float currentValue) {
				pBtn->SetAlpha(currentValue);
				pBtn->CalcUICoord();
			}, nullptr, EEaseType::EaseOutQuad, delay);
}

E::CUIObject* CButton::SafeGetOBJ(CHandle pHandle)
{
	if (nullptr != E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(pHandle))
		return E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(pHandle);

	return nullptr;
}

E::UPtr<CButton> CButton::Create()
{
	auto pInstance = E::ToUPtr(new CButton{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CButton");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CButton::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CButton{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CButton");
		return nullptr;
	}

	return pInstance;
}

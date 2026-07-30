#include "pch.h"
#include "HPBar.h"
#include "GameInstance.h"
#include "CameraObject.h"
#include "Resources.h"
#include "UIManager.h"
#include "Client_Defines.h"
#include "Level_Defines.h"

NS_USING(Client)

CHPBar::CHPBar()
{
}

CHPBar::~CHPBar()
{
}

HRESULT CHPBar::InitializePrototype(void* pArg)
{
	return S_OK;
}

HRESULT CHPBar::Initialize(void* pArg)
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

		if (FAILED(AddComponentFromProto(ES_EngineProtoMajorType::UI, "Prototype_Component_Tween", "Com_Tween", &CDesc, &m_pComTween)))
		{
			return E_FAIL;
		};


		if (FAILED(AddComponentFromProto(ES_EngineProtoMajorType::UI, "Prototype_Component_ButtonUI", "Com_Button", &CDesc, &m_pComCButton)))
		{
			return E_FAIL;
		};
	}

	m_UIINFO.UIType = ETOUI(UI_TYPE::HPBAR);

	const auto& srv = E::CGameInstance::GetConst().GetResourceFirst<E::CResTexture2D>("LEVEL_UIEDITOR", m_UIINFO.Restag);
	const D3D11_TEXTURE2D_DESC& texDesc = srv->GetTexture2DDesc();
	//m_UIINFO.SizeX = static_cast<float>(texDesc.Width);
	//m_UIINFO.SizeY = static_cast<float>(texDesc.Height);

	return S_OK;
}

void CHPBar::PriorityUpdate(E::_float fTimeDelta)
{
	if (m_UIINFO.UIType == ETOUI(UI_TYPE::LEFTHPFILL))
		m_fFillDir = 0.f;
}

void CHPBar::Update(E::_float fTimeDelta)
{
	if (!m_isActive)
		return;

	// 디버깅용
	if (m_UIINFO.UIType == ETOUI(UI_TYPE::HPFILL))
	{
		if (CGameInstance::Get().KeyDown(DIK_8))
		{
			m_fcurrentFill -= 400.f;
			UpdateFill();
		}
	}
	else if (m_UIINFO.UIType == ETOUI(UI_TYPE::LEFTHPFILL))
	{
		if (CGameInstance::Get().KeyDown(DIK_9))
		{
			m_fcurrentFill -= 100.f;
			UpdateFill();
		}
	}

	CUIObject::Update(fTimeDelta);

	_float2 mousePos = E::CGameInstance::Get().GetMousePos();

	if (m_UIINFO.UIType == ETOUI(UI_TYPE::HPFILL) &&
		m_fcurrentFill <= 0.f && !m_bDead)
	{
		m_bDead = true;
		m_pComCButton->SetDisappear(true);

		for (auto child : m_vChildren)
		{
			CUIObject* pUi = CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(child);
			pUi->GetComponent<CButtonComponent>("Com_Button")->SetDisappear(true);
		}
	}

	m_pComCButton->CheckPixelPerfectCollision(mousePos, true);

	for (auto& pComponent : m_UIComponents)
	{
		pComponent->Update(fTimeDelta, mousePos);
	}

	if (m_pComTween != nullptr)
	{
		m_pComTween->Tick(fTimeDelta);
	}
}

void CHPBar::LateUpdate(E::_float fTimeDelta)
{
	if (!m_isActive)
		return;

	E::CGameInstance::Get().AddRenderObject(E::RENDERGROUP::UI, this);
	GetTransform().Update();
}

HRESULT CHPBar::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
	//std::string currentLevel = "LEVEL_UIEDITOR";
	std::string currentLevel = _string("LEVEL_") + MagicEnumToStringView(static_cast<LEVEL>(E::CGameInstance::Get().GetCurrentLevelID())).data();
	//VS_QuadTex
	const auto& vs = E::CGameInstance::Get().GetResourceFirst<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_9SliceUI");
	const auto& ps = E::CGameInstance::Get().GetResourceFirst<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_9SliceUI");
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
		const auto& srv = E::CGameInstance::GetConst().GetResourceFirst<E::CResTexture2D>(currentLevel, m_UIINFO.Restag);
		const D3D11_TEXTURE2D_DESC& texDesc = srv->GetTexture2DDesc();

		E::CB_PER_UI perUI{};
		perUI.texCoord = { 0.f, 0.f };
		perUI.uvSize = { m_fCurrentAmount, m_fFillDir };
		perUI.color = { m_UIINFO.Color.x, m_UIINFO.Color.y, m_UIINFO.Color.z, m_UIINFO.Alpha };
		perUI.texSize = { static_cast<float>(texDesc.Width), static_cast<float>(texDesc.Height) };
		perUI.quadSize = { m_UIINFO.SizeX, m_UIINFO.SizeY };
		perUI.margins = { 3.f, 5.f, 3.f, 5.f }; // left top right bottom

		if(m_UIINFO.Restag == "TEX_UI_T_TutorialMediaBorder")
			perUI.margins = { 97.f, 57.f, 97.f, 57.f };
		else if (m_UIINFO.Restag == "TEX_UI_T_NotificationGoldLeafWShadow_Bottom_Flip")
			perUI.margins = { 53.f, 0.f, 0.f, 0.f };
		else if (m_UIINFO.Restag == "TEX_UI_T_ConversationDivider_4k")
			perUI.margins = { 38.f, 0.f, 38.f, 0.f };
		else if (m_UIINFO.Restag == "TEX_UI_T_ToolTipBack")
			perUI.margins = { 9.f, 0.f, 9.f, 0.f };
		else if (m_UIINFO.Restag == "TEX_UI_T_MenuTextButtonBorder_4K")
			perUI.margins = { 68.f, 0.f, 68.f, 0.f };

		if (FAILED(m_pComCBufferPerUI->MapDiscard(pContext, &perUI, sizeof(perUI))))
		{
			return E_FAIL;
		}
		pContext->VSSetConstantBuffers(ETOUI(B_SLOTNUMBER::UI), 1, m_pComCBufferPerUI->GetAdressOfBuffer());
		pContext->PSSetConstantBuffers(ETOUI(B_SLOTNUMBER::UI), 1, m_pComCBufferPerUI->GetAdressOfBuffer());
	}

	{
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

void CHPBar::PlayEffect(uint32_t uiState)
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
}

void CHPBar::UpdateFill()
{
	if (m_fMaxFill <= 0.f) return;

	float targetAmount = m_fcurrentFill / m_fMaxFill;

	targetAmount = std::clamp(targetAmount, 0.0f, 1.0f);

	auto pTween = GetTweenCom();
	if (!pTween)
	{
		m_fCurrentAmount = targetAmount;
		return;
	}

	if (m_fCurrentAmount == targetAmount)
		return;

	pTween->ClearTweens();
	pTween->PlayTween(m_fCurrentAmount, targetAmount, 0.1f, 
		[this](float currentValue) {
			m_fCurrentAmount = currentValue;
		},
		[this]() {
			// 트윈 종료 후 처리
		},
			EEaseType::EaseOutQuad,
			0.0f,
			false
			);
}

E::UPtr<CHPBar> CHPBar::Create()
{
	auto pInstance = E::ToUPtr(new CHPBar{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CHPBar");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CHPBar::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CHPBar{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CHPBar");
		return nullptr;
	}

	return pInstance;
}

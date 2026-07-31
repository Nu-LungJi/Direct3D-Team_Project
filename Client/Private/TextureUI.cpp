#include "pch.h"
#include "TextureUI.h"
#include "GameInstance.h"
#include "CameraObject.h"
#include "Resources.h"
#include "UIManager.h"
#include "Client_Defines.h"
#include "Level_Defines.h"
#include "UIController.h"

NS_USING(Client)

CTextureUI::CTextureUI()
{

}

CTextureUI::~CTextureUI()
{
}

HRESULT CTextureUI::InitializePrototype(void* pArg)
{


	return S_OK;
}

HRESULT CTextureUI::Initialize(void* pArg)
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

	m_UIINFO.UIType = ETOUI(UI_TYPE::TEXUI);

	return S_OK;
}

void CTextureUI::PriorityUpdate(E::_float fTimeDelta)
{

}

void CTextureUI::Update(E::_float fTimeDelta)
{
	_float2 mousePos = E::CGameInstance::Get().GetMousePos();

	if (!m_isActive)
		return;

	CUIObject::Update(fTimeDelta);

	//m_pComCButton->CheckPixelPerfectCollision(mousePos, true);
	//
	//if (m_bMouseTracking)
	//{
	//	m_UIINFO.fX = mousePos.x;
	//	m_UIINFO.fY = mousePos.y;
	//	CalcUICoord();
	//}

	m_UIINFO.Alpha;

	if (m_bWorldSpace)
	{
		E::_float scaleFactor = 0.01f;
		GetTransform().SetScale(E::_float3{ m_UIINFO.SizeX * scaleFactor, m_UIINFO.SizeY * scaleFactor, 1.f });
		// 캐릭터를 따라다녀야 한다면 여기서 SetPosition을 갱신
	}
	else
	{
		_float2 mousePos = E::CGameInstance::Get().GetMousePos();
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
	}

	if (m_UIINFO.UIType == ETOUI(UI_TYPE::SHORTCUT_ICON))
	{
		if (!E::CGameInstance::Get().MousePressing(MOUSEKEYSTATE::LB))
		{
			GET_SINGLE(UIManager)->DeleteUIRecursive(this->GetHandle());

			std::optional<CHandle> hController = GET_SINGLE(UIManager)->GetUIController();

			if (hController != std::nullopt &&
				nullptr != E::CGameInstance::Get().GetGameObjectByHandleT<CUIController>(*hController))
			{
				CUIController* pController = E::CGameInstance::Get().GetGameObjectByHandleT<CUIController>(*hController);

				//pController->SetTargetIcon(ETOUI(SPELL_TYPE::NONE));
			}
		}
	}
	else if (m_UIINFO.UIType == ETOUI(UI_TYPE::DISOLVE))
	{
		m_fAmount += fTimeDelta * 0.2f;
		m_fAmount = std::min(1.f, m_fAmount);
		m_UIINFO.Color = { 0.f, 0.f, 0.f };
		m_UIINFO.Alpha = 1.f;
	}
}

void CTextureUI::LateUpdate(E::_float fTimeDelta)
{
	if (!m_isActive)
		return;

	CUIObject::LateUpdate(fTimeDelta);

	//E::CGameInstance::Get().AddRenderObject(E::RENDERGROUP::UI, this);
	//GetTransform().Update();
}

HRESULT CTextureUI::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
	std::string currentLevel = _string("LEVEL_") + MagicEnumToStringView(static_cast<LEVEL>(E::CGameInstance::Get().GetCurrentLevelID())).data();

	const auto& viBuffer = E::CGameInstance::Get().GetResourceFirst<E::CResQuadTexBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "VIBuffer_QuadTex");
	auto vs = E::CGameInstance::Get().GetResourceFirst<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_QuadTexUI");
	auto ps = E::CGameInstance::Get().GetResourceFirst<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_QuadTexUI");

	if (m_UIINFO.UIType == ETOUI(UI_TYPE::DISOLVE))
	{
		vs = E::CGameInstance::Get().GetResourceFirst<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_DISOLVE");
		ps = E::CGameInstance::Get().GetResourceFirst<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_DISOLVE");
	}

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
		perUI.texCoord = { m_fAmount, 0.f };
		perUI.uvSize = { 0.f, 0.f };
		perUI.color = { m_UIINFO.Color.x, m_UIINFO.Color.y, m_UIINFO.Color.z, m_UIINFO.Alpha };

		if (FAILED(m_pComCBufferPerUI->MapDiscard(pContext, &perUI, sizeof(perUI))))
		{
			return E_FAIL;
		}
		pContext->VSSetConstantBuffers(7, 1, m_pComCBufferPerUI->GetAdressOfBuffer());
		pContext->PSSetConstantBuffers(7, 1, m_pComCBufferPerUI->GetAdressOfBuffer());
	}

	{
		//auto pUICam = E::CGameInstance::Get().GetActiveUICamera();
		{
			auto pCbPerObject = E::CGameInstance::Get().GetResourceFirst<E::CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "CB_PerObject");
			D3D11_MAPPED_SUBRESOURCE mappedSubResource;
			if (SUCCEEDED(pContext->Map(pCbPerObject->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubResource)))
			{

				E::CB_PER_OBJECT cbPerObject{};

				if (m_bWorldSpace)
				{
					_matrix world = GetTransform().GetLoadedWorldMatrix();
					_matrix matWVP = GetTransform().GetLoadedWorldMatrix() * ctx.matView * ctx.matProj;
					XMStoreFloat4x4(&cbPerObject.matWVP, matWVP);
				}
				else
				{
					_matrix matWVP = GetTransform().GetLoadedWorldMatrix() * ctx.matProj;
					XMStoreFloat4x4(&cbPerObject.matWVP, matWVP);
				}
				//cbPerObject.matWorld = *GetTransform().GetWorldMatrix();
				//XMStoreFloat4x4(&cbPerObject.matWVP, GetTransform().GetLoadedWorldMatrix() * ctx.matProj);

				memcpy(mappedSubResource.pData, &cbPerObject, sizeof(cbPerObject));
				pContext->Unmap(pCbPerObject->GetCBuffer().Get(), 0);
			}
			pContext->VSSetConstantBuffers(0, 1, pCbPerObject->GetCBuffer().GetAddressOf());
			pContext->PSSetConstantBuffers(0, 1, pCbPerObject->GetCBuffer().GetAddressOf());
		}
	}

	{
		auto& tmp = E::CGameInstance::Get();
		const auto& srv = E::CGameInstance::GetConst().GetResourceFirst<E::CResTexture2D>(currentLevel, m_UIINFO.Restag);
		pContext->PSSetShaderResources(0, 1, srv->GetSRV().GetAddressOf());
	}

	{
		if (m_UIINFO.UIType == ETOUI(UI_TYPE::DISOLVE))
		{
			const auto& baseSrv = E::CGameInstance::GetConst().GetResourceFirst<E::CResTexture2D>(currentLevel, "TEX_UI_T_HeaderHouseBack");
			const auto& disolveSrv = E::CGameInstance::GetConst().GetResourceFirst<E::CResTexture2D>(currentLevel, m_UIINFO.Restag);

			ID3D11ShaderResourceView* srvs[2] = {
				baseSrv->GetSRV().Get(),
				disolveSrv->GetSRV().Get(),
			};

			pContext->PSSetShaderResources(0, 2, srvs);
		}
	}

	pContext->DrawIndexed(viBuffer->GetNumIndices(), 0, 0);

	return S_OK;
}

void CTextureUI::PlayEffect(uint32_t uiState)
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
	}
	
	if (uiState & ETOUI(UI_STATE::EXIT))
	{
		if (OnHoverExit) {
			ClearEffectTweens();
			OnHoverExit(this);
		}
	}
	
	if (uiState & ETOUI(UI_STATE::CLICK))
	{
	
		if (OnClicked) {
			ClearEffectTweens();
			OnClicked(this);
		}
	}
}

E::UPtr<CTextureUI> CTextureUI::Create()
{
	auto pInstance = E::ToUPtr(new CTextureUI{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CTexUI");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CTextureUI::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CTextureUI{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CTextureUI");
		return nullptr;
	}

	return pInstance;
}

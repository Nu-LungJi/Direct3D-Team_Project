#include "pch.h"
#include "EffectUI.h"
#include "GameInstance.h"
#include "CameraObject.h"
#include "Resources.h"
#include "Client_Resources.h"
#include "ComConstantBuffer.h"
#include "Resources.h"
#include "Client_Defines.h"
#include "UIManager.h"
#include "TweenComponent.h"
#include "Level_Defines.h"

NS_USING(Client)

CEffectUI::CEffectUI()
{
}

CEffectUI::~CEffectUI()
{
}

HRESULT CEffectUI::InitializePrototype(void* pArg)
{
	return S_OK;
}

HRESULT CEffectUI::Initialize(void* pArg)
{
	auto		pDesc = static_cast<CFlipbookUI::FLIPBOOK_DESC*>(pArg);

	if (FAILED(CFlipbookUI::Initialize(pDesc)))
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

	m_UIINFO.UIType = ETOUI(UI_TYPE::FLIPBOOK);

	return S_OK;
}

void CEffectUI::PriorityUpdate(E::_float fTimeDelta)
{
}

void CEffectUI::Update(E::_float fTimeDelta)
{
	_float2 mousePos = E::CGameInstance::Get().GetMousePos();

	if (!m_isActive)
	{
		if (m_UIINFO.Restag == "TEX_UI_T_PointFlame")
			m_bWasFlameActive = false;
		return;
	}

	if (m_UIINFO.Restag == "TEX_UI_T_PointFlame")
	{
		constexpr _float FLAME_DEFAULT_DELTA = 1.f / 60.f;
		constexpr _float FLAME_MAX_DELTA = 1.f / 30.f;
		constexpr _float FLAME_DELTA_RESPONSE = 10.f;

		const _float safeDelta = std::isfinite(fTimeDelta) && fTimeDelta > 0.f ?
			std::min(fTimeDelta, FLAME_MAX_DELTA) : 0.f;

		if (!m_bWasFlameActive)
		{
			m_fStableFlameTime = 0.f;
			m_fSmoothedFlameDelta = safeDelta > 0.f ? safeDelta : FLAME_DEFAULT_DELTA;
			m_bWasFlameActive = true;
		}
		else
		{
			const _float response = 1.f - std::exp(-FLAME_DELTA_RESPONSE * safeDelta);
			m_fSmoothedFlameDelta += (safeDelta - m_fSmoothedFlameDelta) * response;
		}

		if (safeDelta > 0.f)
		{
			m_fStableFlameTime = std::fmod(
				m_fStableFlameTime + m_fSmoothedFlameDelta,
				4096.f);
		}
	}

	CFlipbookUI::Update(fTimeDelta);

	m_pComCButton->CheckPixelPerfectCollision(mousePos, true);

	if (m_pComTween != nullptr)
	{
		m_pComTween->Tick(fTimeDelta);
	}
}

void CEffectUI::LateUpdate(E::_float fTimeDelta)
{
	if (!m_isActive)
		return;

	CUIObject::LateUpdate(fTimeDelta);

	// 쉐이더 분기
	if (m_UIINFO.Restag == "TEX_T_FX_Stupify_Core_Center_D")
	{
		m_VSShaderTag = "VS_QuadTexFlipBook";
		m_PSShaderTag = "PS_SpellBoostCore";
	}
	else if (m_UIINFO.Restag == "TEX_VFX_T_TMB_SmokeWispy_D")
	{
		m_VSShaderTag = "VS_TwoTone";
		m_PSShaderTag = "PS_TwoTone";
	}
	else if (m_UIINFO.Restag == "TEX_UI_T_PointFlame")
	{
		m_VSShaderTag = "VS_SpellMiniGameFlame";
		m_PSShaderTag = "PS_SpellMiniGameFlame";
	}
	else
	{
		m_VSShaderTag = "VS_QuadTexFlipBook";
		m_PSShaderTag = "PS_QuadTexFlipBook";
	}
		
}

HRESULT CEffectUI::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
	std::string currentLevel = _string("LEVEL_") + MagicEnumToStringView(static_cast<LEVEL>(E::CGameInstance::Get().GetCurrentLevelID())).data();

	//VS_QuadTex
	const auto& vs = E::CGameInstance::Get().GetResourceFirst<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, m_VSShaderTag);
	const auto& ps = E::CGameInstance::Get().GetResourceFirst<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, m_PSShaderTag);
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
		perUI.texCoord = m_texcoord;
		perUI.uvSize = m_uvSize;
		perUI.color = { m_UIINFO.Color.x, m_UIINFO.Color.y, m_UIINFO.Color.z, m_UIINFO.Alpha };
		perUI.uvFlip = { m_UIINFO.FlipX ? 1.f : 0.f, m_UIINFO.FlipY ? 1.f : 0.f };
		if (m_UIINFO.Restag == "TEX_UI_T_PointFlame")
			perUI.margins.x = m_fStableFlameTime;

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
		if (m_UIINFO.Restag == "TEX_UI_T_PointFlame")
		{
			const auto& flame = E::CGameInstance::GetConst().GetResourceFirst<E::CResTexture2D>(currentLevel, m_UIINFO.Restag);
			const auto& mask = E::CGameInstance::GetConst().GetResourceFirst<E::CResTexture2D>(currentLevel, "TEX_VFX_T_GradientSpline_D");
			const auto& flow = E::CGameInstance::GetConst().GetResourceFirst<E::CResTexture2D>(currentLevel, "TEX_T_FX_Flowmap_FM");
			const auto& noise = E::CGameInstance::GetConst().GetResourceFirst<E::CResTexture2D>(currentLevel, "TEX_VFX_T_DistortionNoise_N");
			const auto& sparks = E::CGameInstance::GetConst().GetResourceFirst<E::CResTexture2D>(currentLevel, "TEX_UI_T_MagicSplatter");
			ID3D11ShaderResourceView* srvs[] = {
				flame->GetSRV().Get(),
				mask->GetSRV().Get(),
				flow->GetSRV().Get(),
				noise->GetSRV().Get(),
				flame->GetSRV().Get(),
				sparks->GetSRV().Get()
			};
			pContext->PSSetShaderResources(0, static_cast<UINT>(std::size(srvs)), srvs);
		}
		else
		{
			const auto& srv = E::CGameInstance::GetConst().GetResourceFirst<E::CResTexture2D>(currentLevel, m_UIINFO.Restag);
			pContext->PSSetShaderResources(0, 1, srv->GetSRV().GetAddressOf());
		}

		const auto& sampler = E::CGameInstance::GetConst().GetResourceFirst<E::CResSamplerState>(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_SS_LINEAR_WRAP);
		pContext->PSSetSamplers(0, 1, sampler->GetSamplerState().GetAddressOf());
	}

	const _bool useAdditiveBlend =
		m_bAdditiveBlend ||
		m_UIINFO.Restag == "TEX_UI_T_PointFlame" ||
		m_UIINFO.Restag == "TEX_VFX_T_SmokeWispy_Tile_D";
	if (useAdditiveBlend)
	{
		const auto& additive = E::CGameInstance::Get().
			GetResourceFirst<E::CResBlendState>(
				TAG_RES_GRP_PERMANENT_STATE,
				"BS_ADDITIVE");
		pContext->OMSetBlendState(
			additive->GetBlendState().Get(),
			nullptr,
			0xffffffff);
	}

	pContext->DrawIndexed(viBuffer->GetNumIndices(), 0, 0);

	if (useAdditiveBlend)
	{
		const auto& alphaBlend = E::CGameInstance::Get().
			GetResourceFirst<E::CResBlendState>(
				TAG_RES_GRP_PERMANENT_STATE,
				"BS_ALPHA_BLEND");
		pContext->OMSetBlendState(
			alphaBlend->GetBlendState().Get(),
			nullptr,
			0xffffffff);
	}


	return S_OK;
}

void CEffectUI::PlayEffect(uint32_t uiState)
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

	if (m_UIINFO.EffectType != ETOUI(UI_EFFECT_TYPE::NONE))
	{
		switch (uiState)
		{
		case ETOUI(UI_STATE::HOVERED):
			GET_SINGLE(UIManager)->LoadPrefab("Magic");
			break;
		}
	}

}

E::UPtr<CEffectUI> CEffectUI::Create()
{
	auto pInstance = E::ToUPtr(new CEffectUI{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CFlipBook");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CEffectUI::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CEffectUI{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CFlipBook");
		return nullptr;
	}

	return pInstance;
}

#include "pch.h"
#include "SpellMeter.h"
#include "GameInstance.h"
#include "CameraObject.h"
#include "Resources.h"
#include "UIManager.h"
#include "Client_Defines.h"
#include "Level_Defines.h"
#include "UI_Enums.h"
#include "UIController.h"

NS_USING(Client)

CSpellMeter::CSpellMeter()
{
}

CSpellMeter::~CSpellMeter()
{
}

HRESULT CSpellMeter::InitializePrototype(void* pArg)
{
	return S_OK;
}

HRESULT CSpellMeter::Initialize(void* pArg)
{
	auto		pDesc = static_cast<CUIObject::UIOBJECT_DESC*>(pArg);

	if (FAILED(CUIObject::Initialize(pDesc)))
		return E_FAIL;

	{
		/* Buffer */
		CComConstantBuffer::DESC Desc{};
		Desc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, "CB_SpellMeter" };
		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ConstantBuffer", "ComCBufferPerSpellMeter", &Desc, &m_pComCBufferPerSpellMeter)))
		{
			return E_FAIL;
		};

		/* Component */
		CComponent::DESC CDesc{};
		CDesc.pGameObject = this;

		if (FAILED(AddComponentFromProto(ES_EngineProtoMajorType::UI, "Prototype_Component_Tween", "Com_Tween", &CDesc, &m_pComTween)))
		{
			return E_FAIL;
		};

		if (FAILED(AddComponentFromProto(ES_EngineProtoMajorType::UI, "Prototype_Component_ButtonUI", "Com_Button", &CDesc, &m_pComCButton)))
		{
			return E_FAIL;
		};
	}

	m_UIINFO.UIType = ETOUI(UI_TYPE::SPELLMETER);
	s_fAccumulatedTime = 0.f;

	return S_OK;
}

void CSpellMeter::PriorityUpdate(E::_float fTimeDelta)
{
}

void CSpellMeter::Update(E::_float fTimeDelta)
{
	if (!m_isActive)
		return;

	CUIObject::Update(fTimeDelta);

	_float2 mousePos = E::CGameInstance::Get().GetMousePos();

	for (auto& pComponent : m_UIComponents)
	{
		pComponent->Update(fTimeDelta, mousePos);
	}

	if (m_bWorldSpace)
	{
		E::_float scaleFactor = 0.01f;
		GetTransform().SetScale(E::_float3{ m_UIINFO.SizeX * scaleFactor, m_UIINFO.SizeY * scaleFactor, 1.f });

		// 캐릭터를 따라다녀야 한다면 여기서 SetPosition을 갱신
	}
	else
	{
		m_pComCButton->CheckPixelPerfectCollision(mousePos, true);
	}


	if (m_pComTween != nullptr)
	{
		m_pComTween->Tick(fTimeDelta);
	}

	s_fAccumulatedTime += fTimeDelta;
	if (s_fAccumulatedTime > 10000.f) s_fAccumulatedTime -= 10000.f; // 오버플로우 방지

	// ************** 스펠 리소스 세팅
	if (m_ResTag_DirtyFlag)
		SetResTag();
	// x
	if ( m_SpellType >= ETOUI(SPELL_TYPE::B_NONE) && E::CGameInstance::Get().MouseUp(MOUSEKEYSTATE::LB) && m_pComCButton->CheckPixelPerfectCollision(mousePos, true))
	{
		std::optional<CHandle> hController = GET_SINGLE(UIManager)->GetUIController();

		if (hController != std::nullopt &&
			nullptr != E::CGameInstance::Get().GetGameObjectByHandleT<CUIController>(*hController))
		{
			CUIController* pController = E::CGameInstance::Get().GetGameObjectByHandleT<CUIController>(*hController);
			//if (ETOUI(SPELL_TYPE::NONE) != pController->GetTargetIcon())
			{
				m_ResTag_DirtyFlag = true;
				m_SpellType = pController->GetTargetIcon();
				CHandle SpellEffect = GET_SINGLE(UIManager)->LoadPrefab("SpellChoiceEffect").front();
				CUIObject* pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(SpellEffect);
				pUI->SetPos({ GetPos() });
				pUI->GetUIInfo().Weight = 350.f;

				pController->SetTargetIcon(ETOUI(SPELL_TYPE::B_NONE));
			}
		}
	}
}

void CSpellMeter::LateUpdate(E::_float fTimeDelta)
{
	if (!m_isActive)
		return;

	E::CGameInstance::Get().AddRenderObject(E::RENDERGROUP::UI, this);
	GetTransform().Update();
}

HRESULT CSpellMeter::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
	std::string currentLevel = _string("LEVEL_") + MagicEnumToStringView(static_cast<LEVEL>(E::CGameInstance::Get().GetCurrentLevelID())).data();

	//VS_QuadTex
	const auto& vs = E::CGameInstance::Get().GetResourceFirst<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_SpellMeter");
	const auto& ps = E::CGameInstance::Get().GetResourceFirst<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_SpellMeter");
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
		E::CB_SPELLMETER perSpellMeter{};

		perSpellMeter.fAmount = m_fCurrentAmount;

		// 왜곡
		perSpellMeter.fDistSpeed = 0.1f;       // 일렁이는 속도
		perSpellMeter.fDistStrength = m_DistStrength;   // 일렁이는 강도
		perSpellMeter.fTime = s_fAccumulatedTime;

		// 색상 
		perSpellMeter.vFillColor = m_BGColor;				// 채워진 마법 색상
		perSpellMeter.vEmptyColor = { 0.023f, 0.024f, 0.019f, 1.0f };	// 빈 배경 (매우 어두운 색)
		perSpellMeter.vRippleColor = { 1.0f, 1.0f, 1.0f, 1.0f };		// 경계선 파동 (흰색 발광)
		perSpellMeter.vWispyColor = { 0.5f, 0.5f, 0.5f, 0.5f };

		// 3. 버퍼 업데이트 및 쉐이더로 전송
		if (FAILED(m_pComCBufferPerSpellMeter->MapDiscard(pContext, &perSpellMeter, sizeof(perSpellMeter))))
		{
			return E_FAIL;
		}
		pContext->VSSetConstantBuffers(10, 1, m_pComCBufferPerSpellMeter->GetAdressOfBuffer());
		pContext->PSSetConstantBuffers(10, 1, m_pComCBufferPerSpellMeter->GetAdressOfBuffer());
	}

	{
		{
			auto pCbPerObject = E::CGameInstance::Get().GetResourceFirst<E::CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "CB_PerObject");
			D3D11_MAPPED_SUBRESOURCE mappedSubResource;
			if (SUCCEEDED(pContext->Map(pCbPerObject->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubResource)))
			{

				E::CB_PER_OBJECT cbPerObject{};

				_matrix matWVP = GetTransform().GetLoadedWorldMatrix() * ctx.matProj;
				XMStoreFloat4x4(&cbPerObject.matWVP, matWVP);

				memcpy(mappedSubResource.pData, &cbPerObject, sizeof(cbPerObject));
				pContext->Unmap(pCbPerObject->GetCBuffer().Get(), 0);
			}
			pContext->VSSetConstantBuffers(ETOUI(B_SLOTNUMBER::PER_OBJECT), 1, pCbPerObject->GetCBuffer().GetAddressOf());
			pContext->PSSetConstantBuffers(ETOUI(B_SLOTNUMBER::PER_OBJECT), 1, pCbPerObject->GetCBuffer().GetAddressOf());
		}
	}
	//m_UIINFO.Restag = "TEX_UI_T_spellmeter_Glacius_Overlay";
	{
		const auto& baseSrv = E::CGameInstance::GetConst().GetResourceFirst<E::CResTexture2D>(currentLevel, "TEX_UI_T_spellmeter_Generic");
		const auto& causticSrv = E::CGameInstance::GetConst().GetResourceFirst<E::CResTexture2D>(currentLevel, "TEX_T_WaterCaustics_Disorder_A");
		const auto& wispySrv = E::CGameInstance::GetConst().GetResourceFirst<E::CResTexture2D>(currentLevel, "TEX_VFX_T_WispyNoise_D");
		const auto& normalSrv = E::CGameInstance::GetConst().GetResourceFirst<E::CResTexture2D>(currentLevel, "TEX_VFX_T_Wavy_N");
		const auto& rippleSrv = E::CGameInstance::GetConst().GetResourceFirst<E::CResTexture2D>(currentLevel, "TEX_T_CollectionsMeterLine_A");
		const auto& iconSrv = E::CGameInstance::GetConst().GetResourceFirst<E::CResTexture2D>(currentLevel, m_UIINFO.Restag);
		
		ID3D11ShaderResourceView* srvs[6] = {
			baseSrv->GetSRV().Get(),      // t0: 다이아몬드
			causticSrv->GetSRV().Get(),   // t1: 코스틱
			wispySrv->GetSRV().Get(),     // t2: 위스피 노이즈
			normalSrv->GetSRV().Get(),     // t3: 웨이비 노말맵
			rippleSrv->GetSRV().Get(),
			iconSrv->GetSRV().Get()
		};

		pContext->PSSetShaderResources(0, 6, srvs);
	}

	pContext->DrawIndexed(viBuffer->GetNumIndices(), 0, 0);

	ID3D11Buffer* nullBuffer = nullptr;
	pContext->VSSetConstantBuffers(10, 1, &nullBuffer);
	pContext->PSSetConstantBuffers(10, 1, &nullBuffer);

	return S_OK;
}

void CSpellMeter::PlayEffect(uint32_t uiState)
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

void CSpellMeter::SetResTag()
{
	switch (m_SpellType)
	{
	case ETOUI(SPELL_TYPE::B_NONE):
	case ETOUI(SPELL_TYPE::NONE):
		m_UIINFO.Restag = "TEX_T_BlankAlpha_A";
		break;
	case ETOUI(SPELL_TYPE::B_ARRESTOMOMENTUM):
	case ETOUI(SPELL_TYPE::ARRESTOMOMENTUM):
		m_UIINFO.Restag = "TEX_UI_T_spellmeter_ArrestoMomentum_Overlay";
		m_colorType = 2;
		m_CoolTime = 5.f;
		break;
	case ETOUI(SPELL_TYPE::B_GLACIUS):
	case ETOUI(SPELL_TYPE::GLACIUS):
		m_UIINFO.Restag = "TEX_UI_T_spellmeter_Glacius_Overlay";
		m_colorType = 2;
		m_CoolTime = 5.f;
		break;
	case ETOUI(SPELL_TYPE::B_LEVIOSO):
	case ETOUI(SPELL_TYPE::LEVIOSO):
		m_UIINFO.Restag = "TEX_UI_T_spellmeter_Levioso_Overlay";
		m_colorType = 2;
		m_CoolTime = 5.f;
		break;
	case ETOUI(SPELL_TYPE::B_TRANSFORMATION):
	case ETOUI(SPELL_TYPE::TRANSFORMATION):
		m_UIINFO.Restag = "TEX_UI_T_spellmeter_TransformationOverlandOverlay";
		m_colorType = 2;
		m_CoolTime = 5.f;
		break;
	case ETOUI(SPELL_TYPE::B_ASSIO):
	case ETOUI(SPELL_TYPE::ASSIO):
		m_UIINFO.Restag = "TEX_UI_T_spellmeter_Accio_Overlay";
		m_colorType = 3;
		m_CoolTime = 5.f;
		break;
	case ETOUI(SPELL_TYPE::B_DEPULSO):
	case ETOUI(SPELL_TYPE::DEPULSO):
		m_UIINFO.Restag = "TEX_UI_T_spellmeter_Depulso_Overlay";
		m_colorType = 3;
		m_CoolTime = 5.f;
		break;
	case ETOUI(SPELL_TYPE::B_DESENDO):
	case ETOUI(SPELL_TYPE::DESENDO):
		m_UIINFO.Restag = "TEX_UI_T_spellmeter_Descendo_Overlay";
		m_colorType = 3;
		m_CoolTime = 5.f;
		break;
	case ETOUI(SPELL_TYPE::B_FLIPENDO):
	case ETOUI(SPELL_TYPE::FLIPENDO):
		m_UIINFO.Restag = "TEX_UI_T_spellmeter_Flipendo_Overlay";
		m_colorType = 3;
		m_CoolTime = 5.f;
		break;
	case ETOUI(SPELL_TYPE::B_CONFRINGO):
	case ETOUI(SPELL_TYPE::CONFRINGO):
		m_UIINFO.Restag = "TEX_UI_T_spellmeter_Confringo_Overlay";
		m_colorType = 0;
		m_CoolTime = 5.f;
		break;
	case ETOUI(SPELL_TYPE::B_DIFFINDO):
	case ETOUI(SPELL_TYPE::DIFFINDO):
		m_UIINFO.Restag = "TEX_UI_T_spellmeter_Diffindo_Overlay";
		m_colorType = 0;
		m_CoolTime = 5.f;
		break;
	case ETOUI(SPELL_TYPE::B_EXPELLIARMUS):
	case ETOUI(SPELL_TYPE::EXPELLIARMUS):
		m_UIINFO.Restag = "TEX_UI_T_spellmeter_Expelliarmus_Overlay";
		m_colorType = 0;
		m_CoolTime = 5.f;
		break;
	case ETOUI(SPELL_TYPE::B_BOMBARDA):
	case ETOUI(SPELL_TYPE::BOMBARDA):
		m_UIINFO.Restag = "TEX_UI_T_spellmeter_Bombarda_Overlay";
		m_colorType = 0;
		m_CoolTime = 10.f;
		break;
	case ETOUI(SPELL_TYPE::B_INCENDIO):
	case ETOUI(SPELL_TYPE::INCENDIO):
		m_UIINFO.Restag = "TEX_UI_T_spellmeter_Incendio_Overlay";
		m_colorType = 0;
		m_CoolTime = 5.f;
		break;
	case ETOUI(SPELL_TYPE::B_DISILLUSIONMENT):
	case ETOUI(SPELL_TYPE::DISILLUSIONMENT):
		m_UIINFO.Restag = "TEX_UI_T_spellmeter_Disillusionment_Overlay";
		m_colorType = 4;
		m_CoolTime = 5.f;
		break;
	case ETOUI(SPELL_TYPE::B_LUMOS):
	case ETOUI(SPELL_TYPE::LUMOS):
		m_UIINFO.Restag = "TEX_UI_T_spellmeter_Lumos_Overlay";
		m_colorType = 4;
		m_CoolTime = 5.f;
		break;
	case ETOUI(SPELL_TYPE::B_REPARO):
	case ETOUI(SPELL_TYPE::REPARO):
		m_UIINFO.Restag = "TEX_UI_T_spellmeter_Reparo_Overlay";
		m_colorType = 4;
		m_CoolTime = 5.f;
		break;
	case ETOUI(SPELL_TYPE::B_WINGARDIUM):
	case ETOUI(SPELL_TYPE::WINGARDIUM):
		m_UIINFO.Restag = "TEX_UI_T_spellmeter_WingardiumLeviosa_Overlay";
		m_colorType = 4;
		m_CoolTime = 5.f;
		break;
	case ETOUI(SPELL_TYPE::B_AVADAKEDAVRA):
	case ETOUI(SPELL_TYPE::AVADAKEDAVRA):
		m_UIINFO.Restag = "TEX_UI_T_spellmeter_AvadaKedavra_Overlay";
		m_colorType = 1;
		m_CoolTime = 5.f;
		break;
	case ETOUI(SPELL_TYPE::B_CRUCIO):
	case ETOUI(SPELL_TYPE::CRUCIO):
		m_UIINFO.Restag = "TEX_UI_T_spellmeter_Crucio_Overlay";
		m_colorType = 1;
		m_CoolTime = 5.f;
		break;
	case ETOUI(SPELL_TYPE::B_IMPERIO):
	case ETOUI(SPELL_TYPE::IMPERIO):
		m_UIINFO.Restag = "TEX_UI_T_spellmeter_Imperio_Overlay";
		m_colorType = 1;
		m_CoolTime = 5.f;
		break;
	default:
		m_UIINFO.Restag = "TEX_T_BlankAlpha_A";
		m_colorType = 5;
		break;
	}

	if (m_SpellType == 0)
	{
		m_DistStrength = 0.f;

		auto pTween = GetTweenCom();

		_float scaleRatio = GetScaleRatio();
		_float Alpah = GetAlpha();

		pTween->PlayTween(scaleRatio, 0.8f, 0.2f,
			[this](float currentValue) {
				SetScaleRatio(currentValue);
				CalcUICoord();
			}, nullptr, EEaseType::EaseOutQuad);

		pTween->PlayTween(Alpah, 0.5f, 0.1f,
			[this](float currentValue) {
				SetAlpha(currentValue);
				CalcUICoord();
			}, nullptr, EEaseType::EaseOutQuad);
	}
	else if(m_SpellType < ETOUI(SPELL_TYPE::B_NONE))
	{
		m_DistStrength = 0.05f;

		auto pTween = GetTweenCom();

		_float scaleRatio = GetScaleRatio();
		_float Alpah = GetAlpha();

		pTween->PlayTween(scaleRatio, 1.f, 0.2f,
			[this](float currentValue) {
				SetScaleRatio(currentValue);
				CalcUICoord();
			}, nullptr, EEaseType::EaseOutQuad);

		pTween->PlayTween(Alpah, 1.f, 0.1f,
			[this](float currentValue) {
				SetAlpha(currentValue);
				CalcUICoord();
			}, nullptr, EEaseType::EaseOutQuad);
	}
	else if (m_SpellType == ETOUI(SPELL_TYPE::B_NONE))
	{
		m_DistStrength = 0.f;

		auto pTween = GetTweenCom();

		_float scaleRatio = GetScaleRatio();
		_float Alpah = GetAlpha();

		pTween->PlayTween(scaleRatio, 1.3f, 0.2f,
			[this](float currentValue) {
				SetScaleRatio(currentValue);
				CalcUICoord();
			}, nullptr, EEaseType::EaseOutQuad);

		//pTween->PlayTween(Alpah, 0.5f, 0.1f,
		//	[this](float currentValue) {
		//		SetAlpha(currentValue);
		//		CalcUICoord();
		//	}, nullptr, EEaseType::EaseOutQuad);
	}
	else if (m_SpellType > ETOUI(SPELL_TYPE::B_NONE))
	{
		m_DistStrength = 0.05f;

		auto pTween = GetTweenCom();

		_float scaleRatio = GetScaleRatio();
		_float Alpah = GetAlpha();

		pTween->PlayTween(scaleRatio, 1.5f, 0.2f,
			[this](float currentValue) {
				SetScaleRatio(currentValue);
				CalcUICoord();
			}, nullptr, EEaseType::EaseOutQuad);

		//pTween->PlayTween(Alpah, 1.f, 0.1f,
		//	[this](float currentValue) {
		//		SetAlpha(currentValue);
		//		CalcUICoord();
		//	}, nullptr, EEaseType::EaseOutQuad);
	}


	// 배경색 설정
	switch (m_colorType)
	{
	case 0:
		m_BGColor = { 1.2f, 0.f, 0.f, 1.f };//빨간색
		break;
	case 1:
		m_BGColor = { 0.f, 1.2f, 0.f, 1.f };// 초록색
		break;
	case 2:
		m_BGColor = { 1.5f, 1.5f, 0.f, 1.f };// 노란색
		break;
	case 3:
		m_BGColor = { 0.62f * 1.2f, 0.12f * 1.2f, 0.94f * 1.2f, 1.0f }; // 보라색
		break;
	case 4:
		m_BGColor = { 0.714f * 1.5f, 0.857f * 1.5f, 1.f * 1.5f, 1.0f }; // 하늘색
		break;
	case 5:
		m_BGColor = { 0.01f, 0.f, 0.f, 1.f }; // 검은색
		break;
	default:
		m_BGColor = { 0.f, 0.f, 0.f, 1.f };
		break;
	}
	m_ResTag_DirtyFlag = false;
}

void CSpellMeter::StartCooldown()
{
	if (m_SpellType == 0)
		return;
	if (m_fCurrentAmount != 1.f)
		return;

	auto pTween = GetTweenCom(); // 트윈 컴포넌트 가져오기
	if (!pTween) return;
	m_fCurrentAmount = 0.0f;

	pTween->PlayTween(0.0f, 1.0f, m_CoolTime,
		[this](float currentValue) {
			m_fCurrentAmount = currentValue;
		},
		[this]() {
			// (선택) 쿨타임이 100% 꽉 차서 트윈이 끝났을 때의 처리
			// 예: "띠링~" 하는 충전 완료 사운드 재생, 번쩍이는 이펙트 활성화 등
			// m_bIsReady = true;
		},
			EEaseType::Linear,
			0.0f,           
			false       
			);
}

E::UPtr<CSpellMeter> CSpellMeter::Create()
{
	auto pInstance = E::ToUPtr(new CSpellMeter{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CSpellMeter");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CSpellMeter::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CSpellMeter{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CSpellMeter");
		return nullptr;
	}

	return pInstance;
}

#include "pch.h"
#include "SpellMeter.h"
#include "GameInstance.h"
#include "CameraObject.h"
#include "Resources.h"
#include "UIManager.h"
#include "Client_Defines.h"

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

	m_UIINFO.UIType = ETOUI(UI_TYPE::SPELLMETER);

	return S_OK;
}

void CSpellMeter::PriorityUpdate(E::_float fTimeDelta)
{
}

void CSpellMeter::Update(E::_float fTimeDelta)
{
	
	if(CGameInstance::Get().KeyDown(DIK_1))
		StartCooldown(5.0f);

	if (!m_isActive)
		return;

	CUIObject::Update(fTimeDelta);

	_float2 mousePos = E::CGameInstance::Get().GetMousePos();

	m_pComCButton->CheckPixelPerfectCollision(mousePos, true);

	for (auto& pComponent : m_UIComponents)
	{
		pComponent->Update(fTimeDelta, mousePos);
	}

	if (m_pComTween != nullptr)
	{
		m_pComTween->Tick(fTimeDelta);
	}

	s_fAccumulatedTime += fTimeDelta;
	if (s_fAccumulatedTime > 10000.f) s_fAccumulatedTime -= 10000.f; // 오버플로우 방지
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
	std::string currentLevel = "LEVEL_UIEDITOR";

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
		perSpellMeter.fDistStrength = 0.05f;   // 일렁이는 강도
		perSpellMeter.fTime = s_fAccumulatedTime;

		// 색상
		perSpellMeter.vFillColor = { 1.f, 0.f, 0.f, 1.0f };				// 채워진 마법 색상
		perSpellMeter.vEmptyColor = { 0.023f, 0.024f, 0.019f, 1.0f };	// 빈 배경 (매우 어두운 색)
		perSpellMeter.vRippleColor = { 1.0f, 1.0f, 1.0f, 1.0f };		// 경계선 파동 (흰색 발광)
		perSpellMeter.vWispyColor = { 0.5f, 0.5f, 0.5f, 0.5f };

		// 3. 버퍼 업데이트 및 쉐이더로 전송
		if (FAILED(m_pComCBufferPerSpellMeter->MapDiscard(pContext, &perSpellMeter, sizeof(perSpellMeter))))
		{
			return E_FAIL;
		}
		pContext->VSSetConstantBuffers(9, 1, m_pComCBufferPerSpellMeter->GetAdressOfBuffer());
		pContext->PSSetConstantBuffers(9, 1, m_pComCBufferPerSpellMeter->GetAdressOfBuffer());
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
			pContext->VSSetConstantBuffers(0, 1, pCbPerObject->GetCBuffer().GetAddressOf());
			pContext->PSSetConstantBuffers(0, 1, pCbPerObject->GetCBuffer().GetAddressOf());
		}
	}

	{
		const auto& baseSrv = E::CGameInstance::GetConst().GetResourceFirst<E::CResTexture2D>(currentLevel, "TEX_UI_T_spellmeter_Generic");
		const auto& causticSrv = E::CGameInstance::GetConst().GetResourceFirst<E::CResTexture2D>(currentLevel, "TEX_T_WaterCaustics_Disorder_A");
		const auto& wispySrv = E::CGameInstance::GetConst().GetResourceFirst<E::CResTexture2D>(currentLevel, "TEX_VFX_T_WispyNoise_D");
		const auto& normalSrv = E::CGameInstance::GetConst().GetResourceFirst<E::CResTexture2D>(currentLevel, "TEX_VFX_T_Wavy_N");
		const auto& rippleSrv = E::CGameInstance::GetConst().GetResourceFirst<E::CResTexture2D>(currentLevel, "TEX_T_CollectionsMeterLine_A");
		const auto& iconSrv = E::CGameInstance::GetConst().GetResourceFirst<E::CResTexture2D>(currentLevel, "TEX_UI_T_arrestomomentum");

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

	if (m_bInputLocked)
		return;
}

void CSpellMeter::StartCooldown(float fCooldownTime)
{
	auto pTween = GetTweenCom(); // 트윈 컴포넌트 가져오기
	if (!pTween) return;
	m_fCurrentAmount = 0.0f;

	pTween->PlayTween(0.0f, 1.0f, fCooldownTime,
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

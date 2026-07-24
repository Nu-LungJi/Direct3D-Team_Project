#include "pch.h"

#include "FontManager.h"
#include "Resources.h"

NS_USING(Engine)

CFontManager::CFontManager(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext)
    : m_pDevice { pDevice }
    , m_pContext {pContext}
{
}

CFontManager::~CFontManager()
{
}

void CFontManager::UpdateGUI()
{
}

void CFontManager::Render3DFont()
{
	if (m_vecRender3D.empty())
		return;

	// 1. 블렌드 스테이트 가져오기 (기존 2D 코드와 동일)
	auto Alphablend = E::CGameInstance::Get().GetResourceFirst<E::CResBlendState>(TAG_RES_GRP_PERMANENT_STATE, "BS_ALPHA_BLEND");
	ComPtr<ID3D11BlendState> pAlphaBlend = Alphablend->GetBlendState();

	// 2. 파이프라인 이전 상태 백업 (SpriteBatch가 덮어씌우는 것을 방지)
	ComPtr<ID3D11BlendState>        pPrevBlendState;
	FLOAT                           prevBlendFactor[4];
	UINT                            prevSampleMask;
	ComPtr<ID3D11DepthStencilState> pPrevDepthStencilState;
	UINT                            prevStencilRef;
	ComPtr<ID3D11RasterizerState>   pPrevRasterizerState;
	ComPtr<ID3D11SamplerState>      pPrevSamplerState;

	m_pContext->OMGetBlendState(&pPrevBlendState, prevBlendFactor, &prevSampleMask);
	m_pContext->OMGetDepthStencilState(&pPrevDepthStencilState, &prevStencilRef);
	m_pContext->RSGetState(&pPrevRasterizerState);
	m_pContext->PSGetSamplers(0, 1, &pPrevSamplerState);

	// 3. SpriteBatch의 2D 직교 투영을 무효화할 역행렬 계산 (1회만 계산)
	float w = (float)CGameInstance::Get().GetClientScreenSize().x;
	float h = (float)CGameInstance::Get().GetClientScreenSize().y;

	_matrix matOrtho = XMMatrixOrthographicOffCenterRH(0.f, w, h, 0.f, 0.f, 1.f);
	_matrix matOrthoInv = XMMatrixInverse(nullptr, matOrtho);

	auto RasterizerState = E::CGameInstance::Get().GetResourceFirst<E::CResRasterizerState>(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_RS_SOLID_NOCULL);
	ComPtr<ID3D11RasterizerState> pCullNoneState = RasterizerState ? RasterizerState->GetRasterizerState() : nullptr;

	// 4. 예약된 3D 텍스트들을 순회하며 렌더링
	for (const auto& desc : m_vecRender3D)
	{
		// CResFontCustom 가져오기
		auto font = CGameInstance::Get().GetResourceFirst<CResFontCustom>("FONT", desc.sFontTag);
		if (font == nullptr)
			continue;

		// 3D WVP 행렬 * 2D 역행렬 적용
		_matrix matWVP = XMLoadFloat4x4(&desc.matWVP);
		_matrix finalTransform = matWVP * matOrthoInv;

		// Begin의 7번째 매개변수에 우리가 만든 3D 변환 행렬을 넘겨줍니다.
		m_pBatch->Begin(
			DirectX::SpriteSortMode_Deferred,
			pAlphaBlend.Get(),  // 블렌드 상태 적용
			nullptr, nullptr, pCullNoneState.Get(), nullptr,
			finalTransform      // 3D 행렬 적용
		);

		//font->GetFont()->DrawString(
		//	m_pBatch.get(),
		//	desc.sText.c_str(),
		//	XMFLOAT2(0.f, 0.f), // 위치는 행렬에 포함되어 있으므로 0,0
		//	XMLoadFloat4(&desc.vColor),
		//	0.f,                // 회전도 행렬에 포함
		//	desc.vPivot,
		//	1.f                 // 스케일도 행렬에 포함
		//);

		font->GetFont()->DrawString(
			m_pBatch.get(),
			L"TEST",
			XMFLOAT2(100, 100),
			Colors::White);

		m_pBatch->End();
	}

	// 5. 렌더링 큐 비우기
	m_vecRender3D.clear();

	// 6. 파이프라인 상태 원상 복구 (기존 2D 코드와 동일)
	m_pContext->OMSetBlendState(pPrevBlendState.Get(), prevBlendFactor, prevSampleMask);
	m_pContext->OMSetDepthStencilState(pPrevDepthStencilState.Get(), prevStencilRef);
	m_pContext->RSSetState(pPrevRasterizerState.Get());
	ID3D11SamplerState* ppSamplers[] = { pPrevSamplerState.Get() };
	m_pContext->PSSetSamplers(0, 1, ppSamplers);
}

void CFontManager::Draw(const StringID& fontName, const _tchar* pText, const _float2& vPosition, float fScale, _fvector vColor, _float fRotation, const _float2& vOrigin)
{
    if (auto font = CGameInstance::Get().GetResourceFirst<CResFontCustom>("FONT", fontName))
    {
        m_pBatch->Begin();
        font->GetFont()->DrawString(m_pBatch.get(), pText, vPosition, vColor, fRotation, vOrigin, fScale);
        m_pBatch->End();
    }
}

void CFontManager::AddLateDraw(RENDERGROUP eRenderGroup, const StringID& fontName, const _wstring& pText, const _float2& vPosition, float fScale, _fvector vColor, _float fRotation, const _float2& vOrigin)
{
    LateDrawDesc Desc{};
    Desc.txt = _wstring{ pText };
    Desc.vPosition = vPosition;
    Desc.fScale = fScale;
    XMStoreFloat4(&Desc.vColor, vColor);
    Desc.fRotation = fRotation;
    Desc.vOrigin = vOrigin;

    auto iter = m_mapLateDraws[ETOUI(eRenderGroup)].find(fontName);
    if (iter == m_mapLateDraws[ETOUI(eRenderGroup)].end())
    {
        std::vector<LateDrawDesc> v{ Desc };
        m_mapLateDraws[ETOUI(eRenderGroup)].emplace(fontName, v);
    }
    else
    {
        iter->second.push_back(Desc);
    }

}

_float2 CFontManager::MeasureString(const StringID& fontName, const wchar_t* txt, float scale) const
{
    if (auto font = CGameInstance::Get().GetResourceFirst<CResFontCustom>("FONT", fontName))
    {
        XMVECTOR size = font->GetFont()->MeasureString(txt);
        XMFLOAT2 result;
        XMStoreFloat2(&result, size * scale);
        return result;
    }
    return { 0.f, 0.f };
}

void CFontManager::FontAddLateDraw3D(const std::string& fontTag, const std::wstring& text, _fmatrix matWVP, _fvector color, _float2 pivot)
{
	FONT_DESC_3D desc;
	desc.sFontTag = fontTag;
	desc.sText = text;
	XMStoreFloat4x4(&desc.matWVP, matWVP);
	XMStoreFloat4(&desc.vColor, color);
	desc.vPivot = pivot;

	m_vecRender3D.push_back(desc);
}

void CFontManager::LateDraw(RENDERGROUP eRenderGroup)
{
	//std::unique_ptr<DirectX::CommonStates> states = std::make_unique<DirectX::CommonStates>(m_pDevice);
	auto Alphablend = E::CGameInstance::Get().GetResourceFirst<E::CResBlendState>(TAG_RES_GRP_PERMANENT_STATE, "BS_ALPHA_BLEND");
	ComPtr<ID3D11BlendState> pAlphaBlend = Alphablend->GetBlendState();

	ComPtr<ID3D11BlendState>        pPrevBlendState;
	FLOAT                           prevBlendFactor[4];
	UINT                            prevSampleMask;

	ComPtr<ID3D11DepthStencilState> pPrevDepthStencilState;
	UINT                            prevStencilRef;

	ComPtr<ID3D11RasterizerState>   pPrevRasterizerState;

	ComPtr<ID3D11SamplerState>      pPrevSamplerState;

	m_pContext->OMGetBlendState(&pPrevBlendState, prevBlendFactor, &prevSampleMask);
	m_pContext->OMGetDepthStencilState(&pPrevDepthStencilState, &prevStencilRef);
	m_pContext->RSGetState(&pPrevRasterizerState);
	m_pContext->PSGetSamplers(0, 1, &pPrevSamplerState);


    for (const auto& [fontName, vecDesc] : m_mapLateDraws[ETOUI(eRenderGroup)])
    {
        if (auto font = CGameInstance::Get().GetResourceFirst<CResFontCustom>("FONT", fontName))
        {
			m_pBatch->Begin(DirectX::SpriteSortMode_Deferred, pAlphaBlend.Get());
            for (const auto& Desc : vecDesc)
            {
                font->GetFont()->DrawString(m_pBatch.get(), Desc.txt.data(), Desc.vPosition, XMLoadFloat4(&Desc.vColor), Desc.fRotation, Desc.vOrigin, Desc.fScale);
            }

			m_pBatch->End();
        }
    }


    m_mapLateDraws[ETOUI(eRenderGroup)].clear();


    {
		m_pContext->OMSetBlendState(pPrevBlendState.Get(), prevBlendFactor, prevSampleMask);
		m_pContext->OMSetDepthStencilState(pPrevDepthStencilState.Get(), prevStencilRef);
		m_pContext->RSSetState(pPrevRasterizerState.Get());
		ID3D11SamplerState* ppSamplers[] = { pPrevSamplerState.Get() };
		m_pContext->PSSetSamplers(0, 1, ppSamplers);
    }
}

HRESULT CFontManager::Initialize()
{
    //MakeSpriteFont "Neo둥근모 Pro" /FontSize:20 /FastPack /CharacterRegion:0x0020-0x00FF /CharacterRegion:0x3131-0x3163 /CharacterRegion:0xAC00-0xD800 /DefaultCharacter:0xAC00 NeoDGM_20px.spritefont
    if (auto res = CGameInstance::Get().AddResource("FONT", "NeoDGM_20px", CResFontCustom::Create("./Resources/Engine/Font/NeoDGM_20px.spritefont")))
    {
        if (FAILED(res->Load()))
        {
            return E_FAIL;
        }
    }

    //MakeSpriteFont "Neo둥근모 Pro" /FontSize:15 /FastPack /CharacterRegion:0x0020-0x00FF /CharacterRegion:0x3131-0x3163 /CharacterRegion:0xAC00-0xD800 /DefaultCharacter:0xAC00 NeoDGM_15px.spritefont
    if (auto res = CGameInstance::Get().AddResource("FONT", "NeoDGM_15px", CResFontCustom::Create("./Resources/Engine/Font/NeoDGM_15px.spritefont")))
    {
        if (FAILED(res->Load()))
        {
            return E_FAIL;
        }
    }

    //MakeSpriteFont "Neo둥근모 Pro" /FontSize:10 /FastPack /CharacterRegion:0x0020-0x00FF /CharacterRegion:0x3131-0x3163 /CharacterRegion:0xAC00-0xD800 /DefaultCharacter:0xAC00 NeoDGM_10px.spritefont
    if (auto res = CGameInstance::Get().AddResource("FONT", "NeoDGM_10px", CResFontCustom::Create("./Resources/Engine/Font/NeoDGM_10px.spritefont")))
    {
        if (FAILED(res->Load()))
        {
            return E_FAIL;
        }
    }

	if (auto res = CGameInstance::Get().AddResource("FONT", "Pretendard", CResFontCustom::Create("./Resources/Engine/Font/Pretendard.spritefont")))
	{
		if (FAILED(res->Load()))
		{
			return E_FAIL;
		}
	}
    m_pBatch = std::make_unique<SpriteBatch>(m_pContext.Get());
	return S_OK;
}

UPtr<CFontManager> CFontManager::Create(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext)
{
    auto pInstance = ToUPtr(new CFontManager{ pDevice, pContext });

    if (FAILED(pInstance->Initialize()))
    {
        return nullptr;
    }

    return pInstance;
}

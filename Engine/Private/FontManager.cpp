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

void CFontManager::LateDraw(RENDERGROUP eRenderGroup)
{
    for (const auto& [fontName, vecDesc] : m_mapLateDraws[ETOUI(eRenderGroup)])
    {
        if (auto font = CGameInstance::Get().GetResourceFirst<CResFontCustom>("FONT", fontName))
        {
            m_pBatch->Begin();

            for (const auto& Desc : vecDesc)
            {
                font->GetFont()->DrawString(m_pBatch.get(), Desc.txt.data(), Desc.vPosition, XMLoadFloat4(&Desc.vColor), Desc.fRotation, Desc.vOrigin, Desc.fScale);
            }
            
            m_pBatch->End();
        }
    }

    m_mapLateDraws[ETOUI(eRenderGroup)].clear();

    {
        m_pContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);

        // 2. 깊이/스텐실 스테이트 복구 (3D 렌더링을 위해 다시 켜기)
        m_pContext->OMSetDepthStencilState(nullptr, 0); // 엔진 내 기본 DepthStencilState가 있다면 nullptr 대신 그걸 대입

        // 3. 래스터라이저 스테이트 복구 (CullMode 등을 다시 원래대로)
        m_pContext->RSSetState(nullptr); // 엔진 내 기본 RasterizerState가 있다면 대입
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

#pragma once
#include "Engine_Defines.h"

NS_BEGIN(Engine)

class CFontManager final : public CEngineBase
{
private:
	struct LateDrawDesc
	{
		_wstring txt{};
		_float2 vPosition{};
		float fScale{};
		_float4 vColor{};
		_float fRotation{};
		_float2 vOrigin{};
	};
private:
	explicit CFontManager(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
	~CFontManager() override;

public:
	void UpdateGUI();

public:
	void Draw(const StringID& fontName, const _tchar* pText, const _float2& vPosition, float fScale, _fvector vColor, _float fRotation, const _float2& vOrigin);
	void AddLateDraw(RENDERGROUP eRenderGroup, const StringID& fontName, const _wstring& pText, const _float2& vPosition, float fScale, _fvector vColor, _float fRotation, const _float2& vOrigin);
	_float2 MeasureString(const StringID& fontName, const wchar_t* txt, float scale = 1.f) const;

public:
	void LateDraw(RENDERGROUP eRenderGroup);

private:
	HRESULT Initialize();

private:
	std::unordered_map<StringID, std::vector<LateDrawDesc>> m_mapLateDraws[ETOUI(RENDERGROUP::END)]{};

private:
	ComPtr<ID3D11Device> m_pDevice{};
	ComPtr<ID3D11DeviceContext> m_pContext{};

	std::unique_ptr<SpriteBatch> m_pBatch = { };



public:
	static UPtr<CFontManager> Create(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
};

NS_END
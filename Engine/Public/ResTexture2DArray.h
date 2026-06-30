#pragma once
#include "ResTexture.h"

NS_BEGIN(Engine)

class ENGINE_DLL CResTexture2DArray final: public CResTexture
{
public:
	DECLARE_DERIVED_TYPE(CResTexture2DArray, CResTexture)

public:
	typedef struct tagDesc
	{
		std::pair<StringID, StringID> textureId;
		//std::vector<std::pair<StringID, StringID>> textureIds;
	} DESC;

private:
	explicit CResTexture2DArray(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
	~CResTexture2DArray() override;

public:
	ComPtr<ID3D11ShaderResourceView> GetSRV() const { return m_pSRV; }
public:
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {}) override;

private:
	ComPtr<ID3D11Texture2D> m_pTextureArray{};
	ComPtr<ID3D11ShaderResourceView> m_pSRV{};
	std::optional<std::pair<StringID, StringID>> m_textureId{};

public:
	static SPtr<CResTexture2DArray> Create();
};

NS_END
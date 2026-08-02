#pragma once

#include "UITex.h"
#include "Client_Defines.h"

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>

#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")

NS_BEGIN(Engine)
class CComConstantBuffer;
class CButtonComponent;
NS_END

NS_BEGIN(Client)

class CVideoObject final : public E::CUITex
{
public:
	DECLARE_DERIVED_TYPE(CVideoObject, E::CUITex)

private:
	CVideoObject();
	~CVideoObject() override;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) override;

private:
	virtual void PlayEffect(uint32_t uiState);

private:
	CComConstantBuffer* m_pComCBufferPerUI = nullptr;

private:
	// Media Foundation 객체
	ComPtr<IMFSourceReader> m_pSourceReader;

	// DX11 동적 텍스처 및 리소스 뷰
	ComPtr<ID3D11Texture2D> m_pVideoTexture;
	ComPtr<ID3D11ShaderResourceView> m_pVideoSRV;

	UINT m_iVideoWidth = 0;
	UINT m_iVideoHeight = 0;
	float m_fTimeAcc = 0.f;     // 누적 시간
	float m_fFrameRate = 30.f;  // 영상 프레임 레이트 (예: 30fps

	const std::wstring m_path = L"./Resources/SampleClient/Textures/UI/Video/FMV_Accio.avi";
public:
	static E::UPtr<CVideoObject> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END

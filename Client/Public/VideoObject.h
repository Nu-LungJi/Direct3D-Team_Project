#pragma once

#include "UITex.h"
#include "Client_Defines.h"

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <vector>

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
	void PlayEffect(uint32_t uiState) override;

private:
	struct VIDEO_FRAME
	{
		std::vector<BYTE> Pixels{};
		UINT Width{};
		UINT Height{};
		UINT RowPitch{};
		LONGLONG TimeStamp{};
	};

	struct VIDEO_ASYNC_STATE
	{
		std::mutex Mutex{};
		std::condition_variable Condition{};
		std::deque<VIDEO_FRAME> Frames{};
		std::atomic_bool Cancel{ false };
		std::atomic_bool Failed{ false };
	};

	enum class VIDEO_LOAD_STATE
	{
		IDLE,
		LOADING,
		READY,
		FAILED
	};

	void BeginAsyncVideoLoad();
	void ProcessDecodedFrame();
	HRESULT CreateVideoResources(UINT width, UINT height);
	HRESULT UploadFrameToTexture(const VIDEO_FRAME& frame);

private:
	CComConstantBuffer* m_pComCBufferPerUI = nullptr;

	ComPtr<ID3D11Texture2D> m_pVideoTexture;
	ComPtr<ID3D11ShaderResourceView> m_pVideoSRV;

	std::shared_ptr<VIDEO_ASYNC_STATE> m_pAsyncState{};
	VIDEO_LOAD_STATE m_eLoadState{ VIDEO_LOAD_STATE::IDLE };

	UINT m_iVideoWidth = 0;
	UINT m_iVideoHeight = 0;
	float m_fTimeAcc = 0.f;
	float m_fFrameRate = 30.f;
	float m_fStartDelay = 1.f;

	std::wstring m_path = L"./Resources/SampleClient/Textures/UI/Video/FMV_Accio.avi";

public:
	void SetPath(std::wstring videoPath) { m_path = std::move(videoPath); }

public:
	static E::UPtr<CVideoObject> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END

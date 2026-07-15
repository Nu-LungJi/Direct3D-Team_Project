#include "pch.h"
#include "MyFSR2_2.h"
#include "ffx_fsr2.h"
#include "ffx_fsr2_dx11.h"

#include "GameInstance.h"

NS_BEGIN(Engine)

struct CMyFSR2_2::Impl
{
	FfxFsr2Context context;
	void* scratchBuffer = nullptr;
	~Impl() { ffxFsr2ContextDestroy(&context); if (scratchBuffer) free(scratchBuffer);}

};
CMyFSR2_2::CMyFSR2_2()
{
}
CMyFSR2_2::~CMyFSR2_2()
{
}
HRESULT CMyFSR2_2::Initialize()
{
	m_pImpl = std::make_unique<Impl>();
	
	FfxFsr2ContextDescription fsrDesc = {};

	_float2 clientSize = CGameInstance::Get().GetClientScreenSize();
	_float2 displaySize = CGameInstance::Get().GetDisplayScreenSize();


	// 1. 기본 설정
	fsrDesc.flags = FFX_FSR2_ENABLE_HIGH_DYNAMIC_RANGE | FFX_FSR2_ENABLE_AUTO_EXPOSURE;
	fsrDesc.maxRenderSize.width = clientSize.x;
	fsrDesc.maxRenderSize.height = clientSize.y;
	fsrDesc.displaySize.width = displaySize.x;
	fsrDesc.displaySize.height = displaySize.y;

	// 2. Device (DX11 디바이스 포인터)
	// FfxDevice는 내부적으로 void*로 정의된 경우가 많으므로 캐스팅해서 넣습니다.
	fsrDesc.device = (FfxDevice*)CGameInstance::Get().GetGraphicDevice().Get();


	LOG_MEMORY("Before Malloc FSR ScratchBuffer");

	// 3. Callbacks (가장 중요!)
	// ffxFsr2GetInterfaceDX11 함수는 FfxFsr2Interface 구조체를 채워주는 역할을 합니다.
	const size_t scratchBufferSize = ffxFsr2GetScratchMemorySizeDX11();
	void* scratchBuffer = malloc(scratchBufferSize);

	LOG_MEMORY("After Malloc FSR ScratchBuffer");

	// 2. Impl에 저장
	m_pImpl->scratchBuffer = scratchBuffer;

	// DX11 백엔드 인터페이스를 가져와 callbacks에 할당
	FfxErrorCode errorCode = ffxFsr2GetInterfaceDX11(
		&fsrDesc.callbacks,
		CGameInstance::Get().GetGraphicDevice().Get(),
		scratchBuffer,
		scratchBufferSize
	);

	if (errorCode != FFX_OK)
	{
		MSG_BOX("FSR ERR");
		return E_FAIL;
	}

	if (ffxFsr2ContextCreate(&m_pImpl->context, &fsrDesc) != FFX_OK)
	{
		MSG_BOX("FSR ERR");
		return E_FAIL;
	}
	return S_OK;
}
HRESULT CMyFSR2_2::Execute(const ExecuteDesc& desc)
{
	FfxFsr2DispatchDescription dispatchDesc{};
	// 1. 커맨드 리스트 (DX11 디바이스 컨텍스트 래퍼)
	dispatchDesc.commandList = reinterpret_cast<FfxCommandList>(CGameInstance::Get().GetGraphicDeviceContext().Get());


	// 2. 리소스 설정 (Wrapper 함수 사용)
	dispatchDesc.color = ffxGetResourceDX11(&m_pImpl->context, desc.pColorTex2D, L"desc.pColorTex2D");
	dispatchDesc.depth = ffxGetResourceDX11(&m_pImpl->context, desc.pDepthTex2D, L"desc.pDepthTex2D");
	dispatchDesc.motionVectors = ffxGetResourceDX11(&m_pImpl->context, desc.pMotionVectorTex2D, L"desc.pMotionVectorTex2D");
	dispatchDesc.output = ffxGetResourceDX11(&m_pImpl->context, desc.pOutputUAVTex2D, L"desc.pOutputUAVTex2D");


	_float2 clientSize = CGameInstance::Get().GetClientScreenSize();
	

	// 3. 카메라/프레임 정보
	dispatchDesc.renderSize = { (uint32_t)clientSize.x, (uint32_t)clientSize.y };
	dispatchDesc.jitterOffset = { desc.fCurrentJitterX, desc.fCurrentJitterY };
	dispatchDesc.motionVectorScale = { -clientSize.x, -clientSize.y }; // 엔진 모션벡터 사양에 맞게
	dispatchDesc.frameTimeDelta = desc.fDeltaTime * 1000.0f; // ms변환
	dispatchDesc.cameraNear = desc.fNear;
	dispatchDesc.cameraFar = desc.fFar;
	dispatchDesc.cameraFovAngleVertical = desc.fFov;
	dispatchDesc.reset = desc.bCameraReset;

	// 4. 선명도
	dispatchDesc.enableSharpening = true;
	dispatchDesc.sharpness = 0.5f;

	// FSR 2.2 디스패치 실행
	FfxErrorCode errorCode = ffxFsr2ContextDispatch(&m_pImpl->context, &dispatchDesc);
	if (errorCode != FFX_OK)
	{
		MSG_BOX("FSR 2.2 Dispatch Failed!");
		return E_FAIL;
	}

	return S_OK;
}
UPtr<CMyFSR2_2> CMyFSR2_2::Create()
{
	auto pInstance = ToUPtr(new CMyFSR2_2);
	if (FAILED(pInstance->Initialize()))
	{
		return nullptr;
	}
	return pInstance;
}
void CMyFSR2_2::Free()
{
	CEngineBase::Free();
}

NS_END

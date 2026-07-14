#include "pch.h"
#include "MyFSR2_2.h"
#include "ffx_fsr2.h"
#include "ffx_fsr2_dx11.h"

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

	// 1. 기본 설정
	fsrDesc.flags = FFX_FSR2_ENABLE_HIGH_DYNAMIC_RANGE | FFX_FSR2_ENABLE_AUTO_EXPOSURE;
	fsrDesc.maxRenderSize.width = 1280;
	fsrDesc.maxRenderSize.height = 720;
	fsrDesc.displaySize.width = 1920;
	fsrDesc.displaySize.height = 1080;

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
void CMyFSR2_2::Execute(const ExecuteDesc& desc)
{
	FfxFsr2DispatchDescription dispatchDesc{};
	// 1. 커맨드 리스트 (DX11 디바이스 컨텍스트 래퍼)
	dispatchDesc.commandList = reinterpret_cast<FfxCommandList>(CGameInstance::Get().GetGraphicDeviceContext().Get());


	// 2. 리소스 설정 (Wrapper 함수 사용)
	//dispatchDesc.color = ffxGetResourceDX11(pColorSRV, ...);
	//dispatchDesc.depth = ffxGetResourceDX11(pDepthSRV, ...);
	//dispatchDesc.motionVectors = ffxGetResourceDX11(pMotionVecSRV, ...);
	//dispatchDesc.output = ffxGetResourceDX11(pOutputUAV, ...);

	// 3. 카메라/프레임 정보
	dispatchDesc.renderSize = { 1280, 720 };
	dispatchDesc.jitterOffset = { desc.fCurrentJitterX, desc.fCurrentJitterY };
	dispatchDesc.motionVectorScale = { -1280.0f, -720.0f }; // 엔진 모션벡터 사양에 맞게
	dispatchDesc.frameTimeDelta = desc.fDeltaTime * 1000.0f;
	dispatchDesc.cameraNear = desc.fNear;
	dispatchDesc.cameraFar = desc.fFar;
	dispatchDesc.cameraFovAngleVertical = desc.fFov;
	dispatchDesc.reset = false;

	// 4. 선명도
	dispatchDesc.enableSharpening = true;
	dispatchDesc.sharpness = 0.5f;

	// FSR 2.2 디스패치 실행
	FfxErrorCode errorCode = ffxFsr2ContextDispatch(&m_pImpl->context, &dispatchDesc);

	if (errorCode != FFX_OK)
	{
		// 디버깅을 위해 에러 로그 확인
		OutputDebugStringA("FSR 2.2 Dispatch Failed!\n");
	}
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

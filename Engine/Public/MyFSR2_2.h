#pragma once
#include "Engine_Defines.h"

NS_BEGIN(Engine)
/*
1. 디스패치에 필요한 필수 데이터
ffxFsr2ContextDispatch를 호출하려면 매 프레임마다 다음 리소스들이 준비되어 있어야 합니다. 이것들이 없거나 형식이 맞지 않으면 화면이 깨지거나 검은 화면이 나옵니다.
Color (Input): 현재 프레임의 업스케일 전 렌더 타겟 (SRV)
Depth (Input): 현재 프레임의 뎁스 버퍼 (SRV)
Motion Vectors (Input): 현재 프레임의 모션 벡터 버퍼 (SRV)

주의: FSR 2.x는 모션 벡터 없이는 업스케일링을 제대로 수행할 수 없습니다.
Exposure (Input/Optional): 자동 노출 기능을 쓰지 않는다면 텍스처나 값으로 넘겨야 합니다.
Jitter Offset: 가장 중요합니다. 렌더링할 때 매 프레임마다 투영 행렬(Projection Matrix)에 미세한 흔들림(Jitter)을 주어야 FSR이 정보를 수집하여 화질을 개선합니다.
*/
class CMyFSR2_2 final : public CEngineBase
{
private:
	CMyFSR2_2();
	~CMyFSR2_2() override;

private:
	HRESULT Initialize();

public:
	struct ExecuteDesc
	{
		float fCurrentJitterX{};
		float fCurrentJitterY{};
		float fDeltaTime{};
		float fNear{};
		float fFar{};
		float fFov{};

	};
	void Execute(const ExecuteDesc& desc);

private:
	struct Impl; // 내부 구현 구조체 선언
	std::unique_ptr<Impl> m_pImpl; // pImpl

public:
	static UPtr<CMyFSR2_2> Create();

private:
	void Free() override;
};

NS_END

#pragma once
#include "Engine_Defines.h"

NS_BEGIN(Engine)

class CHizBuffer final : public CEngineBase
{
public:
	DECLARE_DERIVED_TYPE(CHizBuffer, CEngineBase)

private:
	CHizBuffer(ComPtr<ID3D11Device> device, ComPtr<ID3D11DeviceContext> context);
	CHizBuffer(const CHizBuffer& Prototype);
	~CHizBuffer() override;

public:
	HRESULT Initialize(uint32_t width, uint32_t height); // 뷰포트 사이즈 받아와야함
	HRESULT Resize(uint32_t width, uint32_t height);

	HRESULT Build(ID3D11ShaderResourceView* depthSRV); // 원본 Depth받아와서 Build

	// 이 함수는 GPU 결과를 CPU가 읽는 거라 Map()에서 stall (GPU가 원래 자기 할 일을 못함)
	HRESULT ReadMipToCPU(uint32_t mip, std::vector<float>& outDepths, uint32_t& outWidth, uint32_t& outHeight) const; // 나중에 GPU-Driven-Rendering으로 바꾸면서 안 쓸 거임, 임시검증용

	ComPtr<ID3D11ShaderResourceView> GetSRV() const { return m_pSRV; }
	ComPtr<ID3D11ShaderResourceView> GetMipSRV(uint32_t mip) const { return m_MipSRVs[mip]; }
	ComPtr<ID3D11UnorderedAccessView> GetMipUAV(uint32_t mip) const { return m_MipUAVs[mip]; }

	uint32_t GetMipCount() const { return m_iMipCount; }
	uint32_t GetWidth() const { return m_iWidth; }
	uint32_t GetHeight() const { return m_iHeight; }

private:
	HRESULT CreateResources();
	HRESULT CreateViews();

private:
	ComPtr<ID3D11Device> m_pDevice;
	ComPtr<ID3D11DeviceContext> m_pContext;

	uint32_t m_iWidth = 0;
	uint32_t m_iHeight = 0;
	uint32_t m_iMipCount = 0;

	ComPtr<ID3D11Texture2D> m_pTexture; // 원본 Depth
	ComPtr<ID3D11ShaderResourceView> m_pSRV; // 원본 텍스쳐의 srv

	std::vector<ComPtr<ID3D11ShaderResourceView>> m_MipSRVs; // mip0, mip1, mip2 ..들의 srv
	std::vector<ComPtr<ID3D11UnorderedAccessView>> m_MipUAVs; // mip0, mip1, mip2 ..들의 uav


	// Texture2D 
	// 	├─ Mip 0 
	// 	├─ Mip 1 
	// 	├─ Mip 2 
	// 	└─ Mip 3 

	// GPU작동방식 : m_MipSRVs[0] 읽기->m_MipUAVs[1] 쓰기
	// GPU작동방식 : m_MipSRVs[1] 읽기->m_MipUAVs[2] 쓰기
	// GPU작동방식 : m_MipSRVs[2] 읽기->m_MipUAVs[3] 쓰기
public:
	static UPtr<CHizBuffer> Create(ComPtr<ID3D11Device> device, ComPtr<ID3D11DeviceContext> context, uint32_t width, uint32_t height);

};

NS_END

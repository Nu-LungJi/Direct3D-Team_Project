#include "pch.h"
#include "HizBuffer.h"

NS_USING(Engine)

CHizBuffer::CHizBuffer(ComPtr<ID3D11Device> device, ComPtr<ID3D11DeviceContext> context)
	: m_pDevice(device), m_pContext(context)
{
}

CHizBuffer::CHizBuffer(const CHizBuffer& Prototype)
{
}

CHizBuffer::~CHizBuffer()
{
}

HRESULT CHizBuffer::Initialize(uint32_t width, uint32_t height)
{
	Resize(width, height);

	return S_OK;
}

HRESULT CHizBuffer::Resize(uint32_t width, uint32_t height)
{
	m_iWidth = width;
	m_iHeight = height;

	//m_iMipCount 계산
	uint32_t mipCount = 1;
	uint32_t size = std::max(width, height);
	while (size > 1)
	{
		size >>= 1;
		++mipCount;
	}
	m_iMipCount = mipCount;

	m_pTexture.Reset();
	m_pSRV.Reset();
	for (auto& pMipSRV : m_MipSRVs)
	{
		pMipSRV.Reset();
	}
	for (auto& pMipUAV : m_MipUAVs)
	{
		pMipUAV.Reset();
	}

	if (FAILED(CreateResources()))
		return E_FAIL;

	if (FAILED(CreateViews()))
		return E_FAIL;

	return S_OK;
}

HRESULT CHizBuffer::Build(ID3D11ShaderResourceView* depthSRV)
{
	if (depthSRV == nullptr) return E_FAIL;

	// m_MipSRVs[0] 읽기->m_MipUAVs[1] 쓰기
	// m_MipSRVs[1] 읽기->m_MipUAVs[2] 쓰기
	// m_MipSRVs[2] 읽기->m_MipUAVs[3] 쓰기

	// 같은 mip을 동시에 SRV / UAV로 바인딩하면 안됨
	// 예를 들어 Mip 1 SRV로 읽으면서 동시에 Mip 1 UAV로 쓰는 건 위험

	SPtr<CResComputeShader> pCopyDepthShader = CGameInstance::Get().GetResourceFirst<CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_HizCopyDepth");
	if (pCopyDepthShader == nullptr)
	{
		MSG_BOX("CHizBuffer 클래스 Build 함수 터짐 : CS_HizCopyDepth가 없음");
		return E_FAIL;
	}

	SPtr<CResComputeShader> pMipPyramidShader = CGameInstance::Get().GetResourceFirst<CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_HizMipPyramid");
	if (pMipPyramidShader == nullptr)
	{
		MSG_BOX("CHizBuffer 클래스 Build 함수 터짐 : CS_HizMipPyramid가 없음");
		return E_FAIL;
	}

	// 원본 depth SRV로 mip0의 UAV 만들어줌
	{
		m_pContext->CSSetShader(pCopyDepthShader->GetComputeShader().Get(), nullptr, 0);

		ID3D11ShaderResourceView* srvs[] = { depthSRV };
		m_pContext->CSSetShaderResources(0, 1, srvs);

		ID3D11UnorderedAccessView* uavs[] = { m_MipUAVs[0].Get() };
		m_pContext->CSSetUnorderedAccessViews(0, 1, uavs, nullptr);

		uint32_t groupX = (m_iWidth + 7) / 8;
		uint32_t groupY = (m_iHeight + 7) / 8;
		m_pContext->Dispatch(groupX, groupY, 1);

		// 바인딩 해제
		ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
		ID3D11UnorderedAccessView* nullUAV[1] = { nullptr };
		m_pContext->CSSetShaderResources(0, 1, nullSRV);
		m_pContext->CSSetUnorderedAccessViews(0, 1, nullUAV, nullptr);
	}

	for (uint32_t mip = 0; mip < m_iMipCount - 1; ++mip)
	{
		uint32_t outWidth = std::max(1u, m_iWidth >> (mip + 1));
		uint32_t outHeight = std::max(1u, m_iHeight >> (mip + 1));
		uint32_t groupX = (outWidth + 7) / 8;
		uint32_t groupY = (outHeight + 7) / 8;

		m_pContext->CSSetShader(pMipPyramidShader->GetComputeShader().Get(), nullptr, 0);

		ID3D11ShaderResourceView* srvs[] = { m_MipSRVs[mip].Get()};
		m_pContext->CSSetShaderResources(0, 1, srvs);

		ID3D11UnorderedAccessView* uavs[] = { m_MipUAVs[mip+1].Get()};
		m_pContext->CSSetUnorderedAccessViews(0, 1, uavs, nullptr);

		m_pContext->Dispatch(groupX, groupY, 1);

		// 바인딩 해제
		ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
		ID3D11UnorderedAccessView* nullUAV[1] = { nullptr };
		m_pContext->CSSetShaderResources(0, 1, nullSRV);
		m_pContext->CSSetUnorderedAccessViews(0, 1, nullUAV, nullptr);
	}

	m_pContext->CSSetShader(nullptr, nullptr, 0);


	return S_OK;
}

HRESULT CHizBuffer::ReadMipToCPU(uint32_t mip, std::vector<float>& outDepths, uint32_t& outWidth, uint32_t& outHeight) const
{
	if (mip >= m_iMipCount || m_pTexture == nullptr)
	{
		return E_FAIL;
	}

	outWidth = std::max(1u, m_iWidth >> mip);
	outHeight = std::max(1u, m_iHeight >> mip);

	D3D11_TEXTURE2D_DESC stagingDesc{};
	stagingDesc.Width = outWidth;
	stagingDesc.Height = outHeight;
	stagingDesc.MipLevels = 1;
	stagingDesc.ArraySize = 1;
	stagingDesc.Format = DXGI_FORMAT_R32_FLOAT;
	stagingDesc.SampleDesc.Count = 1;
	stagingDesc.SampleDesc.Quality = 0;
	stagingDesc.Usage = D3D11_USAGE_STAGING;
	stagingDesc.BindFlags = 0;
	stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	stagingDesc.MiscFlags = 0;

	ComPtr<ID3D11Texture2D> stagingTexture{};
	if (FAILED(m_pDevice->CreateTexture2D(&stagingDesc, nullptr, stagingTexture.GetAddressOf())))
	{
		return E_FAIL;
	}

	const uint32_t srcSubresource = D3D11CalcSubresource(mip, 0, m_iMipCount);
	m_pContext->CopySubresourceRegion(
		stagingTexture.Get(),
		0,
		0,
		0,
		0,
		m_pTexture.Get(),
		srcSubresource,
		nullptr);

	// GPU 결과를 CPU가 읽는 거라 Map()에서 stall (GPU가 원래 자기 할 일을 못함)
	D3D11_MAPPED_SUBRESOURCE mapped{};
	if (FAILED(m_pContext->Map(stagingTexture.Get(), 0, D3D11_MAP_READ, 0, &mapped)))
	{
		return E_FAIL;
	}

	outDepths.resize(static_cast<size_t>(outWidth) * static_cast<size_t>(outHeight));

	for (uint32_t y = 0; y < outHeight; ++y)
	{
		const auto* srcRow = reinterpret_cast<const float*>(
			static_cast<const uint8_t*>(mapped.pData) + static_cast<size_t>(mapped.RowPitch) * y);
		auto* dstRow = outDepths.data() + static_cast<size_t>(outWidth) * y;
		std::memcpy(dstRow, srcRow, sizeof(float) * outWidth);
	}

	m_pContext->Unmap(stagingTexture.Get(), 0);
	return S_OK;
}


HRESULT CHizBuffer::CreateResources()
{
	D3D11_TEXTURE2D_DESC desc{};
	desc.Width = m_iWidth;
	desc.Height = m_iHeight;
	desc.MipLevels = m_iMipCount;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R32_FLOAT;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;

	if (FAILED(m_pDevice->CreateTexture2D(&desc, nullptr, m_pTexture.GetAddressOf())))
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CHizBuffer::CreateViews()
{
	if (FAILED(m_pDevice->CreateShaderResourceView(m_pTexture.Get(), 0, m_pSRV.GetAddressOf()))) //원본 SRV생성
	{
		return E_FAIL;
	}

	m_MipSRVs.resize(m_iMipCount);
	m_MipUAVs.resize(m_iMipCount);

	for (uint32_t mip = 0; mip < m_iMipCount; ++mip)
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = mip;
		srvDesc.Texture2D.MipLevels = 1;

		if (FAILED(m_pDevice->CreateShaderResourceView(m_pTexture.Get(), &srvDesc, m_MipSRVs[mip].GetAddressOf())))
		{
			return E_FAIL;
		}


		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
		uavDesc.Format = DXGI_FORMAT_R32_FLOAT;
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Texture2D.MipSlice = mip; //m_MipUAVs[2]는 texture의 Mip 2에 쓰는 UAV가 됨

		if (FAILED(m_pDevice->CreateUnorderedAccessView(m_pTexture.Get(), &uavDesc, m_MipUAVs[mip].GetAddressOf())))
		{
			return E_FAIL;
		}
	}


	return S_OK;
}

UPtr<CHizBuffer> CHizBuffer::Create(ComPtr<ID3D11Device> device, ComPtr<ID3D11DeviceContext> context, uint32_t width, uint32_t height)
{
	auto pInstance = ToUPtr(new CHizBuffer(device, context));
	if (FAILED(pInstance->Initialize(width, height)))
	{
		MSG_BOX("Failed to Created : CHizBuffer");
		return nullptr;
	}
	return pInstance;
}

#include "pch.h"
#include "VideoObject.h"
#include "GameInstance.h"
#include "CameraObject.h"
#include "Resources.h"
#include "UIManager.h"
#include "Client_Defines.h"
#include "Level_Defines.h"

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>

#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")

NS_USING(Client)

CVideoObject::CVideoObject()
{
}

CVideoObject::~CVideoObject()
{
}

HRESULT CVideoObject::InitializePrototype(void* pArg)
{
	return S_OK;
}

HRESULT CVideoObject::Initialize(void* pArg)
{
	auto		pDesc = static_cast<CUIObject::UIOBJECT_DESC*>(pArg);

	if (FAILED(CUIObject::Initialize(pDesc)))
		return E_FAIL;


	{
		/* Buffer */
		CComConstantBuffer::DESC Desc{};
		Desc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, "CB_PerUI" };
		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ConstantBuffer", "ComCBufferPerUI", &Desc, &m_pComCBufferPerUI)))
		{
			return E_FAIL;
		};

		/* Component */
		CComponent::DESC CDesc{};
		Desc.pGameObject = this;

		if (FAILED(AddComponentFromProto(ES_EngineProtoMajorType::UI, "Prototype_Component_Tween", "Com_Tween", &CDesc, &m_pComTween)))
		{
			return E_FAIL;
		};
	}

	m_UIINFO.UIType = ETOUI(UI_TYPE::VIDEOOBJ);

	return S_OK;
}

void CVideoObject::PriorityUpdate(E::_float fTimeDelta)
{

}

void CVideoObject::Update(E::_float fTimeDelta)
{
	// 초기설정
	if (!SourceReader)
	{
		ComPtr<IMFAttributes> pAttributes;
		MFCreateAttributes(&pAttributes, 1);
		pAttributes->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);

		HRESULT hr = MFCreateSourceReaderFromURL(m_path.c_str(), pAttributes.Get(), m_pSourceReader.GetAddressOf());

		if (FAILED(hr))
		{
			// OutputDebugStringA("비디오 파일을 찾을 수 없거나 로드에 실패했습니다.\n");

			return;
		}

		SourceReader = true;

		{
			// 2. 비디오 파일을 읽어올 Source Reader 생성
			ComPtr<IMFAttributes> pAttributes;
			MFCreateAttributes(&pAttributes, 1);
			pAttributes->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);

			MFCreateSourceReaderFromURL(m_path.c_str(), pAttributes.Get(), m_pSourceReader.GetAddressOf());

			// 3. 디코더 출력 포맷 설정
			ComPtr<IMFMediaType> pMediaType;
			MFCreateMediaType(&pMediaType);
			pMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
			pMediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
			m_pSourceReader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, NULL, pMediaType.Get());

			// 4. 영상의 원본 가로세로 해상도 얻어오기
			ComPtr<IMFMediaType> pCurrentType;
			m_pSourceReader->GetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, &pCurrentType);
			MFGetAttributeSize(pCurrentType.Get(), MF_MT_FRAME_SIZE, &m_iVideoWidth, &m_iVideoHeight);

			// 5. DX11 동적 텍스처 생성
			D3D11_TEXTURE2D_DESC desc = {};
			desc.Width = m_iVideoWidth;
			desc.Height = m_iVideoHeight;
			desc.MipLevels = 1;
			desc.ArraySize = 1;
			desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // RGB32에 대응하는 포맷
			desc.SampleDesc.Count = 1;
			desc.Usage = D3D11_USAGE_DYNAMIC;         // 동적 텍스처
			desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; // CPU Write 허용

			E::CGameInstance::Get().GetGraphicDevice()->CreateTexture2D(&desc, nullptr, m_pVideoTexture.GetAddressOf());
			E::CGameInstance::Get().GetGraphicDevice()->CreateShaderResourceView(m_pVideoTexture.Get(), nullptr, m_pVideoSRV.GetAddressOf());
		}
	}

	_float2 mousePos = E::CGameInstance::Get().GetMousePos();

	if (!m_isActive)
		return;

	CUIObject::Update(fTimeDelta);

	if (m_pComTween != nullptr)
	{
		m_pComTween->Tick(fTimeDelta);
	}

	if (m_fStartDelay > 0.f)
	{
		m_fStartDelay -= fTimeDelta;
		return; // 아직 딜레이 시간이 남았다면 여기서 멈추고 프레임을 갱신하지 않음
	}

	m_fTimeAcc += fTimeDelta;
	float fTimePerFrame = 1.0f / m_fFrameRate;

	// 영상 프레임 갱신 주기가 도달했는지 확인
	if (m_fTimeAcc >= fTimePerFrame)
	{
		m_fTimeAcc -= fTimePerFrame;

		DWORD streamIndex, flags;
		LONGLONG llTimeStamp;
		ComPtr<IMFSample> pSample;

		// 다음 프레임 샘플을 하나 가져옵니다.
		m_pSourceReader->ReadSample(
			MF_SOURCE_READER_FIRST_VIDEO_STREAM,
			0, &streamIndex, &flags, &llTimeStamp, pSample.GetAddressOf()
		);
		
		// 정상적으로 프레임을 가져왔다면 텍스처에 복사
		if (pSample)
		{
			ComPtr<IMFMediaBuffer> pBuffer;
			pSample->ConvertToContiguousBuffer(pBuffer.GetAddressOf());

			BYTE* pVideoData = nullptr;
			DWORD maxLength, currentLength;
			// 버퍼를 잠그고 실제 픽셀 데이터 포인터를 얻어옴
			pBuffer->Lock(&pVideoData, &maxLength, &currentLength);

			// --- DX11 텍스처 덮어쓰기 (Map/Unmap) ---
			D3D11_MAPPED_SUBRESOURCE mappedResource;
			if (SUCCEEDED(E::CGameInstance::Get().GetGraphicDeviceContext()->Map(m_pVideoTexture.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
			{
				BYTE* pDest = (BYTE*)mappedResource.pData;
				BYTE* pSrc = pVideoData;
				UINT rowPitch = m_iVideoWidth * 4; // 가로 픽셀 수 * 4바이트(RGBA)

				for (UINT y = 0; y < m_iVideoHeight; ++y)
				{
					memcpy(pDest, pSrc, rowPitch);
					pDest += mappedResource.RowPitch;
					pSrc += rowPitch;
				}

				E::CGameInstance::Get().GetGraphicDeviceContext()->Unmap(m_pVideoTexture.Get(), 0);
			}
			pBuffer->Unlock(); // Unlock은 성공/실패 여부 상관없이 무조건 해줘야 함
		}
		else if (flags & MF_SOURCE_READERF_ENDOFSTREAM)
		{
			// 영상이 끝났을 경우 다시 처음으로 되감기
			PROPVARIANT var = { 0 };
			var.vt = VT_I8;
			m_pSourceReader->SetCurrentPosition(GUID_NULL, var);
		}
	}
}

void CVideoObject::LateUpdate(E::_float fTimeDelta)
{
	if (!m_isActive)
		return;

	CUIObject::LateUpdate(fTimeDelta);
}

HRESULT CVideoObject::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
	std::string currentLevel = _string("LEVEL_") + MagicEnumToStringView(static_cast<LEVEL>(E::CGameInstance::Get().GetCurrentLevelID())).data();

	const auto& viBuffer = E::CGameInstance::Get().GetResourceFirst<E::CResQuadTexBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "VIBuffer_QuadTex");
	const auto& vs = E::CGameInstance::Get().GetResourceFirst<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_VIDEOOBJECT");
	const auto& ps = E::CGameInstance::Get().GetResourceFirst<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_VIDEOOBJECT");

	pContext->IASetInputLayout(vs->GetInputLayout().Get());
	pContext->VSSetShader(vs->GetVertexShader().Get(), nullptr, 0);
	pContext->PSSetShader(ps->GetPixelShader().Get(), nullptr, 0);

	ID3D11Buffer* vertexBuffers[] = {
			viBuffer->GetVertexBuffer().Get()
	};
	uint32_t strides[] = {
		viBuffer->GetVertexStride()
	};
	uint32_t offsets[] = {
		0
	};
	pContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
	pContext->IASetIndexBuffer(viBuffer->GetIndexBuffer().Get(), viBuffer->GetIndexFormat(), 0);
	pContext->IASetPrimitiveTopology(viBuffer->GetPrimitiveType());

	{
		E::CB_PER_UI perUI{};
		perUI.texCoord = { 0.f, 0.f };
		perUI.uvSize = { 1.f, 1.f };
		perUI.color = { m_UIINFO.Color.x, m_UIINFO.Color.y, m_UIINFO.Color.z, m_UIINFO.Alpha };

		if (FAILED(m_pComCBufferPerUI->MapDiscard(pContext, &perUI, sizeof(perUI))))
		{
			return E_FAIL;
		}
		pContext->VSSetConstantBuffers(7, 1, m_pComCBufferPerUI->GetAdressOfBuffer());
		pContext->PSSetConstantBuffers(7, 1, m_pComCBufferPerUI->GetAdressOfBuffer());
	}

	{
		{
			auto pCbPerObject = E::CGameInstance::Get().GetResourceFirst<E::CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "CB_PerObject");
			D3D11_MAPPED_SUBRESOURCE mappedSubResource;
			if (SUCCEEDED(pContext->Map(pCbPerObject->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubResource)))
			{
				E::CB_PER_OBJECT cbPerObject{};

				_matrix matWVP = GetTransform().GetLoadedWorldMatrix() * ctx.matProj;
				XMStoreFloat4x4(&cbPerObject.matWVP, matWVP);

				memcpy(mappedSubResource.pData, &cbPerObject, sizeof(cbPerObject));
				pContext->Unmap(pCbPerObject->GetCBuffer().Get(), 0);
			}
			pContext->VSSetConstantBuffers(0, 1, pCbPerObject->GetCBuffer().GetAddressOf());
			pContext->PSSetConstantBuffers(0, 1, pCbPerObject->GetCBuffer().GetAddressOf());
		}
	}

	{
		ID3D11ShaderResourceView* srvs[] = { m_pVideoSRV.Get() };
		pContext->PSSetShaderResources(0, 1, srvs);
	}

	pContext->DrawIndexed(viBuffer->GetNumIndices(), 0, 0);

	return S_OK;
}

void CVideoObject::PlayEffect(uint32_t uiState)
{
	if (m_pComTween == nullptr)
		return;

	if (uiState & ETOUI(UI_STATE::APPEAR))
	{
		ClearEffectTweens();
		if (Appear) Appear(this);
	}

	if (uiState & ETOUI(UI_STATE::DISAPPEAR))
	{
		ClearEffectTweens();
		if (Disappear) Disappear(this);
	}

	if (m_bInputLocked)
		return;

	if (uiState & ETOUI(UI_STATE::ENTER))
	{
		if (OnHoverEnter) {
			ClearEffectTweens();
			OnHoverEnter(this);
		}
	}

	if (uiState & ETOUI(UI_STATE::EXIT))
	{
		if (OnHoverExit) {
			ClearEffectTweens();
			OnHoverExit(this);
		}
	}

	if (uiState & ETOUI(UI_STATE::CLICK))
	{

		if (OnClicked) {
			ClearEffectTweens();
			OnClicked(this);
		}
	}
}

E::UPtr<CVideoObject> CVideoObject::Create()
{
	auto pInstance = E::ToUPtr(new CVideoObject{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CVideoObject");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CVideoObject::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CVideoObject{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CVideoObject");
		return nullptr;
	}

	return pInstance;
}

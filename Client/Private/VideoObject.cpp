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
#include <objbase.h>
#include <propvarutil.h>
#include <cmath>

#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")

NS_USING(Client)

CVideoObject::CVideoObject()
{
}

CVideoObject::~CVideoObject()
{
	if (m_pAsyncState)
	{
		m_pAsyncState->Cancel.store(true);
		m_pAsyncState->Condition.notify_all();
	}
}

HRESULT CVideoObject::InitializePrototype(void* pArg)
{
	return S_OK;
}

HRESULT CVideoObject::Initialize(void* pArg)
{
	auto pDesc = static_cast<CUIObject::UIOBJECT_DESC*>(pArg);

	if (FAILED(CUIObject::Initialize(pDesc)))
		return E_FAIL;

	CComConstantBuffer::DESC Desc{};
	Desc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, "CB_PerUI" };
	if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ConstantBuffer",
		"ComCBufferPerUI", &Desc, &m_pComCBufferPerUI)))
		return E_FAIL;

	CComponent::DESC CDesc{};
	CDesc.pGameObject = this;
	if (FAILED(AddComponentFromProto(ES_EngineProtoMajorType::UI, "Prototype_Component_Tween",
		"Com_Tween", &CDesc, &m_pComTween)))
		return E_FAIL;

	m_UIINFO.UIType = ETOUI(UI_TYPE::VIDEOOBJ);
	return S_OK;
}

void CVideoObject::PriorityUpdate(E::_float fTimeDelta)
{
}

void CVideoObject::Update(E::_float fTimeDelta)
{
	if (m_eLoadState == VIDEO_LOAD_STATE::IDLE)
		BeginAsyncVideoLoad();

	if (!m_isActive)
		return;

	CUIObject::Update(fTimeDelta);

	if (m_pComTween != nullptr)
		m_pComTween->Tick(fTimeDelta);

	if (m_fStartDelay > 0.f)
	{
		m_fStartDelay -= fTimeDelta;
		return;
	}

	m_fTimeAcc += fTimeDelta;
	const float timePerFrame = 1.f / m_fFrameRate;

	if (m_fTimeAcc >= timePerFrame)
	{
		m_fTimeAcc = std::fmod(m_fTimeAcc, timePerFrame);
		ProcessDecodedFrame();
	}
}

void CVideoObject::BeginAsyncVideoLoad()
{
	m_eLoadState = VIDEO_LOAD_STATE::LOADING;
	m_pAsyncState = std::make_shared<VIDEO_ASYNC_STATE>();

	const std::wstring path = m_path;
	const auto state = m_pAsyncState;

	const _bool enqueued = E::CGameInstance::Get().WorkerEnqueue("VIDEO_DECODE", [path, state]()
		{
			const HRESULT comResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
			const _bool shouldUninitializeCOM = SUCCEEDED(comResult);

			if (FAILED(comResult) && comResult != RPC_E_CHANGED_MODE)
			{
				state->Failed.store(true);
				return;
			}

			ComPtr<IMFAttributes> attributes;
			ComPtr<IMFSourceReader> reader;
			ComPtr<IMFMediaType> outputType;
			ComPtr<IMFMediaType> currentType;

			HRESULT hr = MFCreateAttributes(attributes.GetAddressOf(), 1);
			if (SUCCEEDED(hr))
				hr = attributes->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);
			if (SUCCEEDED(hr))
				hr = MFCreateSourceReaderFromURL(path.c_str(), attributes.Get(), reader.GetAddressOf());
			if (SUCCEEDED(hr))
				hr = MFCreateMediaType(outputType.GetAddressOf());
			if (SUCCEEDED(hr))
				hr = outputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
			if (SUCCEEDED(hr))
				hr = outputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
			if (SUCCEEDED(hr))
				hr = reader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, outputType.Get());
			if (SUCCEEDED(hr))
				hr = reader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, currentType.GetAddressOf());

			UINT width{};
			UINT height{};
			if (SUCCEEDED(hr))
				hr = MFGetAttributeSize(currentType.Get(), MF_MT_FRAME_SIZE, &width, &height);

			while (SUCCEEDED(hr) && !state->Cancel.load())
			{
				{
					std::unique_lock<std::mutex> lock(state->Mutex);
					state->Condition.wait(lock, [state]()
						{
							return state->Cancel.load() || state->Frames.size() < 3;
						});
				}

				if (state->Cancel.load())
					break;

				DWORD streamIndex{};
				DWORD flags{};
				LONGLONG timeStamp{};
				ComPtr<IMFSample> sample;

				hr = reader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0,
					&streamIndex, &flags, &timeStamp, sample.GetAddressOf());

				if (FAILED(hr))
					break;

				if (flags & MF_SOURCE_READERF_ENDOFSTREAM)
				{
					PROPVARIANT position;
					PropVariantInit(&position);
					position.vt = VT_I8;
					position.hVal.QuadPart = 0;
					hr = reader->SetCurrentPosition(GUID_NULL, position);
					PropVariantClear(&position);
					continue;
				}

				if (!sample)
					continue;

				ComPtr<IMFMediaBuffer> buffer;
				hr = sample->ConvertToContiguousBuffer(buffer.GetAddressOf());
				if (FAILED(hr))
					break;

				BYTE* source{};
				DWORD maxLength{};
				DWORD currentLength{};
				hr = buffer->Lock(&source, &maxLength, &currentLength);
				if (FAILED(hr))
					break;

				VIDEO_FRAME frame{};
				frame.Width = width;
				frame.Height = height;
				frame.RowPitch = width * 4;
				frame.TimeStamp = timeStamp;
				frame.Pixels.assign(source, source + currentLength);
				buffer->Unlock();

				{
					std::lock_guard<std::mutex> lock(state->Mutex);
					state->Frames.push_back(std::move(frame));
				}
				state->Condition.notify_one();
			}

			if (FAILED(hr))
				state->Failed.store(true);

			if (shouldUninitializeCOM)
				CoUninitialize();
		});

	if (!enqueued)
	{
		m_eLoadState = VIDEO_LOAD_STATE::FAILED;
		m_pAsyncState.reset();
	}
}

void CVideoObject::ProcessDecodedFrame()
{
	const auto state = m_pAsyncState;
	if (!state)
		return;

	if (state->Failed.load())
	{
		m_eLoadState = VIDEO_LOAD_STATE::FAILED;
		return;
	}

	VIDEO_FRAME frame{};
	{
		std::lock_guard<std::mutex> lock(state->Mutex);
		if (state->Frames.empty())
			return;

		frame = std::move(state->Frames.front());
		state->Frames.pop_front();
	}
	state->Condition.notify_one();

	if (!m_pVideoTexture && FAILED(CreateVideoResources(frame.Width, frame.Height)))
	{
		m_eLoadState = VIDEO_LOAD_STATE::FAILED;
		state->Cancel.store(true);
		state->Condition.notify_all();
		return;
	}

	if (FAILED(UploadFrameToTexture(frame)))
	{
		m_eLoadState = VIDEO_LOAD_STATE::FAILED;
		return;
	}

	m_eLoadState = VIDEO_LOAD_STATE::READY;
}

HRESULT CVideoObject::CreateVideoResources(UINT width, UINT height)
{
	m_iVideoWidth = width;
	m_iVideoHeight = height;

	D3D11_TEXTURE2D_DESC desc{};
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	if (FAILED(E::CGameInstance::Get().GetGraphicDevice()->CreateTexture2D(
		&desc, nullptr, m_pVideoTexture.ReleaseAndGetAddressOf())))
		return E_FAIL;

	if (FAILED(E::CGameInstance::Get().GetGraphicDevice()->CreateShaderResourceView(
		m_pVideoTexture.Get(), nullptr, m_pVideoSRV.ReleaseAndGetAddressOf())))
		return E_FAIL;

	return S_OK;
}

HRESULT CVideoObject::UploadFrameToTexture(const VIDEO_FRAME& frame)
{
	if (!m_pVideoTexture || frame.Pixels.size() < static_cast<size_t>(frame.RowPitch) * frame.Height)
		return E_FAIL;

	D3D11_MAPPED_SUBRESOURCE mappedResource{};
	auto context = E::CGameInstance::Get().GetGraphicDeviceContext();
	if (FAILED(context->Map(m_pVideoTexture.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
		return E_FAIL;

	const BYTE* source = frame.Pixels.data();
	BYTE* destination = static_cast<BYTE*>(mappedResource.pData);
	for (UINT y = 0; y < frame.Height; ++y)
	{
		memcpy(destination, source, frame.RowPitch);
		destination += mappedResource.RowPitch;
		source += frame.RowPitch;
	}

	context->Unmap(m_pVideoTexture.Get(), 0);
	return S_OK;
}

void CVideoObject::LateUpdate(E::_float fTimeDelta)
{
	if (!m_isActive)
		return;

	CUIObject::LateUpdate(fTimeDelta);
}

HRESULT CVideoObject::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
	if (!m_pVideoSRV)
		return S_OK;

	const auto& viBuffer = E::CGameInstance::Get().GetResourceFirst<E::CResQuadTexBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "VIBuffer_QuadTex");
	const auto& vs = E::CGameInstance::Get().GetResourceFirst<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_VIDEOOBJECT");
	const auto& ps = E::CGameInstance::Get().GetResourceFirst<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_VIDEOOBJECT");

	pContext->IASetInputLayout(vs->GetInputLayout().Get());
	pContext->VSSetShader(vs->GetVertexShader().Get(), nullptr, 0);
	pContext->PSSetShader(ps->GetPixelShader().Get(), nullptr, 0);

	ID3D11Buffer* vertexBuffers[] = { viBuffer->GetVertexBuffer().Get() };
	uint32_t strides[] = { viBuffer->GetVertexStride() };
	uint32_t offsets[] = { 0 };
	pContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
	pContext->IASetIndexBuffer(viBuffer->GetIndexBuffer().Get(), viBuffer->GetIndexFormat(), 0);
	pContext->IASetPrimitiveTopology(viBuffer->GetPrimitiveType());

	E::CB_PER_UI perUI{};
	perUI.texCoord = { 0.f, 0.f };
	perUI.uvSize = { 1.f, 1.f };
	perUI.color = { m_UIINFO.Color.x, m_UIINFO.Color.y, m_UIINFO.Color.z, m_UIINFO.Alpha };

	if (FAILED(m_pComCBufferPerUI->MapDiscard(pContext, &perUI, sizeof(perUI))))
		return E_FAIL;

	pContext->VSSetConstantBuffers(7, 1, m_pComCBufferPerUI->GetAdressOfBuffer());
	pContext->PSSetConstantBuffers(7, 1, m_pComCBufferPerUI->GetAdressOfBuffer());

	auto pCbPerObject = E::CGameInstance::Get().GetResourceFirst<E::CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "CB_PerObject");
	D3D11_MAPPED_SUBRESOURCE mappedSubResource{};
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

	ID3D11ShaderResourceView* srvs[] = { m_pVideoSRV.Get() };
	pContext->PSSetShaderResources(0, 1, srvs);
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
		if (Appear)
			Appear(this);
	}

	if (uiState & ETOUI(UI_STATE::DISAPPEAR))
	{
		ClearEffectTweens();
		if (Disappear)
			Disappear(this);
	}

	if (m_bInputLocked)
		return;

	if (uiState & ETOUI(UI_STATE::ENTER))
	{
		if (OnHoverEnter)
		{
			ClearEffectTweens();
			OnHoverEnter(this);
		}
	}

	if (uiState & ETOUI(UI_STATE::EXIT))
	{
		if (OnHoverExit)
		{
			ClearEffectTweens();
			OnHoverExit(this);
		}
	}

	if (uiState & ETOUI(UI_STATE::CLICK))
	{
		if (OnClicked)
		{
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
	return pInstance;
}

E::UPtr<E::CPrototype> CVideoObject::Clone(void* pArg)
{
	auto pInstance = E::ToUPtr(new CVideoObject{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CVideoObject");
		return nullptr;
	}

	return pInstance;
}

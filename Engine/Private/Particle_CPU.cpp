#include "pch.h"
#include "Particle_CPU.h"
#include "GameInstance.h"
#include "Resources.h"

NS_USING(Engine)

CParticle_CPU::CParticle_CPU()
{
}



CParticle_CPU::~CParticle_CPU()
{
}

HRESULT CParticle_CPU::Initialize(void* pArg)
{
    m_vecInstancedData.clear();



    auto pDesc = static_cast<DESC*>(pArg);
    if (pDesc == nullptr)
        return E_FAIL;

    m_Desc = *pDesc;
    m_iNumElements = m_Desc.iMaxParticles;
    m_viBufferID = m_Desc.viBufferID;
    m_eType = pDesc->type;

    m_Particles.assign(m_iNumElements, PARTICLE_CPU_DATA{});

    if (auto res = CResDynamicBuffer::Create())
    {
        CResDynamicBuffer::DESC bufDesc{};
        bufDesc.desc = {
            .ByteWidth = (uint32_t)sizeof(VTX_PARTICLE_INSTANCED_DATA) * m_iNumElements,
            .Usage = D3D11_USAGE_DYNAMIC,
            .BindFlags = D3D11_BIND_VERTEX_BUFFER,
            .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
            .MiscFlags = 0,
            .StructureByteStride = 0,
        };

        if (FAILED(res->Load(bufDesc)))
            return E_FAIL;

        m_pResInstancedBuffer = res;
    }
    m_pResSamplerState = CGameInstance::Get().GetResourceFirst<CResSamplerState>(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_SS_LINEAR_WRAP);
    if (!m_pResSamplerState)
        return E_FAIL;
	m_pNoiseTexture = CGameInstance::Get().GetResourceFirst<CResTexture2D>("SAMPLE_CLINET_TEXTURE", "TEX_NOISE");
	switch (m_Desc.blendState) {
		case 0:
			m_pBlendState = CGameInstance::Get().GetResourceFirst<CResBlendState>(TAG_RES_GRP_PERMANENT_STATE, "BS_ALPHA_EFFECT");
			break;
		case 1:
			m_pBlendState = CGameInstance::Get().GetResourceFirst<CResBlendState>(TAG_RES_GRP_PERMANENT_STATE, "BS_ADDITIVE");
			break;
		case 2:
			m_pBlendState = CGameInstance::Get().GetResourceFirst<CResBlendState>(TAG_RES_GRP_PERMANENT_STATE, "BS_BLEND_NONE");
			break;
		default:
			m_pBlendState = CGameInstance::Get().GetResourceFirst<CResBlendState>(TAG_RES_GRP_PERMANENT_STATE, "BS_ALPHA_EFFECT");
			break;
	}


	m_pResVertexShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(pDesc->VSID.first, pDesc->VSID.second);

	if (FAILED(m_pResVertexShader->Load(CResShader::DESC{ .sEntryPoint = m_Desc.sVEntryPoint,  .sTarget = "vs_5_0" })))
		return E_FAIL;

	m_pResPixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(pDesc->PSID.first, pDesc->PSID.second);
	if (FAILED(m_pResPixelShader->Load(CResShader::DESC{ .sEntryPoint = m_Desc.sPEntryPoint,  .sTarget = "ps_5_0" })))
		return E_FAIL;


    if (m_Desc.whatKind == MESHORTEXTURE::TEX) {
		if (m_Desc.normalTextureID.first != "") {
			m_pNormalTexture = CGameInstance::Get().GetResourceFirst<CResTexture2D>(m_Desc.normalTextureID.first, m_Desc.normalTextureID.second);
		}
		if (m_Desc.distortionTextureID.first != "") {
			m_pDistortionTexture = CGameInstance::Get().GetResourceFirst<CResTexture2D>(m_Desc.distortionTextureID.first, m_Desc.distortionTextureID.second);
		}
		if (m_Desc.noiseTextureID.first != "") {
			m_pNoiseTexture = CGameInstance::Get().GetResourceFirst<CResTexture2D>(m_Desc.noiseTextureID.first, m_Desc.noiseTextureID.second);
		}
        if (FAILED(LoadParticleTexture(m_Desc.textureID)))
            return E_FAIL;

    }
    else if (m_Desc.whatKind == MESHORTEXTURE::MESH) {

		if (m_Desc.noiseTextureID.first != "") {
			m_pNoiseTexture = CGameInstance::Get().GetResourceFirst<CResTexture2D>(m_Desc.noiseTextureID.first, m_Desc.noiseTextureID.second);
		}
        m_pComCBuffer = CGameInstance::Get().GetResourceFirst<CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_OBJECT);
        if (!m_pComCBuffer)
            return E_FAIL;

        {
            CComStaticModelInstance::DESC modelDesc{};
            modelDesc.sGroupTag = m_Desc.sGroupTag;   // 밖에서 주입
            modelDesc.sResTag = m_Desc.sResTag;     // 밖에서 주입


            auto pProto = CGameInstance::Get().ClonePrototype("PERMANENT", "Prototype_Component_StaticModelInstance", &modelDesc);
            if (pProto == nullptr)
                return E_FAIL;
            m_pComModelInstance = UPtr<CComStaticModelInstance>(static_cast<CComStaticModelInstance*>(pProto.release()));

            if (!m_pComModelInstance)
                return E_FAIL;
        }
		if (m_Desc.hdrPositionTextureID.first != "") {
			m_pHdrPositionTexture = CGameInstance::Get().GetResourceFirst<CResTexture2D>(m_Desc.hdrPositionTextureID.first, m_Desc.hdrPositionTextureID.second);
		}
		if (m_Desc.hdrNormalTextureID.first != "") {
			m_pHdrNormalTexture = CGameInstance::Get().GetResourceFirst<CResTexture2D>(m_Desc.hdrNormalTextureID.first, m_Desc.hdrNormalTextureID.second);
		}
		if (m_Desc.anyTextureID.second != "") {
			m_pAnyTexture = CGameInstance::Get().GetResourceFirst<CResTexture2D>(m_Desc.anyTextureID.first, m_Desc.anyTextureID.second);
		}
    }

	{
		m_waveCb.g_fBurstRatio = Randf(0.3f, 0.6f);
		m_waveCb.g_fBurstSpeed = Randf(1.f, 1.f);
		m_waveCb.g_fFlowSpeed = Randf(1.f, 3.f);
		m_waveCb.g_fTransitionRatio = Randf(0.2f, 0.6f);
		m_waveCb.g_fWaveAmplitude = Randf(0.f, 3.f);
		m_waveCb.g_fWaveFrequency = Randf(0.f, 3.f);
		m_waveCb.g_fWaveSpeed = Randf(0.5f, 1.f);
		m_waveCb.g_vFlowDirection = _float3(Randf(-1, 1), Randf(-1, 1), Randf(-1, 1));
	}
    return S_OK;

}

void CParticle_CPU::PriorityUpdate(E::_float fTimeDelta)
{
}

void CParticle_CPU::Update(E::_float fTimeDelta)
{
	ProcessPendingSpawns(fTimeDelta);
    Simulate(fTimeDelta);

}

void CParticle_CPU::LateUpdate(E::_float fTimeDelta)
{
}

void CParticle_CPU::Simulate(E::_float fTimeDelta)
{
	m_vecInstancedData.clear();
	uint32_t totalFrames = m_Desc.TexRows * m_Desc.TexColumns;

	for (auto& p : m_Particles)
	{
		if (!p.bAlive)
			continue;

		p.life += fTimeDelta; // "지난 시간" 방식 그대로 유지 (원래 Simulate 스타일)

		if (p.life >= p.fMaxLife)
		{
			if (p.loop)
			{
				p.life = 0.f;                       // 지난 시간 리셋
				p.vPosition = p.originalPosition;    // 원래 위치로 복귀
				continue;                            // 리셋된 프레임은 렌더링 스킵
			}
			else
			{
				p.bAlive = false;
				continue;
			}
		}
		float ageRatio = std::clamp(p.life / p.fMaxLife, 0.f, 1.f);
		XMStoreFloat3(&p.vPosition, XMLoadFloat3(&p.vPosition) + XMLoadFloat3(&p.vVelocity) * fTimeDelta);
		UpdateBehavior(p, fTimeDelta);

		if (m_vecInstancedData.size() >= m_iNumElements)
			continue;

		if (totalFrames > 1)
		{
			uint32_t frame = (uint32_t)(ageRatio * totalFrames);
			p.iFrameIndex = std::min(frame, totalFrames - 1);
		}
		else
		{
			p.iFrameIndex = 0;
		}

		VTX_PARTICLE_INSTANCED_DATA inst{};
		inst.iBehaviorType = p.iBehaviorType;
		
		p.fSize.x = std::lerp(p.fStartSize.x, p.fEndSize.x, ageRatio);
		p.fSize.y = std::lerp(p.fStartSize.y, p.fEndSize.y, ageRatio);
		p.fSize.z = std::lerp(p.fStartSize.z, p.fEndSize.z, ageRatio);

		
		_matrix matScale = XMMatrixScaling(p.fSize.x, p.fSize.y, p.fSize.z);
		_matrix matTrans = XMMatrixTranslation(p.vPosition.x, p.vPosition.y, p.vPosition.z);

		_matrix matWorld;
		if ((p.iBehaviorType & CParticle::BEHAVIOR_BILLBOARD) != 0 && m_Desc.whatKind == MESHORTEXTURE::TEX)
		{
			auto camera = CGameInstance::Get().GetActiveCamera();
			if (!camera)
				return;
			_matrix matView = camera->GetView();
			_matrix matInvView = XMMatrixInverse(nullptr, matView);
			XMVECTOR camRight = matInvView.r[0];
			XMVECTOR camUp = matInvView.r[1];
			XMVECTOR camForward = matInvView.r[2];

			_matrix matBillboardRot = XMMatrixIdentity();
			matBillboardRot.r[0] = camRight;
			matBillboardRot.r[1] = camUp;
			matBillboardRot.r[2] = camForward;
			matBillboardRot.r[3] = XMVectorSet(0.f, 0.f, 0.f, 1.f);

			matWorld = matScale * matBillboardRot * matTrans;
		}
		else
		{
			_matrix matRotation = XMMatrixRotationRollPitchYaw(p.rotation.x, p.rotation.y, p.rotation.z);
			matWorld = matScale * matRotation * matTrans;
		}

		XMStoreFloat4x4(&inst.matWorld, matWorld);
		inst.vColor = p.vColor;
		inst.originalEmissive = p.originalEmissive;
		inst.emissive = p.emissive;
		inst.endEmissive = p.endEmissive;
		inst.life = p.life;
		inst.maxLife = p.fMaxLife;
		//inst.iBehaviorType = p.iBehaviorType;

		if (m_Desc.TexColumns > 0 && m_Desc.TexRows > 0)
		{
			uint32_t col = p.iFrameIndex % m_Desc.TexColumns;
			uint32_t row = p.iFrameIndex / m_Desc.TexColumns;
			inst.vUVSize = _float2(1.0f / m_Desc.TexColumns, 1.0f / m_Desc.TexRows);
			inst.vUVOffset = _float2(col * inst.vUVSize.x, row * inst.vUVSize.y);
		}
		else
		{
			inst.vUVSize = _float2(1.0f, 1.0f);
			inst.vUVOffset = _float2(0.0f, 0.0f);
		}

		m_vecInstancedData.push_back(inst);
	}
}
void CParticle_CPU::UpdateBehavior(PARTICLE_CPU_DATA& p, E::_float fTimeDelta)
{

	if ((p.iBehaviorType & CParticle::BEHAVIOR_DISTORTION) != 0) {
		int a = 0;
	}
	if ((p.iBehaviorType & CParticle::BEHAVIOR_GRAVITY) != 0) {
		const float kGravity = -9.8f;

		p.vVelocity.y += kGravity * fTimeDelta;

		// 기존 위치에 이동량을 더해야 함
		XMVECTOR vPos = XMLoadFloat3(&p.vPosition);
		XMVECTOR vVel = XMLoadFloat3(&p.vVelocity);
		vPos = XMVectorAdd(vPos, XMVectorScale(vVel, fTimeDelta));
		XMStoreFloat3(&p.vPosition, vPos);
	}

	// UpdateBehavior - waveCb / waveParticleIndex(클래스 멤버) 대신 p의 값을 사용, 증가 로직 제거
	if ((p.iBehaviorType & BEHAVIOR_CIRCLE_TO_WAVE) != 0)
	{
		uint32_t particleIndex = (uint32_t)(&p - m_Particles.data());

		float rnd1 = Hash01(particleIndex);
		float rnd2 = Hash01(particleIndex ^ 0x9E3779B9u);
		float angle = rnd1 * XM_2PI;
		float phaseOffset = rnd2 * XM_2PI;

		const auto& waveCb = m_waveCb; // 파티클 고유값, 매 프레임 그대로 유지

		auto Saturate = [](float v) { return std::max(0.0f, std::min(1.0f, v)); };
		auto SmoothStep = [](float edge0, float edge1, float x) -> float
			{
				float denom = edge1 - edge0;
				float t = (denom > 1e-6f) ? (x - edge0) / denom : 0.0f;
				t = std::max(0.0f, std::min(1.0f, t));
				return t * t * (3.0f - 2.0f * t);
			};

		float ageRatio = Saturate(p.life / p.fMaxLife);

		float burstT = Saturate(ageRatio / std::max(waveCb.g_fBurstRatio, 0.0001f));
		float burstProfile = sinf(XM_PI * burstT);

		float radialX = cosf(angle);
		float radialY = sinf(angle);
		float burstVelX = radialX * waveCb.g_fBurstSpeed * burstProfile;
		float burstVelY = radialY * waveCb.g_fBurstSpeed * burstProfile;

		float flowDirX = radialX;
		float flowDirY = radialY;
		float perpX = -flowDirY;
		float perpY = flowDirX;

		float elapsed = p.life;
		float wavePhase = waveCb.g_fWaveSpeed * elapsed + phaseOffset
			+ (p.vPosition.x * flowDirX + p.vPosition.y * flowDirY) * waveCb.g_fWaveFrequency;
		float waveBob = sinf(wavePhase);

		float waveVelX = flowDirX * waveCb.g_fFlowSpeed + perpX * (waveBob * waveCb.g_fWaveAmplitude);
		float waveVelY = flowDirY * waveCb.g_fFlowSpeed + perpY * (waveBob * waveCb.g_fWaveAmplitude);

		float blend = SmoothStep(waveCb.g_fBurstRatio - waveCb.g_fTransitionRatio * 0.5f,
			waveCb.g_fBurstRatio + waveCb.g_fTransitionRatio * 0.5f,
			ageRatio);

		p.vVelocity.x = burstVelX + (waveVelX - burstVelX) * blend;
		p.vVelocity.y = burstVelY + (waveVelY - burstVelY) * blend;
		p.vVelocity.z = 0.0f;

		XMVECTOR vPos = XMLoadFloat3(&p.vPosition);
		XMVECTOR vVel = XMLoadFloat3(&p.vVelocity);
		vPos = XMVectorAdd(vPos, XMVectorScale(vVel, fTimeDelta));
		XMStoreFloat3(&p.vPosition, vPos);
		// waveParticleIndex++;  <- 제거: 더 이상 필요 없음, p.iParticleID로 대체
	}

	if ((p.iBehaviorType & CParticle::BEHAVIOR_SMOKE) != 0) {
		MakeSmoke(p, fTimeDelta);
	}if ((p.iBehaviorType & CParticle::BEHAVIOR_SMOKEJUMP) != 0) {
		JumpSmoke(p, fTimeDelta);
	}if ((p.iBehaviorType & CParticle::BEHAVIOR_SMOKEGV) != 0) {
		GVBurstSmoke(p, fTimeDelta);
	}if ((p.iBehaviorType & CParticle::BEHAVIOR_SMOKEGW) != 0) {
		GWWaveSmoke(p, fTimeDelta);
	}

	
}
void CParticle_CPU::MakeSmoke(PARTICLE_CPU_DATA& p,_float fTimeDelta)
{
	_float fSpeed = 1.5f;
	
	_float t = p.life / p.fMaxLife;
	p.vColor.w = 0.7f + (0.f - 0.7f) * t;
	
	if(t >=0.5f)
		p.vVelocity.y += fSpeed * fTimeDelta;
	
	  
}
void CParticle_CPU::JumpSmoke(PARTICLE_CPU_DATA& p, _float fTimeDelta)
{

	_float fSpeed = 5.5f;

	_float t = p.life / p.fMaxLife;
	p.vColor.w = 0.7f + (0.f - 0.7f) * t;

	if (t >= 0.2f)
		p.vVelocity.y += fSpeed * fTimeDelta;
}

void CParticle_CPU::GVBurstSmoke(PARTICLE_CPU_DATA& p, _float fTimeDelta)
{
	XMStoreFloat3(&p.vVelocity,XMLoadFloat3(&p.vVelocity) * expf(-2.f * fTimeDelta));
}

void CParticle_CPU::GWWaveSmoke(PARTICLE_CPU_DATA& p, _float fTimeDelta)
{
	XMStoreFloat3(&p.vVelocity, XMLoadFloat3(&p.vVelocity) * expf(-4.f * fTimeDelta));

}


HRESULT CParticle_CPU::Spawn(uint32_t count, const PARTICLE_SPAWN_DATA* pSpawnData)
{
	if (pSpawnData == nullptr || count == 0)
		return E_FAIL;

	uint32_t iSpawned = 0;


	for (uint32_t i = 0; i < m_Particles.size() && iSpawned < count; ++i)
	{
		if (m_Particles[i].bAlive)
			continue;

		const auto& src = pSpawnData[iSpawned];
		m_Particles[i].vPosition = src.position;
		m_Particles[i].vVelocity = src.velocity;
		m_Particles[i].originalPosition = src.originalPosition;
		m_Particles[i].life = 0.f;
		m_Particles[i].fMaxLife = src.life; 
		m_Particles[i].bAlive = true;
		m_Particles[i].fSize = src.fSize;
		m_Particles[i].fStartSize = src.fSize;
		m_Particles[i].fEndSize = src.fEndSize;
		m_Particles[i].vColor = src.color;
		m_Particles[i].originalEmissive = src.originalEmissive;
		m_Particles[i].emissive = src.emissive;
		m_Particles[i].endEmissive = src.endEmissive;
		m_Particles[i].spawnDelay = src.spawnDelay;
		m_Particles[i].ownerID = src.ownerID;
		m_Particles[i].rotation = src.rotation;
		m_Particles[i].iBehaviorType = src.iBehaviorType;
		m_Particles[i].loop = src.loop;


	
		++iSpawned;
	}

	return (iSpawned == count) ? S_OK : E_FAIL;
}

HRESULT CParticle_CPU::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{

    if (m_vecInstancedData.empty())
        return S_OK;

    if (m_Desc.whatKind == MESHORTEXTURE::MESH)
        return Render_Mesh(pContext, ctx);

    return Render_Texture(pContext, ctx); // 기존 텍스처 파티클 렌더 코드
}


HRESULT CParticle_CPU::Render_Mesh(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{

	pContext->OMSetBlendState(m_pBlendState->GetBlendState().Get(), nullptr, 0xffffffff);

    if (m_vecInstancedData.empty())
        return S_OK;


	if (m_pHdrPositionTexture) {
		ID3D11ShaderResourceView* pHdrSRV = m_pHdrPositionTexture->GetSRV().Get();
		pContext->VSSetShaderResources(1, 1, &pHdrSRV);
	}

	if (m_pHdrNormalTexture) {
		ID3D11ShaderResourceView* pHdrSRV = m_pHdrNormalTexture->GetSRV().Get();
		pContext->VSSetShaderResources(2, 1, &pHdrSRV);
	}

	if (m_pNoiseTexture)
	{
		ID3D11ShaderResourceView* pNoiseSRV = m_pNoiseTexture->GetSRV().Get();
		pContext->PSSetShaderResources(5, 1, &pNoiseSRV);

	}
	if (m_pDistortionTexture)
	{
		ID3D11ShaderResourceView* pDistortionSRV = m_pDistortionTexture->GetSRV().Get();
		pContext->PSSetShaderResources(6, 1, &pDistortionSRV);

	}
	if (m_pAnyTexture)
	{
		ID3D11ShaderResourceView* pAnySRV = m_pAnyTexture->GetSRV().Get();
		pContext->PSSetShaderResources(8, 1, &pAnySRV);

	}

    const auto& vs = m_pResVertexShader;
    const auto& ps = m_pResPixelShader;
    pContext->IASetInputLayout(vs->GetInputLayout().Get());
    pContext->VSSetShader(vs->GetVertexShader().Get(), nullptr, 0);
    pContext->PSSetShader(ps->GetPixelShader().Get(), nullptr, 0);

    auto pModel = m_pComModelInstance->GetModel();
    uint32_t iNumMeshes = pModel->Get_NumMeshes();




    // 인스턴스 데이터(월드행렬/컬러) 업로드 -- 텍스처 버전과 동일
    {
        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (SUCCEEDED(pContext->Map(m_pResInstancedBuffer->GetBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            std::memcpy(mapped.pData, m_vecInstancedData.data(),
                sizeof(VTX_PARTICLE_INSTANCED_DATA) * m_vecInstancedData.size());
            pContext->Unmap(m_pResInstancedBuffer->GetBuffer().Get(), 0);
        }
    }
  //  auto& viBuffer0 = pModel->GetMeshes()[1];
    for (uint32_t i = 0; i < iNumMeshes; ++i)
    {
        const auto& viBuffer = pModel->GetMeshes()[i];
        ID3D11Buffer* vertexBuffers[] = {
            viBuffer->GetVertexBuffer().Get(),
            m_pResInstancedBuffer->GetBuffer().Get()  // 슬롯1: 인스턴스별 월드행렬/컬러
        };
        uint32_t strides[] = {
            viBuffer->GetVertexStride(),
            (uint32_t)sizeof(VTX_PARTICLE_INSTANCED_DATA)
        };
        uint32_t offsets[] = { 0, 0 };


        pContext->IASetVertexBuffers(0, 2, vertexBuffers, strides, offsets);
        pContext->IASetIndexBuffer(viBuffer->GetIndexBuffer().Get(), viBuffer->GetIndexFormat(), 0);
        pContext->IASetPrimitiveTopology(viBuffer->GetPrimitiveType());

		SPtr<CResTexture2D> DiffuseTexture = E::CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_DIFFUSE");
		if (auto Resource = m_pComModelInstance->Get_MeshTexture(i, AI_TEXTURE_TYPE::aiTextureType_DIFFUSE, 0)) {
			DiffuseTexture = Resource;
		}
		pContext->PSSetShaderResources(0, 1, DiffuseTexture->GetSRV().GetAddressOf());
		SPtr<CResTexture2D> NormalTexture = E::CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_NORMAL");
		if (auto Resource = m_pComModelInstance->Get_MeshTexture(i, AI_TEXTURE_TYPE::aiTextureType_NORMALS, 0)) {
			NormalTexture = Resource;
		}
		pContext->PSSetShaderResources(1, 1, NormalTexture->GetSRV().GetAddressOf());

		SPtr<CResTexture2D> SMROTexture = E::CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_SMRO");
		if (auto Resource = m_pComModelInstance->Get_MeshTexture(i, AI_TEXTURE_TYPE::aiTextureType_METALNESS, 0)) {
			SMROTexture = Resource;
		}
		pContext->PSSetShaderResources(2, 1, SMROTexture->GetSRV().GetAddressOf());

		SPtr<CResTexture2D> EmissiveTexture = E::CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_EMISSIVE");
		if (auto Resource = m_pComModelInstance->Get_MeshTexture(i, AI_TEXTURE_TYPE::aiTextureType_EMISSIVE, 0)) {
			EmissiveTexture = Resource;
		}
		pContext->PSSetShaderResources(3, 1, EmissiveTexture->GetSRV().GetAddressOf());
        //m_pComModelInstance->Bind_Materials(pContext, i, AI_TEXTURE_TYPE::aiTextureType_DIFFUSE, 0);
        //m_pComModelInstance->Bind_Materials(pContext, i, AI_TEXTURE_TYPE::aiTextureType_NORMALS, 0);

      //  pContext->PSSetSamplers(0, 1, m_pResSamplerState->GetSamplerState().GetAddressOf());

        pContext->DrawIndexedInstanced((UINT)viBuffer->GetNumIndices(), (UINT)m_vecInstancedData.size(), 0, 0, 0);
    }



	ID3D11ShaderResourceView* pSRVs[1] = { nullptr };
	pContext->PSSetShaderResources(0, 1, pSRVs);
	pContext->PSSetShaderResources(1, 1, pSRVs);
	pContext->PSSetShaderResources(2, 1, pSRVs);
	pContext->PSSetShaderResources(3, 1, pSRVs);
	pContext->PSSetShaderResources(5, 1, pSRVs);
	pContext->PSSetShaderResources(6, 1, pSRVs);
	pContext->PSSetShaderResources(8, 1, pSRVs);
	pContext->VSSetShaderResources(1, 1, pSRVs);
	pContext->VSSetShaderResources(2, 1, pSRVs);
	ID3D11Buffer* nullCB[] = { nullptr };

	pContext->VSSetConstantBuffers(5, 1, nullCB);
	pContext->PSSetConstantBuffers(5, 1, nullCB);
	{
		ID3D11ShaderResourceView* nullSRV[] = { nullptr };
		pContext->VSSetShaderResources(4, 1, nullSRV);
	}
	pContext->OMSetDepthStencilState(nullptr, 0);

	pContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);

    return S_OK;
}


HRESULT CParticle_CPU::Render_Texture(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
    if (m_vecInstancedData.empty())
        return S_OK;

	pContext->OMSetBlendState(m_pBlendState->GetBlendState().Get(), nullptr, 0xffffffff);

	SPtr<CResDepthStencilState> DepthState = CGameInstance::Get().GetResourceFirst<CResDepthStencilState>(TAG_RES_GRP_PERMANENT_STATE, "DS_ALPHA_BLEND_DEPTH");
	pContext->OMSetDepthStencilState(DepthState->GetDepthStencilState().Get(), 0);

    const auto& viBuffer = CGameInstance::Get().GetResourceFirst<CResVIBuffer>(m_viBufferID.first, m_viBufferID.second);

    pContext->IASetInputLayout(m_pResVertexShader->GetInputLayout().Get());
    pContext->VSSetShader(m_pResVertexShader->GetVertexShader().Get(), nullptr, 0);
    pContext->PSSetShader(m_pResPixelShader->GetPixelShader().Get(), nullptr, 0);

    ID3D11Buffer* vertexBuffers[] = {
        viBuffer->GetVertexBuffer().Get(),
        m_pResInstancedBuffer->GetBuffer().Get()
    };
    uint32_t strides[] = {
        viBuffer->GetVertexStride(),
        (uint32_t)sizeof(VTX_PARTICLE_INSTANCED_DATA),
    };
    uint32_t offsets[] = { 0, 0 };

    pContext->IASetVertexBuffers(0, 2, vertexBuffers, strides, offsets);
    pContext->IASetIndexBuffer(viBuffer->GetIndexBuffer().Get(), viBuffer->GetIndexFormat(), 0);
    pContext->IASetPrimitiveTopology(viBuffer->GetPrimitiveType());

    {
        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (SUCCEEDED(pContext->Map(m_pResInstancedBuffer->GetBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            std::memcpy(mapped.pData, m_vecInstancedData.data(),
                sizeof(VTX_PARTICLE_INSTANCED_DATA) * m_vecInstancedData.size());
            pContext->Unmap(m_pResInstancedBuffer->GetBuffer().Get(), 0);
        }
    }

	SPtr<CResTexture2D> DiffuseTexture = E::CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_DIFFUSE");
	if (m_pParticleTexture) {
		DiffuseTexture = m_pParticleTexture;
	}
	pContext->PSSetShaderResources(1, 1, DiffuseTexture->GetSRV().GetAddressOf());
	SPtr<CResTexture2D> NormalTexture = E::CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_NORMAL");
	if (m_pNormalTexture) {
		NormalTexture = m_pNormalTexture;
	}
	pContext->PSSetShaderResources(2, 1, NormalTexture->GetSRV().GetAddressOf());


	if (m_pDistortionTexture)
	{
		ID3D11ShaderResourceView* pDistortionSRV = m_pDistortionTexture->GetSRV().Get();
		pContext->PSSetShaderResources(3, 1, &pDistortionSRV);
	}
	if (m_pNoiseTexture)
	{
		ID3D11ShaderResourceView* pNoiseSRV = m_pNoiseTexture->GetSRV().Get();
		pContext->PSSetShaderResources(4, 1, &pNoiseSRV);
	}
	if (m_pAnyTexture)
	{
		ID3D11ShaderResourceView* pAnySRV = m_pAnyTexture->GetSRV().Get();
		pContext->PSSetShaderResources(5, 1, &pAnySRV);

	}


    pContext->DrawIndexedInstanced((UINT)viBuffer->GetNumIndices(), (UINT)m_vecInstancedData.size(), 0, 0, 0);

    ID3D11ShaderResourceView* nullSRV[] = { nullptr,nullptr,nullptr,nullptr,nullptr,nullptr };
    pContext->PSSetShaderResources(0, 6, nullSRV);

	pContext->OMSetDepthStencilState(nullptr, 0);
	pContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);


    return S_OK;
}
UPtr<CParticle> CParticle_CPU::Create(void* pArg)
{
	auto pInstance = E::ToUPtr(new CParticle_CPU{});
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Created : CParticle_CPU");
		return nullptr;
	}
	return  pInstance;
}
void CParticle_CPU::ClearByOwner(uint32_t ownerID)
{
	for (auto& p : m_Particles)
	{
		if (p.bAlive && p.ownerID == ownerID)
		{
			p = PARTICLE_CPU_DATA{}; // 통째로 기본값으로 리셋 (bAlive=false 포함)
		}
	}
}
void CParticle_CPU::SetPosition(const _float3& pos)
{
	if (m_Particles.empty())
		return;

	m_Particles[0].vPosition = pos;
}

void CParticle_CPU::SetVelocity(const _float3& vel)
{
	if (m_Particles.empty())
		return;

	m_Particles[0].vVelocity = vel;
}

void CParticle_CPU::SetSize(const _float3& size)
{
	if (m_Particles.empty())
		return;

	m_Particles[0].fSize = size;
}
void CParticle_CPU::SetColor(const _float4& color)
{
	if (m_Particles.empty())
		return;

	m_Particles[0].vColor = color;
}

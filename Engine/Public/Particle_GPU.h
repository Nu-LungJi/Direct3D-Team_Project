#pragma once
#include "Engine_Defines.h"
#include "Particle.h"

#include "ISerializable.h"

NS_BEGIN(Engine)

// GPU 파티클: 하나의 concrete 클래스가 모든 GPU 이펙트를 처리한다.
// 이펙트마다 클래스를 새로 만들지 않고, Initialize(pArg)에 DESC를 넘겨서
// 텍스처/behaviorType/최대 파티클 개수만 다르게 주입한다.
// Update/Spawn/Render 파이프라인(Compute Shader 3종 + StructuredBuffer 3개)은
// 모든 이펙트가 공유하며, HLSL 쪽에서 g_iBehaviorType 분기로 실제 움직임을 다르게 처리한다.
class ENGINE_DLL CParticle_GPU final : public CParticle
{
public:
    DECLARE_DERIVED_TYPE(CParticle_GPU, CParticle)

public:

	struct TESTDESC final : public ISerializable
	{
		//std::vector<DESC> testDesc{};
	};
    // Initialize(void* pArg)에 이 구조체의 포인터를 넘긴다.
    // 이펙트별로 달라지는 값은 전부 여기로 뺐다 ? 하드코딩 금지.
    struct DESC 
    {
        uint32_t     iMaxParticles = 1000;   
        std::pair<StringID, StringID> textureID;  // 파티클 텍스처
        MESHORTEXTURE                  whatKind = MESHORTEXTURE::END;
        std::pair<StringID, StringID> VSID;  // 버텍스 쉐이더
        std::pair<StringID, StringID> PSID;  // 픽셀 쉐이더
		std::pair<StringID, StringID> normalTextureID;
		std::pair<StringID, StringID> distortionTextureID;
		std::pair<StringID, StringID> noiseTextureID;
		std::pair<StringID, StringID> hdrPositionTextureID;
		std::pair<StringID, StringID> hdrNormalTextureID;
        //모델이면 넣어줌
        StringID sGroupTag;
        StringID sResTag;
		uint32_t TexRows = 1;
		uint32_t TexColumns = 1;

		_string sVEntryPoint = "";
		_string sPEntryPoint = "";
		uint32_t blendState = 0;
    };

	struct PENDING_SPAWN
	{
		PARTICLE_SPAWN_DATA data;
		E::_float remainingDelay;
	};

private:
    explicit CParticle_GPU();

    virtual ~CParticle_GPU();

public:
    virtual HRESULT Initialize(void* pArg) override;
    void DebugPrintDeadListCount();
    virtual void PriorityUpdate(E::_float fTimeDelta) override;
    virtual void Update(E::_float fTimeDelta) override;
    virtual void LateUpdate(E::_float fTimeDelta) override;
    virtual HRESULT Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) override;
    HRESULT Render_Texture(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx);
    HRESULT Render_Mesh(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx);
    virtual HRESULT Spawn(uint32_t count, const PARTICLE_SPAWN_DATA* pSpawnData) override;

    uint32_t GetDeadListCounterSync();
	MESHORTEXTURE GetWhatKind() const { return m_Desc.whatKind; }
	virtual void ClearByOwner(uint32_t ownerID) override;
private:
    DESC m_Desc;

private:
    uint32_t m_iNumElements = 0;

    SPtr<class CResStructuredBuffer> m_pParticleStructuredBuffer = nullptr;
    SPtr<CResStructuredBuffer>       m_pDeadListBuffer = nullptr;
    SPtr<CResStructuredBuffer>       m_pSpawnListBuffer = nullptr;

    SPtr<class CResComputeShader>    m_pResSpawnComputeShader = nullptr;
    SPtr<CResComputeShader>          m_pResUpdateComputeShader = nullptr;
    SPtr<CResComputeShader>          m_pResInitDeadCS = nullptr;
    SPtr<class CResSamplerState>     m_pResSamplerState = nullptr;

    SPtr<class CResCBuffer>          m_pComCBuffer;
    SPtr<class CResCBuffer>          m_pComWaveCBuffer;
    SPtr<CResCBuffer>                m_pComSpawnCBuffer;
    SPtr<CResCBuffer>                m_pComInitCBuffer;
	SPtr<CResCBuffer> m_pComClearCBuffer;

    uint32_t                         m_iCurrentSpawnCount = 0;
    uint32_t                         m_iDeadCount = 0;
	SPtr<CResComputeShader> m_pResClearByOwnerCS;
	_float				m_fTime{};
	
	ComPtr<ID3D11Buffer> m_pDeadCountStaging[2];
	ComPtr<ID3D11Buffer> pCounterStaging;

	uint32_t m_iDeadCountReadIdx = 0;

private:
	ComPtr<ID3D11Buffer>               m_pDeadCountGPUBuffer;   // DeadList 카운터를 GPU 안에서만 읽기용 (CPU readback 없음)
	ComPtr<ID3D11ShaderResourceView>   m_pDeadCountGPUSRV;
	ComPtr<ID3D11Buffer>               m_pIndirectArgsMesh;     // DrawIndexedInstancedIndirect 인자 (5 uint)
	ComPtr<ID3D11UnorderedAccessView>  m_pIndirectArgsMeshUAV;
	ComPtr<ID3D11Buffer>               m_pIndirectArgsQuad;     // DrawInstancedIndirect 인자 (4 uint)
	ComPtr<ID3D11UnorderedAccessView>  m_pIndirectArgsQuadUAV;
	SPtr<CResCBuffer>                  m_pComBuildArgsCBuffer;
	SPtr<CResComputeShader>            m_pResBuildIndirectArgsCS;
	uint32_t                          m_iCachedMeshIndexCount = 0;
public:
	static UPtr<CParticle> Create(void* pArg);
};

NS_END

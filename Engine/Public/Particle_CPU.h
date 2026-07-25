#pragma once
#include "Engine_Defines.h"
#include "Particle.h"

NS_BEGIN(Engine)

struct PARTICLE_CPU_DATA
{
    _float3 vPosition;
    _float3 vVelocity;
    _float4 vColor = { 1.f, 1.f, 1.f, 1.f };
    _float3  fSize = { 0.f, 0.f, 0.f};
    _float3  fStartSize = { 0.f, 0.f, 0.f};
    _float3  fEndSize = { 0.f, 0.f, 0.f };
    _float4  rotation = { 0.f, 0.f, 0.f, 0.f };
    _float  life = 0.f;
    _float  fMaxLife = 1.f;
    _bool   bAlive = false;
    _float4 emissive;
    _float4 endEmissive;
    _float4 originalEmissive;
	_float spawnDelay;
	uint32_t iFrameIndex = 0;
	uint32_t ownerID = 0;
	uint32_t iBehaviorType = 0;
	_float3 originalPosition = { 0.f, 0.f, 0.f };
	_float3 originalVelocity = { 0.f, 0.f, 0.f };
	_bool loop = false;
	_float fStopSizeTime = 0.f;
};


struct VTX_PARTICLE_INSTANCED_DATA
{
    _float4x4 matWorld;
    _float4   vColor;
    _float4 originalEmissive;
    _float4 emissive;
    _float4 endEmissive;
	_float2   vUVOffset; 
	_float2   vUVSize;
	_float    life;
	_float    maxLife;
	uint32_t iBehaviorType = 0;
};
 
typedef struct tagParticleCircleToWave
{
	_float3 g_vFlowDirection; // 물결이 흘러가는 방향 (정규화, XZ 평면 기준)
	_float g_fBurstRatio; // ageRatio 기준, 이 시점까지 원형 확산 (예: 0.3)

	_float g_fTransitionRatio; // 전환 구간 폭 (ageRatio 기준, 예: 0.15)
	_float g_fBurstSpeed; // 원형 확산 초기 속도
	_float g_fFlowSpeed; // 물결이 흘러가는 이동 속도
	_float g_fWaveAmplitude; // 상하 진폭

	_float g_fWaveFrequency; // 공간적 파장 (위치에 따른 위상차)
	_float g_fWaveSpeed; // 시간에 따른 위상 변화 속도
	_float2 g_fPadding2;
}CIRCLE_TO_WAVE;
// CPU 파티클 중간 추상 클래스.
// 슬롯 관리(Spawn/재활용), 인스턴스 버퍼 업로드, 렌더링은 여기서 공통으로 처리하고,
// "파티클이 실제로 어떻게 움직이는가"만 자식 클래스에게 맡긴다 (UpdateBehavior).
class ENGINE_DLL CParticle_CPU final : public CParticle
{
public:
    struct DESC
    {
        uint32_t                     iMaxParticles;
        std::pair<StringID, StringID> viBufferID; // 파티클 쿼드 메쉬 (공유 리소스 조회 키)
        std::pair<StringID, StringID> textureID;  // 파티클 텍스처 (CResTexture2D 하나)
        std::pair<StringID, StringID> VSID;  // 버텍스 쉐이더
        std::pair<StringID, StringID> PSID;  // 픽셀 쉐이더
		std::pair<StringID, StringID> normalTextureID;
		std::pair<StringID, StringID> distortionTextureID;
		std::pair<StringID, StringID> noiseTextureID;
		std::pair<StringID, StringID> hdrPositionTextureID;
		std::pair<StringID, StringID> hdrNormalTextureID;
		std::pair<StringID, StringID> anyTextureID;
        PARTICLE_TYPE                  type;
        MESHORTEXTURE                  whatKind = MESHORTEXTURE::END;
		uint32_t TexRows = 1;
		uint32_t TexColumns = 1;
        //모델이면 넣어줌
        StringID sGroupTag;
        StringID sResTag;

		_string sVEntryPoint = "";
		_string sPEntryPoint = "";
		uint32_t blendState = 0;
		SPtr<CParticleShaderCache> pShaderCache;

    };
public:
    DECLARE_DERIVED_TYPE(CParticle_CPU, CParticle)
private:
    explicit CParticle_CPU();
    virtual ~CParticle_CPU();


public:
    virtual HRESULT Initialize(void* pArg) override;
    virtual void PriorityUpdate(E::_float fTimeDelta) override;
    virtual void Update(E::_float fTimeDelta) override;
    virtual void LateUpdate(E::_float fTimeDelta) override;
    virtual HRESULT Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) override;
    virtual HRESULT Spawn(uint32_t count, const PARTICLE_SPAWN_DATA* pSpawnData) override;
    HRESULT Render_Texture(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx);
    HRESULT Render_Mesh(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx);
	MESHORTEXTURE GetWhatKind() const { return m_Desc.whatKind; }
	virtual void ClearByOwner(uint32_t ownerID) override;
	virtual void SetPosition(const _float3& pos) override;
	virtual void SetVelocity(const _float3& vel) override;
	virtual void SetSize(const _float3& size) override;
	virtual void SetColor(const _float4& color) override;
	virtual void TranslateOwner(uint32_t ownerId, const _float3& delta) override;
	virtual void TransformOwner(uint32_t ownerId, const _float4x4& deltaMatrixData) override;
private:
    virtual void UpdateBehavior(PARTICLE_CPU_DATA& p, E::_float fTimeDelta);

	void		 MakeSmoke(PARTICLE_CPU_DATA& p, _float fTimeDelta);
	void         JumpSmoke(PARTICLE_CPU_DATA& p, _float fTimeDelta);
	void	     GVBurstSmoke(PARTICLE_CPU_DATA& p, _float fTimeDelta);
	void		 GWWaveSmoke(PARTICLE_CPU_DATA& p, _float fTimeDelta);

	void		 Lightning(PARTICLE_CPU_DATA& p, _float fTimeDelta);
	void		 ExtraLightning(PARTICLE_CPU_DATA& p, _float fTimeDelta);
    void		 SizeLerp(PARTICLE_CPU_DATA& p, _float fTimeDelta);
private:
    // m_Particles를 순회하며 수명/UpdateBehavior 처리 후 m_vecInstancedData 재구성
    void Simulate(E::_float fTimeDelta);
protected:
    DESC     m_Desc;
    uint32_t m_iNumElements = 0;
    std::vector<PARTICLE_CPU_DATA>           m_Particles;       // 슬롯 iMaxParticles개 고정 (재활용)
    std::vector<VTX_PARTICLE_INSTANCED_DATA> m_vecInstancedData; // 이번 프레임 살아있는 것만
    SPtr<class CResDynamicBuffer> m_pResInstancedBuffer;
    std::pair<StringID, StringID>  m_viBufferID;
    SPtr<CResCBuffer>       m_pCBuffer;
    SPtr<class CResCBuffer> m_pComCBuffer;
    SPtr<CResSamplerState> m_pResSamplerState{};

    // m_pParticleTexture는 부모 CParticle이 CResTexture2D로 공통 소유
public:
	static UPtr<CParticle> Create(void* pArg);

private:
	CIRCLE_TO_WAVE m_waveCb{};
};
NS_END

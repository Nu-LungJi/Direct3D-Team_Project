#pragma once
#include "Engine_Defines.h"
#include "ParticleParams.h"

NS_BEGIN(Engine)
class CParticle;


struct PARTICLE_LOOP_REQUEST
{
    StringID                          sGroupTag;
    StringID                          sTypeTag;
    std::vector<PARTICLE_SPAWN_DATA>  vecSpawnData;
    _float                            fSpawnInterval = 0.1f;
    _float                            fElapsed = 0.f;
};

typedef struct tagParticlePreset
{
	std::string presetName;
	StringID sGroupTag;     
	StringID sTypeTag;
	uint32_t groupTypeIndex = 0;      // 0:PARTICLE_CPU, 1:PARTICLE_GPU, 2:BEAM_CPU, 3:RIBBON_CPU
	uint32_t whatKindFilterIndex = 0;
	_float maxLife = 1.f;
	_float fStartSize = 1.f;
	_float fEndSize = 1.f;
	_float4   rotation = { 0.f, 0.f, 0.f, 0.f };

	//EndColor는 보간 로직 만든 뒤에 추가
	_float4 StartColor = { 1.f, 1.f, 1.f, 1.f };
	_float4 Emissive = { 1.f, 1.f, 1.f, 0.f };
	uint32_t iBehaviorType = 0;

} PARTICLE_PRESET;

struct STANDARD_PARAMS
{
	bool bRandomPos = false;
	_float3 posMin = { 0,0,0 };
	_float3 posMax = { 0,0,0 };

	bool bRandomVel = false;
	_float3 velMin = { 0,0,0 };
	_float3 velMax = { 0,0,0 };

    uint32_t count = 1;
    _float3  position = {};
    _float3  velocity = {};
    _float   life = 1.f;
    _float   fSize = 1.f;
    _float   fEndSize = 1.f;
    _float4   rotation = { 0.f, 0.f, 0.f, 0.f };
    _float4  color = { 1.f, 1.f, 1.f, 1.f };
    _float4  emissive = { 1.f, 1.f, 1.f, 0.f };
    _bool    bLoop = false;
    _float   fSpawnInterval = 0.1f;
	_float	 fSpawnDelay = 0.f;
	uint32_t	iBehaviorType;
};

struct BEAM_PARAMS
{
    _float4  beamStart = {};
    _float4  beamEnd = {};
    _float4  color = { 1.f, 1.f, 1.f, 1.f };
    _float4  emissive = { 1.f, 1.f, 1.f, 0.f };
    int      iDisplacementIterations = 6;
    _float   fDisplacementAmplitude = 2.5f;
    _float   fDisplacementDamping = 0.25f;
    _float   flickerTimeInverval = 0.25f;
    _float   beamDuration = 0.f;
	_float	 fSpawnDelay = 0.f;
	uint32_t ownerId = 0;
	uint32_t geometryType = 0;

};
struct TextureSlotState
{
	std::string label;              // "Diffuse", "Normal", "Distortion", "Noise"
	char szTextureID1[128] = "SAMPLE_CLINET_TEXTURE";
	char szTextureID2[128] = "";
	int selectedIndex = -1;
	std::string selectedPath;
};

// 나중에 새 파티클 종류(예: RIBBON, DECAL 등) 추가되면 여기 구조체만 추가하면 됨
enum class SPAWN_COMMAND_KIND { STANDARD, BEAM, PATTERN };

struct SPAWN_COMMAND
{
    SPAWN_COMMAND_KIND sGroupTag_KindTag{};
    StringID sGroupTag{};
    StringID sTypeTag{};
	uint32_t ownerId = 0;
    std::variant<STANDARD_PARAMS, BEAM_PARAMS, PatternParamVariant, std::vector<PARTICLE_SPAWN_DATA>> params;
};
struct PARTICLE_EFFECT_PRESET
{
    std::string sEffectName;              // 저장 시 식별용 이름 (예: "Explosion_Fire")
    std::vector<SPAWN_COMMAND> commands;  // 이 이펙트를 구성하는 여러 스폰 명령
};

class ENGINE_DLL CParticleManager final : public CEngineBase, public IRenderable
{
private:
    explicit CParticleManager();
    virtual ~CParticleManager();
public:
    CParticleManager(const CParticleManager&) = delete;
    CParticleManager& operator=(const CParticleManager& rhs) = delete;
public:
    void Update(_float fTimeDelta);
    HRESULT Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx) override;
    void UpdateGUI();
    bool HasRenderPass(RENDERPASS ePass) const override { return ePass == RENDERPASS::DEFAULT; };

public:
    // 사전 등록 - [대분류][소분류]로 저장
    HRESULT Add_Particle(const StringID& sGroupTag, const StringID& sTypeTag, UPtr<CParticle> particle);



    // 정확히 지정해서 스폰
    HRESULT Spawn(const StringID& sGroupTag, const StringID& sTypeTag,
        uint32_t count, const PARTICLE_SPAWN_DATA* pSpawnData,
        _bool bLoop = false, _float fSpawnInterval = 0.1f);



    HRESULT SpawnRibbon(uint32_t quantity, const _float4& start, const _float4& end,
        _float fDisplacementAmplitude, _float iDisplacementIterations, _float fDisplacementDamping,
        _float fFlickerInterval, const _float4& vColor, _float4 emissive, _float fDuration);


public:
	HRESULT Save_Binary_Json(std::string outpath, const std::string& fbxFullPath, const std::string& whatKind, const std::string& particleType,
		const std::string& particleName, int iMaxParticles, const std::string& VSGroup, const std::string& VSID,
		const std::string& PSGroup, const std::string& PSID, const std::string& sGroupTag, const std::string& sResTag, 
		const std::string& textureID1 = "", const std::string& textureID2 = "", const std::string& viBufferID1 ="",
		const std::string& viBufferID2= "",
		int RowCount = 1,
		int ColCount = 1, const std::string& normalTexID1 = "", const std::string& normalTexID2 = "",
		const std::string& distortionTexID1 = "", const std::string& distortionTexID2 = "",
		const std::string& noiseTexID1 = "", const std::string& noiseTexID2 = "",
		const std::string& normalTexPath = "",
		const std::string& distortionTexPath = "",
		const std::string& noiseTexPath = "");

	HRESULT Save_Beam_Json(std::string outpath, const std::string& FullPath, const std::string& whatKind, const std::string& particleType,
		const std::string& particleName, int iMaxParticles, const std::string& VSGroup, const std::string& VSID,
		const std::string& PSGroup, const std::string& PSID, int geometryType,const std::string& textureID1 = "", const std::string& textureID2 = "",int RowCount = 1,int ColCount = 1);
	HRESULT LoadParticleJson(const std::string& strJsonPath);
	ID3D11ShaderResourceView* GetOrLoadTextureThumbnail(const std::string& fullPath);
	HRESULT SaveCommandQueue(const std::string& strJsonPath);
	HRESULT LoadCommandQueue(const std::string& strJsonPath);
	HRESULT LoadParticlePresets(const std::string& strJsonPath);

	HRESULT Spawn(uint32_t owenrId, const std::string& strJsonPath, _fvector startPos, _fvector endPos);

	HRESULT SaveEffectPreset(const std::string& strJsonPath, const PARTICLE_PRESET& preset);
	HRESULT PlayEffect(const std::string& presetName, const _float3& position, uint32_t count = 1);
	HRESULT DeleteEffectPreset(const std::string& strJsonPath, const std::string& presetName);
	std::vector<PARTICLE_SPAWN_DATA> BuildSpawnData(const PatternParamVariant& v);
	void ApplyStartEndToPattern(PatternParamVariant& pv, _fvector startPos, _fvector endPos);
	// 조회 헬퍼
	CParticle* GetParticle(const StringID& sGroupTag, const StringID& sTypeTag) const;
	bool HasGroup(const StringID& sGroupTag) const;
public:
    static UPtr<CParticleManager> Create();

private:
    // [대분류][소분류] -> 파티클 인스턴스
    std::unordered_map<StringID, std::unordered_map<StringID, UPtr<CParticle>>> m_Particles;
	std::unordered_map<StringID, PARTICLE_PRESET> m_ParticlePresets;
    std::vector<PARTICLE_LOOP_REQUEST> m_LoopRequests;
	std::vector<SPAWN_COMMAND> m_vecCommandQueue;
	// 큐 전체를 실행
	std::string m_sLastResultMsg;
	bool m_bLastResultSuccess = false;

	 bool bNeedTypeIndexSync = false;
	 StringID pendingSyncGroup, pendingSyncType;

private:
    HRESULT ExecuteCommandQueue(std::vector<SPAWN_COMMAND>& queue);

private:
	std::unordered_map<std::string, ComPtr<ID3D11ShaderResourceView>> m_TextureThumbnailCache;
};
NS_END

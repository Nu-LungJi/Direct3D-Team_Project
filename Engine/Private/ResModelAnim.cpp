#include "pch.h"
#include "ResModelAnim.h"
#include <fstream>

NS_USING(Engine)

CResModelAnim::CResModelAnim(const _string& sPath)
	: CResource{ sPath }
{
}

CResModelAnim::~CResModelAnim()
{
}

HRESULT CResModelAnim::Load(const std::any& arg)
{
    auto descArg = std::any_cast<DESC>(&arg);
    if (!descArg)
        return E_FAIL;

    if (m_eState == STATE::LOADED)
        return S_OK;

    m_eState = STATE::LOADING;

    m_AnimPath = descArg->path;

    std::ifstream file(m_AnimPath, std::ios::binary | std::ios::ate);
    if (!file.is_open())
        return E_FAIL;

    size_t fileSize = static_cast<size_t>(file.tellg());
    file.seekg(0, std::ios::beg);

    std::unique_ptr<char[]> buffer = std::make_unique<char[]>(fileSize);

    file.read(buffer.get(), fileSize);
    if (!file)
        return E_FAIL;

    char* ptr = buffer.get();
    char* end = buffer.get() + fileSize;

    if (ptr + sizeof(MODEL_FILE_HEADER) > end)
        return E_FAIL;

    MODEL_FILE_HEADER* fh = reinterpret_cast<MODEL_FILE_HEADER*>(ptr);
    ptr += sizeof(MODEL_FILE_HEADER);

    if (ptr + sizeof(ChunkHeader) > end)
        return E_FAIL;

    ChunkHeader* chAnim = reinterpret_cast<ChunkHeader*>(ptr);
    ptr += sizeof(ChunkHeader);
	char* const chunkEnd = ptr + chAnim->size;
	if (chunkEnd > end)
		return E_FAIL;

    if (ptr + sizeof(_float) * 2 + sizeof(uint32_t) > end)
        return E_FAIL;

    memcpy(&m_fDuration, ptr, sizeof(_float));
    ptr += sizeof(_float);

    memcpy(&m_fTickPerSecond, ptr, sizeof(_float));
    ptr += sizeof(_float);

    memcpy(&m_iNumChannels, ptr, sizeof(uint32_t));
    ptr += sizeof(uint32_t);

    m_Channels.clear();
    m_Channels.reserve(m_iNumChannels);

    for (uint32_t i = 0; i < m_iNumChannels; ++i)
    {
        if (ptr + sizeof(uint32_t) > end)
            return E_FAIL;

        uint32_t channelSize = 0;
        memcpy(&channelSize, ptr, sizeof(uint32_t));
        ptr += sizeof(uint32_t);

        if (channelSize == 0)
            return E_FAIL;

        if (ptr + channelSize > end)
            return E_FAIL;

        auto pChannel = CResModelChanel::Create();
        if (nullptr == pChannel)
            return E_FAIL;

        CResModelChanel::DESC channelDesc{};
        channelDesc.ptr = ptr;

        if (FAILED(pChannel->Load(channelDesc)))
            return E_FAIL;

        m_Channels.push_back(pChannel);

        ptr += channelSize;
	}

	// [LSY] Clip은 특정 모델을 참조하지 않는다. 파일에 기록된 가장 큰 BoneIndex를
	// 기준으로 조회 테이블을 만들고, 실제 모델에 없는 Bone 채널은 Animator/GPU
	// 평탄화 단계에서 건너뛴다.
	size_t iChannelMapSize = 0;
	for (const auto& pChannel : m_Channels)
	{
		if (!pChannel)
			continue;

		const int32_t iBoneIndex = pChannel->Get_BoneIndex();
		if (iBoneIndex >= 0)
		{
			iChannelMapSize = std::max(
				iChannelMapSize,
				static_cast<size_t>(iBoneIndex) + 1);
		}
	}

	m_ChannelsByBone.assign(iChannelMapSize, nullptr);
	for (const auto& pChannel : m_Channels)
	{
		if (pChannel == nullptr)
			continue;

		const int32_t iBoneIndex = pChannel->Get_BoneIndex();
		if (iBoneIndex < 0)
			continue;

		m_ChannelsByBone[iBoneIndex] = pChannel;
	}


    m_eState = STATE::LOADED;
    return S_OK;
}
HRESULT CResModelAnim::Unload(const std::any& arg)
{
	m_ChannelsByBone.clear();

	m_eState = STATE::UNLOAD;
	return S_OK;
}

CResModelChanel* CResModelAnim::GetChannelByBoneIndex(uint32_t iBoneIndex) const
{
	if (iBoneIndex >= m_ChannelsByBone.size())
		return nullptr;

	return m_ChannelsByBone[iBoneIndex].get();
}

SPtr<CResModelChanel> CResModelAnim::FindRootChannel(uint32_t iRootBoneIndex) const
{
	for (auto& pChannel : m_Channels)
	{
		if (pChannel && pChannel->Get_BoneIndex() == iRootBoneIndex)
			return pChannel;
	}

	return nullptr;
}

SPtr<CResModelAnim> CResModelAnim::Create(const _string& sPath)
{
	return ToSPtr(new CResModelAnim{ sPath });
}

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
    auto& pModel = descArg->pModel;

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

    m_CurrentKeyFrameIndices.clear();
    m_CurrentKeyFrameIndices.resize(m_iNumChannels, 0);

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
        channelDesc.pModel = pModel;

        if (FAILED(pChannel->Load(channelDesc)))
            return E_FAIL;

        m_Channels.push_back(pChannel);

        ptr += channelSize;
    
		m_iRootBoneIndex = pModel->Get_BoneIndex("Reference");
	
	}

	// m_Channels is serialized in channel order, not bone-index order.
	// Build this once while loading so socket sampling can look up only the
	// channels belonging to its cached bone chain.
	m_ChannelsByBone.assign(pModel->GetBones().size(), nullptr);
	for (const auto& pChannel : m_Channels)
	{
		if (pChannel == nullptr)
			continue;

		const int32_t iBoneIndex = pChannel->Get_BoneIndex();
		if (iBoneIndex < 0 || iBoneIndex >= static_cast<int32_t>(m_ChannelsByBone.size()))
			return E_FAIL;

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


void CResModelAnim::SetCurrentTrackPosition(float fPos)
{
	m_fCurrentTrackPosition = fPos;

	RebuildCurrentKeyFrameIndices();
}

SPtr<CResModelChanel> CResModelAnim::FindRootChannel(uint32_t iRootBoneIndex)
{
	for (auto& pChannel : m_Channels)
	{
		if (pChannel && pChannel->Get_BoneIndex() == iRootBoneIndex)
			return pChannel;
	}

	return nullptr;
}

void CResModelAnim::RebuildCurrentKeyFrameIndices()
{
	for (uint32_t i = 0; i < m_iNumChannels; ++i)
	{
		m_CurrentKeyFrameIndices[i] =
			m_Channels[i]->FindKeyFrameIndex(
				m_fCurrentTrackPosition);
	}
}

SPtr<CResModelAnim> CResModelAnim::Create(const _string& sPath)
{
	return ToSPtr(new CResModelAnim{ sPath });
}

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

	// Optional extension appended after the legacy bone channels.
	if (ptr + sizeof(uint32_t) * 2 <= chunkEnd)
	{
		uint32_t magic = 0;
		memcpy(&magic, ptr, sizeof(uint32_t));
		if (magic == MORPH_BINARY_MAGIC)
		{
			ptr += sizeof(uint32_t);
			uint32_t channelCount = 0;
			memcpy(&channelCount, ptr, sizeof(uint32_t));
			ptr += sizeof(uint32_t);
			m_MorphChannels.clear();
			m_MorphChannels.reserve(channelCount);

			for (uint32_t channelIndex = 0; channelIndex < channelCount; ++channelIndex)
			{
				if (ptr + sizeof(uint32_t) > chunkEnd)
					return E_FAIL;
				uint32_t nameLength = 0;
				memcpy(&nameLength, ptr, sizeof(uint32_t));
				ptr += sizeof(uint32_t);
				if (ptr + nameLength + sizeof(uint32_t) > chunkEnd)
					return E_FAIL;

				MORPH_CHANNEL channel{};
				channel.sMeshName.assign(ptr, nameLength);
				ptr += nameLength;
				uint32_t keyCount = 0;
				memcpy(&keyCount, ptr, sizeof(uint32_t));
				ptr += sizeof(uint32_t);
				channel.Keys.reserve(keyCount);

				for (uint32_t keyIndex = 0; keyIndex < keyCount; ++keyIndex)
				{
					if (ptr + sizeof(_float) + sizeof(uint32_t) > chunkEnd)
						return E_FAIL;
					MORPH_KEY key{};
					memcpy(&key.fTrackPosition, ptr, sizeof(_float));
					ptr += sizeof(_float);
					uint32_t valueCount = 0;
					memcpy(&valueCount, ptr, sizeof(uint32_t));
					ptr += sizeof(uint32_t);
					if (ptr + valueCount * (sizeof(uint32_t) + sizeof(_float)) > chunkEnd)
						return E_FAIL;
					key.TargetIndices.reserve(valueCount);
					key.Weights.reserve(valueCount);
					for (uint32_t valueIndex = 0; valueIndex < valueCount; ++valueIndex)
					{
						uint32_t targetIndex = 0;
						_float weight = 0.f;
						memcpy(&targetIndex, ptr, sizeof(uint32_t));
						ptr += sizeof(uint32_t);
						memcpy(&weight, ptr, sizeof(_float));
						ptr += sizeof(_float);
						key.TargetIndices.push_back(targetIndex);
						key.Weights.push_back(weight);
					}
					channel.Keys.push_back(std::move(key));
				}
				m_MorphChannels.push_back(std::move(channel));
			}
		}
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

_bool CResModelAnim::SampleMorphWeights(
	_float fTrackPosition, DirectX::XMUINT4& outIndices, _float4& outWeights) const
{
	outIndices = { UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX };
	outWeights = {};
	if (m_MorphChannels.empty() || m_MorphChannels.front().Keys.empty())
		return false;

	const auto& keys = m_MorphChannels.front().Keys;
	const MORPH_KEY* left = &keys.front();
	const MORPH_KEY* right = &keys.back();
	for (size_t keyIndex = 1; keyIndex < keys.size(); ++keyIndex)
	{
		if (fTrackPosition <= keys[keyIndex].fTrackPosition)
		{
			left = &keys[keyIndex - 1];
			right = &keys[keyIndex];
			break;
		}
	}

	const _float duration = right->fTrackPosition - left->fTrackPosition;
	const _float alpha = duration > 1.e-6f
		? std::clamp((fTrackPosition - left->fTrackPosition) / duration, 0.f, 1.f)
		: 0.f;
	std::unordered_map<uint32_t, std::pair<_float, _float>> samples;
	for (size_t i = 0; i < left->TargetIndices.size() && i < left->Weights.size(); ++i)
		samples[left->TargetIndices[i]].first = left->Weights[i];
	for (size_t i = 0; i < right->TargetIndices.size() && i < right->Weights.size(); ++i)
		samples[right->TargetIndices[i]].second = right->Weights[i];

	std::vector<std::pair<uint32_t, _float>> active;
	active.reserve(samples.size());
	for (const auto& [index, pair] : samples)
	{
		const _float weight = std::lerp(pair.first, pair.second, alpha);
		if (std::abs(weight) > 1.e-4f)
			active.emplace_back(index, weight);
	}
	std::ranges::sort(active, [](const auto& lhs, const auto& rhs)
	{
		return std::abs(lhs.second) > std::abs(rhs.second);
	});

	uint32_t* indices = &outIndices.x;
	_float* weights = &outWeights.x;
	const size_t count = std::min<size_t>(active.size(), MAX_ACTIVE_MORPH_TARGETS);
	for (size_t i = 0; i < count; ++i)
	{
		indices[i] = active[i].first;
		weights[i] = active[i].second;
	}
	return count > 0;
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

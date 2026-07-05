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
	{
		return E_FAIL;
	}

	if (m_eState == STATE::LOADED)
	{
		return S_OK;
	}
	m_eState = STATE::LOADING;

	auto& pPath = descArg->path;
	auto& pModel = descArg->pModel;
	{

		std::ifstream file(pPath, std::ios::binary | std::ios::ate);


		if (!file.is_open())
		{
			return E_FAIL;
		}

		file.seekg(0, std::ios::end);
		size_t size = file.tellg();
		file.seekg(0, std::ios::beg);

		std::shared_ptr<char[]> buffer = std::make_shared<char[]>(size);
		file.read(buffer.get(), size);

		file.close();

		char* ptr = buffer.get();

		MODEL_FILE_HEADER* fh = (MODEL_FILE_HEADER*)ptr;
		ptr += sizeof(MODEL_FILE_HEADER);

		ChunkHeader* chAnim = (ChunkHeader*)ptr;
		ptr += sizeof(ChunkHeader);


		uint32_t m_iNumMeshes = fh->MeshCount;
		uint32_t m_iAnimCnt = fh->AnimationCount;
		uint32_t m_iNumMaterials = fh->MaterialCount;
		uint32_t m_iNumBones = fh->BoneCount;

		m_fDuration = *(_float*)ptr;
		ptr += sizeof(_float);

		m_fTickPerSecond = *(_float*)ptr;
		ptr += sizeof(_float);

		m_iNumChannels = *(uint32_t*)ptr;
		ptr += sizeof(uint32_t);


		m_CurrentKeyFrameIndices.resize(m_iNumChannels);

		for (size_t i = 0; i < m_iNumChannels; i++)
		{

			uint32_t size = *(uint32_t*)ptr;
			ptr += sizeof(uint32_t);
		

			auto    pChannel = CResModelChanel::Create();
			if (nullptr == pChannel)
				return E_FAIL;

			if (FAILED(pChannel->Load(CResModelChanel::DESC{ .ptr = ptr,.pModel = pModel }))) {
				return E_FAIL;
			}


			m_Channels.push_back(pChannel);

			ptr += size;


		}
	}

	m_eState = STATE::LOADED;
	return S_OK;
}

HRESULT CResModelAnim::Unload(const std::any& arg)
{

	m_eState = STATE::UNLOAD;
	return S_OK;
}

_bool CResModelAnim::Update_TransformationMatrices(_float fTimeDelta, const std::vector<SPtr<CResModelBone>>& Bones, _bool isLoop)
{

	m_fCurrentTrackPosition += m_fTickPerSecond * fTimeDelta;

	if (m_fCurrentTrackPosition >= m_fDuration)
	{
		if (true == isLoop)
			m_fCurrentTrackPosition = 0.f;
		else
			return true;
	}



	for (uint32_t i = 0; i < m_iNumChannels; ++i)
	{
		m_Channels[i]->Update_TransformationMatrix(m_CurrentKeyFrameIndices[i], m_fCurrentTrackPosition, Bones);
	}

	return false;

}

void CResModelAnim::SetCurrentTrackPosition(float fPos)
{
	m_fCurrentTrackPosition = fPos;

	RebuildCurrentKeyFrameIndices();
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

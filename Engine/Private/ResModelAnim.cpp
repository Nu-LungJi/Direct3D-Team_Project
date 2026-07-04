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
	//auto& pAIAnimation = descArg->pAIAnimation;
	//auto& pModel = descArg->pModel;
	//{
	//	m_fDuration = pAIAnimation->mDuration;

	//	m_fTickPerSecond = pAIAnimation->mTicksPerSecond;

	//	m_iNumChannels = pAIAnimation->mNumChannels;

	//	m_CurrentKeyFrameIndices.resize(m_iNumChannels);

	//	for (size_t i = 0; i < m_iNumChannels; i++)
	//	{

	//		auto    pChannel = CResTestModelChanel::Create();
	//		if (nullptr == pChannel)
	//			return E_FAIL;

	//		if (FAILED(pChannel->Load(CResTestModelChanel::DESC{ .pAIChannel = pAIAnimation->mChannels[i], .pModel = pModel }))) {
	//			return E_FAIL;
	//		}


	//		m_Channels.push_back(pChannel);
	//	}
	//}

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

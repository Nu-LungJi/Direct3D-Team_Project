#include "pch.h"
#include "ResTestModelAnim.h"
#include "ResTestModelBone.h"
#include "ResTestModelChanel.h"
#ifdef _DEBUG
#ifdef new
#pragma push_macro("new")
#undef new
#define RESTORE_NEW_MACRO
#endif
#endif

#include "assimp/scene.h"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"

#ifdef _DEBUG
#ifdef RESTORE_NEW_MACRO
#pragma pop_macro("new")
#undef RESTORE_NEW_MACRO
#endif
#endif

#include <fstream>

NS_USING(Engine)

CResTestModelAnim::CResTestModelAnim(const _string& sPath)
	: CResource{ sPath }
{
}

CResTestModelAnim::~CResTestModelAnim()
{
}

HRESULT CResTestModelAnim::Load(const std::any& arg)
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
	auto& pAIAnimation = descArg->pAIAnimation;
	auto& pModel = descArg->pModel;
	{
		m_fDuration = pAIAnimation->mDuration;

		m_fTickPerSecond = pAIAnimation->mTicksPerSecond;

		m_iNumChannels = pAIAnimation->mNumChannels;

		m_CurrentKeyFrameIndices.resize(m_iNumChannels);

		for (size_t i = 0; i < m_iNumChannels; i++)
		{
			
			auto    pChannel = CResTestModelChanel::Create();
			if (nullptr == pChannel)
				return E_FAIL;

			if (FAILED(pChannel->Load(CResTestModelChanel::DESC{ .pAIChannel = pAIAnimation->mChannels[i], .pModel = pModel }))) {
				return E_FAIL;
			}


			m_Channels.push_back(pChannel);
		}
	}

	m_eState = STATE::LOADED;
	return S_OK;
}

HRESULT CResTestModelAnim::Unload(const std::any& arg)
{

	m_eState = STATE::UNLOAD;
	return S_OK;
}

_bool CResTestModelAnim::Update_TransformationMatrices(_float fTimeDelta, const std::vector<SPtr<CResTestModelBone>>& Bones, _bool isLoop)
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


SPtr<CResTestModelAnim> CResTestModelAnim::Create(const _string& sPath)
{
	return ToSPtr(new CResTestModelAnim{ sPath });
}

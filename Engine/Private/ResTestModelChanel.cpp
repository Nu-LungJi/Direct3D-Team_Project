#include "pch.h"
#include "ResTestModelChanel.h"
#include "ResTestModelBone.h"
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

CResTestModelChanel::CResTestModelChanel(const _string& sPath)
	: CResource{ sPath }
{
}

CResTestModelChanel::~CResTestModelChanel()
{
}

HRESULT CResTestModelChanel::Load(const std::any& arg)
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
    
    auto& pModel = descArg->pModel;
    auto& pAIChannel = descArg->pAIChannel;
	{
        m_iBoneIndex = pModel->Get_BoneIndex(pAIChannel->mNodeName.C_Str());
        if (-1 == m_iBoneIndex)
            return E_FAIL;

        m_iNumKeyFrames = std::max(pAIChannel->mNumScalingKeys, pAIChannel->mNumRotationKeys);
        m_iNumKeyFrames = std::max(m_iNumKeyFrames, pAIChannel->mNumPositionKeys);

        _float3     vScale = {};
        _float4     vRotation = {};
        _float3     vTranslation = {};

        for (size_t i = 0; i < m_iNumKeyFrames; i++)
        {
            KEYFRAME            KeyFrame = {};

            if (i < pAIChannel->mNumScalingKeys)
            {
                memcpy(&vScale, &pAIChannel->mScalingKeys[i].mValue, sizeof vScale);
                KeyFrame.fTrackPosition = pAIChannel->mScalingKeys[i].mTime;
            }

            if (i < pAIChannel->mNumRotationKeys)
            {
                // memcpy(&vRotation, &pAIChannel->mRotationKeys[i].mValue, sizeof vRotation);
                vRotation.x = pAIChannel->mRotationKeys[i].mValue.x;
                vRotation.y = pAIChannel->mRotationKeys[i].mValue.y;
                vRotation.z = pAIChannel->mRotationKeys[i].mValue.z;
                vRotation.w = pAIChannel->mRotationKeys[i].mValue.w;
                KeyFrame.fTrackPosition = pAIChannel->mRotationKeys[i].mTime;
            }

            if (i < pAIChannel->mNumPositionKeys)
            {
                memcpy(&vTranslation, &pAIChannel->mPositionKeys[i].mValue, sizeof vTranslation);
                KeyFrame.fTrackPosition = pAIChannel->mPositionKeys[i].mTime;
            }

            KeyFrame.vScale = vScale;
            KeyFrame.vRotation = vRotation;
            KeyFrame.vTranslation = vTranslation;

            m_KeyFrames.push_back(KeyFrame);
        }

	}

	m_eState = STATE::LOADED;
	return S_OK;
}

HRESULT CResTestModelChanel::Unload(const std::any& arg)
{

	m_eState = STATE::UNLOAD;
	return S_OK;
}

void CResTestModelChanel::Update_TransformationMatrix(uint32_t& iCurrentKeyFrameIndex, _float fCurrentTrackPosition, const std::vector<SPtr<CResTestModelBone>>& Bones)
{
    if (0.f == fCurrentTrackPosition)
        iCurrentKeyFrameIndex = 0;

    KEYFRAME        LastKeyFrame = m_KeyFrames.back();

    _vector         vScale, vRotation, vTranslation;

    if (fCurrentTrackPosition >= LastKeyFrame.fTrackPosition)
    {
        vScale = XMLoadFloat3(&LastKeyFrame.vScale);
        vRotation = XMLoadFloat4(&LastKeyFrame.vRotation);
        vTranslation = XMVectorSetW(XMLoadFloat3(&LastKeyFrame.vTranslation), 1.f);
    }
    else
    {
        while (fCurrentTrackPosition >= m_KeyFrames[iCurrentKeyFrameIndex + 1].fTrackPosition)
            ++iCurrentKeyFrameIndex;

        _float      fRatio = (fCurrentTrackPosition - m_KeyFrames[iCurrentKeyFrameIndex].fTrackPosition) /
            (m_KeyFrames[iCurrentKeyFrameIndex + 1].fTrackPosition - m_KeyFrames[iCurrentKeyFrameIndex].fTrackPosition);

        vScale = XMVectorLerp(
            XMLoadFloat3(&m_KeyFrames[iCurrentKeyFrameIndex].vScale),
            XMLoadFloat3(&m_KeyFrames[iCurrentKeyFrameIndex + 1].vScale),
            fRatio
        );

        vRotation = XMQuaternionSlerp(
            XMLoadFloat4(&m_KeyFrames[iCurrentKeyFrameIndex].vRotation),
            XMLoadFloat4(&m_KeyFrames[iCurrentKeyFrameIndex + 1].vRotation),
            fRatio
        );

        vTranslation = XMVectorSetW(XMVectorLerp(
            XMLoadFloat3(&m_KeyFrames[iCurrentKeyFrameIndex].vTranslation),
            XMLoadFloat3(&m_KeyFrames[iCurrentKeyFrameIndex + 1].vTranslation),
            fRatio
        ), 1.f);
    }

    _matrix         TransformationMatrix = XMMatrixAffineTransformation(vScale, XMVectorSet(0.f, 0.f, 0.f, 1.f), vRotation, vTranslation);

    Bones[m_iBoneIndex]->Set_TransformationMatrix(TransformationMatrix);
}


SPtr<CResTestModelChanel> CResTestModelChanel::Create(const _string& sPath)
{
	return ToSPtr(new CResTestModelChanel{ sPath });
}

#include "pch.h"
#include "GameInstance.h"
#include "ComModelInstance.h"
#include "ResModelBone.h"
#include "ResModelMesh.h"
#include "ResModelMaterial.h"
#include "ResModel.h"
#include "ComAnimator.h"
NS_USING(Engine)



void CComModelInstance::UpdateGUI()
{
#ifdef _DEBUG
	if (!m_pModel)
		return;

	const auto& Bones = m_pModel->GetBones();

	if (Bones.empty())
		return;

	EnsureDebugBoneOffsetSize();

	ImGui::Begin("Bone Editor");

	ImGui::Checkbox("Enable Bone Edit", &m_bDebugBoneEdit);

	ImGui::Text("Bone Count : %d", static_cast<int>(Bones.size()));

	if (m_iDebugSelectedBone < 0)
		m_iDebugSelectedBone = 0;

	if (m_iDebugSelectedBone >= static_cast<int>(Bones.size()))
		m_iDebugSelectedBone = static_cast<int>(Bones.size()) - 1;

	std::string previewName = "None";

	if (Bones[m_iDebugSelectedBone])
	{
		// Bone 이름 getter 있으면 그걸로 바꿔
		// previewName = Bones[m_iDebugSelectedBone]->Get_BoneName();
		previewName = Bones[m_iDebugSelectedBone]->GetBoneName();
	}

	if (ImGui::BeginCombo("Selected Bone", previewName.c_str()))
	{
		for (int i = 0; i < static_cast<int>(Bones.size()); ++i)
		{
			if (!Bones[i])
				continue;

			std::string boneLabel;

			// 이름 getter 있으면 이걸 추천
			// boneLabel = std::to_string(i) + " : " + Bones[i]->Get_BoneName();

			boneLabel = Bones[i]->GetBoneName();

			bool bSelected = (m_iDebugSelectedBone == i);

			if (ImGui::Selectable(boneLabel.c_str(), bSelected))
			{
				m_iDebugSelectedBone = i;
			}

			if (bSelected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}

		ImGui::EndCombo();
	}

	if (Bones[m_iDebugSelectedBone])
	{
		int iParentIndex = Bones[m_iDebugSelectedBone]->GetParendBoneIndex();

		ImGui::Separator();

		ImGui::Text("Selected Bone Index : %d", m_iDebugSelectedBone);
		ImGui::Text("Parent Bone Index   : %d", iParentIndex);


		_matrix matCombined =
			Bones[m_iDebugSelectedBone]->Get_CombinedTransformationMatrix();
		_float3 vBonePos{};
		XMStoreFloat3(&vBonePos, matCombined.r[3]);

		ImGui::Separator();
		ImGui::Text("Current Combined Bone Position");
		ImGui::Text("X : %.4f", vBonePos.x);
		ImGui::Text("Y : %.4f", vBonePos.y);
		ImGui::Text("Z : %.4f", vBonePos.z);

	}
	ImGui::End();
#endif
}

CComModelInstance::CComModelInstance()
{
  

}

CComModelInstance::~CComModelInstance()
{
}


HRESULT CComModelInstance::Initialize(void* pArg)
{
    if (FAILED(CComponent::Initialize(pArg)))
    {
        return E_FAIL;
    }

    if (pArg != nullptr) {
        // 모델 Instance는 하나의 메모리를 모두 공유한다.
        CComModelInstance::DESC* pDesc = reinterpret_cast<CComModelInstance::DESC*>(pArg);
		m_sGroupTag = pDesc->sGroupTag;
		m_sResTag = pDesc->sResTag;

        m_pModel = CGameInstance::Get().GetResourceFirst<CResModel>(pDesc->sGroupTag, pDesc->sResTag);
        if (m_pModel == nullptr)
        {
			return E_FAIL;
        }

        m_Buffer = CGameInstance::Get().GetResourceFirst<CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_BONE);
    }
    return S_OK;
}

HRESULT CComModelInstance::Bind_BoneMatrices(ID3D11DeviceContext* pContext, uint32_t iMeshIndex)
{
    // 나중에 Bind 할떄 animation 정보를 던져준 GPU에서 Animatino 돌린다. 나중에 수정
    auto& pMesh = m_pModel->GetMeshes()[iMeshIndex];
	auto& Bones = m_pModel->GetBones();
	auto  BonesSize = Bones.size();
    auto& BoneMatrices = pMesh->GetBoneMatrices();
    auto& BoneIndices = pMesh->GetBoneIndices();
    auto& OffsetMatrix = pMesh->GetOffsetMatrices();

    for (uint32_t i = 0; i < pMesh->Get_BoneIndex(); i++)
    {
		if (m_CombinedBoneMatrices.size() != 0) {

			XMStoreFloat4x4(&BoneMatrices[i],XMLoadFloat4x4(&OffsetMatrix[i]) *XMLoadFloat4x4(&m_CombinedBoneMatrices[BoneIndices[i]]));
		}
    }

    if (!BoneMatrices.empty())
    {
        D3D11_MAPPED_SUBRESOURCE MappedResource{};
    
        if (FAILED(pContext->Map(m_Buffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource)))
        {
            return E_FAIL;
        }

        _float4x4* pBoneMatrices = reinterpret_cast<_float4x4*>(MappedResource.pData);

        for (uint32_t i = 0; i < BonesSize; ++i)
        {
            XMStoreFloat4x4(&pBoneMatrices[i], XMMatrixIdentity());
        }

        const uint32_t iBoneCount = static_cast<uint32_t>(std::min<size_t>(BoneMatrices.size(), 512));

        memcpy(pBoneMatrices, BoneMatrices.data(), sizeof(_float4x4) * iBoneCount);

        pContext->Unmap(m_Buffer->GetCBuffer().Get(), 0);

        ID3D11Buffer* pCBBones = m_Buffer->GetCBuffer().Get();

        pContext->VSSetConstantBuffers(2, 1, &pCBBones);
    }




    return S_OK;

}


VOID CComModelInstance::Bind_Textures(ID3D11DeviceContext* pContext, uint32_t _MeshIndex) {
	SPtr<CResTexture2D> DiffuseTexture = E::CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_DIFFUSE");
	if (auto Resource = Get_MeshTexture(_MeshIndex, AI_TEXTURE_TYPE::aiTextureType_DIFFUSE, 0)) {
		DiffuseTexture = Resource;
	}
	pContext->PSSetShaderResources(0, 1, DiffuseTexture->GetSRV().GetAddressOf());

	SPtr<CResTexture2D> NormalTexture = E::CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_NORMAL");
	if (auto Resource = Get_MeshTexture(_MeshIndex, AI_TEXTURE_TYPE::aiTextureType_NORMALS, 0)) {
		NormalTexture = Resource;
	}
	pContext->PSSetShaderResources(1, 1, NormalTexture->GetSRV().GetAddressOf());

	SPtr<CResTexture2D> SMROTexture = E::CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_SMRO");
	if (auto Resource = Get_MeshTexture(_MeshIndex, AI_TEXTURE_TYPE::aiTextureType_METALNESS, 0)) {
		SMROTexture = Resource;
	}
	pContext->PSSetShaderResources(2, 1, SMROTexture->GetSRV().GetAddressOf());

	SPtr<CResTexture2D> EmissiveTexture = E::CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_EMISSIVE");
	if (auto Resource = Get_MeshTexture(_MeshIndex, AI_TEXTURE_TYPE::aiTextureType_EMISSIVE, 0)) {
		EmissiveTexture = Resource;
	}
	pContext->PSSetShaderResources(3, 1, EmissiveTexture->GetSRV().GetAddressOf());
}

VOID CComModelInstance::Bind_Materials(ID3D11DeviceContext* pContext, _float3 _EmissiveColor, _float _EmissiveIntensity, _float3 _DissolveColor, _float _DissolveIntensity, _float _ObjectAlpha)
{
	auto MaterialConstantBuffer = E::CGameInstance::Get().GetResourceFirst<E::CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "CB_MATERIAL");
	D3D11_MAPPED_SUBRESOURCE MRES;
	if (SUCCEEDED(pContext->Map(MaterialConstantBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MRES)))
	{
		CB_MATERIAL   CMMAT;

		CMMAT.EmissiveColor = _EmissiveColor;
		CMMAT.EmissiveIntensity = _EmissiveIntensity;

		CMMAT.DissolveColor = _DissolveColor;
		CMMAT.DissolveIntensity = _DissolveIntensity;

		CMMAT.ObjectAlpha = _ObjectAlpha;

		memcpy(MRES.pData, &CMMAT, sizeof(CB_MATERIAL));
		pContext->Unmap(MaterialConstantBuffer->GetCBuffer().Get(), 0);
	}
	pContext->PSSetConstantBuffers(3, 1, MaterialConstantBuffer->GetCBuffer().GetAddressOf());
}

HRESULT CComModelInstance::ChangeModel(const StringID& sGroupTag, const StringID& sResTag)
{
    auto pModel = CGameInstance::Get().GetResourceFirst<CResModel>(sGroupTag, sResTag);
    if (pModel == nullptr)
    {
        return E_FAIL;
    }

    m_pModel = pModel;
    return S_OK;
}

// 모델의 단일 텍스쳐 반환
SPtr<CResTexture2D> CComModelInstance::Get_MeshTexture(uint32_t iMeshIndex, AI_TEXTURE_TYPE eMaterialType, uint32_t iTextureIndex) {
    auto Materials = m_pModel->GetMaterials();
    auto Mesh = m_pModel->GetMeshes();
    auto Textures = Materials[Mesh[iMeshIndex]->Get_MaterialIndex()]->GetTextures();

    if (Textures[eMaterialType].size() == 0)
    {
        return Textures[1].front();
    }

    return Textures[eMaterialType][iTextureIndex];
}

UPtr<CComModelInstance> CComModelInstance::Create()
{
    auto pInstance = ToUPtr(new CComModelInstance{});
    if (FAILED(pInstance->InitializePrototype()))
    {
        MSG_BOX("Failed to Created : CComModelInstance");
        return nullptr;
    }
    return pInstance;
}

UPtr<CPrototype> CComModelInstance::Clone(void* pArg)
{
    auto pInstance = ToUPtr(new CComModelInstance{ *this });
    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned: CComModelInstance");
        return nullptr;
    }
    return pInstance;
}

void CComModelInstance::DebugDraw_Bones(const _float4x4& WorldMatrix)
{
	
	_matrix matWorld = XMLoadFloat4x4(&WorldMatrix);

	const auto& Bones = m_pModel->GetBones();

	for (auto& pBone : Bones)
	{
		if (!pBone)
			continue;

		_matrix matBone = pBone->Get_CombinedTransformationMatrix();
		_matrix matBoneWorld = matBone * matWorld;

		if (Bones[m_iDebugSelectedBone]->Compare_Name(pBone->GetBoneName().c_str())) {

			CGameInstance::Get().GetDbgLineRender()->SetColor({ 0.f, 1.f, 0.f, 1.f });
			CGameInstance::Get().GetDbgLineRender()->AddBox({ 0.001f , 0.001f , 0.001f }, matBoneWorld);
		}
		else {
			CGameInstance::Get().GetDbgLineRender()->SetColor({ 1.f, 0.f, 0.f, 1.f });
			CGameInstance::Get().GetDbgLineRender()->AddBox({ 0.0001f , 0.0001f , 0.0001f }, matBoneWorld);

		}
		
		CGameInstance::Get().GetDbgLineRender()->AddAxis(0.1f, matBoneWorld);
		CGameInstance::Get().GetDbgLineRender()->SetColor();
		_float3 vBonePos{};
		XMStoreFloat3(&vBonePos, matBoneWorld.r[3]);



		int iParentIndex = pBone->GetParendBoneIndex();



		if (iParentIndex > 0)
		{
			auto pParent = Bones[iParentIndex];

			_matrix matParent = pParent->Get_CombinedTransformationMatrix();
			_matrix matParentWorld = matParent * matWorld;

			_float3 vParentPos{};
			XMStoreFloat3(&vParentPos, matParentWorld.r[3]);
			
			auto mag = XMLoadFloat3(&vParentPos)- XMLoadFloat3(&vBonePos);
			auto dir = XMVector3Normalize(mag);
			_float3 fdir;
			XMStoreFloat3(&fdir, dir);
			CGameInstance::Get().GetDbgLineRender()->SetColor({ 1.f, 0.f, 1.f, 1.f });
			//CGameInstance::Get().GetDbgLineRender()->AddArrow(vBonePos, fdir, XMVectorGetX(XMVector3Length(mag)),0.1f);
			CGameInstance::Get().GetDbgLineRender()->SetColor();
		
		}
	}
}

HRESULT CComModelInstance::Bind_GPUAnimationSRVs_CS(ID3D11DeviceContext* pContext)
{
	if (!pContext || !m_pModel)
		return E_FAIL;

	ID3D11ShaderResourceView* pSRVs[] =
	{
		m_pModel->Get_GPUBoneSRV(),            // t0
		m_pModel->Get_GPUAnimationSRV(),       // t1
		m_pModel->Get_GPUChannelSRV(),         // t2
		m_pModel->Get_GPUKeyFrameSRV(),        // t3
		m_pModel->Get_GPUBoneChannelMapSRV(),  // t4
		m_pModel->Get_GPUSkinBoneSRV()         // t5
	};

	for (ID3D11ShaderResourceView* pSRV : pSRVs)
	{
		if (!pSRV)
			return E_FAIL;
	}

	pContext->CSSetShaderResources(0,static_cast<UINT>(std::size(pSRVs)),pSRVs);

	return S_OK;
}

HRESULT CComModelInstance::Bind_GPUSkinBones_VS(ID3D11DeviceContext* pContext)
{
	if (!pContext || !m_pModel)
		return E_FAIL;

	ID3D11ShaderResourceView* pSRV = m_pModel->Get_GPUSkinBoneSRV();
	if (!pSRV)
		return E_FAIL;

	pContext->VSSetShaderResources(8, 1, &pSRV);
	return S_OK;
}

void CComModelInstance::Unbind_GPUAnimationSRVs_CS(ID3D11DeviceContext* pContext)
{
	if (!pContext)
		return;

	ID3D11ShaderResourceView* pNullSRVs[6]{};

	pContext->CSSetShaderResources(
		0,
		6,
		pNullSRVs
	);
}

void CComModelInstance::EnsureDebugBoneOffsetSize()
{
	if (!m_pModel)
		return;

	const auto& Bones = m_pModel->GetBones();

	if (m_DebugBoneLocalOffsets.size() != Bones.size())
	{
		m_DebugBoneLocalOffsets.clear();
		m_DebugBoneLocalOffsets.resize(Bones.size(), _float3{ 0.f, 0.f, 0.f });

		m_iDebugSelectedBone = 0;
	}
}

void CComModelInstance::ApplyDebugBoneLocalOffsets()
{
	if (!m_bDebugBoneEdit)
		return;

	if (!m_pModel)
		return;

	const auto& Bones = m_pModel->GetBones();

	if (Bones.empty())
		return;

	EnsureDebugBoneOffsetSize();

	for (uint32_t i = 0; i < Bones.size(); ++i)
	{
		if (!Bones[i])
			continue;

		const _float3& vOffset = m_DebugBoneLocalOffsets[i];

		if (vOffset.x == 0.f &&
			vOffset.y == 0.f &&
			vOffset.z == 0.f)
		{
			continue;
		}

		_matrix matLocal = Bones[i]->Get_CombinedTransformationMatrix();

		XMVECTOR vTranslation = matLocal.r[3];

		vTranslation += XMVectorSet(
			vOffset.x,
			vOffset.y,
			vOffset.z,
			0.f
		);

		matLocal.r[3] = XMVectorSetW(vTranslation, 1.f);

		Bones[i]->Set_TransformationMatrix(matLocal);
	}
}



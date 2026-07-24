#include "pch.h"
#include "TestModel.h"
#include "Client_Resources.h"
#include "ComConstantBuffer.h"
#include "ComModelInstance.h"
#include "ComAnimator.h"
#include "Resources.h"
#include "GameInstance.h"
#include "TestPartObject.h"
#include "ComSocket.h"

NS_USING(Client)

CTestModel::CTestModel()
	: CAnimationObject{}
{
}

CTestModel::~CTestModel()
{
}

void CTestModel::UpdateGUI()
{
	CAnimationObject::UpdateGUI();


	if (!m_pComModelInstance->GetModel())
		return;

	const auto& Bones = m_pComModelInstance->GetModel()->GetBones();

	if (Bones.empty())
		return;


	ImGui::Begin("Bone Editor");

	if (m_pModelAnimator)
	{
		constexpr CComAnimator::EVALUATION_MODE EvaluationModes[] =
		{
			CComAnimator::EVALUATION_MODE::GPU,
			CComAnimator::EVALUATION_MODE::CPU_GPU,
		};
		constexpr const char* EvaluationModeNames[] = { "GPU", "CPU + GPU" };
		int iEvaluationMode = m_pModelAnimator->GetEvaluationMode() == CComAnimator::EVALUATION_MODE::GPU ? 0 : 1;

		if (ImGui::Combo("Animation Evaluation", &iEvaluationMode, EvaluationModeNames, IM_ARRAYSIZE(EvaluationModeNames)))
		{
			m_pModelAnimator->SetEvaluationMode(EvaluationModes[iEvaluationMode]);
		}

		ImGui::TextDisabled("GPU: existing GPU animation path");
		ImGui::TextDisabled("CPU + GPU: CPU pose + VS skinning + instanced draw");
		ImGui::Separator();
	}


	ImGui::Text("Bone Count : %d", static_cast<int>(Bones.size()));

	if (m_iDebugSelectedBone < 0)
		m_iDebugSelectedBone = 0;

	if (m_iDebugSelectedBone >= static_cast<int>(Bones.size()))
		m_iDebugSelectedBone = static_cast<int>(Bones.size()) - 1;

	std::string previewName = "None";

	if (Bones[m_iDebugSelectedBone])
	{
		// Bone ?�름 getter ?�으�?그걸�?바꿔
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

			// ?�름 getter ?�으�??�걸 추천
			// boneLabel = std::to_string(i) + " : " + Bones[i]->Get_BoneName();

			boneLabel =
				std::to_string(i) + " : " + Bones[i]->GetBoneName();

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

	ImGui::Separator();
	ImGui::Text("CPU + GPU Skinning Diagnostics");

	const auto DrawMatrix = [](const char* pLabel, const _float4x4& matrix)
		{
			if (!ImGui::TreeNode(pLabel))
				return;

			for (uint32_t row = 0; row < 4; ++row)
			{
				ImGui::Text(
					"[%u] % .6f  % .6f  % .6f  % .6f",
					row,
					matrix.m[row][0],
					matrix.m[row][1],
					matrix.m[row][2],
					matrix.m[row][3]);
			}
			ImGui::TreePop();
		};

	const auto DrawMatrixStatus = [&](const char* pLabel, const _float4x4& matrix)
		{
			bool bFinite = true;
			for (uint32_t row = 0; row < 4; ++row)
			{
				for (uint32_t column = 0; column < 4; ++column)
					bFinite &= std::isfinite(matrix.m[row][column]);
			}

			const _matrix loadedMatrix = XMLoadFloat4x4(&matrix);
			const float fDeterminant =
				XMVectorGetX(XMMatrixDeterminant(loadedMatrix));

			_vector scale{};
			_vector rotation{};
			_vector translation{};
			const bool bDecomposed = XMMatrixDecompose(
				&scale,
				&rotation,
				&translation,
				loadedMatrix);

			_float3 scaleValue{};
			_float3 translationValue{};
			XMStoreFloat3(&scaleValue, scale);
			XMStoreFloat3(&translationValue, translation);

			// 0.01 uniform scale has a valid determinant of 1e-6.
			// Only treat a matrix as singular when its determinant is effectively zero.
			if (!bFinite || !bDecomposed || std::abs(fDeterminant) < 0.000000000001f)
			{
				ImGui::TextColored(
					ImVec4(1.f, 0.2f, 0.2f, 1.f),
					"%s : INVALID",
					pLabel);
			}
			else
			{
				ImGui::TextColored(
					ImVec4(0.2f, 1.f, 0.2f, 1.f),
					"%s : VALID",
					pLabel);
			}

			ImGui::Text(
				"Det %.8f | Scale %.4f %.4f %.4f | Pos %.4f %.4f %.4f",
				fDeterminant,
				scaleValue.x,
				scaleValue.y,
				scaleValue.z,
				translationValue.x,
				translationValue.y,
				translationValue.z);
			DrawMatrix(pLabel, matrix);
		};

	auto pDebugModel = m_pComModelInstance->GetModel();
	const auto& cpuCombinedMatrices =
		m_pComModelInstance->Get_CombinedBoneMatrices();

	ImGui::Text(
		"CPU Palette Count : %u / Skeleton : %u",
		static_cast<uint32_t>(cpuCombinedMatrices.size()),
		static_cast<uint32_t>(Bones.size()));

	if (m_iDebugSelectedBone >= cpuCombinedMatrices.size())
	{
		ImGui::TextColored(
			ImVec4(1.f, 0.2f, 0.2f, 1.f),
			"Selected bone is missing from the CPU palette.");
	}
	else
	{
		const _float4x4& combinedMatrix =
			cpuCombinedMatrices[m_iDebugSelectedBone];
		DrawMatrixStatus("CPU Combined Matrix", combinedMatrix);
		DrawMatrix("PreTransform Matrix", pDebugModel->Get_PreTransformMatrix());

		uint32_t iMatchingMeshBoneCount = 0;
		const auto& meshes = pDebugModel->GetMeshes();
		for (uint32_t iMeshIndex = 0;
			iMeshIndex < static_cast<uint32_t>(meshes.size());
			++iMeshIndex)
		{
			const auto& mesh = meshes[iMeshIndex];
			if (!mesh)
				continue;

			const auto& boneIndices = mesh->GetBoneIndices();
			const auto& offsetMatrices = mesh->GetOffsetMatrices();
			for (uint32_t iMeshBoneIndex = 0;
				iMeshBoneIndex < static_cast<uint32_t>(boneIndices.size());
				++iMeshBoneIndex)
			{
				if (boneIndices[iMeshBoneIndex] != m_iDebugSelectedBone)
					continue;

				++iMatchingMeshBoneCount;
				ImGui::Separator();
				ImGui::Text(
					"Mesh %u | Mesh Bone %u | Skeleton Bone %u",
					iMeshIndex,
					iMeshBoneIndex,
					boneIndices[iMeshBoneIndex]);

				if (iMeshBoneIndex >= offsetMatrices.size())
				{
					ImGui::TextColored(
						ImVec4(1.f, 0.2f, 0.2f, 1.f),
						"Offset matrix is missing.");
					continue;
				}

				_float4x4 skinMatrix{};
				XMStoreFloat4x4(
					&skinMatrix,
					XMLoadFloat4x4(&offsetMatrices[iMeshBoneIndex]) *
						XMLoadFloat4x4(&combinedMatrix));

				DrawMatrixStatus("Offset Matrix", offsetMatrices[iMeshBoneIndex]);
				DrawMatrixStatus("CPU Final Skin Matrix", skinMatrix);
			}
		}

		if (iMatchingMeshBoneCount == 0)
		{
			ImGui::TextDisabled(
				"Selected skeleton bone does not influence any mesh.");
		}
	}
	ImGui::End();


}

HRESULT CTestModel::InitializePrototype(void* pArg)
{
	m_pResVertexShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelAnim");
	//m_pResVertexShader = CResVertexShader::Create("./ShaderFiles/Shader_VtxNorTex.hlsl");
	if (FAILED(m_pResVertexShader->Load()))
	{
		return E_FAIL;
	}
	m_pResPixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_TestModelAnim");
	//m_pResPixelShader = CResPixelShader::Create("./ShaderFiles/Shader_VtxNorTex.hlsl");
	if (FAILED(m_pResPixelShader->Load()))
	{
		return E_FAIL;
	}
	m_pResVertexInstancedShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelAnim_Instanced");
	if (!m_pResVertexInstancedShader || FAILED(m_pResVertexInstancedShader->Load()))
	{
		return E_FAIL;
	}
	m_pResVertexCPUSkinningInstancedShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>( TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelAnim_CPU_Skinning_Instanced");
	if (!m_pResVertexCPUSkinningInstancedShader || FAILED(m_pResVertexCPUSkinningInstancedShader->Load()))
	{
		return E_FAIL;
	}
	m_pResSkinMeshCBuffer = CGameInstance::Get().GetResourceFirst<CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "CB_GPU_SKIN_MESH");
	if (!m_pResSkinMeshCBuffer)
	{
		return E_FAIL;
	} 
	
	m_pAnimComputeShader = CGameInstance::Get().GetResourceFirst<CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_Animation");
	if (FAILED(m_pAnimComputeShader->Load()))
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CTestModel::Initialize(void* pArg)
{
	if (FAILED(CGameObject::Initialize(pArg)))
	{
		return E_FAIL;
	}

	{
		CComConstantBuffer::DESC Desc{};
		Desc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_OBJECT };
		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ConstantBuffer", "ComCBufferPerObject", &Desc, &m_pComCBufferPerObject)))
		{
			return E_FAIL;
		};
	}

	{
		CComModelInstance::DESC Desc{};
		Desc.sGroupTag = "TEST";
		Desc.sResTag = "Model_Resource";

		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ModelInstance", "ComCModelIntance", &Desc, &m_pComModelInstance)))
		{
			return E_FAIL;
		};
	}

	{
		CComAnimator::DESC DescAnim{};
		DescAnim.sComTag = "ComCModelIntance";
	
		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_Animator", "ComCModelAnimator", &DescAnim, &m_pModelAnimator)))
		{
			return E_FAIL;
		};

		// TestModel은 생성 직후부터 CPU pose + VS skinning 경로를 사용한다.
		m_pModelAnimator->SetEvaluationMode(
			CComAnimator::EVALUATION_MODE::CPU_GPU);
		m_pModelAnimator->Play_Anim(1.f, true, 0.2f);
	}



	//CTestPartObject::DESC WeaponDesc{};
	//WeaponDesc.sObjectTag = "Weapon";
	//WeaponDesc.hOwner = GetHandle();
	//WeaponDesc.iBoneIndex = m_pComModelInstance->GetModel()->Get_BoneIndex("SKT_RightHandSocket");
	//WeaponDesc.vBoneOffset = {0.f,0.f,0.f};
	//WeaponDesc.sGroupTag = "TEST"; 
	//WeaponDesc.sResTag = "Static_Axe_Model_Resource";

	//auto Weapon = E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_TEST", "Prototype_GameObject_TestPartObject", "Weapon", &WeaponDesc);
	//if (!Weapon.has_value())
	//{
	//	MSG_BOX("Create Failed Weapon");
	//	return E_FAIL;
	//}

	//m_Partes[ETOUI(PARTES::WEAPON)] = Weapon.value();
	return S_OK;

}

void CTestModel::PriorityUpdate(E::_float fTimeDelta)
{ 
}

void CTestModel::Update(E::_float fTimeDelta)
{
	ZoneScopedN("Update TestModel");

	if (m_pComModelInstance->GetModel()->GetAnimations().size() != 0) {
		
		m_pModelAnimator->Update(fTimeDelta);
	}

}

void CTestModel::LateUpdate(E::_float fTimeDelta)
{
	GetTransform().Update();

	const auto& pModel = m_pComModelInstance->GetModel();

	if (!pModel)
		return;

	if (!pModel->GetAnimations().empty())
	{
		CGameInstance::Get().Add_Instance(m_pComModelInstance, m_pModelAnimator, *GetTransform().GetCombinedWorldMatrix());
		return;
	}


	CGameInstance::Get().AddRenderObject(RENDERGROUP::NONBLEND,this);
}

HRESULT CTestModel::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{

	{
		E::CB_PER_OBJECT cbPerObject{};
		cbPerObject.matWorld = *GetTransform().GetCombinedWorldMatrix();
		XMStoreFloat4x4(&cbPerObject.matWVP, GetTransform().GetLoadedCombinedWorldMatrix() * ctx.matViewProj);
		if (FAILED(m_pComCBufferPerObject->MapDiscard(pContext, &cbPerObject, sizeof(cbPerObject))))
		{
			return E_FAIL; 
		}
		pContext->VSSetConstantBuffers(0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());
		pContext->PSSetConstantBuffers(0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());


		m_pComModelInstance->DebugDraw_Bones(cbPerObject.matWorld);
		
	}
	const auto& vs = m_pResVertexShader;
	
	const auto& ps = m_pResPixelShader;


	pContext->IASetInputLayout(vs->GetInputLayout().Get());
	pContext->VSSetShader(vs->GetVertexShader().Get(), nullptr, 0);
	pContext->PSSetShader(ps->GetPixelShader().Get(), nullptr, 0);

	auto pModel = m_pComModelInstance->GetModel(); 

	uint32_t	iNumMeshes = pModel->Get_NumMeshes();
	for (uint32_t i = 0; i < iNumMeshes; ++i) {
		const auto& viBuffer = pModel->GetMeshes()[i];


		ID3D11Buffer* vertexBuffers[] = {
				viBuffer->GetVertexBuffer().Get()
		};
		uint32_t strides[] = {
			viBuffer->GetVertexStride()
		};
		uint32_t offsets[] = {
			0
		};
		pContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
		pContext->IASetIndexBuffer(viBuffer->GetIndexBuffer().Get(), viBuffer->GetIndexFormat(), 0);
		pContext->IASetPrimitiveTopology(viBuffer->GetPrimitiveType());

		{
			if(!m_pComModelInstance->GetModel()->GetAnimations().empty())
				if (FAILED(m_pComModelInstance->Bind_BoneMatrices(pContext, i))) {
					return E_FAIL;
				}
		}

		{
			m_pComModelInstance->Bind_Textures(pContext, i);
			m_pComModelInstance->Bind_Materials(pContext, { 1.f, 1.f, 1.f }, 0.f, { 1.f, 1.f, 1.f }, 0.f, 1.f);	// EmissiveColor -> EmissiveIntensity -> Alpha ??
		}
		                                                                                                                                                                                     
		pContext->DrawIndexed(viBuffer->GetNumIndices(), 0, 0);
		//pContext->DrawIndexedInstancedIndirect(viBuffer->GetNumIndices(), 0, 0);
	}


	
	return S_OK;
}

HRESULT CTestModel::Render_CPU_GPU(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
	if (!pContext || !m_pCPUGPUSkinningComputeShader || !m_pResVertexCPUGPUShader || !m_pResPixelShader)
		return E_FAIL;

	auto pModel = m_pComModelInstance->GetModel();
	if (!pModel)
		return E_FAIL;

	GPU_ANIM_INSTANCE_DATA instanceData{};
	instanceData.WorldMatrix = *GetTransform().GetCombinedWorldMatrix();
	if (FAILED(Update_InstanceBuffer(pContext, { instanceData })))
		return E_FAIL;

	auto pSkinPaletteBuffer = CGameInstance::Get().GetResourceFirst<CResStructuredBuffer>(
	TAG_RES_GRP_PERMANENT_BUFFER, "SBUFFER_FINALBONEMATRIX");
	if (!pSkinPaletteBuffer)
		return E_FAIL;

	E::_float4x4 identity{};
	XMStoreFloat4x4(&identity, XMMatrixIdentity());
	std::vector<E::_float4x4> skinPalette(512, identity);
	const auto& combinedMatrices = m_pComModelInstance->Get_CombinedBoneMatrices();
	if (combinedMatrices.size() > skinPalette.size())
		return E_FAIL;
	memcpy(skinPalette.data(), combinedMatrices.data(),
		sizeof(E::_float4x4) * combinedMatrices.size());
	if (FAILED(pSkinPaletteBuffer->UpdateData(skinPalette.data(),
		static_cast<uint32_t>(skinPalette.size() * sizeof(E::_float4x4)))))
		return E_FAIL;

	const auto& vs = m_pResVertexCPUGPUShader;
	const auto& ps = m_pResPixelShader;
	pContext->IASetInputLayout(vs->GetInputLayout().Get());
	pContext->VSSetShader(vs->GetVertexShader().Get(), nullptr, 0);
	pContext->PSSetShader(ps->GetPixelShader().Get(), nullptr, 0);

	for (uint32_t iMeshIndex = 0; iMeshIndex < pModel->Get_NumMeshes(); ++iMeshIndex)
	{
		const auto& mesh = pModel->GetMeshes()[iMeshIndex];
		if (!mesh)
			return E_FAIL;

		if (FAILED(mesh->EnsureSkinnedVertexBuffer(1)))
			return E_FAIL;

		auto inputBuffer = mesh->GetSkinningInputBuffer();
		auto outputBuffer = mesh->GetSkinnedVertexBuffer();
		if (!inputBuffer || !outputBuffer)
			return E_FAIL;

		ID3D11ShaderResourceView* nullVSSRV = nullptr;
		pContext->VSSetShaderResources(7, 1, &nullVSSRV);

		ID3D11ShaderResourceView* inputSRV = inputBuffer->GetSRV().Get();
		ID3D11UnorderedAccessView* outputUAV = outputBuffer->GetUAV().Get();
		if (!inputSRV || !outputUAV)
			return E_FAIL;

		E::GPU_SKIN_MESH_CONSTANTS skinningConstants{};
		skinningConstants.iSkinBoneOffset = pModel->Get_GPUMeshSkinRange(iMeshIndex).iSkinBoneOffset;
		skinningConstants.iVertexCount = mesh->GetNumVertices();
		D3D11_MAPPED_SUBRESOURCE mapped{};
		if (FAILED(pContext->Map(m_pResSkinMeshCBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
			return E_FAIL;
		memcpy(mapped.pData, &skinningConstants, sizeof(skinningConstants));
		pContext->Unmap(m_pResSkinMeshCBuffer->GetCBuffer().Get(), 0);
		ID3D11Buffer* skinningCB = m_pResSkinMeshCBuffer->GetCBuffer().Get();
		pContext->CSSetConstantBuffers(5, 1, &skinningCB);
		ID3D11ShaderResourceView* paletteSRV = pSkinPaletteBuffer->GetSRV().Get();
		ID3D11ShaderResourceView* skinBonesSRV = pModel->Get_GPUSkinBoneSRV();
		if (!paletteSRV || !skinBonesSRV)
			return E_FAIL;
		ID3D11ShaderResourceView* computeSRVs[] = { inputSRV, paletteSRV, skinBonesSRV };
		pContext->CSSetShaderResources(0, 3, computeSRVs);
		pContext->CSSetUnorderedAccessViews(0, 1, &outputUAV, nullptr);
		pContext->CSSetShader(m_pCPUGPUSkinningComputeShader->GetComputeShader().Get(), nullptr, 0);
		pContext->Dispatch((mesh->GetNumVertices() + 63) / 64, 1, 1);

		ID3D11ShaderResourceView* nullCSSRVs[3]{};
		ID3D11UnorderedAccessView* nullUAV = nullptr;
		pContext->CSSetShaderResources(0, 3, nullCSSRVs);
		pContext->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr);
		pContext->CSSetShader(nullptr, nullptr, 0);

		if (FAILED(Bind_InstanceBuffer_VS(pContext)))
			return E_FAIL;
		ID3D11ShaderResourceView* outputSRV = outputBuffer->GetSRV().Get();
		pContext->VSSetShaderResources(7, 1, &outputSRV);

		ID3D11Buffer* vertexBuffer = mesh->GetVertexBuffer().Get();
		const UINT stride = mesh->GetVertexStride();
		const UINT offset = 0;
		pContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
		pContext->IASetIndexBuffer(mesh->GetIndexBuffer().Get(), mesh->GetIndexFormat(), 0);
		pContext->IASetPrimitiveTopology(mesh->GetPrimitiveType());
		m_pComModelInstance->Bind_Textures(pContext, iMeshIndex);
		m_pComModelInstance->Bind_Materials(pContext, { 1.f, 1.f, 1.f }, 0.f, { 1.f, 1.f, 1.f }, 0.f, 1.f);
		pContext->DrawIndexed(mesh->GetNumIndices(), 0, 0);
	}

	ID3D11ShaderResourceView* nullVSSRV1[2]{};
	pContext->VSSetShaderResources(6, 2, nullVSSRV1);
	return S_OK;
}
HRESULT CTestModel::Render_CPU_GPU_Instanced(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx, const E::MODEL_INSTANCE_BATCH& Batch)
{
	if (!pContext || !m_pResVertexCPUSkinningInstancedShader || !m_pResPixelShader)
		return E_FAIL;

	const auto& vs = m_pResVertexCPUSkinningInstancedShader;
	const auto& ps = m_pResPixelShader;
	pContext->IASetInputLayout(vs->GetInputLayout().Get());
	pContext->VSSetShader(vs->GetVertexShader().Get(), nullptr, 0);
	pContext->PSSetShader(ps->GetPixelShader().Get(), nullptr, 0);

	const uint32_t iInstanceCount = static_cast<uint32_t>(Batch.Instances.size());
	if (iInstanceCount == 0 || iInstanceCount > 512 || Batch.CombinedBoneMatrices.size() != iInstanceCount)
		return E_FAIL;

	if (FAILED(Update_InstanceBuffer(pContext, Batch.Instances)))
		return E_FAIL;

	auto pModel = CGameInstance::Get().GetResourceFirst<CResModel>(Batch.Key.modelGroup, Batch.Key.modelTag);
	auto pCPUBonePaletteBuffer = CGameInstance::Get().GetResourceFirst<CResStructuredBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "SBUFFER_CPU_BONEMATRIX");
	if (!pModel || !pCPUBonePaletteBuffer)
		return E_FAIL;

	_float4x4 identity{};
	XMStoreFloat4x4(&identity, XMMatrixIdentity());
	std::vector<_float4x4> combinedPalette(iInstanceCount * 512, identity);
	for (uint32_t instanceIndex = 0; instanceIndex < iInstanceCount; ++instanceIndex)
	{
		const auto& combinedMatrices = Batch.CombinedBoneMatrices[instanceIndex];
		if (combinedMatrices.empty() || combinedMatrices.size() > 512)
			return E_FAIL;

		// DirectXMath로 계산한 CPU Combined 행렬을 VS의 t7 행렬 규약에 맞춘다.
		// CPU 원본은 다른 CPU 기능에서도 사용하므로 업로드 복사본만 전치한다.
		for (uint32_t boneIndex = 0;
			boneIndex < static_cast<uint32_t>(combinedMatrices.size());
			++boneIndex)
		{
			XMStoreFloat4x4(
				&combinedPalette[instanceIndex * 512 + boneIndex],
				XMMatrixTranspose(
					XMLoadFloat4x4(&combinedMatrices[boneIndex])));
		}
	}

	// CPU가 계산한 CombinedBone palette는 batch당 한 번만 갱신한다.
	ID3D11ShaderResourceView* nullPaletteSRV = nullptr;
	pContext->VSSetShaderResources(7, 1, &nullPaletteSRV);
	if (FAILED(pCPUBonePaletteBuffer->UpdateData(
		combinedPalette.data(),
		static_cast<uint32_t>(combinedPalette.size() * sizeof(_float4x4)))))
		return E_FAIL;



	if (FAILED(Bind_InstanceBuffer_VS(pContext)))
		return E_FAIL;
	ID3D11ShaderResourceView* cpuBonePaletteSRV = pCPUBonePaletteBuffer->GetSRV().Get();
	if (!cpuBonePaletteSRV)
		return E_FAIL;

	ID3D11ShaderResourceView* skinBonesSRV = pModel->Get_GPUSkinBoneSRV();
	if (!skinBonesSRV)
		return E_FAIL;

	pContext->VSSetShaderResources(7, 1, &cpuBonePaletteSRV);
	pContext->VSSetShaderResources(8, 1, &skinBonesSRV);

	for (uint32_t iMeshIndex = 0; iMeshIndex < pModel->Get_NumMeshes(); ++iMeshIndex)
	{
		const auto& mesh = pModel->GetMeshes()[iMeshIndex];
		if (!mesh)
			continue;

		const auto& skinRange = pModel->Get_GPUMeshSkinRange(iMeshIndex);
		if (skinRange.iSkinBoneCount == 0)
			return E_FAIL;

		E::GPU_SKIN_MESH_CONSTANTS skinningConstants{};
		skinningConstants.iSkinBoneOffset = skinRange.iSkinBoneOffset;
		skinningConstants.iVertexCount = mesh->GetNumVertices();
		skinningConstants.iSkinBoneCount = skinRange.iSkinBoneCount;
		D3D11_MAPPED_SUBRESOURCE mapped{};
		if (FAILED(pContext->Map(m_pResSkinMeshCBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
			return E_FAIL;
		memcpy(mapped.pData, &skinningConstants, sizeof(skinningConstants));
		pContext->Unmap(m_pResSkinMeshCBuffer->GetCBuffer().Get(), 0);
		ID3D11Buffer* skinningCB = m_pResSkinMeshCBuffer->GetCBuffer().Get();
		pContext->VSSetConstantBuffers(5, 1, &skinningCB);
		ID3D11Buffer* vertexBuffer = mesh->GetVertexBuffer().Get();
		const UINT stride = mesh->GetVertexStride();
		const UINT offset = 0;
		pContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
		pContext->IASetIndexBuffer(mesh->GetIndexBuffer().Get(), mesh->GetIndexFormat(), 0);
		pContext->IASetPrimitiveTopology(mesh->GetPrimitiveType());
		m_pComModelInstance->Bind_Textures(pContext, iMeshIndex);
		m_pComModelInstance->Bind_Materials(pContext, { 1.f, 1.f, 1.f }, 0.f, { 1.f, 1.f, 1.f }, 0.f, 1.f);
		pContext->DrawIndexedInstanced(mesh->GetNumIndices(), iInstanceCount, 0, 0, 0);
	}

#ifdef _DEBUG
	if (m_pComModelInstance->IsDebugBoneDrawEnabled())
	{
		for (uint32_t instanceIndex = 0; instanceIndex < iInstanceCount; ++instanceIndex)
		{
			m_pComModelInstance->DebugDraw_Bones(
				Batch.Instances[instanceIndex].WorldMatrix,
				Batch.CombinedBoneMatrices[instanceIndex]);
		}
	}
#endif

	ID3D11ShaderResourceView* nullVSSRVs[3]{};
	pContext->VSSetShaderResources(6, 3, nullVSSRVs);
	return S_OK;
}
HRESULT CTestModel::Render_Instanced(ID3D11DeviceContext* pContext,const E::RENDER_CTX& ctx,const E::MODEL_INSTANCE_BATCH& Batch)
{
	const auto evaluationMode =
		static_cast<CComAnimator::EVALUATION_MODE>(Batch.Key.iEvaluationMode);

	// CPU+GPU: CPU가 계산한 palette를 VS에 직접 전달한다. CS는 사용하지 않는다.
	if (evaluationMode == CComAnimator::EVALUATION_MODE::CPU_GPU)
		return Render_CPU_GPU_Instanced(pContext, ctx, Batch);

	// GPU: 기존 Animation CS -> FinalBone SRV -> GPU skinning VS 경로만 허용한다.
	if (evaluationMode != CComAnimator::EVALUATION_MODE::GPU)
		return E_FAIL;

	ZoneScopedN("Render TestModel");

	if (!pContext)
		return E_INVALIDARG;

	const uint32_t iInstanceCount = Batch.Instances.size();

	if (iInstanceCount == 0)
		return S_OK;



	if (!m_pAnimComputeShader ||!m_pAnimComputeShader->GetComputeShader())
	{
		return E_FAIL;
	}

	// -------------------------------------------------
	// Compute Shader
	// -------------------------------------------------

	if (FAILED(Update_InstanceBuffer(pContext,Batch.Instances)))
	{
		return E_FAIL;
	}

	// CS t0 ~ t5
	if (FAILED(m_pComModelInstance->Bind_GPUAnimationSRVs_CS(pContext)))
	{
		return E_FAIL;
	}

	// CS t6
	if (FAILED(Bind_InstanceBuffer_CS(pContext)))
	{
		return E_FAIL;
	}

	// CS u0
	if (FAILED(Bind_FinalBoneUAV_CS(pContext)))
	{
		return E_FAIL;
	}

	pContext->CSSetShader(m_pAnimComputeShader->GetComputeShader().Get(),nullptr,0);

	/*
	 * ??Thread Group = ???�스?�스?�는 ?�제.
	 */
	pContext->Dispatch(iInstanceCount,1,1);

	if (FAILED(Unbind_AnimationCompute(pContext)))
	{
		return E_FAIL;
	}

	// -------------------------------------------------
	// Vertex Shader??SRV
	// -------------------------------------------------

	// VS t6 = InstanceData
	if (FAILED(Bind_InstanceBuffer_VS(pContext)))
	{
		return E_FAIL;
	}

	// VS t7 = Compute 결과 FinalBoneMatrix
	if (FAILED(Bind_FinalBoneSRV_VS(pContext)))
	{
		return E_FAIL;
	}
	if (FAILED(m_pComModelInstance->Bind_GPUSkinBones_VS(pContext)))
	{
		return E_FAIL;
	}

	// -------------------------------------------------
	// Graphics Shader
	// -------------------------------------------------

	const auto& vs =m_pResVertexInstancedShader;

	const auto& ps =m_pResPixelShader;

	if (!vs || !ps)
		return E_FAIL;

	pContext->IASetInputLayout(vs->GetInputLayout().Get());

	pContext->VSSetShader(vs->GetVertexShader().Get(),nullptr,0);

	pContext->PSSetShader(ps->GetPixelShader().Get(),nullptr,0);

	auto pModel =CGameInstance::Get().GetResourceFirst<CResModel>(Batch.Key.modelGroup,Batch.Key.modelTag);

	const uint32_t iNumMeshes =pModel->Get_NumMeshes();

	for (uint32_t iMeshIndex = 0;iMeshIndex < iNumMeshes;++iMeshIndex)
	{
		const auto& viBuffer =pModel->GetMeshes()[iMeshIndex];

		if (!viBuffer)
			continue;

		ID3D11Buffer* vertexBuffers[] =
		{
			viBuffer->GetVertexBuffer().Get()
		};

		UINT strides[] =
		{
			viBuffer->GetVertexStride()
		};

		UINT offsets[] =
		{
			0
		};

		pContext->IASetVertexBuffers(0,1,vertexBuffers,strides,offsets);

		pContext->IASetIndexBuffer(viBuffer->GetIndexBuffer().Get(),viBuffer->GetIndexFormat(),0);

		pContext->IASetPrimitiveTopology(viBuffer->GetPrimitiveType());

		E::GPU_SKIN_MESH_CONSTANTS skinConstants{};
		skinConstants.iSkinBoneOffset = pModel->Get_GPUMeshSkinRange(iMeshIndex).iSkinBoneOffset;
		D3D11_MAPPED_SUBRESOURCE mappedResource{};
		if (FAILED(pContext->Map(m_pResSkinMeshCBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
			return E_FAIL;
		memcpy(mappedResource.pData, &skinConstants, sizeof(skinConstants));
		pContext->Unmap(m_pResSkinMeshCBuffer->GetCBuffer().Get(), 0);
		ID3D11Buffer* pSkinMeshCB = m_pResSkinMeshCBuffer->GetCBuffer().Get();
		pContext->VSSetConstantBuffers(5, 1, &pSkinMeshCB);


		m_pComModelInstance->Bind_Textures(pContext, iMeshIndex);
		m_pComModelInstance->Bind_Materials(pContext, { 1.f, 1.f, 1.f }, 0.f, { 1.f, 1.f, 1.f }, 0.f, 1.f);

		pContext->DrawIndexedInstanced(viBuffer->GetNumIndices(),iInstanceCount,0,0,0);
	}



	if (FAILED(Unbind_AnimationVS(pContext)))
	{
		return E_FAIL;
	}

	return S_OK;
}
HRESULT CTestModel::Update_InstanceBuffer(ID3D11DeviceContext* pContext,const std::vector<GPU_ANIM_INSTANCE_DATA>& Instances)
{

	m_iCurrentInstanceCount =static_cast<uint32_t>(Instances.size());



	if (Instances.empty())
		return S_OK;

	constexpr uint32_t MAX_INSTANCE_COUNT = 512;

	if (m_iCurrentInstanceCount > MAX_INSTANCE_COUNT)
		return E_FAIL;

	auto pStructuredBuffer =CGameInstance::Get().GetResourceFirst<CResStructuredBuffer>(TAG_RES_GRP_PERMANENT_BUFFER,"SBUFFER_ANIMAITON");

	if (!pStructuredBuffer)
		return E_FAIL;

	ID3D11Buffer* pBuffer =pStructuredBuffer->GetBuffer().Get();

	if (!pBuffer)
		return E_FAIL;

	ID3D11ShaderResourceView* pNullSRV = nullptr;


	pContext->VSSetShaderResources(6,1,&pNullSRV);

	const size_t iCopySize = sizeof(GPU_ANIM_INSTANCE_DATA) * m_iCurrentInstanceCount;


	D3D11_BOX updateBox{};
	updateBox.left = 0;
	updateBox.right = static_cast<UINT>(iCopySize);
	updateBox.top = 0;
	updateBox.bottom = 1;
	updateBox.front = 0;
	updateBox.back = 1;

	pContext->UpdateSubresource(pBuffer, 0, &updateBox, Instances.data(), 0, 0);

	return S_OK;

}

HRESULT CTestModel::Bind_InstanceBuffer_CS(ID3D11DeviceContext* pContext)
{
	auto pStructuredBuffer = CGameInstance::Get().GetResourceFirst<CResStructuredBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "SBUFFER_ANIMAITON");

	if (!pStructuredBuffer)
		return E_FAIL;

	ID3D11ShaderResourceView* pSRV = pStructuredBuffer->GetSRV().Get();

	if (!pSRV)
		return E_FAIL;

	pContext->CSSetShaderResources(6, 1, &pSRV);

	return S_OK;
}
HRESULT CTestModel::Bind_FinalBoneUAV_CS(ID3D11DeviceContext* pContext)
{
	auto pStructuredBuffer =CGameInstance::Get().GetResourceFirst<CResStructuredBuffer>(TAG_RES_GRP_PERMANENT_BUFFER,"SBUFFER_FINALBONEMATRIX");

	if (!pStructuredBuffer)
		return E_FAIL;

	ID3D11UnorderedAccessView* pUAV = pStructuredBuffer->GetUAV().Get();

	if (!pUAV)
		return E_FAIL;


	ID3D11ShaderResourceView* pNullSRV = nullptr;

	pContext->VSSetShaderResources(7,1,&pNullSRV);


	pContext->CSSetUnorderedAccessViews(0,1,&pUAV,nullptr);

	return S_OK;
}
HRESULT CTestModel::Unbind_AnimationCompute(ID3D11DeviceContext* pContext)
{

	ID3D11ShaderResourceView* pNullSRVs[7] =
	{
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr
	};

	pContext->CSSetShaderResources(0,7,pNullSRVs);


	ID3D11UnorderedAccessView* pNullUAV = nullptr;

	pContext->CSSetUnorderedAccessViews(0,1,&pNullUAV,nullptr);


	pContext->CSSetShader(nullptr,nullptr,0);

	return S_OK;
}

HRESULT CTestModel::Bind_InstanceBuffer_VS(ID3D11DeviceContext* pContext)
{

	auto pStructuredBuffer = CGameInstance::Get().GetResourceFirst<CResStructuredBuffer>(TAG_RES_GRP_PERMANENT_BUFFER,"SBUFFER_ANIMAITON");

	if (!pStructuredBuffer)
		return E_FAIL;

	ID3D11ShaderResourceView* pSRV = pStructuredBuffer->GetSRV().Get();

	if (!pSRV)
		return E_FAIL;


	pContext->VSSetShaderResources(6,1,&pSRV);

	return S_OK;
}

HRESULT CTestModel::Bind_FinalBoneSRV_VS( ID3D11DeviceContext* pContext)
{

	auto pStructuredBuffer =CGameInstance::Get().GetResourceFirst<CResStructuredBuffer>(TAG_RES_GRP_PERMANENT_BUFFER,"SBUFFER_FINALBONEMATRIX");

	if (!pStructuredBuffer)
		return E_FAIL;

	ID3D11ShaderResourceView* pSRV = pStructuredBuffer->GetSRV().Get();

	if (!pSRV)
		return E_FAIL;


	pContext->VSSetShaderResources(7,1,&pSRV);

	return S_OK;
}
HRESULT CTestModel::Unbind_AnimationVS(ID3D11DeviceContext* pContext)
{
	if (!pContext)
		return E_INVALIDARG;

	ID3D11ShaderResourceView* pNullSRVs[3]{};

	pContext->VSSetShaderResources(6,3,pNullSRVs);

	return S_OK;
}

E::UPtr<CTestModel> CTestModel::Create()
{
	auto pInstance = E::ToUPtr(new CTestModel{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CTestModel");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CTestModel::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CTestModel{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CTestModel");
		return nullptr;
	}

	return pInstance;
}

#include "pch.h"
#include "RagdollEditorGUI.h"

#include "CameraObject.h"
#include "ComConstantBuffer.h"
#include "ComModelInstance.h"
#include "ComPxRagdoll.h"
#include "DbgLineRender.h"
#include "GameObject.h"
#include "GameInstance.h"
#include "IRenderable.h"
#include "ResModel.h"
#include "ResModelBone.h"
#include "ResModelMesh.h"
#include "ResPixelShader.h"
#include "ResVertexShader.h"
#include "Resource.h"

#include <filesystem>

NS_USING(Engine)

namespace Engine
{
	class CRagdollPhysicsPreviewOwner final :
		public CGameObject
	{
	public:
		DECLARE_DERIVED_TYPE(
			CRagdollPhysicsPreviewOwner,
			CGameObject)

	private:
		CRagdollPhysicsPreviewOwner() =
			default;
		~CRagdollPhysicsPreviewOwner()
			override = default;

	public:
		_bool Activate(
			const _float3& vLinearVelocity,
			const _float3&
				vAngularVelocityDegrees)
		{
			if (!m_pRagdoll)
				return false;

			return m_pRagdoll->
				ActivateRagdoll(
					vLinearVelocity,
					{
						XMConvertToRadians(
							vAngularVelocityDegrees.x),
						XMConvertToRadians(
							vAngularVelocityDegrees.y),
						XMConvertToRadians(
							vAngularVelocityDegrees.z)
					});
		}

		_bool WritePhysicsPoseToBones(
			std::vector<_float4x4>&
				InOutCombinedBoneMatrices) const
		{
			return m_pRagdoll &&
				m_pRagdoll->
					WritePhysicsPoseToBones(
						InOutCombinedBoneMatrices,
						GetTransform().
							GetLoadedCombinedWorldMatrix());
		}

		_bool GetBodyWorldMatrix(
			size_t iBodyIndex,
			_float4x4& OutWorldMatrix) const
		{
			return m_pRagdoll &&
				m_pRagdoll->
					GetBodyWorldMatrix(
						iBodyIndex,
						OutWorldMatrix);
		}

		static UPtr<
			CRagdollPhysicsPreviewOwner> Create(
				const PX_RAGDOLL_DESC& tRagdoll,
				CResModel& Model,
				const std::vector<_float4x4>&
					BindPoses,
				const _float3& vPosition)
		{
			auto pInstance =
				ToUPtr(
					new CRagdollPhysicsPreviewOwner{});

			GAMEOBJECT_DESC ObjectDesc{};
			ObjectDesc.sObjectTag =
				"RagdollEditorPhysicsPreview";
			if (FAILED(
				pInstance->
					CGameObject::Initialize(
						&ObjectDesc)))
			{
				return nullptr;
			}
			pInstance->GetTransform().
				SetPosition(vPosition);
			pInstance->GetTransform().Update();

			CComPxRagdoll::DESC RagdollDesc{};
			RagdollDesc.tRagdoll =
				tRagdoll;
			if (FAILED(
				pInstance->
					AddComponentFromProto(
						ES_EngineProtoMajorType::
							PHYSX,
						ES_EngineProtoPhysXComponent::
							Prototype_Component_ComPxRagdoll,
						"ComPxRagdoll",
						&RagdollDesc,
						&pInstance->
							m_pRagdoll)) ||
				!pInstance->m_pRagdoll ||
				!pInstance->m_pRagdoll->
					BindSkeleton(Model) ||
				!pInstance->m_pRagdoll->
					CacheAnimationPose(
						BindPoses,
						pInstance->
							GetTransform().
								GetLoadedCombinedWorldMatrix()))
			{
				return nullptr;
			}

			return pInstance;
		}

		UPtr<CPrototype> Clone(
			void* pArg) override
		{
			return nullptr;
		}

	private:
		CComPxRagdoll* m_pRagdoll{};
	};

	class CRagdollPreviewRenderer final :
		public CEngineBase,
		public IRenderable
	{
	private:
		CRagdollPreviewRenderer() = default;
		~CRagdollPreviewRenderer() override =
			default;

	public:
		HRESULT Initialize(
			const StringID& sGroupTag,
			const StringID& sResourceTag,
			const std::vector<_float4x4>&
				BindPoses)
		{
			CComModelInstance::DESC ModelDesc{};
			ModelDesc.sGroupTag = sGroupTag;
			ModelDesc.sResTag = sResourceTag;
			auto pModelPrototype =
				CGameInstance::Get().
					ClonePrototype(
						"PERMANENT",
						"Prototype_Component_ModelInstance",
						&ModelDesc);
			if (!pModelPrototype ||
				!pModelPrototype->IsA(
					CComModelInstance::StaticType))
			{
				return E_FAIL;
			}
			m_pModelInstance =
				static_uptr_cast<
					CComModelInstance>(
						std::move(
							pModelPrototype));

			CComConstantBuffer::DESC BufferDesc{};
			BufferDesc.cBufferId = {
				TAG_RES_GRP_PERMANENT_BUFFER,
				TAG_RES_CBUFFER_OBJECT
			};
			auto pBufferPrototype =
				CGameInstance::Get().
					ClonePrototype(
						"PERMANENT",
						"Prototype_Component_ConstantBuffer",
						&BufferDesc);
			if (!pBufferPrototype ||
				!pBufferPrototype->IsA(
					CComConstantBuffer::StaticType))
			{
				return E_FAIL;
			}
			m_pObjectBuffer =
				static_uptr_cast<
					CComConstantBuffer>(
						std::move(
							pBufferPrototype));

			m_pVertexShader =
				CGameInstance::Get().
					GetResourceFirst<
						CResVertexShader>(
							TAG_RES_GRP_PERMANENT_SHADER,
							"VS_TestModelAnim");
			m_pPixelShader =
				CGameInstance::Get().
					GetResourceFirst<
						CResPixelShader>(
							TAG_RES_GRP_PERMANENT_SHADER,
							"PS_TestModelAnim");
			if (!m_pModelInstance ||
				!m_pModelInstance->GetModel() ||
				!m_pObjectBuffer ||
				!m_pVertexShader ||
				!m_pPixelShader ||
				FAILED(m_pVertexShader->Load()) ||
				FAILED(m_pPixelShader->Load()))
			{
				return E_FAIL;
			}

			m_pModelInstance->
				Get_CombinedBoneMatrices() =
					BindPoses;
			XMStoreFloat4x4(
				&m_WorldMatrix,
				XMMatrixIdentity());
			return S_OK;
		}

		void SetWorldMatrix(
			FXMMATRIX WorldMatrix)
		{
			XMStoreFloat4x4(
				&m_WorldMatrix,
				WorldMatrix);
		}

		HRESULT Render(
			ID3D11DeviceContext* pContext,
			const RENDER_CTX& tContext) override
		{
			if (!pContext ||
				!m_pModelInstance ||
				!m_pObjectBuffer ||
				!m_pVertexShader ||
				!m_pPixelShader)
			{
				return E_FAIL;
			}

			auto pModel =
				m_pModelInstance->GetModel();
			if (!pModel)
				return E_FAIL;

			CB_PER_OBJECT PerObject{};
			PerObject.matWorld =
				m_WorldMatrix;
			XMStoreFloat4x4(
				&PerObject.matWVP,
				XMLoadFloat4x4(
					&m_WorldMatrix) *
				tContext.matViewProj);
			if (FAILED(
				m_pObjectBuffer->MapDiscard(
					pContext,
					&PerObject,
					sizeof(PerObject))))
			{
				return E_FAIL;
			}
			pContext->VSSetConstantBuffers(
				0,
				1,
				m_pObjectBuffer->
					GetAdressOfBuffer());
			pContext->PSSetConstantBuffers(
				0,
				1,
				m_pObjectBuffer->
					GetAdressOfBuffer());

			pContext->IASetInputLayout(
				m_pVertexShader->
					GetInputLayout().Get());
			pContext->VSSetShader(
				m_pVertexShader->
					GetVertexShader().Get(),
				nullptr,
				0);
			pContext->PSSetShader(
				m_pPixelShader->
					GetPixelShader().Get(),
				nullptr,
				0);

			for (uint32_t iMesh = 0;
				iMesh < pModel->Get_NumMeshes();
				++iMesh)
			{
				const auto& pMesh =
					pModel->GetMeshes()[iMesh];
				if (!pMesh)
					continue;

				ID3D11Buffer* pVertexBuffer =
					pMesh->GetVertexBuffer().Get();
				const uint32_t iStride =
					pMesh->GetVertexStride();
				const uint32_t iOffset = 0;
				pContext->IASetVertexBuffers(
					0,
					1,
					&pVertexBuffer,
					&iStride,
					&iOffset);
				pContext->IASetIndexBuffer(
					pMesh->GetIndexBuffer().Get(),
					pMesh->GetIndexFormat(),
					0);
				pContext->IASetPrimitiveTopology(
					pMesh->GetPrimitiveType());

				if (FAILED(
					m_pModelInstance->
						Bind_BoneMatrices(
							pContext,
							iMesh)))
				{
					return E_FAIL;
				}
				m_pModelInstance->Bind_Textures(
					pContext,
					iMesh);
				m_pModelInstance->Bind_Materials(
					pContext,
					{ 1.f, 1.f, 1.f },
					0.f,
					{ 1.f, 1.f, 1.f },
					0.f,
					1.f);
				pContext->DrawIndexed(
					pMesh->GetNumIndices(),
					0,
					0);
			}

			return S_OK;
		}

		bool HasRenderPass(
			RENDERPASS ePass) const override
		{
			return ePass ==
				RENDERPASS::DEFAULT;
		}

		static UPtr<
			CRagdollPreviewRenderer> Create(
				const StringID& sGroupTag,
				const StringID& sResourceTag,
				const std::vector<_float4x4>&
					BindPoses)
		{
			auto pInstance =
				ToUPtr(
					new CRagdollPreviewRenderer{});
			if (FAILED(
				pInstance->Initialize(
					sGroupTag,
					sResourceTag,
					BindPoses)))
			{
				return nullptr;
			}
			return pInstance;
		}

		void SetCombinedBoneMatrices(
			const std::vector<_float4x4>&
				CombinedBoneMatrices)
		{
			if (m_pModelInstance)
			{
				m_pModelInstance->
					Get_CombinedBoneMatrices() =
						CombinedBoneMatrices;
			}
		}

		std::vector<_float4x4>*
			GetCombinedBoneMatrices()
		{
			return m_pModelInstance
				? &m_pModelInstance->
					Get_CombinedBoneMatrices()
				: nullptr;
		}

	private:
		UPtr<CComModelInstance>
			m_pModelInstance{};
		UPtr<CComConstantBuffer>
			m_pObjectBuffer{};
		SPtr<CResVertexShader>
			m_pVertexShader{};
		SPtr<CResPixelShader>
			m_pPixelShader{};
		_float4x4 m_WorldMatrix{};
	};
}

namespace
{
	constexpr const char* RAGDOLL_ROOT_NAME =
		"Ragdoll";
	constexpr int RAGDOLL_EDITOR_GIZMO_ID =
		0x52414744;
	constexpr _float MIN_RAGDOLL_SHAPE_SIZE =
		0.001f;

	struct MODEL_ENTRY
	{
		std::string sLabel{};
		SPtr<CResModel> pModel{};
		StringID sGroupTag{};
		StringID sResourceTag{};
	};

	std::vector<MODEL_ENTRY> CollectModelEntries()
	{
		std::vector<MODEL_ENTRY> Entries{};
		const auto Resources =
			CGameInstance::Get().GetResources();
		for (const auto& [GroupTag, Group] :
			Resources)
		{
			for (const auto& [ResourceTag, Values] :
				Group)
			{
				for (const auto& pResource : Values)
				{
					if (!pResource ||
						!pResource->IsA(
							CResModel::StaticType))
					{
						continue;
					}

					auto pModel =
						std::static_pointer_cast<
							CResModel>(pResource);
					std::string sLabel{};
					const std::string_view sGroup =
						GroupTag.GetDbgStr();
					const std::string_view sResource =
						ResourceTag.GetDbgStr();
					sLabel.reserve(
						sGroup.size() +
						sResource.size() +
						pModel->GetPath().size() +
						8);
					sLabel.append(
						sGroup.data(),
						sGroup.size());
					sLabel += " / ";
					sLabel.append(
						sResource.data(),
						sResource.size());
					sLabel += " | ";
					sLabel += pModel->GetPath();
					Entries.push_back({
						std::move(sLabel),
						std::move(pModel),
						GroupTag,
						ResourceTag
						});
				}
			}
		}

		std::ranges::sort(
			Entries,
			{},
			&MODEL_ENTRY::sLabel);
		return Entries;
	}

	_bool HasSingleBit(uint32_t iValue)
	{
		return iValue != 0 &&
			(iValue & (iValue - 1)) == 0;
	}

	_matrix MakePoseMatrix(
		const _float3& vPosition,
		const _float4& vRotation)
	{
		_vector vQuaternion =
			XMLoadFloat4(&vRotation);
		if (XMVectorGetX(
			XMVector4LengthSq(vQuaternion)) <=
			1e-8f)
		{
			vQuaternion =
				XMQuaternionIdentity();
		}
		else
		{
			vQuaternion =
				XMQuaternionNormalize(vQuaternion);
		}

		return XMMatrixRotationQuaternion(
			vQuaternion) *
			XMMatrixTranslation(
				vPosition.x,
				vPosition.y,
				vPosition.z);
	}

	_float3 GetMatrixPosition(
		FXMMATRIX Matrix)
	{
		_float3 vPosition{};
		XMStoreFloat3(
			&vPosition,
			Matrix.r[3]);
		return vPosition;
	}
}

CRagdollEditorGUI::~CRagdollEditorGUI() =
	default;

void CRagdollEditorGUI::UpdateGUI()
{
	if (!m_bOpen)
	{
		StopPhysicsPreview();
		return;
	}

	DrawWindow();
	UpdatePhysicsPreviewPose();
	DrawPreview();
	RenderGizmo();
	if (m_bPreviewVisible &&
		m_bPreviewModel &&
		m_pPreviewRenderer)
	{
		CGameInstance::Get().AddRenderObject(
			RENDERGROUP::NONBLEND,
			m_pPreviewRenderer.get());
	}
}

void CRagdollEditorGUI::
SetCollisionLayerNames(
	std::vector<std::pair<uint32_t, std::string>>
		LayerNames)
{
	std::erase_if(
		LayerNames,
		[](const auto& Entry)
		{
			return Entry.second.empty() ||
				!HasSingleBit(Entry.first);
		});
	std::ranges::sort(
		LayerNames,
		{},
		&std::pair<uint32_t, std::string>::first);
	LayerNames.erase(
		std::unique(
			LayerNames.begin(),
			LayerNames.end(),
			[](const auto& Left,
				const auto& Right)
			{
				return Left.first == Right.first;
			}),
		LayerNames.end());
	m_CollisionLayerNames =
		std::move(LayerNames);
}

void CRagdollEditorGUI::DrawWindow()
{
	ImGui::SetNextWindowSize(
		ImVec2(980.f, 720.f),
		ImGuiCond_FirstUseEver);
	if (!ImGui::Begin(
		"Ragdoll Editor",
		&m_bOpen))
	{
		ImGui::End();
		return;
	}

	if (m_bOpenResultPopup)
	{
		ImGui::OpenPopup(
			"Ragdoll Editor Result");
		m_bOpenResultPopup = false;
	}

	DrawModelSelector();
	DrawPreviewControls();
	ImGui::Separator();

	ImGui::Checkbox(
		"Edit File Name",
		&m_bEditFileName);
	ImGui::SameLine();
	if (m_bEditFileName)
	{
		ImGui::SetNextItemWidth(220.f);
		ImGui::InputText(
			"Ragdoll File",
			m_RagdollFileName,
			std::size(m_RagdollFileName));
	}
	else
	{
		ImGui::Text(
			"Ragdoll File: %s",
			m_RagdollFileName);
	}

	if (ImGui::Button(
		"New",
		ImVec2(80.f, 0.f)))
	{
		ImGui::OpenPopup(
			"Confirm Ragdoll Clear");
	}
	ImGui::SameLine();
	if (ImGui::Button(
		"Load",
		ImVec2(80.f, 0.f)))
	{
		ImGui::OpenPopup(
			"Confirm Ragdoll Load");
	}
	ImGui::SameLine();
	if (ImGui::Button(
		"Save",
		ImVec2(80.f, 0.f)))
	{
		ImGui::OpenPopup(
			"Confirm Ragdoll Save");
	}
	ImGui::SameLine();
	if (ImGui::Button(
		"Auto Generate Humanoid",
		ImVec2(190.f, 0.f)))
	{
		const _bool bSuccess =
			GenerateHumanoidPreset();
		QueueResultPopup(
			m_sStatus,
			bSuccess);
	}
	ImGui::SameLine();
	if (ImGui::Button(
		"Validate",
		ImVec2(90.f, 0.f)))
	{
		const _bool bSuccess =
			Validate(m_ValidationErrors);
		m_sStatus = bSuccess
			? "Validation succeeded."
			: "Validation failed.";
		QueueResultPopup(
			m_sStatus,
			bSuccess);
	}

	ImGui::Separator();
	if (m_bPhysicsPreviewActive)
	{
		ImGui::TextColored(
			ImVec4(1.f, 0.8f, 0.2f, 1.f),
			"Physics test is active. Stop the test to edit bodies, shapes, or joints.");
	}
	else if (ImGui::BeginTable(
		"RagdollEditorLayout",
		2,
		ImGuiTableFlags_BordersInnerV |
		ImGuiTableFlags_Resizable))
	{
		ImGui::TableSetupColumn(
			"Hierarchy",
			ImGuiTableColumnFlags_WidthFixed,
			380.f);
		ImGui::TableSetupColumn(
			"Inspector",
			ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableNextColumn();
		DrawHierarchyPanel();
		ImGui::TableNextColumn();
		DrawInspectorPanel();
		ImGui::EndTable();
	}

	ImGui::Separator();
	ImGui::TextWrapped(
		"Status: %s%s",
		m_sStatus.c_str(),
		m_bDirty ? " (modified)" : "");
	if (!m_ValidationErrors.empty())
	{
		if (ImGui::CollapsingHeader(
			"Validation Errors",
			ImGuiTreeNodeFlags_DefaultOpen))
		{
			for (const auto& sError :
				m_ValidationErrors)
			{
				ImGui::BulletText(
					"%s",
					sError.c_str());
			}
		}
	}

	DrawFilePopups();
	ImGui::End();
}

void CRagdollEditorGUI::DrawPreviewControls()
{
	ImGui::Separator();
	ImGui::TextUnformatted("Viewport Preview");
	ImGui::Checkbox(
		"Visible",
		&m_bPreviewVisible);
	ImGui::SameLine();
	ImGui::Checkbox(
		"Model",
		&m_bPreviewModel);
	ImGui::SameLine();
	ImGui::Checkbox(
		"Skeleton",
		&m_bPreviewSkeleton);
	ImGui::SameLine();
	ImGui::Checkbox(
		"Bodies",
		&m_bPreviewBodies);
	ImGui::SameLine();
	ImGui::Checkbox(
		"Joints",
		&m_bPreviewJoints);
	ImGui::SameLine();
	ImGui::Checkbox(
		"Depth",
		&m_bPreviewDepthTest);

	if (ImGui::Button(
		"Place Preview At Camera",
		ImVec2(190.f, 0.f)))
	{
		PlacePreviewAtCamera();
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(260.f);
	ImGui::DragFloat3(
		"Preview Position",
		reinterpret_cast<float*>(
			&m_vPreviewPosition),
		0.05f);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(100.f);
	ImGui::DragFloat(
		"Joint Axis",
		&m_fPreviewJointAxisLength,
		0.01f,
		0.02f,
		2.f);

	ImGui::TextUnformatted("Gizmo");
	ImGui::SameLine();
	if (ImGui::RadioButton(
		"Move",
		m_eGizmoOperation ==
			ImGuizmo::TRANSLATE))
	{
		m_eGizmoOperation =
			ImGuizmo::TRANSLATE;
	}
	ImGui::SameLine();
	if (ImGui::RadioButton(
		"Rotate",
		m_eGizmoOperation ==
			ImGuizmo::ROTATE))
	{
		m_eGizmoOperation =
			ImGuizmo::ROTATE;
	}
	ImGui::SameLine();
	if (ImGui::RadioButton(
		"Scale (Shape only)",
		m_eGizmoOperation ==
			ImGuizmo::SCALE))
	{
		m_eGizmoOperation =
			ImGuizmo::SCALE;
	}

	ImGui::Separator();
	ImGui::TextUnformatted(
		"Physics Test");
	ImGui::SetNextItemWidth(220.f);
	ImGui::DragFloat3(
		"Linear Velocity",
		&m_vPhysicsTestLinearVelocity.x,
		0.1f);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(220.f);
	ImGui::DragFloat3(
		"Angular Velocity (Deg/s)",
		&m_vPhysicsTestAngularVelocityDegrees.x,
		1.f);
	ImGui::SameLine();
	if (!m_bPhysicsPreviewActive)
	{
		if (ImGui::Button(
			"Start Physics Test",
			ImVec2(150.f, 0.f)))
		{
			const _bool bSuccess =
				StartPhysicsPreview();
			QueueResultPopup(
				m_sStatus,
				bSuccess);
		}
	}
	else if (ImGui::Button(
		"Stop Physics Test",
		ImVec2(150.f, 0.f)))
	{
		StopPhysicsPreview();
	}
}

void CRagdollEditorGUI::DrawPreview()
{
	if (!m_bPreviewVisible ||
		!m_pSelectedModel ||
		!m_Authoring.IsReady())
	{
		return;
	}

	auto* pDebug =
		CGameInstance::Get().GetDbgLineRender();
	if (!pDebug)
		return;

	const _float4 vPreviousColor =
		pDebug->GetColor();
	const auto ePreviousDepth =
		pDebug->GetDepthMode();
	pDebug->SetDepthTest(
		m_bPreviewDepthTest);

	const _matrix PreviewWorld =
		XMMatrixTranslation(
			m_vPreviewPosition.x,
			m_vPreviewPosition.y,
			m_vPreviewPosition.z);
	if (m_pPreviewRenderer)
	{
		m_pPreviewRenderer->SetWorldMatrix(
			PreviewWorld);
	}
	const auto& Bones =
		m_pSelectedModel->GetBones();
	const std::vector<_float4x4>*
		pPreviewPose{};
	if (m_bPhysicsPreviewActive &&
		m_pPreviewRenderer)
	{
		pPreviewPose =
			m_pPreviewRenderer->
				GetCombinedBoneMatrices();
	}

	if (m_bPreviewSkeleton)
	{
		for (size_t iBone = 0;
			iBone < Bones.size();
			++iBone)
		{
			if (!Bones[iBone])
				continue;

			_float4x4 BindPose{};
			if (pPreviewPose &&
				iBone < pPreviewPose->size())
			{
				BindPose =
					(*pPreviewPose)[iBone];
			}
			else if (!m_Authoring.
				GetBoneBindPose(
					iBone,
					BindPose))
			{
				continue;
			}

			const _matrix BoneWorld =
				XMLoadFloat4x4(
					&BindPose) *
				PreviewWorld;
			const _float3 vBonePosition =
				GetMatrixPosition(BoneWorld);
			const _bool bSelected =
				m_eSelection ==
					SELECTION_TYPE::BONE &&
				m_iSelectedBone ==
					static_cast<int32_t>(
						iBone);
			pDebug->SetColor(
				bSelected
					? _float4{
						1.f, 0.9f, 0.1f, 1.f }
					: _float4{
						0.65f, 0.65f, 0.65f, 1.f });

			const int32_t iParent =
				Bones[iBone]->
					GetParendBoneIndex();
			if (iParent >= 0)
			{
				_float4x4 ParentBindPose{};
				const size_t iParentIndex =
					static_cast<size_t>(
						iParent);
				_bool bParentReady{};
				if (pPreviewPose &&
					iParentIndex <
						pPreviewPose->size())
				{
					ParentBindPose =
						(*pPreviewPose)[
							iParentIndex];
					bParentReady = true;
				}
				else
				{
					bParentReady =
						m_Authoring.
							GetBoneBindPose(
								iParentIndex,
								ParentBindPose);
				}
				if (bParentReady)
				{
					pDebug->AddLine(
						GetMatrixPosition(
							XMLoadFloat4x4(
								&ParentBindPose) *
							PreviewWorld),
						vBonePosition);
				}
			}

			if (bSelected)
			{
				pDebug->AddCross(
					vBonePosition,
					0.08f);
			}
		}
	}

	std::vector<_float4x4>
		BodyWorldMatrices(
			m_Ragdoll.Bodies.size());
	std::vector<_bool> BodyMatrixValid(
		m_Ragdoll.Bodies.size(),
		false);
	for (size_t iBody = 0;
		iBody < m_Ragdoll.Bodies.size();
		++iBody)
	{
		const auto& tBody =
			m_Ragdoll.Bodies[iBody];
		if (m_bPhysicsPreviewActive &&
			m_pPhysicsPreviewOwner &&
			m_pPhysicsPreviewOwner->
				GetBodyWorldMatrix(
					iBody,
					BodyWorldMatrices[
						iBody]))
		{
			BodyMatrixValid[iBody] = true;
		}
		else
		{
		_float4x4 BoneBindPose{};
		if (!m_Authoring.GetRigidBindPose(
			tBody.sBoneName.c_str(),
			BoneBindPose))
		{
			continue;
		}

		const _matrix BodyWorld =
			MakePoseMatrix(
				tBody.vBoneToActorPosition,
				tBody.vBoneToActorRotation) *
			XMLoadFloat4x4(
				&BoneBindPose) *
			PreviewWorld;
		XMStoreFloat4x4(
			&BodyWorldMatrices[iBody],
			BodyWorld);
		BodyMatrixValid[iBody] = true;
		}

		if (!m_bPreviewBodies)
			continue;

		const _matrix BodyWorld =
			XMLoadFloat4x4(
				&BodyWorldMatrices[iBody]);

		const _bool bBodySelected =
			(m_eSelection ==
				SELECTION_TYPE::BODY &&
			 m_iSelectedBody ==
				static_cast<int32_t>(
					iBody));
		for (size_t iShape = 0;
			iShape < tBody.Shapes.size();
			++iShape)
		{
			const auto& tShape =
				tBody.Shapes[iShape];
			const _bool bShapeSelected =
				m_eSelection ==
					SELECTION_TYPE::SHAPE &&
				m_iSelectedBody ==
					static_cast<int32_t>(
						iBody) &&
				m_iSelectedShape ==
					static_cast<int32_t>(
						iShape);
			pDebug->SetColor(
				(bBodySelected ||
					bShapeSelected)
					? _float4{
						1.f, 0.9f, 0.1f, 1.f }
					: _float4{
						0.f, 0.85f, 1.f, 1.f });

			const _matrix ShapeWorld =
				MakePoseMatrix(
					tShape.vLocalPosition,
					tShape.vLocalRotation) *
				BodyWorld;
			switch (tShape.eType)
			{
			case PX_RAGDOLL_SHAPE_TYPE::BOX:
				pDebug->AddBox(
					tShape.vHalfExtents,
					ShapeWorld);
				break;

			case PX_RAGDOLL_SHAPE_TYPE::SPHERE:
				pDebug->AddSphere(
					tShape.fRadius,
					ShapeWorld);
				break;

			case PX_RAGDOLL_SHAPE_TYPE::CAPSULE:
				pDebug->AddCapsule(
					tShape.fRadius,
					tShape.fHalfHeight,
					ShapeWorld);
				break;
			}
		}
	}

	if (m_bPreviewJoints)
	{
		auto FindBodyIndex =
			[this](std::string_view sBodyName)
			-> size_t
			{
				const auto Iter =
					std::ranges::find_if(
						m_Ragdoll.Bodies,
						[sBodyName](
							const auto& tBody)
						{
							return tBody.sBodyName ==
								sBodyName;
						});
				return Iter ==
					m_Ragdoll.Bodies.end()
					? std::numeric_limits<
						size_t>::max()
					: static_cast<size_t>(
						std::distance(
							m_Ragdoll.Bodies.begin(),
							Iter));
			};

		for (size_t iJoint = 0;
			iJoint < m_Ragdoll.Joints.size();
			++iJoint)
		{
			const auto& tJoint =
				m_Ragdoll.Joints[iJoint];
			const size_t iParent =
				FindBodyIndex(
					tJoint.sParentBodyName);
			const size_t iChild =
				FindBodyIndex(
					tJoint.sChildBodyName);
			if (iParent >=
					BodyWorldMatrices.size() ||
				iChild >=
					BodyWorldMatrices.size() ||
				!BodyMatrixValid[iParent] ||
				!BodyMatrixValid[iChild])
			{
				continue;
			}

			const _matrix ParentBodyWorld =
				XMLoadFloat4x4(
					&BodyWorldMatrices[iParent]);
			const _matrix ChildBodyWorld =
				XMLoadFloat4x4(
					&BodyWorldMatrices[iChild]);
			const _matrix ParentJointWorld =
				MakePoseMatrix(
					tJoint.vParentLocalPosition,
					tJoint.vParentLocalRotation) *
				ParentBodyWorld;
			const _matrix ChildJointWorld =
				MakePoseMatrix(
					tJoint.vChildLocalPosition,
					tJoint.vChildLocalRotation) *
				ChildBodyWorld;
			const _float3 vParentPosition =
				GetMatrixPosition(
					ParentJointWorld);
			const _float3 vChildPosition =
				GetMatrixPosition(
					ChildJointWorld);
			const _bool bSelected =
				m_eSelection ==
					SELECTION_TYPE::JOINT &&
				m_iSelectedJoint ==
					static_cast<int32_t>(
						iJoint);
			pDebug->SetColor(
				bSelected
					? _float4{
						1.f, 0.9f, 0.1f, 1.f }
					: _float4{
						1.f, 0.25f, 0.25f, 1.f });
			pDebug->AddLine(
				vParentPosition,
				vChildPosition);
			pDebug->AddLine(
				GetMatrixPosition(
					ParentBodyWorld),
				vParentPosition);
			pDebug->AddLine(
				GetMatrixPosition(
					ChildBodyWorld),
				vChildPosition);
			pDebug->AddCross(
				vParentPosition,
				bSelected ? 0.1f : 0.06f);
			pDebug->AddAxis(
				m_fPreviewJointAxisLength,
				ParentJointWorld);

			if (bSelected)
			{
				const _float fLimitRadius =
					m_fPreviewJointAxisLength *
					0.8f;
				const auto ToWorldPosition =
					[&ParentJointWorld](
						_vector vLocal)
					{
						_float3 vWorld{};
						XMStoreFloat3(
							&vWorld,
							XMVector3TransformCoord(
								vLocal,
								ParentJointWorld));
						return vWorld;
					};

				if (tJoint.eTwistMotion ==
					PX_RAGDOLL_D6_MOTION::
						LIMITED)
				{
					constexpr uint32_t
						TWIST_SEGMENTS = 24;
					const _float fLower =
						XMConvertToRadians(
							tJoint.
								fTwistLowerDegrees);
					const _float fUpper =
						XMConvertToRadians(
							tJoint.
								fTwistUpperDegrees);
					pDebug->SetColor({
						1.f, 0.75f, 0.1f, 1.f
						});
					_float3 vPrevious =
						ToWorldPosition(
							XMVectorSet(
								0.f,
								std::cos(fLower) *
									fLimitRadius,
								std::sin(fLower) *
									fLimitRadius,
								1.f));
					for (uint32_t i = 1;
						i <= TWIST_SEGMENTS;
						++i)
					{
						const _float fRatio =
							static_cast<_float>(
								i) /
							static_cast<_float>(
								TWIST_SEGMENTS);
						const _float fAngle =
							fLower +
							(fUpper - fLower) *
								fRatio;
						const _float3 vCurrent =
							ToWorldPosition(
								XMVectorSet(
									0.f,
									std::cos(fAngle) *
										fLimitRadius,
									std::sin(fAngle) *
										fLimitRadius,
									1.f));
						pDebug->AddLine(
							vPrevious,
							vCurrent);
						vPrevious = vCurrent;
					}
				}

				if (tJoint.eSwingYMotion ==
						PX_RAGDOLL_D6_MOTION::
							LIMITED ||
					tJoint.eSwingZMotion ==
						PX_RAGDOLL_D6_MOTION::
							LIMITED)
				{
					constexpr uint32_t
						SWING_SEGMENTS = 32;
					const _float fSwingY =
						XMConvertToRadians(
							tJoint.fSwingYDegrees);
					const _float fSwingZ =
						XMConvertToRadians(
							tJoint.fSwingZDegrees);
					const _float fTanY =
						std::tan(fSwingY);
					const _float fTanZ =
						std::tan(fSwingZ);
					pDebug->SetColor({
						0.85f, 0.2f, 1.f, 1.f
						});
					std::array<_float3,
						SWING_SEGMENTS>
						ConePoints{};
					for (uint32_t i = 0;
						i < SWING_SEGMENTS;
						++i)
					{
						const _float fAngle =
							XM_2PI *
							static_cast<_float>(
								i) /
							static_cast<_float>(
								SWING_SEGMENTS);
						_vector vDirection =
							XMVector3Normalize(
								XMVectorSet(
									1.f,
									fTanZ *
										std::cos(
											fAngle),
									fTanY *
										std::sin(
											fAngle),
									0.f));
						ConePoints[i] =
							ToWorldPosition(
								vDirection *
								m_fPreviewJointAxisLength);
					}
					for (uint32_t i = 0;
						i < SWING_SEGMENTS;
						++i)
					{
						pDebug->AddLine(
							ConePoints[i],
							ConePoints[
								(i + 1) %
								SWING_SEGMENTS]);
						if (i % 8 == 0)
						{
							pDebug->AddLine(
								vParentPosition,
								ConePoints[i]);
						}
					}
				}
			}
		}
	}

	pDebug->SetColor(vPreviousColor);
	pDebug->SetDepthMode(ePreviousDepth);
}

void CRagdollEditorGUI::PlacePreviewAtCamera()
{
	StopPhysicsPreview();
	auto* pCamera =
		CGameInstance::Get().GetActiveCamera();
	if (!pCamera)
	{
		m_sStatus =
			"Cannot place preview: no active camera.";
		return;
	}

	const _matrix InverseView =
		XMMatrixInverse(
			nullptr,
			pCamera->GetView());
	XMStoreFloat3(
		&m_vPreviewPosition,
		InverseView.r[3] +
			XMVector3Normalize(
				InverseView.r[2]) *
			5.f);
	m_sStatus =
		"Preview placed 5 meters in front of the active camera.";
}

_bool CRagdollEditorGUI::
StartPhysicsPreview()
{
	StopPhysicsPreview();

	if (!m_pSelectedModel ||
		!m_pPreviewRenderer ||
		m_PreviewBindPoses.empty())
	{
		m_sStatus =
			"Select a valid skeletal model first.";
		return false;
	}

	if (!Validate(m_ValidationErrors))
	{
		m_sStatus =
			"Physics test failed: validate the ragdoll data first.";
		return false;
	}

	m_pPhysicsPreviewOwner =
		CRagdollPhysicsPreviewOwner::Create(
			m_Ragdoll,
			*m_pSelectedModel,
			m_PreviewBindPoses,
			m_vPreviewPosition);
	if (!m_pPhysicsPreviewOwner)
	{
		m_sStatus =
			"Failed to build the temporary PhysX ragdoll.";
		return false;
	}

	if (!m_pPhysicsPreviewOwner->Activate(
		m_vPhysicsTestLinearVelocity,
		m_vPhysicsTestAngularVelocityDegrees))
	{
		m_pPhysicsPreviewOwner.reset();
		m_sStatus =
			"Failed to activate the temporary PhysX ragdoll.";
		return false;
	}

	m_bPhysicsPreviewActive = true;
	m_sStatus =
		"Physics test started. Stop the test before editing.";
	return true;
}

void CRagdollEditorGUI::
StopPhysicsPreview()
{
	m_bPhysicsPreviewActive = false;
	m_pPhysicsPreviewOwner.reset();
	if (m_pPreviewRenderer &&
		!m_PreviewBindPoses.empty())
	{
		m_pPreviewRenderer->
			SetCombinedBoneMatrices(
				m_PreviewBindPoses);
	}
}

void CRagdollEditorGUI::
UpdatePhysicsPreviewPose()
{
	if (!m_bPhysicsPreviewActive ||
		!m_pPhysicsPreviewOwner ||
		!m_pPreviewRenderer)
	{
		return;
	}

	auto* pCombinedBoneMatrices =
		m_pPreviewRenderer->
			GetCombinedBoneMatrices();
	if (!pCombinedBoneMatrices ||
		!m_pPhysicsPreviewOwner->
			WritePhysicsPoseToBones(
				*pCombinedBoneMatrices))
	{
		StopPhysicsPreview();
		m_sStatus =
			"Physics test stopped because pose synchronization failed.";
	}
}

_bool CRagdollEditorGUI::
BuildBodyPreviewWorldMatrix(
	size_t iBodyIndex,
	_float4x4& OutWorldMatrix) const
{
	if (iBodyIndex >=
			m_Ragdoll.Bodies.size() ||
		!m_Authoring.IsReady())
	{
		return false;
	}

	const auto& tBody =
		m_Ragdoll.Bodies[iBodyIndex];
	_float4x4 BoneBindPose{};
	if (!m_Authoring.GetRigidBindPose(
		tBody.sBoneName.c_str(),
		BoneBindPose))
	{
		return false;
	}

	XMStoreFloat4x4(
		&OutWorldMatrix,
		MakePoseMatrix(
			tBody.vBoneToActorPosition,
			tBody.vBoneToActorRotation) *
		XMLoadFloat4x4(
			&BoneBindPose) *
		XMMatrixTranslation(
			m_vPreviewPosition.x,
			m_vPreviewPosition.y,
			m_vPreviewPosition.z));
	return true;
}

void CRagdollEditorGUI::RenderGizmo()
{
	if (!m_bPreviewVisible ||
		!m_pSelectedModel ||
		m_bPhysicsPreviewActive)
	{
		return;
	}

	auto* pCamera =
		CGameInstance::Get().GetActiveCamera();
	if (!pCamera)
		return;

	_float4x4 GizmoWorld{};
	_float4x4 ParentBodyWorld{};
	_float4x4 ChildBodyWorld{};
	PX_RAGDOLL_SHAPE_DESC* pShape{};
	PX_RAGDOLL_D6_JOINT_DESC* pJoint{};

	if (m_eSelection ==
		SELECTION_TYPE::SHAPE)
	{
		pShape = GetSelectedShape();
		if (!pShape ||
			m_iSelectedBody < 0 ||
			!BuildBodyPreviewWorldMatrix(
				static_cast<size_t>(
					m_iSelectedBody),
				ParentBodyWorld))
		{
			return;
		}

		_float3 vDimensions{};
		switch (pShape->eType)
		{
		case PX_RAGDOLL_SHAPE_TYPE::BOX:
			vDimensions = {
				pShape->vHalfExtents.x * 2.f,
				pShape->vHalfExtents.y * 2.f,
				pShape->vHalfExtents.z * 2.f
			};
			break;

		case PX_RAGDOLL_SHAPE_TYPE::SPHERE:
			vDimensions = {
				pShape->fRadius * 2.f,
				pShape->fRadius * 2.f,
				pShape->fRadius * 2.f
			};
			break;

		case PX_RAGDOLL_SHAPE_TYPE::CAPSULE:
			vDimensions = {
				pShape->fRadius * 2.f,
				(pShape->fHalfHeight +
					pShape->fRadius) * 2.f,
				pShape->fRadius * 2.f
			};
			break;
		}

		XMStoreFloat4x4(
			&GizmoWorld,
			XMMatrixScaling(
				vDimensions.x,
				vDimensions.y,
				vDimensions.z) *
			MakePoseMatrix(
				pShape->vLocalPosition,
				pShape->vLocalRotation) *
			XMLoadFloat4x4(
				&ParentBodyWorld));
	}
	else if (m_eSelection ==
		SELECTION_TYPE::JOINT)
	{
		pJoint = GetSelectedJoint();
		if (!pJoint ||
			m_eGizmoOperation ==
				ImGuizmo::SCALE)
		{
			return;
		}

		const auto ParentIter =
			std::ranges::find_if(
				m_Ragdoll.Bodies,
				[pJoint](const auto& tBody)
				{
					return tBody.sBodyName ==
						pJoint->
							sParentBodyName;
				});
		const auto ChildIter =
			std::ranges::find_if(
				m_Ragdoll.Bodies,
				[pJoint](const auto& tBody)
				{
					return tBody.sBodyName ==
						pJoint->
							sChildBodyName;
				});
		if (ParentIter ==
				m_Ragdoll.Bodies.end() ||
			ChildIter ==
				m_Ragdoll.Bodies.end())
		{
			return;
		}

		const size_t iParent =
			static_cast<size_t>(
				std::distance(
					m_Ragdoll.Bodies.begin(),
					ParentIter));
		const size_t iChild =
			static_cast<size_t>(
				std::distance(
					m_Ragdoll.Bodies.begin(),
					ChildIter));
		if (!BuildBodyPreviewWorldMatrix(
				iParent,
				ParentBodyWorld) ||
			!BuildBodyPreviewWorldMatrix(
				iChild,
				ChildBodyWorld))
		{
			return;
		}

		XMStoreFloat4x4(
			&GizmoWorld,
			MakePoseMatrix(
				pJoint->
					vParentLocalPosition,
				pJoint->
					vParentLocalRotation) *
			XMLoadFloat4x4(
				&ParentBodyWorld));
	}
	else
	{
		return;
	}

	_float4x4 View{};
	_float4x4 Projection{};
	XMStoreFloat4x4(
		&View,
		pCamera->GetView());
	XMStoreFloat4x4(
		&Projection,
		pCamera->GetProj());

	ImGuiViewport* pViewport =
		ImGui::GetMainViewport();
	if (!pViewport)
		return;

	ImGuizmo::SetOrthographic(false);
	ImGuizmo::SetDrawlist(
		ImGui::GetForegroundDrawList(
			pViewport));
	ImGuizmo::SetRect(
		pViewport->Pos.x,
		pViewport->Pos.y,
		pViewport->Size.x,
		pViewport->Size.y);
	ImGuizmo::SetID(
		RAGDOLL_EDITOR_GIZMO_ID);

	if (!ImGuizmo::Manipulate(
		&View._11,
		&Projection._11,
		m_eGizmoOperation,
		ImGuizmo::LOCAL,
		&GizmoWorld._11))
	{
		return;
	}

	if (pShape)
	{
		const _matrix ShapeLocal =
			XMLoadFloat4x4(
				&GizmoWorld) *
			XMMatrixInverse(
				nullptr,
				XMLoadFloat4x4(
					&ParentBodyWorld));
		_vector vScale{};
		_vector vRotation{};
		_vector vTranslation{};
		if (!XMMatrixDecompose(
			&vScale,
			&vRotation,
			&vTranslation,
			ShapeLocal))
		{
			return;
		}

		XMStoreFloat3(
			&pShape->vLocalPosition,
			vTranslation);
		XMStoreFloat4(
			&pShape->vLocalRotation,
			XMQuaternionNormalize(
				vRotation));
		_float3 vDimensions{};
		XMStoreFloat3(
			&vDimensions,
			vScale);
		vDimensions = {
			std::max(
				std::abs(vDimensions.x),
				MIN_RAGDOLL_SHAPE_SIZE),
			std::max(
				std::abs(vDimensions.y),
				MIN_RAGDOLL_SHAPE_SIZE),
			std::max(
				std::abs(vDimensions.z),
				MIN_RAGDOLL_SHAPE_SIZE)
		};

		switch (pShape->eType)
		{
		case PX_RAGDOLL_SHAPE_TYPE::BOX:
			pShape->vHalfExtents = {
				vDimensions.x * 0.5f,
				vDimensions.y * 0.5f,
				vDimensions.z * 0.5f
			};
			break;

		case PX_RAGDOLL_SHAPE_TYPE::SPHERE:
			pShape->fRadius =
				std::max({
					vDimensions.x,
					vDimensions.y,
					vDimensions.z
					}) * 0.5f;
			break;

		case PX_RAGDOLL_SHAPE_TYPE::CAPSULE:
			pShape->fRadius =
				std::max(
					(vDimensions.x +
						vDimensions.z) *
						0.25f,
					MIN_RAGDOLL_SHAPE_SIZE);
			pShape->fHalfHeight =
				std::max(
					vDimensions.y * 0.5f -
						pShape->fRadius,
					MIN_RAGDOLL_SHAPE_SIZE);
			break;
		}
	}
	else if (pJoint)
	{
		const _matrix JointWorld =
			XMLoadFloat4x4(
				&GizmoWorld);
		const auto ApplyJointLocal =
			[&JointWorld](
				FXMMATRIX BodyWorld,
				_float3& vPosition,
				_float4& vRotation)
			{
				_vector vScale{};
				_vector vLocalRotation{};
				_vector vLocalPosition{};
				if (!XMMatrixDecompose(
					&vScale,
					&vLocalRotation,
					&vLocalPosition,
					JointWorld *
						XMMatrixInverse(
							nullptr,
							BodyWorld)))
				{
					return false;
				}
				XMStoreFloat3(
					&vPosition,
					vLocalPosition);
				XMStoreFloat4(
					&vRotation,
					XMQuaternionNormalize(
						vLocalRotation));
				return true;
			};

		if (!ApplyJointLocal(
				XMLoadFloat4x4(
					&ParentBodyWorld),
				pJoint->
					vParentLocalPosition,
				pJoint->
					vParentLocalRotation) ||
			!ApplyJointLocal(
				XMLoadFloat4x4(
					&ChildBodyWorld),
				pJoint->
					vChildLocalPosition,
				pJoint->
					vChildLocalRotation))
		{
			return;
		}
	}

	m_bDirty = true;
}

void CRagdollEditorGUI::DrawModelSelector()
{
	ImGui::TextUnformatted("Model Resource");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(-1.f);
	if (ImGui::BeginCombo(
		"##RagdollModel",
		m_sSelectedModelLabel.c_str()))
	{
		const auto Entries =
			CollectModelEntries();
		for (const auto& Entry : Entries)
		{
			const _bool bSelected =
				Entry.pModel ==
				m_pSelectedModel;
			if (ImGui::Selectable(
				Entry.sLabel.c_str(),
				bSelected))
			{
				m_sSelectedModelLabel =
					Entry.sLabel;
				StopPhysicsPreview();
				m_pPreviewRenderer.reset();
				m_PreviewBindPoses.clear();
				SelectModel(Entry.pModel);
				if (m_pSelectedModel)
				{
					m_PreviewBindPoses.resize(
						m_pSelectedModel->
							GetBones().size());
					_bool bBindPosesReady = true;
					for (size_t iBone = 0;
						iBone <
							m_PreviewBindPoses.size();
						++iBone)
					{
						if (!m_Authoring.
							GetBoneBindPose(
								iBone,
								m_PreviewBindPoses[
									iBone]))
						{
							bBindPosesReady =
								false;
							break;
						}
					}

					m_pPreviewRenderer =
						bBindPosesReady
						? CRagdollPreviewRenderer::
							Create(
								Entry.sGroupTag,
								Entry.sResourceTag,
								m_PreviewBindPoses)
						: nullptr;
					if (!m_pPreviewRenderer)
					{
						m_PreviewBindPoses.clear();
						m_sStatus =
							"Model selected, but preview renderer initialization failed.";
					}
				}
			}
			if (bSelected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
}

void CRagdollEditorGUI::DrawHierarchyPanel()
{
	ImGui::BeginChild(
		"RagdollHierarchy",
		ImVec2(0.f, 490.f),
		false);
	if (ImGui::BeginTabBar(
		"RagdollHierarchyTabs"))
	{
		if (ImGui::BeginTabItem("Skeleton"))
		{
			DrawSkeletonHierarchy();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Bodies"))
		{
			DrawBodyHierarchy();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Joints"))
		{
			DrawJointHierarchy();
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
	ImGui::EndChild();
}

void CRagdollEditorGUI::
DrawSkeletonHierarchy()
{
	if (!m_pSelectedModel)
	{
		ImGui::TextDisabled(
			"Select a model first.");
		return;
	}

	const auto& Bones =
		m_pSelectedModel->GetBones();
	if (Bones.empty())
	{
		ImGui::TextDisabled(
			"The selected model has no bones.");
		return;
	}

	auto HasBodyForBone =
		[&](std::string_view sBoneName)
		{
			return std::ranges::any_of(
				m_Ragdoll.Bodies,
				[sBoneName](
					const PX_RAGDOLL_BODY_DESC&
						tBody)
				{
					return tBody.sBoneName ==
						sBoneName;
				});
		};

	auto DrawBone =
		[&](auto&& Self,
			size_t iBone) -> void
		{
			if (iBone >= Bones.size() ||
				!Bones[iBone])
			{
				return;
			}

			const std::string sBoneName =
				Bones[iBone]->GetBoneName();
			_bool bHasChildren{};
			for (const auto& pBone : Bones)
			{
				if (pBone &&
					pBone->
						GetParendBoneIndex() ==
						static_cast<int32_t>(
							iBone))
				{
					bHasChildren = true;
					break;
				}
			}

			ImGuiTreeNodeFlags Flags =
				ImGuiTreeNodeFlags_OpenOnArrow |
				ImGuiTreeNodeFlags_SpanAvailWidth;
			if (!bHasChildren)
				Flags |=
					ImGuiTreeNodeFlags_Leaf |
					ImGuiTreeNodeFlags_NoTreePushOnOpen;
			if (m_eSelection ==
					SELECTION_TYPE::BONE &&
				m_iSelectedBone ==
					static_cast<int32_t>(
						iBone))
			{
				Flags |=
					ImGuiTreeNodeFlags_Selected;
			}

			const _bool bOpen =
				ImGui::TreeNodeEx(
					reinterpret_cast<void*>(
						static_cast<uintptr_t>(
							iBone + 1)),
					Flags,
					"%s%s",
					HasBodyForBone(sBoneName)
						? "[Body] "
						: "",
					sBoneName.c_str());
			if (ImGui::IsItemClicked())
			{
				m_eSelection =
					SELECTION_TYPE::BONE;
				m_iSelectedBone =
					static_cast<int32_t>(
						iBone);
			}

			if (bOpen && bHasChildren)
			{
				for (size_t iChild = 0;
					iChild < Bones.size();
					++iChild)
				{
					if (Bones[iChild] &&
						Bones[iChild]->
							GetParendBoneIndex() ==
							static_cast<int32_t>(
								iBone))
					{
						Self(Self, iChild);
					}
				}
				ImGui::TreePop();
			}
		};

	for (size_t iBone = 0;
		iBone < Bones.size();
		++iBone)
	{
		if (Bones[iBone] &&
			Bones[iBone]->
				GetParendBoneIndex() < 0)
		{
			DrawBone(DrawBone, iBone);
		}
	}

	ImGui::Separator();
	if (ImGui::Button(
		"Add Body For Selected Bone",
		ImVec2(-1.f, 0.f)))
	{
		AddBodyForSelectedBone();
	}
}

void CRagdollEditorGUI::DrawBodyHierarchy()
{
	for (size_t iBody = 0;
		iBody < m_Ragdoll.Bodies.size();
		++iBody)
	{
		auto& tBody =
			m_Ragdoll.Bodies[iBody];
		const _bool bBodySelected =
			m_eSelection ==
				SELECTION_TYPE::BODY &&
			m_iSelectedBody ==
				static_cast<int32_t>(iBody);
		ImGuiTreeNodeFlags Flags =
			ImGuiTreeNodeFlags_OpenOnArrow |
			ImGuiTreeNodeFlags_SpanAvailWidth;
		if (bBodySelected)
			Flags |=
				ImGuiTreeNodeFlags_Selected;

		const _bool bOpen =
			ImGui::TreeNodeEx(
				reinterpret_cast<void*>(
					static_cast<uintptr_t>(
						0x10000 + iBody)),
				Flags,
				"%s -> %s",
				tBody.sBodyName.c_str(),
				tBody.sBoneName.c_str());
		if (ImGui::IsItemClicked())
		{
			m_eSelection =
				SELECTION_TYPE::BODY;
			m_iSelectedBody =
				static_cast<int32_t>(
					iBody);
			m_iSelectedShape = -1;
		}

		if (bOpen)
		{
			for (size_t iShape = 0;
				iShape < tBody.Shapes.size();
				++iShape)
			{
				const _bool bShapeSelected =
					m_eSelection ==
						SELECTION_TYPE::SHAPE &&
					m_iSelectedBody ==
						static_cast<int32_t>(
							iBody) &&
					m_iSelectedShape ==
						static_cast<int32_t>(
							iShape);
				if (ImGui::Selectable(
					tBody.Shapes[iShape].
						sName.c_str(),
					bShapeSelected))
				{
					m_eSelection =
						SELECTION_TYPE::SHAPE;
					m_iSelectedBody =
						static_cast<int32_t>(
							iBody);
					m_iSelectedShape =
						static_cast<int32_t>(
							iShape);
				}
			}
			ImGui::TreePop();
		}
	}

	ImGui::Separator();
	if (ImGui::Button(
		"Remove Selected Body",
		ImVec2(-1.f, 0.f)))
	{
		RemoveSelectedBody();
	}
}

void CRagdollEditorGUI::DrawJointHierarchy()
{
	for (size_t iJoint = 0;
		iJoint < m_Ragdoll.Joints.size();
		++iJoint)
	{
		const auto& tJoint =
			m_Ragdoll.Joints[iJoint];
		const _bool bSelected =
			m_eSelection ==
				SELECTION_TYPE::JOINT &&
			m_iSelectedJoint ==
				static_cast<int32_t>(
					iJoint);
		std::string sLabel =
			tJoint.sJointName +
			" | " +
			tJoint.sParentBodyName +
			" -> " +
			tJoint.sChildBodyName;
		if (ImGui::Selectable(
			sLabel.c_str(),
			bSelected))
		{
			m_eSelection =
				SELECTION_TYPE::JOINT;
			m_iSelectedJoint =
				static_cast<int32_t>(
					iJoint);
		}
	}

	ImGui::Separator();
	DrawJointCreationPanel();
	if (ImGui::Button(
		"Remove Selected Joint",
		ImVec2(-1.f, 0.f)))
	{
		RemoveSelectedJoint();
	}
}

void CRagdollEditorGUI::DrawJointCreationPanel()
{
	if (m_Ragdoll.Bodies.size() < 2)
	{
		ImGui::TextDisabled(
			"Create at least two bodies.");
		return;
	}

	m_iNewJointParentBody =
		std::clamp(
			m_iNewJointParentBody,
			0,
			static_cast<int32_t>(
				m_Ragdoll.Bodies.size() - 1));
	m_iNewJointChildBody =
		std::clamp(
			m_iNewJointChildBody,
			0,
			static_cast<int32_t>(
				m_Ragdoll.Bodies.size() - 1));

	auto DrawBodyCombo =
		[this](const char* pLabel,
			int32_t& iBody)
		{
			const char* pPreview =
				m_Ragdoll.Bodies[iBody].
					sBodyName.c_str();
			if (ImGui::BeginCombo(
				pLabel,
				pPreview))
			{
				for (size_t i = 0;
					i < m_Ragdoll.Bodies.size();
					++i)
				{
					const _bool bSelected =
						iBody ==
							static_cast<int32_t>(
								i);
					if (ImGui::Selectable(
						m_Ragdoll.Bodies[i].
							sBodyName.c_str(),
						bSelected))
					{
						iBody =
							static_cast<int32_t>(
								i);
					}
				}
				ImGui::EndCombo();
			}
		};

	DrawBodyCombo(
		"Parent Body",
		m_iNewJointParentBody);
	DrawBodyCombo(
		"Child Body",
		m_iNewJointChildBody);
	if (ImGui::Button(
		"Create Bind-Pose D6 Joint",
		ImVec2(-1.f, 0.f)))
	{
		AddBindPoseJoint();
	}
}

void CRagdollEditorGUI::DrawInspectorPanel()
{
	ImGui::BeginChild(
		"RagdollInspector",
		ImVec2(0.f, 490.f),
		false);

	switch (m_eSelection)
	{
	case SELECTION_TYPE::BONE:
		if (m_pSelectedModel &&
			m_iSelectedBone >= 0 &&
			static_cast<size_t>(
				m_iSelectedBone) <
				m_pSelectedModel->
					GetBones().size())
		{
			const auto& pBone =
				m_pSelectedModel->
					GetBones()[
						m_iSelectedBone];
			ImGui::TextUnformatted(
				"Selected Bone");
			ImGui::Separator();
			ImGui::Text(
				"Name: %s",
				pBone->GetBoneName().c_str());
			ImGui::Text(
				"Parent Index: %d",
				pBone->
					GetParendBoneIndex());
			ImGui::Text(
				"Depth: %u",
				pBone->Get_Depth());
			if (ImGui::Button(
				"Add Ragdoll Body",
				ImVec2(-1.f, 0.f)))
			{
				AddBodyForSelectedBone();
			}
		}
		break;

	case SELECTION_TYPE::BODY:
		DrawBodyInspector();
		break;

	case SELECTION_TYPE::SHAPE:
		DrawShapeInspector();
		break;

	case SELECTION_TYPE::JOINT:
		DrawJointInspector();
		break;

	case SELECTION_TYPE::NONE:
	default:
		ImGui::TextDisabled(
			"Select a bone, body, shape, or joint.");
		break;
	}

	ImGui::EndChild();
}

void CRagdollEditorGUI::DrawBodyInspector()
{
	auto* pBody = GetSelectedBody();
	if (!pBody)
		return;

	ImGui::TextUnformatted("Body");
	ImGui::Separator();
	ImGui::Text(
		"Name: %s",
		pBody->sBodyName.c_str());
	ImGui::Text(
		"Bone: %s",
		pBody->sBoneName.c_str());
	m_bDirty |= ImGui::DragFloat(
		"Mass",
		&pBody->fMass,
		0.1f,
		0.001f,
		1000.f,
		"%.3f");
	m_bDirty |= ImGui::DragFloat(
		"Linear Damping",
		&pBody->fLinearDamping,
		0.01f,
		0.f,
		100.f,
		"%.3f");
	m_bDirty |= ImGui::DragFloat(
		"Angular Damping",
		&pBody->fAngularDamping,
		0.01f,
		0.f,
		100.f,
		"%.3f");
	m_bDirty |= ImGui::DragFloat(
		"Max Depenetration Velocity",
		&pBody->fMaxDepenetrationVelocity,
		0.1f,
		0.f,
		1000.f,
		"%.3f");
	m_bDirty |= ImGui::Checkbox(
		"Gravity",
		&pBody->bGravityEnabled);
	m_bDirty |= ImGui::DragFloat3(
		"Bone To Actor Position",
		reinterpret_cast<float*>(
			&pBody->vBoneToActorPosition),
		0.01f);
	m_bDirty |= ImGui::DragFloat4(
		"Bone To Actor Quaternion",
		reinterpret_cast<float*>(
			&pBody->vBoneToActorRotation),
		0.01f,
		-1.f,
		1.f);

	ImGui::Separator();
	if (ImGui::Button(
		"Add Shape",
		ImVec2(120.f, 0.f)))
	{
		AddShapeToSelectedBody();
	}
	ImGui::SameLine();
	if (ImGui::Button(
		"Remove Body",
		ImVec2(120.f, 0.f)))
	{
		RemoveSelectedBody();
	}
}

void CRagdollEditorGUI::DrawShapeInspector()
{
	auto* pShape = GetSelectedShape();
	if (!pShape)
		return;

	ImGui::TextUnformatted("Shape");
	ImGui::Separator();
	ImGui::Text(
		"Name: %s",
		pShape->sName.c_str());

	int32_t iType =
		static_cast<int32_t>(
			pShape->eType);
	const char* pTypeNames[] = {
		"Box",
		"Sphere",
		"Capsule"
	};
	if (ImGui::Combo(
		"Type",
		&iType,
		pTypeNames,
		static_cast<int32_t>(
			std::size(pTypeNames))))
	{
		pShape->eType =
			static_cast<
				PX_RAGDOLL_SHAPE_TYPE>(
					iType);
		m_bDirty = true;
	}

	m_bDirty |= ImGui::DragFloat3(
		"Local Position",
		reinterpret_cast<float*>(
			&pShape->vLocalPosition),
		0.01f);
	m_bDirty |= ImGui::DragFloat4(
		"Local Quaternion",
		reinterpret_cast<float*>(
			&pShape->vLocalRotation),
		0.01f,
		-1.f,
		1.f);

	switch (pShape->eType)
	{
	case PX_RAGDOLL_SHAPE_TYPE::BOX:
		m_bDirty |= ImGui::DragFloat3(
			"Half Extents",
			reinterpret_cast<float*>(
				&pShape->vHalfExtents),
			0.01f,
			0.001f,
			1000.f);
		break;

	case PX_RAGDOLL_SHAPE_TYPE::SPHERE:
		m_bDirty |= ImGui::DragFloat(
			"Radius",
			&pShape->fRadius,
			0.01f,
			0.001f,
			1000.f);
		break;

	case PX_RAGDOLL_SHAPE_TYPE::CAPSULE:
		m_bDirty |= ImGui::DragFloat(
			"Radius",
			&pShape->fRadius,
			0.01f,
			0.001f,
			1000.f);
		m_bDirty |= ImGui::DragFloat(
			"Half Height",
			&pShape->fHalfHeight,
			0.01f,
			0.f,
			1000.f);
		break;
	}

	DrawLayerSelector(
		"Layer",
		pShape->iLayer);
	DrawLayerMaskSelector(
		"Simulation Mask",
		pShape->iSimulationMask);
	DrawLayerMaskSelector(
		"Query Mask",
		pShape->iQueryMask);
	m_bDirty |= ImGui::Checkbox(
		"Simulation Enabled",
		&pShape->bSimulationEnabled);
	m_bDirty |= ImGui::Checkbox(
		"Query Enabled",
		&pShape->bQueryEnabled);

	ImGui::Separator();
	if (ImGui::Button(
		"Remove Shape",
		ImVec2(140.f, 0.f)))
	{
		RemoveSelectedShape();
	}
}

void CRagdollEditorGUI::DrawJointInspector()
{
	auto* pJoint = GetSelectedJoint();
	if (!pJoint)
		return;

	ImGui::TextUnformatted("D6 Joint");
	ImGui::Separator();
	ImGui::Text(
		"Name: %s",
		pJoint->sJointName.c_str());
	ImGui::Text(
		"Parent: %s",
		pJoint->
			sParentBodyName.c_str());
	ImGui::Text(
		"Child: %s",
		pJoint->
			sChildBodyName.c_str());

	m_bDirty |= ImGui::DragFloat3(
		"Parent Local Position",
		reinterpret_cast<float*>(
			&pJoint->
				vParentLocalPosition),
		0.01f);
	m_bDirty |= ImGui::DragFloat4(
		"Parent Local Quaternion",
		reinterpret_cast<float*>(
			&pJoint->
				vParentLocalRotation),
		0.01f,
		-1.f,
		1.f);
	m_bDirty |= ImGui::DragFloat3(
		"Child Local Position",
		reinterpret_cast<float*>(
			&pJoint->
				vChildLocalPosition),
		0.01f);
	m_bDirty |= ImGui::DragFloat4(
		"Child Local Quaternion",
		reinterpret_cast<float*>(
			&pJoint->
				vChildLocalRotation),
		0.01f,
		-1.f,
		1.f);

	const char* pMotionNames[] = {
		"Locked",
		"Limited",
		"Free"
	};
	int32_t iTwistMotion =
		static_cast<int32_t>(
			pJoint->eTwistMotion);
	int32_t iSwingYMotion =
		static_cast<int32_t>(
			pJoint->eSwingYMotion);
	int32_t iSwingZMotion =
		static_cast<int32_t>(
			pJoint->eSwingZMotion);
	if (ImGui::Combo(
		"Twist Motion",
		&iTwistMotion,
		pMotionNames,
		3))
	{
		pJoint->eTwistMotion =
			static_cast<
				PX_RAGDOLL_D6_MOTION>(
					iTwistMotion);
		m_bDirty = true;
	}
	if (ImGui::Combo(
		"Swing Y Motion",
		&iSwingYMotion,
		pMotionNames,
		3))
	{
		pJoint->eSwingYMotion =
			static_cast<
				PX_RAGDOLL_D6_MOTION>(
					iSwingYMotion);
		m_bDirty = true;
	}
	if (ImGui::Combo(
		"Swing Z Motion",
		&iSwingZMotion,
		pMotionNames,
		3))
	{
		pJoint->eSwingZMotion =
			static_cast<
				PX_RAGDOLL_D6_MOTION>(
					iSwingZMotion);
		m_bDirty = true;
	}

	m_bDirty |= ImGui::DragFloat(
		"Twist Lower",
		&pJoint->fTwistLowerDegrees,
		1.f,
		-179.9f,
		179.9f,
		"%.1f deg");
	m_bDirty |= ImGui::DragFloat(
		"Twist Upper",
		&pJoint->fTwistUpperDegrees,
		1.f,
		-179.9f,
		179.9f,
		"%.1f deg");
	m_bDirty |= ImGui::DragFloat(
		"Swing Y",
		&pJoint->fSwingYDegrees,
		1.f,
		0.1f,
		179.9f,
		"%.1f deg");
	m_bDirty |= ImGui::DragFloat(
		"Swing Z",
		&pJoint->fSwingZDegrees,
		1.f,
		0.1f,
		179.9f,
		"%.1f deg");
	m_bDirty |= ImGui::DragFloat(
		"Limit Stiffness",
		&pJoint->fLimitStiffness,
		0.1f,
		0.f,
		10000.f);
	m_bDirty |= ImGui::DragFloat(
		"Limit Damping",
		&pJoint->fLimitDamping,
		0.1f,
		0.f,
		10000.f);
	m_bDirty |= ImGui::Checkbox(
		"Connected Collision",
		&pJoint->bCollisionEnabled);
	m_bDirty |= ImGui::Checkbox(
		"Joint Visualization",
		&pJoint->bVisualizationEnabled);
	m_bDirty |= ImGui::Checkbox(
		"Enabled",
		&pJoint->bEnabled);
}

void CRagdollEditorGUI::DrawLayerSelector(
	const char* pLabel,
	uint32_t& iLayer) const
{
	std::string sPreview = "Custom";
	for (const auto& [iValue, sName] :
		m_CollisionLayerNames)
	{
		if (iValue == iLayer)
		{
			sPreview = sName;
			break;
		}
	}

	if (ImGui::BeginCombo(
		pLabel,
		sPreview.c_str()))
	{
		for (const auto& [iValue, sName] :
			m_CollisionLayerNames)
		{
			const _bool bSelected =
				iValue == iLayer;
			if (ImGui::Selectable(
				sName.c_str(),
				bSelected))
			{
				iLayer = iValue;
				const_cast<
					CRagdollEditorGUI*>(this)->
					m_bDirty = true;
			}
		}
		ImGui::EndCombo();
	}
}

void CRagdollEditorGUI::
DrawLayerMaskSelector(
	const char* pLabel,
	uint32_t& iMask) const
{
	std::string sPreview =
		iMask == 0 ? "None" : "Multiple";
	if (HasSingleBit(iMask))
	{
		for (const auto& [iValue, sName] :
			m_CollisionLayerNames)
		{
			if (iValue == iMask)
			{
				sPreview = sName;
				break;
			}
		}
	}

	if (ImGui::BeginCombo(
		pLabel,
		sPreview.c_str()))
	{
		if (ImGui::Selectable(
			"None",
			iMask == 0))
		{
			iMask = 0;
			const_cast<
				CRagdollEditorGUI*>(this)->
				m_bDirty = true;
		}
		for (const auto& [iValue, sName] :
			m_CollisionLayerNames)
		{
			_bool bEnabled =
				(iMask & iValue) != 0;
			if (ImGui::Checkbox(
				sName.c_str(),
				&bEnabled))
			{
				if (bEnabled)
					iMask |= iValue;
				else
					iMask &= ~iValue;
				const_cast<
					CRagdollEditorGUI*>(this)->
					m_bDirty = true;
			}
		}
		ImGui::EndCombo();
	}
}

void CRagdollEditorGUI::DrawFilePopups()
{
	if (ImGui::BeginPopupModal(
		"Confirm Ragdoll Save",
		nullptr,
		ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::TextUnformatted(
			"Save ragdoll data to:");
		ImGui::TextWrapped(
			"%s",
			MakeFilePath().
				generic_string().c_str());
		if (ImGui::Button(
			"Save",
			ImVec2(100.f, 0.f)))
		{
			m_ValidationErrors.clear();
			const _bool bValid =
				Validate(
					m_ValidationErrors);
			const _bool bSuccess =
				bValid &&
				SUCCEEDED(Save());
			QueueResultPopup(
				m_sStatus,
				bSuccess);
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button(
			"Cancel",
			ImVec2(100.f, 0.f)))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	if (ImGui::BeginPopupModal(
		"Confirm Ragdoll Load",
		nullptr,
		ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::TextUnformatted(
			"Load ragdoll data? Unsaved changes will be lost.");
		if (ImGui::Button(
			"Load",
			ImVec2(100.f, 0.f)))
		{
			const _bool bSuccess =
				SUCCEEDED(Load());
			QueueResultPopup(
				m_sStatus,
				bSuccess);
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button(
			"Cancel",
			ImVec2(100.f, 0.f)))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	if (ImGui::BeginPopupModal(
		"Confirm Ragdoll Clear",
		nullptr,
		ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::TextUnformatted(
			"Clear the current ragdoll data?");
		if (ImGui::Button(
			"Clear",
			ImVec2(100.f, 0.f)))
		{
			Clear();
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button(
			"Cancel",
			ImVec2(100.f, 0.f)))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	if (ImGui::BeginPopupModal(
		"Ragdoll Editor Result",
		nullptr,
		ImGuiWindowFlags_AlwaysAutoResize))
	{
		const ImVec4 Color =
			m_bResultPopupSuccess
			? ImVec4(
				0.25f,
				1.f,
				0.35f,
				1.f)
			: ImVec4(
				1.f,
				0.3f,
				0.2f,
				1.f);
		ImGui::TextColored(
			Color,
			"%s",
			m_bResultPopupSuccess
				? "Success"
				: "Failed");
		ImGui::Separator();
		ImGui::TextWrapped(
			"%s",
			m_sResultPopupMessage.c_str());
		if (ImGui::Button(
			"OK",
			ImVec2(120.f, 0.f)))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

void CRagdollEditorGUI::SelectModel(
	const SPtr<CResModel>& pModel)
{
	m_pSelectedModel = pModel;
	if (!m_pSelectedModel)
	{
		m_sStatus =
			"Model selection failed.";
		return;
	}

	if (m_pSelectedModel->GetState() !=
			CResource::STATE::LOADED &&
		FAILED(m_pSelectedModel->Load()))
	{
		m_pSelectedModel.reset();
		m_sStatus =
			"Failed to load the selected model.";
		return;
	}

	if (!m_Authoring.Initialize(
		*m_pSelectedModel))
	{
		m_pSelectedModel.reset();
		m_sStatus =
			"Failed to initialize ragdoll authoring for the model.";
		return;
	}

	m_Ragdoll.sSkeletonTag =
		m_pSelectedModel->GetPath();
	m_eSelection =
		SELECTION_TYPE::NONE;
	m_iSelectedBone = -1;
	m_iSelectedBody = -1;
	m_iSelectedShape = -1;
	m_iSelectedJoint = -1;
	m_sStatus =
		"Model selected: " +
		m_pSelectedModel->GetPath();
	PlacePreviewAtCamera();
}

void CRagdollEditorGUI::
AddBodyForSelectedBone()
{
	if (!m_pSelectedModel ||
		m_iSelectedBone < 0 ||
		static_cast<size_t>(
			m_iSelectedBone) >=
			m_pSelectedModel->
				GetBones().size())
	{
		m_sStatus =
			"Select a skeleton bone first.";
		return;
	}

	const auto& pBone =
		m_pSelectedModel->
			GetBones()[m_iSelectedBone];
	if (!pBone)
		return;

	const std::string sBoneName =
		pBone->GetBoneName();
	if (std::ranges::any_of(
		m_Ragdoll.Bodies,
		[&sBoneName](
			const PX_RAGDOLL_BODY_DESC& tBody)
		{
			return tBody.sBoneName ==
				sBoneName;
		}))
	{
		m_sStatus =
			"A body already exists for bone: " +
			sBoneName;
		return;
	}

	PX_RAGDOLL_BODY_DESC tBody{};
	tBody.sBodyName = sBoneName;
	tBody.sBoneName = sBoneName;
	tBody.fMass = 1.f;
	tBody.fMaxDepenetrationVelocity = 1.f;

	PX_RAGDOLL_SHAPE_DESC tShape{};
	tShape.sName =
		sBoneName + "Shape";
	tShape.eType =
		PX_RAGDOLL_SHAPE_TYPE::SPHERE;
	tShape.fRadius = 0.15f;
	tShape.bQueryEnabled = false;
	tBody.Shapes.emplace_back(
		std::move(tShape));
	m_Ragdoll.Bodies.emplace_back(
		std::move(tBody));

	m_iSelectedBody =
		static_cast<int32_t>(
			m_Ragdoll.Bodies.size() - 1);
	m_iSelectedShape = -1;
	m_eSelection =
		SELECTION_TYPE::BODY;
	m_bDirty = true;
	m_sStatus =
		"Added body: " + sBoneName;
}

void CRagdollEditorGUI::
RemoveSelectedBody()
{
	auto* pBody = GetSelectedBody();
	if (!pBody)
		return;

	const std::string sBodyName =
		pBody->sBodyName;
	std::erase_if(
		m_Ragdoll.Joints,
		[&sBodyName](
			const PX_RAGDOLL_D6_JOINT_DESC& tJoint)
		{
			return tJoint.sParentBodyName ==
				sBodyName ||
				tJoint.sChildBodyName ==
				sBodyName;
		});
	m_Ragdoll.Bodies.erase(
		m_Ragdoll.Bodies.begin() +
		m_iSelectedBody);
	m_eSelection =
		SELECTION_TYPE::NONE;
	m_iSelectedBody = -1;
	m_iSelectedShape = -1;
	m_iSelectedJoint = -1;
	m_bDirty = true;
	m_sStatus =
		"Removed body: " + sBodyName;
}

void CRagdollEditorGUI::
AddShapeToSelectedBody()
{
	auto* pBody = GetSelectedBody();
	if (!pBody)
		return;

	PX_RAGDOLL_SHAPE_DESC tShape{};
	tShape.sName =
		pBody->sBodyName +
		"Shape" +
		std::to_string(
			pBody->Shapes.size());
	tShape.eType =
		PX_RAGDOLL_SHAPE_TYPE::SPHERE;
	tShape.fRadius = 0.15f;
	tShape.bQueryEnabled = false;
	pBody->Shapes.emplace_back(
		std::move(tShape));
	m_iSelectedShape =
		static_cast<int32_t>(
			pBody->Shapes.size() - 1);
	m_eSelection =
		SELECTION_TYPE::SHAPE;
	m_bDirty = true;
}

void CRagdollEditorGUI::
RemoveSelectedShape()
{
	auto* pBody = GetSelectedBody();
	if (!pBody ||
		m_iSelectedShape < 0 ||
		static_cast<size_t>(
			m_iSelectedShape) >=
			pBody->Shapes.size())
	{
		return;
	}

	pBody->Shapes.erase(
		pBody->Shapes.begin() +
		m_iSelectedShape);
	m_iSelectedShape = -1;
	m_eSelection =
		SELECTION_TYPE::BODY;
	m_bDirty = true;
}

void CRagdollEditorGUI::AddBindPoseJoint()
{
	if (!m_Authoring.IsReady() ||
		m_iNewJointParentBody < 0 ||
		m_iNewJointChildBody < 0 ||
		static_cast<size_t>(
			m_iNewJointParentBody) >=
			m_Ragdoll.Bodies.size() ||
		static_cast<size_t>(
			m_iNewJointChildBody) >=
			m_Ragdoll.Bodies.size())
	{
		m_sStatus =
			"Select valid parent and child bodies.";
		return;
	}
	if (m_iNewJointParentBody ==
		m_iNewJointChildBody)
	{
		m_sStatus =
			"A joint cannot connect a body to itself.";
		return;
	}

	const auto& tParent =
		m_Ragdoll.Bodies[
			m_iNewJointParentBody];
	const auto& tChild =
		m_Ragdoll.Bodies[
			m_iNewJointChildBody];
	const _bool bPairExists =
		std::ranges::any_of(
			m_Ragdoll.Joints,
			[&](const auto& tJoint)
			{
				return
					(tJoint.sParentBodyName ==
						tParent.sBodyName &&
					 tJoint.sChildBodyName ==
						tChild.sBodyName) ||
					(tJoint.sParentBodyName ==
						tChild.sBodyName &&
					 tJoint.sChildBodyName ==
						tParent.sBodyName);
			});
	if (bPairExists)
	{
		m_sStatus =
			"A joint already connects the selected bodies.";
		return;
	}

	std::string sJointName =
		tParent.sBodyName +
		"To" +
		tChild.sBodyName;
	std::string sUniqueName =
		sJointName;
	uint32_t iSuffix = 1;
	while (std::ranges::any_of(
		m_Ragdoll.Joints,
		[&sUniqueName](const auto& tJoint)
		{
			return tJoint.sJointName ==
				sUniqueName;
		}))
	{
		sUniqueName =
			sJointName +
			std::to_string(iSuffix++);
	}

	if (!m_Authoring.AddBindPoseD6Joint(
		m_Ragdoll,
		sUniqueName.c_str(),
		tParent.sBodyName.c_str(),
		tParent.sBoneName.c_str(),
		tChild.sBodyName.c_str(),
		tChild.sBoneName.c_str(),
		45.f,
		45.f,
		45.f))
	{
		m_sStatus =
			"Failed to create a bind-pose D6 joint.";
		return;
	}

	m_iSelectedJoint =
		static_cast<int32_t>(
			m_Ragdoll.Joints.size() - 1);
	m_eSelection =
		SELECTION_TYPE::JOINT;
	m_bDirty = true;
	m_sStatus =
		"Created joint: " +
		sUniqueName;
}

void CRagdollEditorGUI::
RemoveSelectedJoint()
{
	if (m_iSelectedJoint < 0 ||
		static_cast<size_t>(
			m_iSelectedJoint) >=
			m_Ragdoll.Joints.size())
	{
		m_sStatus =
			"Select a joint to remove.";
		return;
	}

	const std::string sJointName =
		m_Ragdoll.Joints[
			m_iSelectedJoint].
				sJointName;
	m_Ragdoll.Joints.erase(
		m_Ragdoll.Joints.begin() +
		m_iSelectedJoint);
	m_iSelectedJoint = -1;
	m_eSelection =
		SELECTION_TYPE::NONE;
	m_bDirty = true;
	m_sStatus =
		"Removed joint: " +
		sJointName;
}

_bool CRagdollEditorGUI::
GenerateHumanoidPreset()
{
	if (!m_pSelectedModel ||
		!m_Authoring.IsReady())
	{
		m_sStatus =
			"Select a loaded skeleton model first.";
		return false;
	}
	StopPhysicsPreview();

	struct BODY_PRESET
	{
		const _char* pName{};
		_float fMass{};
		_float fRadius{};
	};
	const BODY_PRESET BodyPresets[] = {
		{ "Hips", 10.f, 0.25f },
		{ "Spine", 8.f, 0.22f },
		{ "Spine3", 12.f, 0.28f },
		{ "Head", 5.f, 0.2f },
		{ "LeftArm", 4.f, 0.18f },
		{ "LeftForeArm", 3.f, 0.15f },
		{ "LeftHand", 1.5f, 0.12f },
		{ "RightArm", 4.f, 0.18f },
		{ "RightForeArm", 3.f, 0.15f },
		{ "RightHand", 1.5f, 0.12f },
		{ "LeftUpLeg", 8.f, 0.2f },
		{ "LeftLeg", 6.f, 0.17f },
		{ "LeftFoot", 2.f, 0.14f },
		{ "RightUpLeg", 8.f, 0.2f },
		{ "RightLeg", 6.f, 0.17f },
		{ "RightFoot", 2.f, 0.14f }
	};

	PX_RAGDOLL_DESC Generated{};
	Generated.sSkeletonTag =
		m_pSelectedModel->GetPath();
	for (const auto& Preset :
		BodyPresets)
	{
		if (m_pSelectedModel->
			Get_BoneIndex(
				Preset.pName) < 0)
		{
			m_sStatus =
				"Humanoid generation failed. Missing bone: " +
				std::string{ Preset.pName };
			return false;
		}

		PX_RAGDOLL_BODY_DESC tBody{};
		tBody.sBodyName = Preset.pName;
		tBody.sBoneName = Preset.pName;
		tBody.fMass = Preset.fMass;
		tBody.fLinearDamping = 0.1f;
		tBody.fAngularDamping = 0.5f;
		tBody.fMaxDepenetrationVelocity =
			1.f;
		tBody.bGravityEnabled = true;

		PX_RAGDOLL_SHAPE_DESC tShape{};
		tShape.sName =
			std::string{ Preset.pName } +
			"Shape";
		tShape.eType =
			PX_RAGDOLL_SHAPE_TYPE::SPHERE;
		tShape.fRadius = Preset.fRadius;
		tShape.bQueryEnabled = false;
		tBody.Shapes.emplace_back(
			std::move(tShape));
		Generated.Bodies.emplace_back(
			std::move(tBody));
	}

	const _bool bShapesReady =
		m_Authoring.FitCapsuleBodyToChild(
			Generated,
			"Spine",
			"Spine",
			"Spine3",
			0.2f) &&
		m_Authoring.FitCapsuleBodyToChild(
			Generated,
			"LeftArm",
			"LeftArm",
			"LeftForeArm",
			0.14f) &&
		m_Authoring.FitCapsuleBodyToChild(
			Generated,
			"LeftForeArm",
			"LeftForeArm",
			"LeftHand",
			0.12f) &&
		m_Authoring.FitCapsuleBodyToChild(
			Generated,
			"RightArm",
			"RightArm",
			"RightForeArm",
			0.14f) &&
		m_Authoring.FitCapsuleBodyToChild(
			Generated,
			"RightForeArm",
			"RightForeArm",
			"RightHand",
			0.12f) &&
		m_Authoring.FitCapsuleBodyToChild(
			Generated,
			"LeftUpLeg",
			"LeftUpLeg",
			"LeftLeg",
			0.16f) &&
		m_Authoring.FitCapsuleBodyToChild(
			Generated,
			"LeftLeg",
			"LeftLeg",
			"LeftFoot",
			0.14f) &&
		m_Authoring.FitCapsuleBodyToChild(
			Generated,
			"RightUpLeg",
			"RightUpLeg",
			"RightLeg",
			0.16f) &&
		m_Authoring.FitCapsuleBodyToChild(
			Generated,
			"RightLeg",
			"RightLeg",
			"RightFoot",
			0.14f) &&
		m_Authoring.FitPelvisBox(
			Generated,
			"Hips",
			"Hips",
			"LeftUpLeg",
			"RightUpLeg",
			"Spine") &&
		m_Authoring.FitChestBox(
			Generated,
			"Spine3",
			"Spine3",
			"LeftArm",
			"RightArm",
			"Head") &&
		m_Authoring.FitFootBox(
			Generated,
			"LeftFoot",
			"LeftFoot",
			"LeftToeBase") &&
		m_Authoring.FitFootBox(
			Generated,
			"RightFoot",
			"RightFoot",
			"RightToeBase");
	if (!bShapesReady)
	{
		m_sStatus =
			"Humanoid shape fitting failed.";
		return false;
	}

	struct JOINT_PRESET
	{
		const _char* pName{};
		const _char* pParent{};
		const _char* pChild{};
		_float fTwist{};
		_float fSwingY{};
		_float fSwingZ{};
	};
	const JOINT_PRESET JointPresets[] = {
		{ "HipsToSpine", "Hips", "Spine", 15.f, 20.f, 20.f },
		{ "SpineToSpine3", "Spine", "Spine3", 20.f, 25.f, 20.f },
		{ "Spine3ToHead", "Spine3", "Head", 30.f, 30.f, 25.f },
		{ "Spine3ToLeftArm", "Spine3", "LeftArm", 45.f, 70.f, 60.f },
		{ "LeftArmToLeftForeArm", "LeftArm", "LeftForeArm", 60.f, 60.f, 60.f },
		{ "LeftForeArmToLeftHand", "LeftForeArm", "LeftHand", 35.f, 30.f, 30.f },
		{ "Spine3ToRightArm", "Spine3", "RightArm", 45.f, 70.f, 60.f },
		{ "RightArmToRightForeArm", "RightArm", "RightForeArm", 60.f, 60.f, 60.f },
		{ "RightForeArmToRightHand", "RightForeArm", "RightHand", 35.f, 30.f, 30.f },
		{ "HipsToLeftUpLeg", "Hips", "LeftUpLeg", 45.f, 50.f, 45.f },
		{ "LeftUpLegToLeftLeg", "LeftUpLeg", "LeftLeg", 60.f, 60.f, 60.f },
		{ "LeftLegToLeftFoot", "LeftLeg", "LeftFoot", 30.f, 30.f, 30.f },
		{ "HipsToRightUpLeg", "Hips", "RightUpLeg", 45.f, 50.f, 45.f },
		{ "RightUpLegToRightLeg", "RightUpLeg", "RightLeg", 60.f, 60.f, 60.f },
		{ "RightLegToRightFoot", "RightLeg", "RightFoot", 30.f, 30.f, 30.f }
	};
	for (const auto& Preset :
		JointPresets)
	{
		if (!m_Authoring.AddBindPoseD6Joint(
			Generated,
			Preset.pName,
			Preset.pParent,
			Preset.pParent,
			Preset.pChild,
			Preset.pChild,
			Preset.fTwist,
			Preset.fSwingY,
			Preset.fSwingZ))
		{
			m_sStatus =
				"Humanoid joint generation failed: " +
				std::string{ Preset.pName };
			return false;
		}
	}

	if (!m_Authoring.
			ConfigureAnatomicalHinge(
				Generated,
				"LeftArmToLeftForeArm",
				"LeftForeArm",
				"LeftHand",
				{ 0.f, -1.f, 0.f },
				-5.f,
				135.f) ||
		!m_Authoring.
			ConfigureAnatomicalHinge(
				Generated,
				"RightArmToRightForeArm",
				"RightForeArm",
				"RightHand",
				{ 0.f, -1.f, 0.f },
				-5.f,
				135.f) ||
		!m_Authoring.
			ConfigureAnatomicalHinge(
				Generated,
				"LeftUpLegToLeftLeg",
				"LeftLeg",
				"LeftFoot",
				{ 0.f, 0.f, 1.f },
				-5.f,
				130.f) ||
		!m_Authoring.
			ConfigureAnatomicalHinge(
				Generated,
				"RightUpLegToRightLeg",
				"RightLeg",
				"RightFoot",
				{ 0.f, 0.f, 1.f },
				-5.f,
				130.f))
	{
		m_sStatus =
			"Humanoid hinge generation failed.";
		return false;
	}

	m_Ragdoll = std::move(Generated);
	m_eSelection =
		SELECTION_TYPE::NONE;
	m_iSelectedBone = -1;
	m_iSelectedBody = -1;
	m_iSelectedShape = -1;
	m_iSelectedJoint = -1;
	m_ValidationErrors.clear();
	m_bDirty = true;
	m_sStatus =
		"Generated humanoid ragdoll: 16 bodies, 15 joints.";
	return true;
}

_bool CRagdollEditorGUI::Validate(
	std::vector<std::string>& Errors) const
{
	Errors.clear();
	if (!m_pSelectedModel)
	{
		Errors.emplace_back(
			"No model is selected.");
	}
	if (m_Ragdoll.iVersion !=
		PX_RAGDOLL_DATA_VERSION)
	{
		Errors.emplace_back(
			"Unsupported ragdoll data version.");
	}
	if (m_Ragdoll.Bodies.empty())
	{
		Errors.emplace_back(
			"Ragdoll has no bodies.");
	}

	std::unordered_set<std::string>
		BodyNames{};
	std::unordered_set<std::string>
		BoneNames{};
	for (const auto& tBody :
		m_Ragdoll.Bodies)
	{
		if (tBody.sBodyName.empty() ||
			!BodyNames.emplace(
				tBody.sBodyName).second)
		{
			Errors.emplace_back(
				"Empty or duplicate body name: " +
				tBody.sBodyName);
		}
		if (tBody.sBoneName.empty() ||
			!BoneNames.emplace(
				tBody.sBoneName).second)
		{
			Errors.emplace_back(
				"Empty or duplicate body bone: " +
				tBody.sBoneName);
		}
		if (m_pSelectedModel &&
			m_pSelectedModel->
				Get_BoneIndex(
					tBody.sBoneName.c_str()) < 0)
		{
			Errors.emplace_back(
				"Bone not found in model: " +
				tBody.sBoneName);
		}
		if (tBody.fMass <= 0.f)
		{
			Errors.emplace_back(
				"Body mass must be positive: " +
				tBody.sBodyName);
		}
		if (tBody.Shapes.empty())
		{
			Errors.emplace_back(
				"Body has no shapes: " +
				tBody.sBodyName);
		}

		std::unordered_set<std::string>
			ShapeNames{};
		for (const auto& tShape :
			tBody.Shapes)
		{
			if (tShape.sName.empty() ||
				!ShapeNames.emplace(
					tShape.sName).second)
			{
				Errors.emplace_back(
					"Empty or duplicate shape in body: " +
					tBody.sBodyName);
			}
		}
	}

	std::unordered_set<std::string>
		JointNames{};
	std::unordered_set<std::string>
		JointChildren{};
	for (const auto& tJoint :
		m_Ragdoll.Joints)
	{
		if (tJoint.sJointName.empty() ||
			!JointNames.emplace(
				tJoint.sJointName).second)
		{
			Errors.emplace_back(
				"Empty or duplicate joint name: " +
				tJoint.sJointName);
		}
		if (!BodyNames.contains(
				tJoint.sParentBodyName) ||
			!BodyNames.contains(
				tJoint.sChildBodyName))
		{
			Errors.emplace_back(
				"Joint references a missing body: " +
				tJoint.sJointName);
		}
		if (!JointChildren.emplace(
				tJoint.sChildBodyName).second)
		{
			Errors.emplace_back(
				"Body has multiple joint parents: " +
				tJoint.sChildBodyName);
		}
		if (tJoint.fTwistLowerDegrees >
			tJoint.fTwistUpperDegrees)
		{
			Errors.emplace_back(
				"Twist lower limit exceeds upper limit: " +
				tJoint.sJointName);
		}
	}

	if (m_Ragdoll.Bodies.size() > 1 &&
		m_Ragdoll.Joints.size() !=
			m_Ragdoll.Bodies.size() - 1)
	{
		Errors.emplace_back(
			"A connected ragdoll tree requires body count - 1 joints.");
	}

	return Errors.empty();
}

HRESULT CRagdollEditorGUI::Save() const
{
	const auto FilePath = MakeFilePath();
	if (FilePath.empty())
		return E_FAIL;

	std::error_code Error{};
	std::filesystem::create_directories(
		FilePath.parent_path(),
		Error);
	if (Error)
		return E_FAIL;

	const HRESULT hr =
		CGameInstance::Get().JsonSerialize(
			FilePath.generic_string(),
			m_Ragdoll,
			RAGDOLL_ROOT_NAME);
	auto* pThis =
		const_cast<CRagdollEditorGUI*>(this);
	if (SUCCEEDED(hr))
	{
		pThis->m_bDirty = false;
		pThis->m_sStatus =
			"Saved: " +
			FilePath.generic_string();
	}
	else
	{
		pThis->m_sStatus =
			"Save failed: " +
			FilePath.generic_string();
	}
	return hr;
}

HRESULT CRagdollEditorGUI::Load()
{
	StopPhysicsPreview();
	const auto FilePath = MakeFilePath();
	if (FilePath.empty() ||
		!std::filesystem::exists(
			FilePath))
	{
		m_sStatus =
			"File not found: " +
			FilePath.generic_string();
		return E_FAIL;
	}

	PX_RAGDOLL_DESC Loaded{};
	if (FAILED(
		CGameInstance::Get().
			JsonDeSerialize(
				FilePath.generic_string(),
				Loaded,
				RAGDOLL_ROOT_NAME)))
	{
		m_sStatus =
			"Load failed: " +
			FilePath.generic_string();
		return E_FAIL;
	}
	if (Loaded.iVersion !=
		PX_RAGDOLL_DATA_VERSION)
	{
		m_sStatus =
			"Unsupported ragdoll version.";
		return E_FAIL;
	}

	m_Ragdoll = std::move(Loaded);
	m_eSelection =
		SELECTION_TYPE::NONE;
	m_iSelectedBone = -1;
	m_iSelectedBody = -1;
	m_iSelectedShape = -1;
	m_iSelectedJoint = -1;
	m_ValidationErrors.clear();
	m_bDirty = false;
	m_sStatus =
		"Loaded: " +
		FilePath.generic_string();
	return S_OK;
}

void CRagdollEditorGUI::Clear()
{
	StopPhysicsPreview();
	const std::string sSkeletonTag =
		m_pSelectedModel
		? m_pSelectedModel->GetPath()
		: std::string{};
	m_Ragdoll = {};
	m_Ragdoll.sSkeletonTag =
		sSkeletonTag;
	m_eSelection =
		SELECTION_TYPE::NONE;
	m_iSelectedBone = -1;
	m_iSelectedBody = -1;
	m_iSelectedShape = -1;
	m_iSelectedJoint = -1;
	m_ValidationErrors.clear();
	m_bDirty = false;
	m_sStatus =
		"Cleared ragdoll data.";
}

std::filesystem::path
CRagdollEditorGUI::MakeFilePath() const
{
	const std::filesystem::path Input{
		m_RagdollFileName
	};
	const std::string sStem =
		Input.stem().string();
	if (sStem.empty())
		return {};

	return std::filesystem::path{
		"./Resources/PhysX/Ragdolls"
	} / (sStem + ".ragdoll.json");
}

void CRagdollEditorGUI::
QueueResultPopup(
	std::string sMessage,
	_bool bSuccess)
{
	m_sResultPopupMessage =
		std::move(sMessage);
	m_bResultPopupSuccess =
		bSuccess;
	m_bOpenResultPopup = true;
}

PX_RAGDOLL_BODY_DESC*
CRagdollEditorGUI::GetSelectedBody()
{
	if (m_iSelectedBody < 0 ||
		static_cast<size_t>(
			m_iSelectedBody) >=
			m_Ragdoll.Bodies.size())
	{
		return nullptr;
	}

	return &m_Ragdoll.Bodies[
		m_iSelectedBody];
}

PX_RAGDOLL_SHAPE_DESC*
CRagdollEditorGUI::GetSelectedShape()
{
	auto* pBody = GetSelectedBody();
	if (!pBody ||
		m_iSelectedShape < 0 ||
		static_cast<size_t>(
			m_iSelectedShape) >=
			pBody->Shapes.size())
	{
		return nullptr;
	}

	return &pBody->Shapes[
		m_iSelectedShape];
}

PX_RAGDOLL_D6_JOINT_DESC*
CRagdollEditorGUI::GetSelectedJoint()
{
	if (m_iSelectedJoint < 0 ||
		static_cast<size_t>(
			m_iSelectedJoint) >=
			m_Ragdoll.Joints.size())
	{
		return nullptr;
	}

	return &m_Ragdoll.Joints[
		m_iSelectedJoint];
}

UPtr<CRagdollEditorGUI>
CRagdollEditorGUI::Create()
{
	return ToUPtr(
		new CRagdollEditorGUI{});
}

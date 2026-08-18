#include "pch.h"
#include "NvClothCollisionEditorGUI.h"

#include "CameraObject.h"
#include "ComConstantBuffer.h"
#include "ComModelInstance.h"
#include "DbgLineRender.h"
#include "GameInstance.h"
#include "IRenderable.h"
#include "ResModel.h"
#include "ResModelBone.h"
#include "ResModelMesh.h"
#include "ResPixelShader.h"
#include "ResVertexShader.h"

#include <filesystem>

NS_USING(Engine)

namespace Engine
{
	class CNvClothCollisionPreviewRenderer final :
		public CEngineBase,
		public IRenderable
	{
	private:
		CNvClothCollisionPreviewRenderer() = default;
		~CNvClothCollisionPreviewRenderer() override = default;

	public:
		HRESULT Initialize(
			const StringID& sGroupTag,
			const StringID& sResourceTag,
			const std::vector<_float4x4>& BindPoses)
		{
			CComModelInstance::DESC ModelDesc{};
			ModelDesc.sGroupTag = sGroupTag;
			ModelDesc.sResTag = sResourceTag;
			auto pModelPrototype =
				CGameInstance::Get().ClonePrototype(
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
				static_uptr_cast<CComModelInstance>(
					std::move(pModelPrototype));

			CComConstantBuffer::DESC BufferDesc{};
			BufferDesc.cBufferId = {
				TAG_RES_GRP_PERMANENT_BUFFER,
				TAG_RES_CBUFFER_OBJECT
			};
			auto pBufferPrototype =
				CGameInstance::Get().ClonePrototype(
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
				static_uptr_cast<CComConstantBuffer>(
					std::move(pBufferPrototype));

			m_pVertexShader =
				CGameInstance::Get().
				GetResourceFirst<CResVertexShader>(
					TAG_RES_GRP_PERMANENT_SHADER,
					"VS_TestModelAnim");
			m_pPixelShader =
				CGameInstance::Get().
				GetResourceFirst<CResPixelShader>(
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

			m_pModelInstance->Get_CombinedBoneMatrices() =
				BindPoses;
			XMStoreFloat4x4(
				&m_WorldMatrix,
				XMMatrixIdentity());
			return S_OK;
		}

		void SetWorldMatrix(FXMMATRIX WorldMatrix)
		{
			XMStoreFloat4x4(
				&m_WorldMatrix,
				WorldMatrix);
		}

		HRESULT Render(
			ID3D11DeviceContext* pContext,
			const RENDER_CTX& Context) override
		{
			if (!pContext ||
				!m_pModelInstance ||
				!m_pObjectBuffer ||
				!m_pVertexShader ||
				!m_pPixelShader)
			{
				return E_FAIL;
			}

			auto pModel = m_pModelInstance->GetModel();
			if (!pModel)
				return E_FAIL;

			CB_PER_OBJECT PerObject{};
			PerObject.matWorld = m_WorldMatrix;
			XMStoreFloat4x4(
				&PerObject.matWVP,
				XMLoadFloat4x4(&m_WorldMatrix) *
				Context.matViewProj);
			if (FAILED(m_pObjectBuffer->MapDiscard(
				pContext,
				&PerObject,
				sizeof(PerObject))))
			{
				return E_FAIL;
			}

			pContext->VSSetConstantBuffers(
				0, 1,
				m_pObjectBuffer->GetAdressOfBuffer());
			pContext->PSSetConstantBuffers(
				0, 1,
				m_pObjectBuffer->GetAdressOfBuffer());
			pContext->IASetInputLayout(
				m_pVertexShader->GetInputLayout().Get());
			pContext->VSSetShader(
				m_pVertexShader->GetVertexShader().Get(),
				nullptr, 0);
			pContext->PSSetShader(
				m_pPixelShader->GetPixelShader().Get(),
				nullptr, 0);

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
				const uint32_t iOffset{};
				pContext->IASetVertexBuffers(
					0, 1, &pVertexBuffer,
					&iStride, &iOffset);
				pContext->IASetIndexBuffer(
					pMesh->GetIndexBuffer().Get(),
					pMesh->GetIndexFormat(), 0);
				pContext->IASetPrimitiveTopology(
					pMesh->GetPrimitiveType());

				if (FAILED(
					m_pModelInstance->Bind_BoneMatrices(
						pContext, iMesh)))
				{
					return E_FAIL;
				}
				m_pModelInstance->Bind_Textures(
					pContext, iMesh);
				m_pModelInstance->Bind_Materials(
					pContext,
					{ 1.f, 1.f, 1.f },
					0.f,
					{ 1.f, 1.f, 1.f },
					0.f,
					1.f);
				pContext->DrawIndexed(
					pMesh->GetNumIndices(),
					0, 0);
			}
			return S_OK;
		}

		bool HasRenderPass(RENDERPASS ePass) const override
		{
			return ePass == RENDERPASS::DEFAULT;
		}

		static UPtr<CNvClothCollisionPreviewRenderer>
		Create(
			const StringID& sGroupTag,
			const StringID& sResourceTag,
			const std::vector<_float4x4>& BindPoses)
		{
			auto pInstance = ToUPtr(
				new CNvClothCollisionPreviewRenderer{});
			if (FAILED(pInstance->Initialize(
				sGroupTag,
				sResourceTag,
				BindPoses)))
			{
				return nullptr;
			}
			return pInstance;
		}

	private:
		UPtr<CComModelInstance> m_pModelInstance{};
		UPtr<CComConstantBuffer> m_pObjectBuffer{};
		SPtr<CResVertexShader> m_pVertexShader{};
		SPtr<CResPixelShader> m_pPixelShader{};
		_float4x4 m_WorldMatrix{};
	};
}

namespace
{
	constexpr int NVCLOTH_COLLISION_GIZMO_ID =
		0x4E564343;
	constexpr float MIN_SHAPE_SIZE = 0.001f;

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
		for (const auto& [GroupTag, Group] : Resources)
		{
			for (const auto& [ResourceTag, Values] : Group)
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
						std::static_pointer_cast<CResModel>(
							pResource);
					std::string sLabel =
						std::string{ GroupTag.GetDbgStr() } +
						" / " +
						std::string{ ResourceTag.GetDbgStr() } +
						" | " +
						pModel->GetPath();
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
			Entries, {},
			&MODEL_ENTRY::sLabel);
		return Entries;
	}

	_matrix MakePoseMatrix(
		const _float3& vPosition,
		const _float4& vRotation)
	{
		_vector qRotation = XMLoadFloat4(&vRotation);
		if (XMVectorGetX(
			XMVector4LengthSq(qRotation)) <= 1.e-8f)
		{
			qRotation = XMQuaternionIdentity();
		}
		else
		{
			qRotation =
				XMQuaternionNormalize(qRotation);
		}
		return
			XMMatrixRotationQuaternion(qRotation) *
			XMMatrixTranslation(
				vPosition.x,
				vPosition.y,
				vPosition.z);
	}

	_bool MakeRigidMatrix(
		FXMMATRIX Matrix,
		_matrix& OutMatrix)
	{
		_vector vScale{};
		_vector qRotation{};
		_vector vTranslation{};
		if (!XMMatrixDecompose(
			&vScale,
			&qRotation,
			&vTranslation,
			Matrix))
		{
			return false;
		}
		OutMatrix =
			XMMatrixRotationQuaternion(
				XMQuaternionNormalize(qRotation)) *
			XMMatrixTranslationFromVector(vTranslation);
		return true;
	}

	_bool BuildModelBindPoses(
		CResModel& Model,
		std::vector<_float4x4>& OutBindPoses)
	{
		const auto& Bones = Model.GetBones();
		if (Bones.empty())
			return false;

		OutBindPoses.resize(Bones.size());
		std::vector<uint8_t> States(
			Bones.size(), 0u);
		const _matrix ModelPreTransform =
			XMLoadFloat4x4(
				&Model.Get_PreTransformMatrix());

		auto BuildPose =
			[&](auto&& Self, size_t iBone) -> _bool
			{
				if (iBone >= Bones.size() ||
					!Bones[iBone])
				{
					return false;
				}
				if (States[iBone] == 2u)
					return true;
				if (States[iBone] == 1u)
					return false;

				States[iBone] = 1u;
				const int32_t iParent =
					Bones[iBone]->
						GetParendBoneIndex();
				_matrix CombinedPose{};
				if (iParent < 0)
				{
					CombinedPose =
						Bones[iBone]->
							Get_TransformationMatrix() *
						ModelPreTransform;
				}
				else
				{
					const size_t iParentIndex =
						static_cast<size_t>(iParent);
					if (iParentIndex >= Bones.size() ||
						!Self(Self, iParentIndex))
					{
						return false;
					}
					CombinedPose =
						Bones[iBone]->
							Get_TransformationMatrix() *
						XMLoadFloat4x4(
							&OutBindPoses[
								iParentIndex]);
				}

				XMStoreFloat4x4(
					&OutBindPoses[iBone],
					CombinedPose);
				States[iBone] = 2u;
				return true;
			};

		for (size_t iBone = 0;
			iBone < Bones.size();
			++iBone)
		{
			if (!BuildPose(BuildPose, iBone))
			{
				OutBindPoses.clear();
				return false;
			}
		}
		return true;
	}

	void NormalizeQuaternion(_float4& Rotation)
	{
		const _vector qRotation =
			XMLoadFloat4(&Rotation);
		if (XMVectorGetX(
			XMVector4LengthSq(qRotation)) <= 1.e-8f)
		{
			Rotation = { 0.f, 0.f, 0.f, 1.f };
		}
		else
		{
			XMStoreFloat4(
				&Rotation,
				XMQuaternionNormalize(qRotation));
		}
	}

	_float3 QuaternionToEulerDegrees(
		const _float4& Quaternion)
	{
		_float4 q = Quaternion;
		NormalizeQuaternion(q);
		const float sinX =
			2.f * (q.w * q.x + q.y * q.z);
		const float cosX =
			1.f - 2.f * (q.x * q.x + q.y * q.y);
		const float sinY = std::clamp(
			2.f * (q.w * q.y - q.z * q.x),
			-1.f, 1.f);
		const float sinZ =
			2.f * (q.w * q.z + q.x * q.y);
		const float cosZ =
			1.f - 2.f * (q.y * q.y + q.z * q.z);
		return {
			XMConvertToDegrees(
				std::atan2(sinX, cosX)),
			XMConvertToDegrees(std::asin(sinY)),
			XMConvertToDegrees(
				std::atan2(sinZ, cosZ))
		};
	}

	_float4 EulerDegreesToQuaternion(
		const _float3& Euler)
	{
		_float4 Quaternion{};
		XMStoreFloat4(
			&Quaternion,
			XMQuaternionRotationRollPitchYaw(
				XMConvertToRadians(Euler.x),
				XMConvertToRadians(Euler.y),
				XMConvertToRadians(Euler.z)));
		NormalizeQuaternion(Quaternion);
		return Quaternion;
	}

	_bool EditString(
		const char* pLabel,
		std::string& Value,
		size_t iCapacity = 256)
	{
		std::vector<char> Buffer(
			iCapacity, '\0');
		strncpy_s(
			Buffer.data(),
			Buffer.size(),
			Value.c_str(),
			_TRUNCATE);
		if (!ImGui::InputText(
			pLabel,
			Buffer.data(),
			Buffer.size()))
		{
			return false;
		}
		Value = Buffer.data();
		return true;
	}

	_float3 GetMatrixPosition(FXMMATRIX Matrix)
	{
		_float3 vPosition{};
		XMStoreFloat3(&vPosition, Matrix.r[3]);
		return vPosition;
	}

	const char* GetShapeTypeName(
		NVCLOTH_COLLISION_SHAPE_TYPE eType)
	{
		switch (eType)
		{
		case NVCLOTH_COLLISION_SHAPE_TYPE::SPHERE:
			return "Sphere";
		case NVCLOTH_COLLISION_SHAPE_TYPE::CAPSULE:
			return "Capsule";
		case NVCLOTH_COLLISION_SHAPE_TYPE::BOX:
			return "Box";
		}
		return "Unknown";
	}
}

CNvClothCollisionEditorGUI::
~CNvClothCollisionEditorGUI() = default;

void CNvClothCollisionEditorGUI::UpdateGUI()
{
	if (!m_bOpen)
		return;

	DrawWindow();
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

void CNvClothCollisionEditorGUI::DrawWindow()
{
	ImGui::SetNextWindowSize(
		ImVec2{ 980.f, 720.f },
		ImGuiCond_FirstUseEver);
	if (!ImGui::Begin(
		"NvCloth Collision Rig Editor",
		&m_bOpen))
	{
		ImGui::End();
		return;
	}

	if (m_bOpenResultPopup)
	{
		ImGui::OpenPopup(
			"NvCloth Collision Rig Result");
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
		ImGui::SetNextItemWidth(260.f);
		ImGui::InputText(
			"Rig File",
			m_FileName,
			std::size(m_FileName));
	}
	else
	{
		ImGui::Text(
			"Rig File: %s",
			m_FileName);
	}

	if (ImGui::Button("New", ImVec2{ 80.f, 0.f }))
		ImGui::OpenPopup("Confirm Rig Clear");
	ImGui::SameLine();
	if (ImGui::Button("Load", ImVec2{ 80.f, 0.f }))
		ImGui::OpenPopup("Confirm Rig Load");
	ImGui::SameLine();
	if (ImGui::Button("Save", ImVec2{ 80.f, 0.f }))
		ImGui::OpenPopup("Confirm Rig Save");
	ImGui::SameLine();
	if (ImGui::Button(
		"Validate", ImVec2{ 90.f, 0.f }))
	{
		const _bool bSuccess =
			Validate(m_ValidationErrors);
		m_sStatus = bSuccess ?
			"Validation succeeded." :
			"Validation failed.";
		QueueResultPopup(m_sStatus, bSuccess);
	}

	ImGui::Separator();
	if (ImGui::BeginTable(
		"NvClothCollisionEditorLayout",
		2,
		ImGuiTableFlags_BordersInnerV |
		ImGuiTableFlags_Resizable))
	{
		ImGui::TableSetupColumn(
			"Hierarchy",
			ImGuiTableColumnFlags_WidthFixed,
			400.f);
		ImGui::TableSetupColumn(
			"Inspector",
			ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableNextColumn();
		DrawHierarchy();
		ImGui::TableNextColumn();
		DrawInspector();
		ImGui::EndTable();
	}

	ImGui::Separator();
	ImGui::TextWrapped(
		"Status: %s%s",
		m_sStatus.c_str(),
		m_bDirty ? " (modified)" : "");
	if (!m_ValidationErrors.empty() &&
		ImGui::CollapsingHeader(
			"Validation Errors",
			ImGuiTreeNodeFlags_DefaultOpen))
	{
		for (const auto& Error :
			m_ValidationErrors)
		{
			ImGui::BulletText(
				"%s", Error.c_str());
		}
	}

	DrawFilePopups();
	ImGui::End();
}

void CNvClothCollisionEditorGUI::
DrawModelSelector()
{
	ImGui::TextUnformatted("Skeleton Model");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(-1.f);
	if (!ImGui::BeginCombo(
		"##NvClothCollisionModel",
		m_sSelectedModelLabel.c_str()))
	{
		return;
	}

	const auto Entries = CollectModelEntries();
	for (const auto& Entry : Entries)
	{
		const _bool bSelected =
			Entry.pModel == m_pSelectedModel;
		if (ImGui::Selectable(
			Entry.sLabel.c_str(),
			bSelected))
		{
			SelectModel(
				Entry.pModel,
				Entry.sGroupTag,
				Entry.sResourceTag,
				Entry.sLabel);
		}
		if (bSelected)
			ImGui::SetItemDefaultFocus();
	}
	ImGui::EndCombo();
}

void CNvClothCollisionEditorGUI::
DrawPreviewControls()
{
	ImGui::Separator();
	ImGui::TextUnformatted("Viewport Preview");
	ImGui::Checkbox("Visible", &m_bPreviewVisible);
	ImGui::SameLine();
	ImGui::Checkbox("Model", &m_bPreviewModel);
	ImGui::SameLine();
	ImGui::Checkbox("Skeleton", &m_bPreviewSkeleton);
	ImGui::SameLine();
	ImGui::Checkbox("Shapes", &m_bPreviewShapes);
	ImGui::SameLine();
	ImGui::Checkbox("Depth", &m_bPreviewDepthTest);

	if (ImGui::Button(
		"Place Preview At Camera",
		ImVec2{ 190.f, 0.f }))
	{
		PlacePreviewAtCamera();
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(260.f);
	ImGui::DragFloat3(
		"Preview Position",
		&m_vPreviewPosition.x,
		0.05f);
	if (ImGui::DragFloat(
		"Preview Scale",
		&m_fPreviewScale,
		0.05f,
		MIN_SHAPE_SIZE,
		100.f))
	{
		m_fPreviewScale = std::max(
			m_fPreviewScale,
			MIN_SHAPE_SIZE);
	}

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
		"Scale",
		m_eGizmoOperation ==
			ImGuizmo::SCALE))
	{
		m_eGizmoOperation =
			ImGuizmo::SCALE;
	}
}

void CNvClothCollisionEditorGUI::DrawHierarchy()
{
	ImGui::TextUnformatted("Bones");
	if (!m_pSelectedModel)
	{
		ImGui::TextDisabled(
			"Select a skeleton model.");
		return;
	}

	if (ImGui::BeginChild(
		"NvClothCollisionBones",
		ImVec2{ 0.f, 250.f },
		true))
	{
		const auto& Bones =
			m_pSelectedModel->GetBones();
		std::vector<std::vector<size_t>> Children(
			Bones.size());
		std::vector<size_t> Roots{};
		Roots.reserve(Bones.size());
		for (size_t i = 0; i < Bones.size(); ++i)
		{
			if (!Bones[i])
				continue;

			const int32_t iParent =
				Bones[i]->GetParendBoneIndex();
			if (iParent < 0 ||
				static_cast<size_t>(iParent) >=
					Bones.size() ||
				!Bones[static_cast<size_t>(iParent)])
			{
				Roots.push_back(i);
			}
			else
			{
				Children[
					static_cast<size_t>(iParent)].
					push_back(i);
			}
		}

		std::vector<uint8_t> DrawStates(
			Bones.size(), 0u);
		auto DrawBoneNode =
			[&](auto&& Self, size_t iBone) -> void
			{
				if (iBone >= Bones.size() ||
					!Bones[iBone] ||
					DrawStates[iBone] != 0u)
				{
					return;
				}

				DrawStates[iBone] = 1u;
				const _bool bLeaf =
					Children[iBone].empty();
				ImGuiTreeNodeFlags Flags =
					ImGuiTreeNodeFlags_OpenOnArrow |
					ImGuiTreeNodeFlags_OpenOnDoubleClick |
					ImGuiTreeNodeFlags_SpanAvailWidth;
				if (bLeaf)
				{
					Flags |=
						ImGuiTreeNodeFlags_Leaf |
						ImGuiTreeNodeFlags_NoTreePushOnOpen;
				}
				if (m_eSelection ==
						SELECTION_TYPE::BONE &&
					m_iSelectedBone ==
						static_cast<int32_t>(iBone))
				{
					Flags |=
						ImGuiTreeNodeFlags_Selected;
				}

				ImGui::PushID(
					static_cast<int>(iBone));
				const _bool bOpen =
					ImGui::TreeNodeEx(
						"##Bone",
						Flags,
						"%s",
						Bones[iBone]->
							GetBoneName().c_str());
				if (ImGui::IsItemClicked())
				{
					m_eSelection =
						SELECTION_TYPE::BONE;
					m_iSelectedBone =
						static_cast<int32_t>(iBone);
					m_iSelectedShape = -1;
					m_sStatus =
						"Bone selected: " +
						Bones[iBone]->
							GetBoneName() +
						". Choose Add Sphere, Add Capsule, or Add Box.";
				}
				if (bOpen && !bLeaf)
				{
					for (const size_t iChild :
						Children[iBone])
					{
						Self(Self, iChild);
					}
					ImGui::TreePop();
				}
				ImGui::PopID();
				DrawStates[iBone] = 2u;
			};

		for (const size_t iRoot : Roots)
			DrawBoneNode(DrawBoneNode, iRoot);
	}
	ImGui::EndChild();

	ImGui::TextUnformatted("Shapes");
	if (ImGui::BeginChild(
		"NvClothCollisionShapes",
		ImVec2{ 0.f, 250.f },
		true))
	{
		for (size_t i = 0;
			i < m_Rig.Shapes.size();
			++i)
		{
			const auto& Shape = m_Rig.Shapes[i];
			const std::string Label =
				Shape.sName + " [" +
				GetShapeTypeName(Shape.eType) +
				"] @ " + Shape.sBoneName +
				"##" + std::to_string(Shape.iID);
			const _bool bSelected =
				m_eSelection ==
					SELECTION_TYPE::SHAPE &&
				m_iSelectedShape ==
					static_cast<int32_t>(i);
			if (ImGui::Selectable(
				Label.c_str(), bSelected))
			{
				m_eSelection =
					SELECTION_TYPE::SHAPE;
				m_iSelectedShape =
					static_cast<int32_t>(i);
				m_iSelectedBone = -1;
			}
		}
	}
	ImGui::EndChild();

	const _bool bCanAdd =
		m_iSelectedBone >= 0;
	if (ImGui::Button("Add Sphere") &&
		bCanAdd)
		AddShape(
			NVCLOTH_COLLISION_SHAPE_TYPE::SPHERE);
	ImGui::SameLine();
	if (ImGui::Button("Add Capsule") &&
		bCanAdd)
		AddShape(
			NVCLOTH_COLLISION_SHAPE_TYPE::CAPSULE);
	ImGui::SameLine();
	if (ImGui::Button("Add Box") &&
		bCanAdd)
		AddShape(
			NVCLOTH_COLLISION_SHAPE_TYPE::BOX);

	if (m_eSelection ==
			SELECTION_TYPE::SHAPE &&
		ImGui::Button("Remove Selected Shape"))
	{
		RemoveSelectedShape();
	}
}

void CNvClothCollisionEditorGUI::DrawInspector()
{
	auto* pShape = GetSelectedShape();
	if (!pShape)
	{
		ImGui::TextDisabled(
			"Select a shape to edit.");
		return;
	}

	_bool bChanged{};
	bChanged |= EditString(
		"Name", pShape->sName);
	bChanged |= ImGui::Checkbox(
		"Enabled", &pShape->bEnabled);

	const char* pCurrentType =
		GetShapeTypeName(pShape->eType);
	if (ImGui::BeginCombo(
		"Type", pCurrentType))
	{
		for (const auto eType : {
			NVCLOTH_COLLISION_SHAPE_TYPE::SPHERE,
			NVCLOTH_COLLISION_SHAPE_TYPE::CAPSULE,
			NVCLOTH_COLLISION_SHAPE_TYPE::BOX })
		{
			if (ImGui::Selectable(
				GetShapeTypeName(eType),
				pShape->eType == eType))
			{
				pShape->eType = eType;
				bChanged = true;
			}
		}
		ImGui::EndCombo();
	}

	if (ImGui::BeginCombo(
		"Bone", pShape->sBoneName.c_str()))
	{
		if (m_pSelectedModel)
		{
			for (const auto& pBone :
				m_pSelectedModel->GetBones())
			{
				if (!pBone)
					continue;
				if (ImGui::Selectable(
					pBone->GetBoneName().c_str(),
					pShape->sBoneName ==
						pBone->GetBoneName()))
				{
					pShape->sBoneName =
						pBone->GetBoneName();
					bChanged = true;
				}
			}
		}
		ImGui::EndCombo();
	}

	bChanged |= ImGui::DragFloat3(
		"Local Position",
		&pShape->vLocalPosition.x,
		0.01f);
	_float3 vEuler =
		QuaternionToEulerDegrees(
			pShape->vLocalRotation);
	if (ImGui::DragFloat3(
		"Local Rotation (Deg)",
		&vEuler.x,
		0.5f))
	{
		pShape->vLocalRotation =
			EulerDegreesToQuaternion(vEuler);
		bChanged = true;
	}

	switch (pShape->eType)
	{
	case NVCLOTH_COLLISION_SHAPE_TYPE::SPHERE:
		bChanged |= ImGui::DragFloat(
			"Radius",
			&pShape->fRadius,
			0.005f,
			MIN_SHAPE_SIZE,
			10.f);
		break;
	case NVCLOTH_COLLISION_SHAPE_TYPE::CAPSULE:
		bChanged |= ImGui::DragFloat(
			"Radius",
			&pShape->fRadius,
			0.005f,
			MIN_SHAPE_SIZE,
			10.f);
		bChanged |= ImGui::DragFloat(
			"Half Height",
			&pShape->fHalfHeight,
			0.005f,
			MIN_SHAPE_SIZE,
			10.f);
		break;
	case NVCLOTH_COLLISION_SHAPE_TYPE::BOX:
		bChanged |= ImGui::DragFloat3(
			"Half Extents",
			&pShape->vHalfExtents.x,
			0.005f,
			MIN_SHAPE_SIZE,
			10.f);
		break;
	}
	bChanged |= ImGui::DragFloat(
		"Collision Margin",
		&pShape->fMargin,
		0.002f,
		0.f,
		1.f);

	ImGui::Separator();
	ImGui::Text(
		"Shape ID: %llu",
		static_cast<unsigned long long>(
			pShape->iID));
	ImGui::TextWrapped(
		"Margin inflates only this shape. "
		"Capsules use local +Y as their axis.");

	if (bChanged)
		m_bDirty = true;
}

void CNvClothCollisionEditorGUI::DrawPreview()
{
	if (!m_bPreviewVisible ||
		!m_pSelectedModel)
	{
		return;
	}

	auto* pDebug =
		CGameInstance::Get().GetDbgLineRender();
	if (!pDebug)
		return;

	const auto PreviousColor = pDebug->GetColor();
	const auto PreviousDepth = pDebug->GetDepthMode();
	pDebug->SetDepthTest(m_bPreviewDepthTest);

	const _matrix PreviewWorld = MakePreviewWorld();
	if (m_pPreviewRenderer)
		m_pPreviewRenderer->SetWorldMatrix(PreviewWorld);

	const auto& Bones =
		m_pSelectedModel->GetBones();
	if (m_bPreviewSkeleton)
	{
		for (size_t i = 0; i < Bones.size(); ++i)
		{
			if (!Bones[i] ||
				i >= m_BindPoses.size())
			{
				continue;
			}
			const _bool bSelected =
				m_eSelection ==
					SELECTION_TYPE::BONE &&
				m_iSelectedBone ==
					static_cast<int32_t>(i);
			pDebug->SetColor(
				bSelected ?
					_float4{
						1.f, 0.9f, 0.1f, 1.f } :
					_float4{
						0.65f, 0.65f, 0.65f, 1.f });
			const _matrix BoneWorld =
				XMLoadFloat4x4(
					&m_BindPoses[i]) *
				PreviewWorld;
			if (bSelected)
			{
				const _float3 vBonePosition =
					GetMatrixPosition(BoneWorld);
				pDebug->AddSphere(
					0.03f,
					XMMatrixTranslation(
						vBonePosition.x,
						vBonePosition.y,
						vBonePosition.z));
			}
			const int32_t iParent =
				Bones[i]->GetParendBoneIndex();
			if (iParent < 0 ||
				static_cast<size_t>(iParent) >=
					m_BindPoses.size())
			{
				continue;
			}
			pDebug->AddLine(
				GetMatrixPosition(
					XMLoadFloat4x4(
						&m_BindPoses[
							static_cast<size_t>(
								iParent)]) *
					PreviewWorld),
				GetMatrixPosition(
					XMLoadFloat4x4(
						&m_BindPoses[i]) *
					PreviewWorld));
		}
	}

	if (m_bPreviewShapes)
	{
		for (size_t i = 0;
			i < m_Rig.Shapes.size();
			++i)
		{
			const auto& Shape = m_Rig.Shapes[i];
			if (!Shape.bEnabled)
				continue;

			_float4x4 BoneWorldFloat{};
			if (!GetBoneRigidWorld(
				Shape.sBoneName,
				BoneWorldFloat))
			{
				continue;
			}
			const _matrix ShapeWorld =
				MakePoseMatrix(
					Shape.vLocalPosition,
					Shape.vLocalRotation) *
				XMLoadFloat4x4(
					&BoneWorldFloat);
			const _bool bSelected =
				m_eSelection ==
					SELECTION_TYPE::SHAPE &&
				m_iSelectedShape ==
					static_cast<int32_t>(i);
			pDebug->SetColor(
				bSelected ?
					_float4{
						1.f, 0.9f, 0.1f, 1.f } :
					_float4{
						0.1f, 0.9f, 1.f, 1.f });

			switch (Shape.eType)
			{
			case NVCLOTH_COLLISION_SHAPE_TYPE::SPHERE:
				pDebug->AddSphere(
					Shape.fRadius +
						Shape.fMargin,
					ShapeWorld);
				break;
			case NVCLOTH_COLLISION_SHAPE_TYPE::CAPSULE:
				pDebug->AddCapsule(
					Shape.fRadius +
						Shape.fMargin,
					Shape.fHalfHeight,
					ShapeWorld);
				break;
			case NVCLOTH_COLLISION_SHAPE_TYPE::BOX:
				pDebug->AddBox(
					{
						Shape.vHalfExtents.x +
							Shape.fMargin,
						Shape.vHalfExtents.y +
							Shape.fMargin,
						Shape.vHalfExtents.z +
							Shape.fMargin
					},
					ShapeWorld);
				break;
			}
		}
	}

	pDebug->SetColor(PreviousColor);
	pDebug->SetDepthMode(PreviousDepth);
}

void CNvClothCollisionEditorGUI::RenderGizmo()
{
	auto* pShape = GetSelectedShape();
	auto* pCamera =
		CGameInstance::Get().GetActiveCamera();
	if (!m_bPreviewVisible ||
		!pShape ||
		!pCamera)
	{
		return;
	}

	_float4x4 BoneWorldFloat{};
	if (!GetBoneRigidWorld(
		pShape->sBoneName,
		BoneWorldFloat))
	{
		return;
	}

	_float3 vDimensions{};
	switch (pShape->eType)
	{
	case NVCLOTH_COLLISION_SHAPE_TYPE::SPHERE:
		vDimensions = {
			pShape->fRadius * 2.f,
			pShape->fRadius * 2.f,
			pShape->fRadius * 2.f
		};
		break;
	case NVCLOTH_COLLISION_SHAPE_TYPE::CAPSULE:
		vDimensions = {
			pShape->fRadius * 2.f,
			(pShape->fHalfHeight +
				pShape->fRadius) * 2.f,
			pShape->fRadius * 2.f
		};
		break;
	case NVCLOTH_COLLISION_SHAPE_TYPE::BOX:
		vDimensions = {
			pShape->vHalfExtents.x * 2.f,
			pShape->vHalfExtents.y * 2.f,
			pShape->vHalfExtents.z * 2.f
		};
		break;
	}

	_float4x4 GizmoWorld{};
	XMStoreFloat4x4(
		&GizmoWorld,
		XMMatrixScaling(
			vDimensions.x,
			vDimensions.y,
			vDimensions.z) *
		MakePoseMatrix(
			pShape->vLocalPosition,
			pShape->vLocalRotation) *
		XMLoadFloat4x4(&BoneWorldFloat));

	_float4x4 View{};
	_float4x4 Projection{};
	XMStoreFloat4x4(&View, pCamera->GetView());
	XMStoreFloat4x4(
		&Projection, pCamera->GetProj());
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
		NVCLOTH_COLLISION_GIZMO_ID);
	if (!ImGuizmo::Manipulate(
		&View._11,
		&Projection._11,
		m_eGizmoOperation,
		ImGuizmo::LOCAL,
		&GizmoWorld._11))
	{
		return;
	}

	const _matrix ShapeLocal =
		XMLoadFloat4x4(&GizmoWorld) *
		XMMatrixInverse(
			nullptr,
			XMLoadFloat4x4(&BoneWorldFloat));
	_vector vScale{};
	_vector qRotation{};
	_vector vTranslation{};
	if (!XMMatrixDecompose(
		&vScale,
		&qRotation,
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
		XMQuaternionNormalize(qRotation));
	_float3 vNewDimensions{};
	XMStoreFloat3(
		&vNewDimensions, vScale);
	vNewDimensions = {
		std::max(
			std::abs(vNewDimensions.x),
			MIN_SHAPE_SIZE),
		std::max(
			std::abs(vNewDimensions.y),
			MIN_SHAPE_SIZE),
		std::max(
			std::abs(vNewDimensions.z),
			MIN_SHAPE_SIZE)
	};
	switch (pShape->eType)
	{
	case NVCLOTH_COLLISION_SHAPE_TYPE::SPHERE:
		pShape->fRadius =
			std::max({
				vNewDimensions.x,
				vNewDimensions.y,
				vNewDimensions.z }) * 0.5f;
		break;
	case NVCLOTH_COLLISION_SHAPE_TYPE::CAPSULE:
		pShape->fRadius = std::max(
			(vNewDimensions.x +
				vNewDimensions.z) * 0.25f,
			MIN_SHAPE_SIZE);
		pShape->fHalfHeight = std::max(
			vNewDimensions.y * 0.5f -
				pShape->fRadius,
			MIN_SHAPE_SIZE);
		break;
	case NVCLOTH_COLLISION_SHAPE_TYPE::BOX:
		pShape->vHalfExtents = {
			vNewDimensions.x * 0.5f,
			vNewDimensions.y * 0.5f,
			vNewDimensions.z * 0.5f
		};
		break;
	}
	m_bDirty = true;
}

void CNvClothCollisionEditorGUI::
DrawFilePopups()
{
	if (ImGui::BeginPopupModal(
		"Confirm Rig Clear",
		nullptr,
		ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::TextUnformatted(
			"Clear the current collision rig?");
		if (ImGui::Button("Clear"))
		{
			Clear();
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel"))
			ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}
	if (ImGui::BeginPopupModal(
		"Confirm Rig Load",
		nullptr,
		ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text(
			"Load %s and replace current data?",
			m_FileName);
		if (ImGui::Button("Load"))
		{
			const HRESULT hr = Load();
			QueueResultPopup(
				m_sStatus,
				SUCCEEDED(hr));
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel"))
			ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}
	if (ImGui::BeginPopupModal(
		"Confirm Rig Save",
		nullptr,
		ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text(
			"Save collision rig as %s?",
			m_FileName);
		if (ImGui::Button("Save"))
		{
			m_ValidationErrors.clear();
			const _bool bValid =
				Validate(m_ValidationErrors);
			const HRESULT hr =
				bValid ? Save() : E_FAIL;
			if (!bValid)
				m_sStatus =
					"Save blocked by validation errors.";
			QueueResultPopup(
				m_sStatus,
				bValid && SUCCEEDED(hr));
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel"))
			ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}
	if (ImGui::BeginPopupModal(
		"NvCloth Collision Rig Result",
		nullptr,
		ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::TextColored(
			m_bResultPopupSuccess ?
				ImVec4{ 0.3f, 0.9f, 0.4f, 1.f } :
				ImVec4{ 1.f, 0.3f, 0.25f, 1.f },
			"%s",
			m_sResultPopupMessage.c_str());
		if (ImGui::Button("OK"))
			ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}
}

void CNvClothCollisionEditorGUI::SelectModel(
	const SPtr<CResModel>& pModel,
	const StringID& sGroupTag,
	const StringID& sResourceTag,
	std::string sLabel)
{
	if (!pModel)
		return;

	if (pModel->GetState() !=
			CResource::STATE::LOADED &&
		FAILED(pModel->Load()))
	{
		m_sStatus =
			"Failed to load selected model.";
		return;
	}

	std::vector<_float4x4> BindPoses{};
	if (!BuildModelBindPoses(
		*pModel,
		BindPoses))
	{
		m_sStatus =
			"Failed to build model bind poses.";
		return;
	}

	auto pPreview =
		CNvClothCollisionPreviewRenderer::Create(
			sGroupTag,
			sResourceTag,
			BindPoses);
	if (!pPreview)
	{
		m_sStatus =
			"Failed to create model preview.";
		return;
	}

	m_pSelectedModel = pModel;
	m_sSelectedModelLabel = std::move(sLabel);
	m_BindPoses = std::move(BindPoses);
	m_pPreviewRenderer = std::move(pPreview);
	if (m_Rig.Shapes.empty())
		m_Rig.sSkeletonTag = pModel->GetPath();
	m_eSelection = SELECTION_TYPE::NONE;
	m_iSelectedBone = -1;
	m_iSelectedShape = -1;
	m_sStatus =
		"Selected model: " +
		pModel->GetPath();
	PlacePreviewAtCamera();
}

_matrix CNvClothCollisionEditorGUI::
MakePreviewWorld() const
{
	return XMMatrixScaling(
		m_fPreviewScale,
		m_fPreviewScale,
		m_fPreviewScale) *
		XMMatrixTranslation(
			m_vPreviewPosition.x,
			m_vPreviewPosition.y,
			m_vPreviewPosition.z);
}

void CNvClothCollisionEditorGUI::
PlacePreviewAtCamera()
{
	auto* pCamera =
		CGameInstance::Get().GetActiveCamera();
	if (!pCamera)
	{
		m_sStatus =
			"Cannot place preview: no active camera.";
		return;
	}

	const _matrix CameraWorld =
		XMMatrixInverse(
			nullptr,
			pCamera->GetView());
	_vector vPosition = CameraWorld.r[3];
	const _vector vForward =
		XMVector3Normalize(CameraWorld.r[2]);
	vPosition += vForward * 5.f;
	XMStoreFloat3(
		&m_vPreviewPosition,
		vPosition);
	m_sStatus =
		"Preview placed 5 meters in front of the active camera.";
}

void CNvClothCollisionEditorGUI::AddShape(
	NVCLOTH_COLLISION_SHAPE_TYPE eType)
{
	if (!m_pSelectedModel ||
		m_iSelectedBone < 0 ||
		static_cast<size_t>(m_iSelectedBone) >=
			m_pSelectedModel->GetBones().size())
	{
		return;
	}
	const auto& pBone =
		m_pSelectedModel->GetBones()[
			static_cast<size_t>(
				m_iSelectedBone)];
	if (!pBone)
		return;

	NVCLOTH_COLLISION_SHAPE_DESC Shape{};
	Shape.iID = m_iNextShapeID++;
	Shape.sName =
		pBone->GetBoneName() +
		GetShapeTypeName(eType);
	Shape.sBoneName = pBone->GetBoneName();
	Shape.eType = eType;
	m_Rig.Shapes.push_back(std::move(Shape));
	m_eSelection = SELECTION_TYPE::SHAPE;
	m_iSelectedShape =
		static_cast<int32_t>(
			m_Rig.Shapes.size() - 1);
	m_iSelectedBone = -1;
	m_bDirty = true;
	m_sStatus =
		"Shape added. Use the inspector or gizmo to edit it.";
}

void CNvClothCollisionEditorGUI::
RemoveSelectedShape()
{
	if (m_iSelectedShape < 0 ||
		static_cast<size_t>(m_iSelectedShape) >=
			m_Rig.Shapes.size())
	{
		return;
	}
	m_Rig.Shapes.erase(
		m_Rig.Shapes.begin() +
		m_iSelectedShape);
	m_iSelectedShape = -1;
	m_eSelection = SELECTION_TYPE::NONE;
	m_bDirty = true;
}

void CNvClothCollisionEditorGUI::Clear()
{
	const std::string sSkeletonTag =
		m_pSelectedModel ?
			m_pSelectedModel->GetPath() :
			std::string{};
	m_Rig = {};
	m_Rig.sSkeletonTag = sSkeletonTag;
	m_iNextShapeID = 1;
	m_eSelection = SELECTION_TYPE::NONE;
	m_iSelectedBone = -1;
	m_iSelectedShape = -1;
	m_ValidationErrors.clear();
	m_bDirty = false;
	m_sStatus =
		"Cleared NvCloth collision rig.";
}

HRESULT CNvClothCollisionEditorGUI::Save() const
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
			m_Rig,
			NVCLOTH_COLLISION_RIG_ROOT);
	auto* pThis = const_cast<
		CNvClothCollisionEditorGUI*>(this);
	pThis->m_sStatus = SUCCEEDED(hr) ?
		"Saved: " + FilePath.generic_string() :
		"Save failed: " +
			FilePath.generic_string();
	if (SUCCEEDED(hr))
		pThis->m_bDirty = false;
	return hr;
}

HRESULT CNvClothCollisionEditorGUI::Load()
{
	const auto FilePath = MakeFilePath();
	if (FilePath.empty() ||
		!std::filesystem::exists(FilePath))
	{
		m_sStatus =
			"File not found: " +
			FilePath.generic_string();
		return E_FAIL;
	}

	NVCLOTH_COLLISION_RIG_DESC Loaded{};
	if (FAILED(
		CGameInstance::Get().JsonDeSerialize(
			FilePath.generic_string(),
			Loaded,
			NVCLOTH_COLLISION_RIG_ROOT)))
	{
		m_sStatus =
			"Load failed: " +
			FilePath.generic_string();
		return E_FAIL;
	}
	if (Loaded.iVersion !=
		NVCLOTH_COLLISION_RIG_VERSION)
	{
		m_sStatus =
			"Unsupported collision rig version.";
		return E_FAIL;
	}

	m_Rig = std::move(Loaded);
	m_iNextShapeID = 1;
	for (const auto& Shape : m_Rig.Shapes)
	{
		m_iNextShapeID = std::max(
			m_iNextShapeID,
			Shape.iID + 1);
	}
	m_eSelection = SELECTION_TYPE::NONE;
	m_iSelectedBone = -1;
	m_iSelectedShape = -1;
	m_bDirty = false;
	m_sStatus =
		"Loaded: " +
		FilePath.generic_string();
	return S_OK;
}

_bool CNvClothCollisionEditorGUI::Validate(
	std::vector<std::string>& Errors) const
{
	Errors.clear();
	if (!m_pSelectedModel)
		Errors.emplace_back(
			"No skeleton model is selected.");
	if (m_Rig.sSkeletonTag.empty())
		Errors.emplace_back(
			"Skeleton tag is empty.");
	if (m_pSelectedModel &&
		m_Rig.sSkeletonTag !=
			m_pSelectedModel->GetPath())
	{
		Errors.emplace_back(
			"Selected model does not match the rig skeleton tag.");
	}

	size_t iSphereCount{};
	size_t iCapsuleCount{};
	size_t iPlaneCount{};
	size_t iConvexCount{};
	std::unordered_set<uint64_t> IDs{};
	for (size_t i = 0;
		i < m_Rig.Shapes.size();
		++i)
	{
		const auto& Shape = m_Rig.Shapes[i];
		const std::string Prefix =
			"Shape " + std::to_string(i) + ": ";
		if (Shape.iID == 0 ||
			!IDs.insert(Shape.iID).second)
		{
			Errors.push_back(
				Prefix +
				"ID is zero or duplicated.");
		}
		if (Shape.sName.empty())
			Errors.push_back(
				Prefix + "name is empty.");
		if (Shape.sBoneName.empty() ||
			!m_pSelectedModel ||
			m_pSelectedModel->Get_BoneIndex(
				Shape.sBoneName.c_str()) < 0)
		{
			Errors.push_back(
				Prefix +
				"bone was not found: " +
				Shape.sBoneName);
		}
		if (!std::isfinite(Shape.fMargin) ||
			Shape.fMargin < 0.f)
		{
			Errors.push_back(
				Prefix + "margin is invalid.");
		}
		if (!Shape.bEnabled)
			continue;

		switch (Shape.eType)
		{
		case NVCLOTH_COLLISION_SHAPE_TYPE::SPHERE:
			++iSphereCount;
			if (!std::isfinite(Shape.fRadius) ||
				Shape.fRadius <= 0.f)
			{
				Errors.push_back(
					Prefix +
						"radius is invalid.");
			}
			break;
		case NVCLOTH_COLLISION_SHAPE_TYPE::CAPSULE:
			iSphereCount += 2;
			++iCapsuleCount;
			if (!std::isfinite(Shape.fRadius) ||
				Shape.fRadius <= 0.f ||
				!std::isfinite(
					Shape.fHalfHeight) ||
				Shape.fHalfHeight < 0.f)
			{
				Errors.push_back(
					Prefix +
						"capsule dimensions are invalid.");
			}
			break;
		case NVCLOTH_COLLISION_SHAPE_TYPE::BOX:
			iPlaneCount += 6;
			++iConvexCount;
			if (Shape.vHalfExtents.x <= 0.f ||
				Shape.vHalfExtents.y <= 0.f ||
				Shape.vHalfExtents.z <= 0.f)
			{
				Errors.push_back(
					Prefix +
						"box extents are invalid.");
			}
			break;
		}
	}
	if (iSphereCount > 32)
		Errors.emplace_back(
			"Enabled shapes require more than 32 NvCloth spheres.");
	if (iCapsuleCount > 32)
		Errors.emplace_back(
			"Enabled shapes require more than 32 NvCloth capsules.");
	if (iPlaneCount > 32)
		Errors.emplace_back(
			"Enabled shapes require more than 32 NvCloth planes (maximum five boxes).");
	if (iConvexCount > 32)
		Errors.emplace_back(
			"Enabled shapes require more than 32 NvCloth convexes.");
	return Errors.empty();
}

std::filesystem::path
CNvClothCollisionEditorGUI::MakeFilePath() const
{
	const std::filesystem::path Input{
		m_FileName
	};
	const std::string sStem =
		Input.stem().string();
	if (sStem.empty())
		return {};
	return std::filesystem::path{
		NVCLOTH_COLLISION_RIG_SAVE_ROOT
	} / (sStem + ".nvclothcollision.json");
}

void CNvClothCollisionEditorGUI::
QueueResultPopup(
	std::string sMessage,
	_bool bSuccess)
{
	m_sResultPopupMessage =
		std::move(sMessage);
	m_bResultPopupSuccess = bSuccess;
	m_bOpenResultPopup = true;
}

NVCLOTH_COLLISION_SHAPE_DESC*
CNvClothCollisionEditorGUI::GetSelectedShape()
{
	if (m_iSelectedShape < 0 ||
		static_cast<size_t>(m_iSelectedShape) >=
			m_Rig.Shapes.size())
	{
		return nullptr;
	}
	return &m_Rig.Shapes[
		static_cast<size_t>(
			m_iSelectedShape)];
}

_bool CNvClothCollisionEditorGUI::
GetBoneRigidWorld(
	std::string_view sBoneName,
	_float4x4& OutWorld) const
{
	if (!m_pSelectedModel)
		return false;
	const int32_t iBone =
		m_pSelectedModel->Get_BoneIndex(
			std::string{ sBoneName }.c_str());
	if (iBone < 0 ||
		static_cast<size_t>(iBone) >=
			m_BindPoses.size())
	{
		return false;
	}

	_matrix RigidWorld{};
	if (!MakeRigidMatrix(
		XMLoadFloat4x4(
			&m_BindPoses[
				static_cast<size_t>(iBone)]) *
		MakePreviewWorld(),
		RigidWorld))
	{
		return false;
	}
	XMStoreFloat4x4(&OutWorld, RigidWorld);
	return true;
}

UPtr<CNvClothCollisionEditorGUI>
CNvClothCollisionEditorGUI::Create()
{
	return ToUPtr(
		new CNvClothCollisionEditorGUI{});
}

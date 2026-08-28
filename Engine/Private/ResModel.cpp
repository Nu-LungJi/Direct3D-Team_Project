#include "pch.h"
#include "ResModel.h"
#include "ResModelMesh.h"
#include "ResModelBone.h"
#include "ResModelMaterial.h"
#include "ResModelAnim.h"
#include <fstream>

NS_USING(Engine)

CResModel::CResModel(const _string& sPath)
	: CResource{ sPath }
{
}

CResModel::~CResModel()
{
}

HRESULT CResModel::Load(const std::any& arg)
{
	auto descArg = std::any_cast<DESC>(&arg);
	if (!descArg)
		return E_FAIL;

	if (m_eState == STATE::LOADED)
	{
		// A resource tag can survive a level reload. Older instances may have
		// been loaded before their split AN_ clips were copied next to the
		// model, so do not permanently keep an empty animation list.
		if (m_Animations.empty())
		{
			if (FAILED(Ready_Animation()))
				return E_FAIL;

			if (!m_Animations.empty() && FAILED(Ready_GPU_Animation()))
			{
				m_Animations.clear();
				return E_FAIL;
			}
		}
		return S_OK;
	}

	m_eState = STATE::LOADING;
	XMStoreFloat4x4(&m_PreTransformMatrix, descArg->PreTransformMatrix);

	{
		if (!std::filesystem::exists(m_sPath))
			return E_FAIL;

		std::ifstream file(m_sPath, std::ios::binary | std::ios::ate);
		if (!file.is_open())
			return E_FAIL;

		size_t size = static_cast<size_t>(file.tellg());
		file.seekg(0, std::ios::beg);

		std::unique_ptr<char[]> buffer = std::make_unique<char[]>(size);
		file.read(buffer.get(), size);

		if (!file)
			return E_FAIL;

		char* ptr = buffer.get();

		MODEL_FILE_HEADER* fh = reinterpret_cast<MODEL_FILE_HEADER*>(ptr);
		ptr += sizeof(MODEL_FILE_HEADER);

		m_iNumMeshes = fh->MeshCount;
		m_iAnimCnt = fh->AnimationCount;
		m_iNumMaterials = fh->MaterialCount;
		m_iNumBones = fh->BoneCount;

		char* base = buffer.get();
		char* end = base + size;

		while (ptr < end)
		{
			CHUCKHEADER* chunk = reinterpret_cast<CHUCKHEADER*>(ptr);
			ptr += sizeof(CHUCKHEADER);

			if (ptr + chunk->size > end)
				return E_FAIL;

			switch (chunk->type)
			{
			case CHUNCK_TYPE::CHUNK_BONE:
				if (FAILED(Ready_Bones(ptr)))
					return E_FAIL;

				if (FAILED(Ready_BoneDepths()))
					return E_FAIL;
				break;

			case CHUNCK_TYPE::CHUNK_MESH:
				if (FAILED(Ready_Meshes(ptr)))
					return E_FAIL;
				break;

			case CHUNCK_TYPE::CHUNK_MATERIAL:
				if (FAILED(Ready_Materials(m_sPath, ptr)))
					return E_FAIL;
				break;
			}

			ptr += chunk->size;
		}
	}

	if (FAILED(Ready_Animation()))
		return E_FAIL;

	if (FAILED(Ready_GPU_Ready()))
		return E_FAIL;
	m_eState = STATE::LOADED;
	return S_OK;
}

HRESULT CResModel::Unload(const std::any& arg)
{

	m_eState = STATE::UNLOAD;
	return S_OK;
}

HRESULT CResModel::Ready_Bones(_char* ptr)
{
	//----------------------------------------------------------------------------
	for (uint32_t i = 0; i < m_iNumBones; ++i) {
		uint32_t consumed = *(uint32_t*)ptr;
		ptr += sizeof(uint32_t);

		auto    pBone = CResModelBone::Create();
		if (nullptr == pBone)
			return E_FAIL;

		E::CResModelBone::DESC pDesc{};
		pDesc.ptr = ptr;
		if (FAILED(pBone->Load(pDesc))) {
			return E_FAIL;
		}

		m_Bones.push_back(pBone);

		ptr += consumed;
	}

	return S_OK;
}

HRESULT CResModel::Ready_Materials(const _string& strModelFilePath, _char* ptr)
{
	m_Materials.resize(m_iNumMaterials);
	for (size_t i = 0; i < m_iNumMaterials; i++)
	{

		uint32_t consumed = *(uint32_t*)ptr;
		ptr += sizeof(uint32_t);

		auto  pMaterial = CResModelMaterial::Create(strModelFilePath);

		if (FAILED(pMaterial->Load(CResModelMaterial::DESC{ .ptr = ptr }))) {
			return E_FAIL;
		}

		m_Materials[pMaterial->GetMaterialTypeNum()] = (pMaterial);


		ptr += consumed;
	}

	return S_OK;
}

HRESULT CResModel::Ready_Meshes(_char* ptr)
{

	for (size_t i = 0; i < m_iNumMeshes; i++)
	{
	
		uint32_t consumed = *(uint32_t*)ptr;
		ptr += sizeof(uint32_t);


		auto    pMesh = CResModelMesh::Create();
		if (nullptr == pMesh)
			return E_FAIL;

		E::CResModelMesh::DESC pDesc{};
		pDesc.eType = m_eModelType;
		pDesc.ptr = ptr;
		pDesc.iRecordSize = consumed;
		pDesc.pModel = this;
		pDesc.PreTransformMatrix = XMLoadFloat4x4(&m_PreTransformMatrix);
	
		if (FAILED(pMesh->Load(pDesc))) {
			return E_FAIL;
		}

		m_Meshes.push_back(pMesh);

		ptr += consumed;
	}

	return S_OK;
}

HRESULT CResModel::Ready_Animation()
{

	std::filesystem::path modelPath(m_sPath);


	std::filesystem::path folderPath = modelPath.parent_path();

	for (const auto& entry : std::filesystem::directory_iterator(folderPath))
	{
		if (!entry.is_regular_file())
			continue;

		const auto& path = entry.path();

		if (path.filename().string().rfind("AN_", 0) != 0)
			continue;

		std::string animPath = path.string();

		if (FAILED(LoadAndAppendSharedAnimation(animPath)))
			return E_FAIL;
	}

	return S_OK;
}

HRESULT CResModel::LoadAndAppendSharedAnimation(
	const _string& sAnimationPath)
{
	const auto animationName = std::filesystem::path{ sAnimationPath }
		.filename().string();

	const auto duplicate = std::ranges::find_if(
		m_Animations,
		[&animationName](const SPtr<CResModelAnim>& pAnimation)
		{
			if (!pAnimation)
				return false;

			return pAnimation->GetAnimName() == animationName;
		});

	if (duplicate != m_Animations.end())
		return S_FALSE;

	auto pAnimation = CGameInstance::Get().GetOrLoadModelAnimation(
		sAnimationPath);
	if (!pAnimation)
		return E_FAIL;

	m_Animations.emplace_back(std::move(pAnimation));
	return S_OK;
}

HRESULT CResModel::Add_SharedAnimation(const _string& sAnimationPath)
{
	const HRESULT result = LoadAndAppendSharedAnimation(sAnimationPath);
	if (FAILED(result) || result == S_FALSE)
		return result;

	if (m_eState == STATE::LOADED && FAILED(Ready_GPU_Animation()))
	{
		m_Animations.pop_back();
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CResModel::Ready_GPU_Ready()
{
	if (FAILED(Ready_GPU_Bone()))
		return E_FAIL;

	if (FAILED(Ready_GPU_Animation()))
		return E_FAIL;

	if (FAILED(Ready_GPU_MeshSkin()))
		return E_FAIL;

	return S_OK;
}
HRESULT CResModel::Ready_GPU_Bone()
{
	std::vector<GPU_BONE_DESC> gpuBones;

	gpuBones.reserve(m_Bones.size());

	for (const auto& pBone : m_Bones)
	{
		if (!pBone)
			return E_FAIL;

		GPU_BONE_DESC gpuBone{};

		gpuBone.BindLocalMatrix =*pBone->Get_TransformationMatrixPtr();

		_vector vScale{};
		_vector vRotation{};
		_vector vTranslation{};
		if (!XMMatrixDecompose(&vScale, &vRotation, &vTranslation,
			XMLoadFloat4x4(&gpuBone.BindLocalMatrix)))
		{
			return E_FAIL;
		}

		XMStoreFloat3(&gpuBone.BindScale, vScale);
		XMStoreFloat4(&gpuBone.BindRotation, XMQuaternionNormalize(vRotation));
		XMStoreFloat3(&gpuBone.BindTranslation, vTranslation);

		gpuBone.iParentBoneIndex =pBone->GetParendBoneIndex();

		gpuBone.iDepth = pBone->Get_Depth();

		gpuBones.push_back(gpuBone);
	}

	auto res = CResStructuredBuffer::Create();

	if (!res)
		return E_FAIL;

	CResStructuredBuffer::DESC desc{};

	desc.iNumElements =static_cast<uint32_t>(gpuBones.size());

	desc.iStructureByteStride = sizeof(GPU_BONE_DESC);

	desc.pInitialData = gpuBones.data();

	desc.bAppendConsume = false;

	if (FAILED(res->Load(desc)))
		return E_FAIL;

	m_pGPUBones = res;

	return S_OK;
}

HRESULT CResModel::Ready_GPU_Animation()
{
	if (m_Animations.empty())
		return S_OK;

	// Animation이 없는 Skeletal Mesh 인 경우
	constexpr uint32_t INVALID_CHANNEL_INDEX = UINT32_MAX;

	const uint32_t iBoneCount = static_cast<uint32_t>(m_Bones.size());

	std::vector<GPU_ANIM_DESC> gpuAnimations;
	std::vector<GPU_CHANNEL_DESC> gpuChannels;
	std::vector<GPU_KEYFRAME_DESC> gpuKeyFrames;
	std::vector<uint32_t> gpuBoneChannelMap;

	gpuAnimations.reserve(m_Animations.size());

	// 애니메이션 전체를 한 번 순회
	for (const auto& pAnimation : m_Animations)
	{
		if (!pAnimation)
			return E_FAIL;

		GPU_ANIM_DESC gpuAnim{};

		gpuAnim.PreTransformMatrix = m_PreTransformMatrix;
		// 이 애니메이션의 Channel이 시작되는 전역 위치
		gpuAnim.iChannelOffset = static_cast<uint32_t>(gpuChannels.size());

		// 이 애니메이션의 BoneChannelMap 시작 위치
		gpuAnim.iBoneChannelMapOffset = static_cast<uint32_t>(gpuBoneChannelMap.size());

		gpuAnim.iBoneCount = iBoneCount;
		gpuAnim.fDuration = pAnimation->GetDuration();

		const auto& channels = pAnimation->GetChannels();

		/*
		 * 현재 애니메이션용 BoneChannelMap 공간 생성
		 *
		 * Bone마다 대응되는 Channel이 없을 수 있으므로
		 * 처음에는 전부 INVALID로 초기화한다.
		 */
		gpuBoneChannelMap.resize(gpuBoneChannelMap.size() + iBoneCount,INVALID_CHANNEL_INDEX);

		// 현재 애니메이션의 채널 순회
		for (const auto& pChannel : channels)
		{
			if (!pChannel)
				return E_FAIL;

			const int32_t iBoneIndex = pChannel->Get_BoneIndex();
			if (iBoneIndex < 0 ||
				static_cast<uint32_t>(iBoneIndex) >= iBoneCount)
			{
				// [LSY] 공유 Clip에만 존재하는 Bone 채널은 이 모델에서 사용하지 않는다.
				// CPU Animator도 동일하게 범위 밖 채널을 건너뛴다.
				continue;
			}

			GPU_CHANNEL_DESC gpuChannel{};

			gpuChannel.iBoneIndex = static_cast<uint32_t>(iBoneIndex);

			// 이 채널의 KeyFrame 시작 위치
			gpuChannel.iKeyFrameOffset = static_cast<uint32_t>(gpuKeyFrames.size());

			const auto& keyFrames = pChannel->Get_KeyFrames();

			gpuChannel.iKeyFrameCount = static_cast<uint32_t>(keyFrames.size());

			const uint32_t iGlobalChannelIndex = static_cast<uint32_t>(gpuChannels.size());

			/*
			 * Anim + Bone으로 Channel을 바로 찾을 수 있게 기록
			 */
			const uint32_t iMapIndex =gpuAnim.iBoneChannelMapOffset + gpuChannel.iBoneIndex;

			gpuBoneChannelMap[iMapIndex] = iGlobalChannelIndex;

			// 현재 Channel의 KeyFrame 순회
			for (const auto& keyFrame : keyFrames)
			{
				GPU_KEYFRAME_DESC gpuKeyFrame{};

				gpuKeyFrame.vScale = keyFrame.vScale;

				gpuKeyFrame.vRotation = keyFrame.vRotation;

				gpuKeyFrame.vTranslation = keyFrame.vTranslation;

				gpuKeyFrame.fTrackPosition = keyFrame.fTrackPosition;

				gpuKeyFrames.push_back(gpuKeyFrame);
			}

			gpuChannels.push_back(gpuChannel);
		}

		gpuAnim.iChannelCount =
			static_cast<uint32_t>(gpuChannels.size()) -
			gpuAnim.iChannelOffset;

		gpuAnimations.push_back(gpuAnim);
	}

	// 모든 채널이 현재 모델의 Bone 범위를 벗어나도 빈 D3D11 Buffer 생성으로
	// 실패하지 않게 더미 원소를 둔다. BoneChannelMap은 INVALID이므로 접근되지 않는다.
	if (gpuChannels.empty())
		gpuChannels.emplace_back();
	if (gpuKeyFrames.empty())
		gpuKeyFrames.emplace_back();

	// 여기까지 오면 CPU 평탄화 데이터가 모두 완성된 상태
	// 이제 각각 Structured Buffer로 올린다.
	{
		auto res = CResStructuredBuffer::Create();

		if (!res)
			return E_FAIL;

		CResStructuredBuffer::DESC desc{};

		desc.iNumElements = static_cast<uint32_t>(gpuAnimations.size());

		desc.iStructureByteStride = sizeof(GPU_ANIM_DESC);

		desc.pInitialData = gpuAnimations.data();

		desc.bAppendConsume = false;

		if (FAILED(res->Load(desc)))
			return E_FAIL;
	
		m_pGPUAnimations = res;
}

	{
		auto res = CResStructuredBuffer::Create();

		if (!res)
			return E_FAIL;

		CResStructuredBuffer::DESC desc{};

		desc.iNumElements =static_cast<uint32_t>(gpuChannels.size());

		desc.iStructureByteStride =sizeof(GPU_CHANNEL_DESC);

		desc.pInitialData = gpuChannels.data();

		desc.bAppendConsume = false;

		if (FAILED(res->Load(desc)))
			return E_FAIL;

		m_pGPUChannels = res;

	}

	{
		auto res = CResStructuredBuffer::Create();

		if (!res)
			return E_FAIL;

		CResStructuredBuffer::DESC desc{};

		desc.iNumElements = static_cast<uint32_t>(gpuKeyFrames.size());

		desc.iStructureByteStride = sizeof(GPU_KEYFRAME_DESC);

		desc.pInitialData = gpuKeyFrames.data();

		desc.bAppendConsume = false;

		if (FAILED(res->Load(desc)))
			return E_FAIL;

		m_pGPUKeyFrames = res;
	}

	{
		auto res = CResStructuredBuffer::Create();

		if (!res)
			return E_FAIL;

		CResStructuredBuffer::DESC desc{};

		desc.iNumElements =static_cast<uint32_t>(gpuBoneChannelMap.size());

		desc.iStructureByteStride =sizeof(uint32_t);

		desc.pInitialData = gpuBoneChannelMap.data();

		desc.bAppendConsume = false;

		if (FAILED(res->Load(desc)))
			return E_FAIL;

		m_pGPUBoneChannelMap = res;
	}

	return S_OK;
}
HRESULT CResModel::Ready_BoneDepths()
{
	const uint32_t iBoneCount =static_cast<uint32_t>(m_Bones.size());

	std::vector<int32_t> depthCache(iBoneCount,-1);

	std::vector<bool> visiting(iBoneCount,false);

	m_iMaxBoneDepth = 0;

	for (uint32_t i = 0;i < iBoneCount;++i)
	{
		uint32_t iDepth = 0;

		if (FAILED(Calculate_BoneDepth(i,depthCache,visiting,iDepth)))
		{
			return E_FAIL;
		}

		m_Bones[i]->Set_Depth(iDepth);

		m_iMaxBoneDepth =std::max(m_iMaxBoneDepth,iDepth);
	}


	return S_OK;
}

HRESULT CResModel::Ready_GPU_MeshSkin()
{
	std::vector<GPU_SKIN_BONE_DESC> gpuSkinBones;
	std::vector<GPU_MESH_SKIN_RANGE> gpuMeshSkinRanges;

	gpuMeshSkinRanges.reserve(m_Meshes.size());

	for (const auto& pMesh : m_Meshes)
	{
		if (!pMesh)
			return E_FAIL;

		GPU_MESH_SKIN_RANGE meshRange{};

		meshRange.iSkinBoneOffset =static_cast<uint32_t>(gpuSkinBones.size());

		const auto& boneIndices = pMesh->GetBoneIndices();

		const auto& offsetMatrices =pMesh->GetOffsetMatrices();

		if (boneIndices.size() != offsetMatrices.size())
			return E_FAIL;

		if (boneIndices.size() != pMesh->Get_BoneIndex())
			return E_FAIL;

		meshRange.iSkinBoneCount = static_cast<uint32_t>(boneIndices.size());

		for (uint32_t i = 0; i < meshRange.iSkinBoneCount;++i)
		{
			if (boneIndices[i] >= m_Bones.size())
				return E_FAIL;

			GPU_SKIN_BONE_DESC gpuSkinBone{};

			gpuSkinBone.iSkeletonBoneIndex = boneIndices[i];

			gpuSkinBone.OffsetMatrix = offsetMatrices[i];

			gpuSkinBones.push_back(gpuSkinBone);
		}

		gpuMeshSkinRanges.push_back(meshRange);
	}

	// 이후 gpuSkinBones와 gpuMeshSkinRanges를
	// 각각 Structured Buffer로 생성
	if (!gpuSkinBones.empty())
	{
		auto res = CResStructuredBuffer::Create();
		if (!res)
			return E_FAIL;

		CResStructuredBuffer::DESC desc{};
		desc.iNumElements =
			static_cast<uint32_t>(gpuSkinBones.size());
		desc.iStructureByteStride =
			sizeof(GPU_SKIN_BONE_DESC);
		desc.pInitialData =
			gpuSkinBones.data();
		desc.bAppendConsume = false;

		if (FAILED(res->Load(desc)))
			return E_FAIL;

		m_pGPUSkinBones = res;
	}

	if (!gpuMeshSkinRanges.empty())
	{
		auto res = CResStructuredBuffer::Create();
		if (!res)
			return E_FAIL;

		CResStructuredBuffer::DESC desc{};
		desc.iNumElements =
			static_cast<uint32_t>(gpuMeshSkinRanges.size());
		desc.iStructureByteStride =
			sizeof(GPU_MESH_SKIN_RANGE);
		desc.pInitialData =
			gpuMeshSkinRanges.data();
		desc.bAppendConsume = false;

		if (FAILED(res->Load(desc)))
			return E_FAIL;

		m_pGPUMeshSkinRanges = res;
		m_GPUMeshSkinRanges = std::move(gpuMeshSkinRanges);
	}

	return S_OK;
}



HRESULT CResModel::Calculate_BoneDepth(uint32_t iBoneIndex,std::vector<int32_t>& depthCache,std::vector<bool>& visiting,uint32_t& outDepth)
{
	// 본 인덱스 예외 처리
	if (iBoneIndex >= m_Bones.size())
		return E_FAIL;

	if (!m_Bones[iBoneIndex])
		return E_FAIL;

	// 이미 계산됨
	if (depthCache[iBoneIndex] >= 0)
	{
		outDepth =static_cast<uint32_t>(depthCache[iBoneIndex]);

		return S_OK;
	}

	// 현재 계산 중인 본을 다시 방문했다면 순환 계층
	if (visiting[iBoneIndex])
		return E_FAIL;

	visiting[iBoneIndex] = true;

	const int32_t iParentBoneIndex = m_Bones[iBoneIndex]->GetParendBoneIndex();

	uint32_t iDepth = 0;

	if (iParentBoneIndex >= 0)
	{
		if (iParentBoneIndex >=static_cast<int32_t>(m_Bones.size()))
		{
			return E_FAIL;
		}

		if (iParentBoneIndex ==static_cast<int32_t>(iBoneIndex))
		{
			return E_FAIL;
		}

		uint32_t iParentDepth = 0;

		if (FAILED(Calculate_BoneDepth(static_cast<uint32_t>(iParentBoneIndex),depthCache,visiting,iParentDepth)))
		{
			return E_FAIL;
		}

		iDepth = iParentDepth + 1;
	}

	visiting[iBoneIndex] = false;

	depthCache[iBoneIndex] = static_cast<int32_t>(iDepth);

	outDepth = iDepth;

	return S_OK;
}
int32_t CResModel::Get_BoneIndex(const _char* pBoneName) const
{
	int32_t iBoneIndex = { 0 };
	auto    iter = find_if(m_Bones.begin(), m_Bones.end(), [&](SPtr<CResModelBone> pBone)->_bool
		{
			if (true == pBone->Compare_Name(pBoneName))
				return true;

			++iBoneIndex;

			return false;
		});

	if (iter == m_Bones.end())
		return -1;

	return iBoneIndex;
}

const _float4x4* CResModel::Get_BoneMatrixPtr(const _char* pBoneName)
{
	auto    iter = find_if(m_Bones.begin(), m_Bones.end(), [&](SPtr<CResModelBone> pBone)->_bool
		{
			if (true == pBone->Compare_Name(pBoneName))
				return true;

			return false;
		});

	if (iter == m_Bones.end())
		return nullptr;

	return (*iter)->Get_CombinedTransformationMatrixPtr();
}

SPtr<CResModel> CResModel::Create(const _string& sPath)
{
	return ToSPtr(new CResModel{ sPath });
}


#include "pch.h"
#include "ResNvClothMesh.h"

#include "GameInstance.h"

#include <cmath>
#include <fstream>
#include <unordered_map>

NS_USING(Engine)

namespace
{
	struct SOURCE_BONE
	{
		_string sName{};
		_float4x4 Local{};
		int32_t iParent{ -1 };
	};

	struct SOURCE_MESH
	{
		uint32_t iMaterialIndex{};
		std::vector<VTXANIMMESH> Vertices{};
		std::vector<uint32_t> Indices{};
		std::vector<uint32_t> BoneIndices{};
		std::vector<_float4x4> OffsetMatrices{};
	};

	struct NVCLOTH_RENDER_VERTEX
	{
		_float2 vTexcoord{};
		DirectX::XMUINT3 vParticleIndices{};
		_float3 vBarycentric{};
		_float3 vPositionOffset{};
		_float3 vNormalFrame{};
		_float4 vTangentFrame{};
	};

	static_assert(
		offsetof(
			NVCLOTH_RENDER_VERTEX,
			vParticleIndices) == sizeof(_float2));
	static_assert(sizeof(NVCLOTH_RENDER_VERTEX) == 72);

	struct WELD_KEY
	{
		int64_t x{};
		int64_t y{};
		int64_t z{};

		bool operator==(const WELD_KEY&) const = default;
	};

	struct WELD_KEY_HASH
	{
		size_t operator()(const WELD_KEY& Key) const noexcept
		{
			size_t Seed = std::hash<int64_t>{}(Key.x);
			Seed ^= std::hash<int64_t>{}(Key.y) +
				0x9e3779b9u + (Seed << 6) + (Seed >> 2);
			Seed ^= std::hash<int64_t>{}(Key.z) +
				0x9e3779b9u + (Seed << 6) + (Seed >> 2);
			return Seed;
		}
	};

	template<typename T>
	bool ReadValue(
		const std::byte*& pCursor,
		const std::byte* pEnd,
		T& OutValue)
	{
		if (static_cast<size_t>(pEnd - pCursor) < sizeof(T))
			return false;

		memcpy(&OutValue, pCursor, sizeof(T));
		pCursor += sizeof(T);
		return true;
	}

	bool ReadBytes(
		const std::byte*& pCursor,
		const std::byte* pEnd,
		void* pOutData,
		size_t iByteCount)
	{
		if (iByteCount > static_cast<size_t>(pEnd - pCursor))
			return false;

		if (iByteCount != 0)
			memcpy(pOutData, pCursor, iByteCount);
		pCursor += iByteCount;
		return true;
	}

	template<typename T>
	bool ReadVector(
		const std::byte*& pCursor,
		const std::byte* pEnd,
		uint32_t iCount,
		std::vector<T>& OutValues)
	{
		if (iCount >
			static_cast<size_t>(pEnd - pCursor) / sizeof(T))
		{
			return false;
		}

		OutValues.resize(iCount);
		return ReadBytes(
			pCursor,
			pEnd,
			OutValues.data(),
			static_cast<size_t>(iCount) * sizeof(T));
	}

	bool LoadSourceModel(
		const _string& sPath,
		std::vector<SOURCE_BONE>& OutBones,
		std::vector<SOURCE_MESH>& OutMeshes)
	{
		std::ifstream File(
			sPath,
			std::ios::binary | std::ios::ate);
		if (!File)
			return false;

		const auto iFileSize = File.tellg();
		if (iFileSize <= 0 ||
			static_cast<uint64_t>(iFileSize) >
				std::numeric_limits<size_t>::max())
		{
			return false;
		}

		File.seekg(0, std::ios::beg);
		std::vector<std::byte> Bytes(
			static_cast<size_t>(iFileSize));
		if (!File.read(
			reinterpret_cast<char*>(Bytes.data()),
			iFileSize))
		{
			return false;
		}

		const std::byte* pCursor = Bytes.data();
		const std::byte* pEnd = pCursor + Bytes.size();
		MODEL_FILE_HEADER Header{};
		if (!ReadValue(pCursor, pEnd, Header) ||
			!Header.bHasBone ||
			Header.MeshCount == 0 ||
			Header.MeshCount > 4096u ||
			Header.BoneCount == 0 ||
			Header.BoneCount > 4096u)
		{
			return false;
		}

		while (pCursor < pEnd)
		{
			CHUCKHEADER Chunk{};
			if (!ReadValue(pCursor, pEnd, Chunk) ||
				Chunk.size > static_cast<size_t>(pEnd - pCursor))
			{
				return false;
			}

			const std::byte* pChunkEnd = pCursor + Chunk.size;
			if (Chunk.type == CHUNCK_TYPE::CHUNK_BONE)
			{
				OutBones.clear();
				OutBones.reserve(Header.BoneCount);
				for (uint32_t i = 0; i < Header.BoneCount; ++i)
				{
					uint32_t iRecordSize{};
					if (!ReadValue(
						pCursor,
						pChunkEnd,
						iRecordSize) ||
						iRecordSize >
							static_cast<size_t>(
								pChunkEnd - pCursor))
					{
						return false;
					}

					const std::byte* pRecordEnd =
						pCursor + iRecordSize;
					uint32_t iNameLength{};
					if (!ReadValue(
						pCursor,
						pRecordEnd,
						iNameLength) ||
						iNameLength >
							static_cast<size_t>(
								pRecordEnd - pCursor))
					{
						return false;
					}
					SOURCE_BONE Bone{};
					Bone.sName.resize(iNameLength);
					if (!ReadBytes(
						pCursor,
						pRecordEnd,
						Bone.sName.data(),
						iNameLength))
					{
						return false;
					}

					uint32_t iParent{};
					if (!ReadValue(
						pCursor,
						pRecordEnd,
						Bone.Local) ||
						!ReadValue(
							pCursor,
							pRecordEnd,
							iParent))
					{
						return false;
					}

					XMStoreFloat4x4(
						&Bone.Local,
						XMMatrixTranspose(
							XMLoadFloat4x4(&Bone.Local)));
					Bone.iParent =
						static_cast<int32_t>(iParent);
					OutBones.push_back(Bone);
					pCursor = pRecordEnd;
				}
			}
			else if (Chunk.type == CHUNCK_TYPE::CHUNK_MESH)
			{
				OutMeshes.clear();
				OutMeshes.reserve(Header.MeshCount);
				for (uint32_t i = 0; i < Header.MeshCount; ++i)
				{
					uint32_t iRecordSize{};
					if (!ReadValue(
						pCursor,
						pChunkEnd,
						iRecordSize) ||
						iRecordSize >
							static_cast<size_t>(
								pChunkEnd - pCursor))
					{
						return false;
					}

					const std::byte* pRecordEnd =
						pCursor + iRecordSize;
					SOURCE_MESH Mesh{};
					uint32_t iVertexCount{};
					uint32_t iIndexCount{};
					if (!ReadValue(
						pCursor,
						pRecordEnd,
						Mesh.iMaterialIndex) ||
						!ReadValue(
							pCursor,
							pRecordEnd,
							iVertexCount) ||
						!ReadValue(
							pCursor,
							pRecordEnd,
							iIndexCount) ||
						iVertexCount < 3 ||
						iIndexCount < 3 ||
						iIndexCount % 3 != 0 ||
						!ReadVector(
							pCursor,
							pRecordEnd,
							iVertexCount,
							Mesh.Vertices) ||
						!ReadVector(
							pCursor,
							pRecordEnd,
							iIndexCount,
							Mesh.Indices))
					{
						return false;
					}

					uint32_t iNumBones{};
					uint32_t iBoneIndexCount{};
					uint32_t iBoneMatrixCount{};
					uint32_t iOffsetMatrixCount{};
					if (!ReadValue(
						pCursor,
						pRecordEnd,
						iNumBones) ||
						!ReadValue(
							pCursor,
							pRecordEnd,
							iBoneIndexCount) ||
						!ReadValue(
							pCursor,
							pRecordEnd,
							iBoneMatrixCount) ||
						!ReadValue(
							pCursor,
							pRecordEnd,
							iOffsetMatrixCount) ||
						iNumBones != iBoneIndexCount ||
						iNumBones != iOffsetMatrixCount ||
						!ReadVector(
							pCursor,
							pRecordEnd,
							iBoneIndexCount,
							Mesh.BoneIndices))
					{
						return false;
					}

					const size_t iBoneMatrixBytes =
						static_cast<size_t>(
							iBoneMatrixCount) *
						sizeof(_float4x4);
					if (iBoneMatrixBytes >
						static_cast<size_t>(
							pRecordEnd - pCursor))
					{
						return false;
					}
					pCursor += iBoneMatrixBytes;

					if (!ReadVector(
						pCursor,
						pRecordEnd,
						iOffsetMatrixCount,
						Mesh.OffsetMatrices))
					{
						return false;
					}

					for (const auto iBoneIndex :
						Mesh.BoneIndices)
					{
						if (iBoneIndex >= Header.BoneCount)
							return false;
					}
					for (const auto iIndex : Mesh.Indices)
					{
						if (iIndex >= Mesh.Vertices.size())
							return false;
					}

					OutMeshes.push_back(std::move(Mesh));
					pCursor = pRecordEnd;
				}
			}

			pCursor = pChunkEnd;
		}

		return OutBones.size() == Header.BoneCount &&
			OutMeshes.size() == Header.MeshCount;
	}

	bool BuildCombinedBones(
		const std::vector<SOURCE_BONE>& Bones,
		_fmatrix PreTransform,
		std::vector<_float4x4>& OutCombined)
	{
		OutCombined.resize(Bones.size());
		for (size_t i = 0; i < Bones.size(); ++i)
		{
			const auto& Bone = Bones[i];
			const _matrix Local =
				XMLoadFloat4x4(&Bone.Local);
			if (Bone.iParent < 0)
			{
				XMStoreFloat4x4(
					&OutCombined[i],
					Local * PreTransform);
				continue;
			}

			if (static_cast<size_t>(Bone.iParent) >= i)
				return false;

			XMStoreFloat4x4(
				&OutCombined[i],
				Local *
				XMLoadFloat4x4(
					&OutCombined[Bone.iParent]));
		}
		return true;
	}

	bool SkinVertex(
		const VTXANIMMESH& Source,
		const SOURCE_MESH& Mesh,
		const std::vector<_float4x4>& CombinedBones,
		VTXMESH& OutVertex)
	{
		const uint32_t Indices[4]{
			Source.vBlendIndices.x,
			Source.vBlendIndices.y,
			Source.vBlendIndices.z,
			Source.vBlendIndices.w
		};
		const float Weights[4]{
			Source.vBlendWeights.x,
			Source.vBlendWeights.y,
			Source.vBlendWeights.z,
			std::max(
				0.f,
				1.f -
				Source.vBlendWeights.x -
				Source.vBlendWeights.y -
				Source.vBlendWeights.z)
		};

		_vector vPosition = XMVectorZero();
		_vector vNormal = XMVectorZero();
		_vector vTangent = XMVectorZero();
		_vector vBinormal = XMVectorZero();
		float fTotalWeight{};
		for (uint32_t i = 0; i < 4; ++i)
		{
			if (Weights[i] <= 0.f)
				continue;
			if (Indices[i] >= Mesh.BoneIndices.size())
				return false;

			const auto iSkeletonBone =
				Mesh.BoneIndices[Indices[i]];
			if (iSkeletonBone >= CombinedBones.size())
				return false;

			const _matrix SkinMatrix =
				XMLoadFloat4x4(
					&Mesh.OffsetMatrices[Indices[i]]) *
				XMLoadFloat4x4(
					&CombinedBones[iSkeletonBone]);
			vPosition += XMVector3TransformCoord(
				XMLoadFloat3(&Source.vPosition),
				SkinMatrix) * Weights[i];
			vNormal += XMVector3TransformNormal(
				XMLoadFloat3(&Source.vNormal),
				SkinMatrix) * Weights[i];
			vTangent += XMVector3TransformNormal(
				XMLoadFloat3(&Source.vTangent),
				SkinMatrix) * Weights[i];
			vBinormal += XMVector3TransformNormal(
				XMLoadFloat3(&Source.vBinormal),
				SkinMatrix) * Weights[i];
			fTotalWeight += Weights[i];
		}

		if (fTotalWeight <= FLT_EPSILON)
			return false;

		vPosition /= fTotalWeight;
		vNormal = XMVector3Normalize(vNormal);
		vTangent = XMVector3Normalize(vTangent);
		vBinormal = XMVector3Normalize(vBinormal);
		XMStoreFloat3(&OutVertex.vPosition, vPosition);
		XMStoreFloat3(&OutVertex.vNormal, vNormal);
		XMStoreFloat3(&OutVertex.vTangent, vTangent);
		XMStoreFloat3(&OutVertex.vBinormal, vBinormal);
		OutVertex.vTexcoord = Source.vTexcoord;
		return true;
	}

	bool MakeRigidMatrix(
		_fmatrix Matrix,
		_matrix& OutRigidMatrix)
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

		qRotation = XMQuaternionNormalize(qRotation);
		OutRigidMatrix =
			XMMatrixRotationQuaternion(qRotation) *
			XMMatrixTranslationFromVector(vTranslation);
		return true;
	}

	void TransformVertex(
		VTXMESH& Vertex,
		_fmatrix Matrix)
	{
		XMStoreFloat3(
			&Vertex.vPosition,
			XMVector3TransformCoord(
				XMLoadFloat3(&Vertex.vPosition),
				Matrix));
		XMStoreFloat3(
			&Vertex.vNormal,
			XMVector3Normalize(
				XMVector3TransformNormal(
					XMLoadFloat3(&Vertex.vNormal),
					Matrix)));
		XMStoreFloat3(
			&Vertex.vTangent,
			XMVector3Normalize(
				XMVector3TransformNormal(
					XMLoadFloat3(&Vertex.vTangent),
					Matrix)));
		XMStoreFloat3(
			&Vertex.vBinormal,
			XMVector3Normalize(
				XMVector3TransformNormal(
					XMLoadFloat3(&Vertex.vBinormal),
					Matrix)));
	}

	WELD_KEY MakeWeldKey(
		const _float3& Position,
		float fTolerance)
	{
		const double fScale = 1.0 /
			static_cast<double>(fTolerance);
		return {
			static_cast<int64_t>(
				std::llround(Position.x * fScale)),
			static_cast<int64_t>(
				std::llround(Position.y * fScale)),
			static_cast<int64_t>(
				std::llround(Position.z * fScale))
		};
	}

	float Dot(
		const _float3& A,
		const _float3& B)
	{
		return A.x * B.x +
			A.y * B.y +
			A.z * B.z;
	}

	_float3 Subtract(
		const _float3& A,
		const _float3& B)
	{
		return {
			A.x - B.x,
			A.y - B.y,
			A.z - B.z
		};
	}

	_float3 AddScaled(
		const _float3& A,
		const _float3& B,
		float fScale)
	{
		return {
			A.x + B.x * fScale,
			A.y + B.y * fScale,
			A.z + B.z * fScale
		};
	}

	bool BuildTriangleFrame(
		const _float3& P0,
		const _float3& P1,
		const _float3& P2,
		_float3& OutTangent,
		_float3& OutBinormal,
		_float3& OutNormal)
	{
		const _vector vEdge0 =
			XMLoadFloat3(&P1) -
			XMLoadFloat3(&P0);
		const _vector vEdge1 =
			XMLoadFloat3(&P2) -
			XMLoadFloat3(&P0);
		const float fEdgeLengthSq =
			XMVectorGetX(
				XMVector3LengthSq(vEdge0));
		const _vector vCross =
			XMVector3Cross(vEdge0, vEdge1);
		const float fCrossLengthSq =
			XMVectorGetX(
				XMVector3LengthSq(vCross));
		if (!std::isfinite(fEdgeLengthSq) ||
			!std::isfinite(fCrossLengthSq) ||
			fEdgeLengthSq <= 1.e-12f ||
			fCrossLengthSq <= 1.e-16f)
		{
			return false;
		}

		const _vector vTangent =
			XMVector3Normalize(vEdge0);
		const _vector vNormal =
			XMVector3Normalize(vCross);
		const _vector vBinormal =
			XMVector3Normalize(
				XMVector3Cross(
					vNormal,
					vTangent));
		XMStoreFloat3(
			&OutTangent,
			vTangent);
		XMStoreFloat3(
			&OutBinormal,
			vBinormal);
		XMStoreFloat3(
			&OutNormal,
			vNormal);
		return true;
	}

	bool ClosestPointOnTriangle(
		const _float3& Point,
		const _float3& P0,
		const _float3& P1,
		const _float3& P2,
		_float3& OutPoint,
		_float3& OutBarycentric)
	{
		_float3 FrameTangent{};
		_float3 FrameBinormal{};
		_float3 FrameNormal{};
		if (!BuildTriangleFrame(
			P0,
			P1,
			P2,
			FrameTangent,
			FrameBinormal,
			FrameNormal))
		{
			return false;
		}

		const _float3 AB = Subtract(P1, P0);
		const _float3 AC = Subtract(P2, P0);
		const _float3 AP = Subtract(Point, P0);
		const float d1 = Dot(AB, AP);
		const float d2 = Dot(AC, AP);
		if (d1 <= 0.f && d2 <= 0.f)
		{
			OutPoint = P0;
			OutBarycentric = { 1.f, 0.f, 0.f };
			return true;
		}

		const _float3 BP = Subtract(Point, P1);
		const float d3 = Dot(AB, BP);
		const float d4 = Dot(AC, BP);
		if (d3 >= 0.f && d4 <= d3)
		{
			OutPoint = P1;
			OutBarycentric = { 0.f, 1.f, 0.f };
			return true;
		}

		const float fVC = d1 * d4 - d3 * d2;
		if (fVC <= 0.f && d1 >= 0.f && d3 <= 0.f)
		{
			const float fV = d1 / (d1 - d3);
			OutPoint = AddScaled(P0, AB, fV);
			OutBarycentric = {
				1.f - fV,
				fV,
				0.f
			};
			return true;
		}

		const _float3 CP = Subtract(Point, P2);
		const float d5 = Dot(AB, CP);
		const float d6 = Dot(AC, CP);
		if (d6 >= 0.f && d5 <= d6)
		{
			OutPoint = P2;
			OutBarycentric = { 0.f, 0.f, 1.f };
			return true;
		}

		const float fVB = d5 * d2 - d1 * d6;
		if (fVB <= 0.f && d2 >= 0.f && d6 <= 0.f)
		{
			const float fW = d2 / (d2 - d6);
			OutPoint = AddScaled(P0, AC, fW);
			OutBarycentric = {
				1.f - fW,
				0.f,
				fW
			};
			return true;
		}

		const float fVA =
			d3 * d6 - d5 * d4;
		if (fVA <= 0.f &&
			d4 - d3 >= 0.f &&
			d5 - d6 >= 0.f)
		{
			const _float3 BC = Subtract(P2, P1);
			const float fW =
				(d4 - d3) /
				((d4 - d3) + (d5 - d6));
			OutPoint = AddScaled(P1, BC, fW);
			OutBarycentric = {
				0.f,
				1.f - fW,
				fW
			};
			return true;
		}

		const float fDenominator =
			fVA + fVB + fVC;
		if (!std::isfinite(fDenominator) ||
			std::fabs(fDenominator) <=
				FLT_EPSILON)
		{
			return false;
		}

		const float fInverseDenominator =
			1.f / fDenominator;
		const float fV =
			fVB * fInverseDenominator;
		const float fW =
			fVC * fInverseDenominator;
		OutPoint =
			AddScaled(
				AddScaled(P0, AB, fV),
				AC,
				fW);
		OutBarycentric = {
			1.f - fV - fW,
			fV,
			fW
		};
		return true;
	}

	_float3 ProjectToFrame(
		const _float3& Vector,
		const _float3& Tangent,
		const _float3& Binormal,
		const _float3& Normal)
	{
		return {
			Dot(Vector, Tangent),
			Dot(Vector, Binormal),
			Dot(Vector, Normal)
		};
	}

	bool BuildRenderVertexMapping(
		const VTXMESH& RenderVertex,
		const NVCLOTH_FABRIC_DESC& Fabric,
		NVCLOTH_RENDER_VERTEX& OutVertex,
		float& OutDistance)
	{
		float fBestDistanceSq =
			std::numeric_limits<float>::max();
		uint32_t iBestTriangle =
			std::numeric_limits<uint32_t>::max();
		_float3 vBestPoint{};
		_float3 vBestBarycentric{};

		for (uint32_t i = 0;
			i + 2 < Fabric.vecIndices.size();
			i += 3)
		{
			const auto i0 = Fabric.vecIndices[i];
			const auto i1 = Fabric.vecIndices[i + 1];
			const auto i2 = Fabric.vecIndices[i + 2];
			const auto& P0 = Fabric.vecPositions[i0];
			const auto& P1 = Fabric.vecPositions[i1];
			const auto& P2 = Fabric.vecPositions[i2];

			_float3 vClosest{};
			_float3 vBarycentric{};
			if (!ClosestPointOnTriangle(
				RenderVertex.vPosition,
				P0,
				P1,
				P2,
				vClosest,
				vBarycentric))
			{
				continue;
			}

			const _float3 vDifference =
				Subtract(
					RenderVertex.vPosition,
					vClosest);
			const float fDistanceSq =
				Dot(vDifference, vDifference);
			if (fDistanceSq < fBestDistanceSq)
			{
				fBestDistanceSq = fDistanceSq;
				iBestTriangle = i;
				vBestPoint = vClosest;
				vBestBarycentric =
					vBarycentric;
			}
		}

		if (iBestTriangle ==
			std::numeric_limits<uint32_t>::max())
		{
			return false;
		}

		const auto i0 =
			Fabric.vecIndices[iBestTriangle];
		const auto i1 =
			Fabric.vecIndices[iBestTriangle + 1];
		const auto i2 =
			Fabric.vecIndices[iBestTriangle + 2];
		_float3 vTangent{};
		_float3 vBinormal{};
		_float3 vNormal{};
		if (!BuildTriangleFrame(
			Fabric.vecPositions[i0],
			Fabric.vecPositions[i1],
			Fabric.vecPositions[i2],
			vTangent,
			vBinormal,
			vNormal))
		{
			return false;
		}

		const _float3 vPositionDelta =
			Subtract(
				RenderVertex.vPosition,
				vBestPoint);
		OutVertex.vTexcoord =
			RenderVertex.vTexcoord;
		OutVertex.vParticleIndices =
			{ i0, i1, i2 };
		OutVertex.vBarycentric =
			vBestBarycentric;
		OutVertex.vPositionOffset =
			ProjectToFrame(
				vPositionDelta,
				vTangent,
				vBinormal,
				vNormal);
		OutVertex.vNormalFrame =
			ProjectToFrame(
				RenderVertex.vNormal,
				vTangent,
				vBinormal,
				vNormal);

		const _float3 vTangentFrame =
			ProjectToFrame(
				RenderVertex.vTangent,
				vTangent,
				vBinormal,
				vNormal);
		const float fHandedness =
			XMVectorGetX(
				XMVector3Dot(
					XMVector3Cross(
						XMLoadFloat3(
							&RenderVertex.vNormal),
						XMLoadFloat3(
							&RenderVertex.vTangent)),
					XMLoadFloat3(
						&RenderVertex.vBinormal))) <
				0.f ?
			-1.f : 1.f;
		OutVertex.vTangentFrame = {
			vTangentFrame.x,
			vTangentFrame.y,
			vTangentFrame.z,
			fHandedness
		};
		OutDistance =
			std::sqrt(fBestDistanceSq);
		return std::isfinite(OutDistance);
	}
}

CResNvClothMesh::CResNvClothMesh(
	const _string& sPath,
	ComPtr<ID3D11Device> pDevice)
	: CResource{ sPath },
	  m_pDevice{ std::move(pDevice) }
{
}

CResNvClothMesh::~CResNvClothMesh()
{
}

HRESULT CResNvClothMesh::Load(const std::any& arg)
{
	const auto* pDesc = std::any_cast<DESC>(&arg);
	if (!pDesc ||
		!m_pDevice ||
		!std::isfinite(pDesc->fWeldTolerance) ||
		pDesc->fWeldTolerance <= 0.f ||
		!std::isfinite(pDesc->fFixedTopRatio) ||
		pDesc->fFixedTopRatio < 0.f ||
		pDesc->fFixedTopRatio > 1.f)
	{
		return E_INVALIDARG;
	}

	if (m_eState == STATE::LOADED)
		return S_OK;

	m_eState = STATE::LOADING;
	std::vector<SOURCE_BONE> Bones{};
	std::vector<SOURCE_MESH> Meshes{};
	if (!LoadSourceModel(m_sPath, Bones, Meshes))
	{
		m_eState = STATE::LOADFAIL;
		return E_FAIL;
	}

	uint32_t iSimulationMeshIndex =
		pDesc->iSimulationMeshIndex;
	uint32_t iRenderMeshIndex =
		pDesc->iRenderMeshIndex;
	if (iSimulationMeshIndex ==
		std::numeric_limits<uint32_t>::max())
	{
		iSimulationMeshIndex =
			static_cast<uint32_t>(
				std::distance(
					Meshes.begin(),
					std::min_element(
						Meshes.begin(),
						Meshes.end(),
						[](const SOURCE_MESH& A,
							const SOURCE_MESH& B)
						{
							return A.Indices.size() <
								B.Indices.size();
						})));
	}
	if (iRenderMeshIndex ==
		std::numeric_limits<uint32_t>::max())
	{
		iRenderMeshIndex =
			static_cast<uint32_t>(
				std::distance(
					Meshes.begin(),
					std::max_element(
						Meshes.begin(),
						Meshes.end(),
						[](const SOURCE_MESH& A,
							const SOURCE_MESH& B)
						{
							return A.Indices.size() <
								B.Indices.size();
						})));
	}
	if (iSimulationMeshIndex >= Meshes.size() ||
		iRenderMeshIndex >= Meshes.size() ||
		iSimulationMeshIndex == iRenderMeshIndex)
	{
		m_eState = STATE::LOADFAIL;
		return E_INVALIDARG;
	}

	std::vector<_float4x4> CombinedBones{};
	if (!BuildCombinedBones(
		Bones,
		pDesc->PreTransformMatrix,
		CombinedBones))
	{
		m_eState = STATE::LOADFAIL;
		return E_FAIL;
	}

	_matrix SimulationAnchorInverse =
		XMMatrixIdentity();
	if (!pDesc->sSimulationAnchorBone.empty())
	{
		const auto BoneIter = std::find_if(
			Bones.begin(),
			Bones.end(),
			[pDesc](const SOURCE_BONE& Bone)
			{
				return Bone.sName ==
					pDesc->sSimulationAnchorBone;
			});
		if (BoneIter == Bones.end())
		{
			m_eState = STATE::LOADFAIL;
			return E_FAIL;
		}

		const size_t iBoneIndex =
			static_cast<size_t>(
				std::distance(Bones.begin(), BoneIter));
		_matrix SimulationAnchor{};
		if (!MakeRigidMatrix(
			XMLoadFloat4x4(
				&CombinedBones[iBoneIndex]),
			SimulationAnchor))
		{
			m_eState = STATE::LOADFAIL;
			return E_FAIL;
		}

		_vector vDeterminant{};
		SimulationAnchorInverse =
			XMMatrixInverse(
				&vDeterminant,
				SimulationAnchor);
		if (std::fabs(
			XMVectorGetX(vDeterminant)) <=
			FLT_EPSILON)
		{
			m_eState = STATE::LOADFAIL;
			return E_FAIL;
		}
	}

	m_tFabricDesc = {};
	m_tFabricDesc.bUseGeodesicTether = true;
	m_Sections.clear();
	m_Sections.reserve(1);

	const auto& SimulationSource =
		Meshes[iSimulationMeshIndex];
	const auto& RenderSource =
		Meshes[iRenderMeshIndex];
	std::vector<VTXMESH> SimulationVertices(
		SimulationSource.Vertices.size());
	std::vector<VTXMESH> RenderVertices(
		RenderSource.Vertices.size());
	float fOriginalMinY =
		std::numeric_limits<float>::max();
	float fOriginalMaxY =
		std::numeric_limits<float>::lowest();

	for (size_t i = 0;
		i < SimulationSource.Vertices.size();
		++i)
	{
		if (!SkinVertex(
			SimulationSource.Vertices[i],
			SimulationSource,
			CombinedBones,
			SimulationVertices[i]))
		{
			m_eState = STATE::LOADFAIL;
			return E_FAIL;
		}

		const float fY =
			SimulationVertices[i].vPosition.y;
		fOriginalMinY =
			std::min(fOriginalMinY, fY);
		fOriginalMaxY =
			std::max(fOriginalMaxY, fY);
	}

	for (size_t i = 0;
		i < RenderSource.Vertices.size();
		++i)
	{
		if (!SkinVertex(
			RenderSource.Vertices[i],
			RenderSource,
			CombinedBones,
			RenderVertices[i]))
		{
			m_eState = STATE::LOADFAIL;
			return E_FAIL;
		}
	}

	const float fOriginalHeight =
		fOriginalMaxY - fOriginalMinY;
	const float fFixedCutoff =
		fOriginalMaxY -
		fOriginalHeight *
			pDesc->fFixedTopRatio;

	std::unordered_map<
		WELD_KEY,
		uint32_t,
		WELD_KEY_HASH> WeldedParticles{};
	std::vector<uint32_t> SimulationParticleMap(
		SimulationVertices.size());
	for (size_t i = 0;
		i < SimulationVertices.size();
		++i)
	{
		const _bool bFixed =
			SimulationVertices[i].vPosition.y >=
				fFixedCutoff;
		TransformVertex(
			SimulationVertices[i],
			SimulationAnchorInverse);

		const auto Key = MakeWeldKey(
			SimulationVertices[i].vPosition,
			pDesc->fWeldTolerance);
		const auto [Iter, bInserted] =
			WeldedParticles.try_emplace(
				Key,
				static_cast<uint32_t>(
					m_tFabricDesc.vecPositions.size()));
		if (bInserted)
		{
			m_tFabricDesc.vecPositions.push_back(
				SimulationVertices[i].vPosition);
			m_tFabricDesc.vecInverseMasses.push_back(
				bFixed ? 0.f : 1.f);
		}
		else if (bFixed)
		{
			m_tFabricDesc.vecInverseMasses[
				Iter->second] = 0.f;
		}
		SimulationParticleMap[i] = Iter->second;
	}

	for (size_t i = 0;
		i < SimulationSource.Indices.size();
		i += 3)
	{
		const auto i0 =
			SimulationParticleMap[
				SimulationSource.Indices[i]];
		const auto i1 =
			SimulationParticleMap[
				SimulationSource.Indices[i + 1]];
		const auto i2 =
			SimulationParticleMap[
				SimulationSource.Indices[i + 2]];
		if (i0 == i1 || i1 == i2 || i2 == i0)
			continue;

		_float3 vTangent{};
		_float3 vBinormal{};
		_float3 vNormal{};
		if (!BuildTriangleFrame(
			m_tFabricDesc.vecPositions[i0],
			m_tFabricDesc.vecPositions[i1],
			m_tFabricDesc.vecPositions[i2],
			vTangent,
			vBinormal,
			vNormal))
		{
			continue;
		}

		m_tFabricDesc.vecIndices.insert(
			m_tFabricDesc.vecIndices.end(),
			{ i0, i1, i2 });
	}

	if (m_tFabricDesc.vecPositions.size() < 3 ||
		m_tFabricDesc.vecIndices.size() < 3)
	{
		m_eState = STATE::LOADFAIL;
		return E_FAIL;
	}

	for (auto& Vertex : RenderVertices)
	{
		TransformVertex(
			Vertex,
			SimulationAnchorInverse);
	}

	std::vector<NVCLOTH_RENDER_VERTEX>
		MappedRenderVertices(RenderVertices.size());
	double fTotalMappingDistance{};
	float fMaxMappingDistance{};
	for (size_t i = 0;
		i < RenderVertices.size();
		++i)
	{
		float fMappingDistance{};
		if (!BuildRenderVertexMapping(
			RenderVertices[i],
			m_tFabricDesc,
			MappedRenderVertices[i],
			fMappingDistance))
		{
			m_eState = STATE::LOADFAIL;
			return E_FAIL;
		}

		fTotalMappingDistance +=
			fMappingDistance;
		fMaxMappingDistance =
			std::max(
				fMaxMappingDistance,
				fMappingDistance);
	}

	SECTION Section{};
	Section.iSourceMeshIndex =
		iRenderMeshIndex;
	Section.iMaterialIndex =
		RenderSource.iMaterialIndex;
	Section.iVertexCount =
		static_cast<uint32_t>(
			MappedRenderVertices.size());
	Section.iIndexCount =
		static_cast<uint32_t>(
			RenderSource.Indices.size());
	Section.iVertexStride =
		sizeof(NVCLOTH_RENDER_VERTEX);

	D3D11_BUFFER_DESC VertexDesc{};
	VertexDesc.ByteWidth =
		static_cast<UINT>(
			MappedRenderVertices.size() *
			sizeof(NVCLOTH_RENDER_VERTEX));
	VertexDesc.Usage = D3D11_USAGE_IMMUTABLE;
	VertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	D3D11_SUBRESOURCE_DATA VertexData{};
	VertexData.pSysMem =
		MappedRenderVertices.data();
	if (FAILED(m_pDevice->CreateBuffer(
		&VertexDesc,
		&VertexData,
		Section.pVertexBuffer.GetAddressOf())))
	{
		m_eState = STATE::LOADFAIL;
		return E_FAIL;
	}

	D3D11_BUFFER_DESC IndexDesc{};
	IndexDesc.ByteWidth =
		static_cast<UINT>(
			RenderSource.Indices.size() *
			sizeof(uint32_t));
	IndexDesc.Usage = D3D11_USAGE_IMMUTABLE;
	IndexDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	D3D11_SUBRESOURCE_DATA IndexData{};
	IndexData.pSysMem =
		RenderSource.Indices.data();
	if (FAILED(m_pDevice->CreateBuffer(
		&IndexDesc,
		&IndexData,
		Section.pIndexBuffer.GetAddressOf())))
	{
		m_eState = STATE::LOADFAIL;
		return E_FAIL;
	}

	m_Sections.push_back(std::move(Section));

	char szLog[512]{};
	sprintf_s(
		szLog,
		"[NvCloth] Simulation mesh %u (%zu triangles), "
		"render mesh %u (%zu triangles), "
		"wrap distance avg=%.6f max=%.6f.\n",
		iSimulationMeshIndex,
		SimulationSource.Indices.size() / 3,
		iRenderMeshIndex,
		RenderSource.Indices.size() / 3,
		RenderVertices.empty() ?
			0.f :
			static_cast<float>(
				fTotalMappingDistance /
				RenderVertices.size()),
		fMaxMappingDistance);
	DEBUG_LOG(szLog);

	m_eState = STATE::LOADED;
	return S_OK;
}

HRESULT CResNvClothMesh::Unload(const std::any&)
{
	m_Sections.clear();
	m_tFabricDesc = {};
	m_eState = STATE::UNLOAD;
	return S_OK;
}

SPtr<CResNvClothMesh> CResNvClothMesh::Create(
	const _string& sPath)
{
	return ToSPtr(new CResNvClothMesh{
		sPath,
		CGameInstance::Get().GetGraphicDevice() });
}

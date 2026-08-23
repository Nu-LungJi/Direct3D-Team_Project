#pragma once
#include "pch.h"
#include "Bone.h"
#include "Mesh.h"
#include "Material.h"
#include "Animation.h"

class CImporter
{
public:
	enum class MODEL_CATEGORY
	{
		STATIC,
		SKELETAL,
		ANIMATION
	};

public:
	CImporter();
	~CImporter();

public:


	HRESULT ImportFBXFolder(const std::string& strLevelName,const std::string& strSourceFolder);
	HRESULT AssimpFBX(const std::string& fbxFileName);
	// Whole-map-only path. Splits a static FBX in root-local XYZ space and writes
	// one regular static-model binary per render chunk plus a JSON manifest.
	HRESULT ImportWholeMapFBX(
		const std::string& fbxFileName,
		const std::string& outputDirectory,
		float chunkSize);
	// Preserves static FBX nodes as independently placeable map objects. Mesh
	// resources are emitted once per shared mesh combination and referenced by a
	// JSON placement manifest.
	HRESULT ImportObjectMapFBX(
		const std::string& fbxFileName,
		const std::string& outputDirectory);
	
	
	HRESULT	ExportFBX(const std::string& outpath);
	HRESULT ExportStatic(const std::string& outpath);
	HRESULT ExportSkeletal(const std::string& outpath);
	HRESULT ExportAnimation(const std::string& outpath);


public:
	HRESULT Ready_Bones(const aiNode* pAINode, int32_t iParentBoneIndex);


	HRESULT Ready_Mesh(const aiScene* scene, bool _bHasBone);

	void ProcessNonAnimMesh(aiMesh* mesh, const aiScene* scene);
	void ProcessNonAnimNode(aiNode* node, const aiScene* scene);
	void ProcessAnimMesh(aiMesh* mesh, const aiScene* scene, std::string name);
	void ProcessAnimNode(aiNode* node, const aiScene* scene);

	HRESULT Ready_Material(const aiScene* scene);
	void	Load_Material(aiMaterial* material, uint32_t materialNum);

	
	HRESULT Ready_Animation(const aiScene* scene);
	HRESULT Load_Animaion(uint32_t iAnimaionCount, const aiAnimation* pAIAnimation);
	HRESULT Load_Channel(CHANNELDATA& ChannelData, const aiNodeAnim* pAIChannel);


public:
	int32_t Get_BoneIndex(const char* pBoneName);



public:
	HRESULT ImportFBXFolder_ForMapJson(
		const std::string& strLevelName,
		const std::string& strSourceFolder,
		const std::string& strJsonFolder
	);
	void CopyUsedTextureFilesToFolder(
		const std::filesystem::path& srcDir,
		const std::filesystem::path& dstDir
	) const;
private:
	HRESULT ExportStaticMeshSubset(
		const std::filesystem::path& outpath,
		const std::vector<std::shared_ptr<CMesh>>& meshes) const;

	std::unordered_set<std::string> LoadMapFBXNamesFromJsonFolder(
		const std::string& strJsonFolder
	);

	bool HasExtractedModelData(
		const std::filesystem::path& modelDir,
		const std::string& modelName
	) const;

	std::string ToLowerFileName(std::string name) const;



public:
	void CopyPngFilesToFolder(const std::filesystem::path& srcDir, const std::filesystem::path& dstDir) const;
	void Clear();
public:
	int m_index{ 0 };
	int32_t m_iBoneIndex{ -1 };
	bool m_bHasAnimation = false;
	bool m_bHasBone = false;

private:
	std::string fileParentName;
	std::string textureParentName;

private:
	std::filesystem::path m_FBXSourceDir;


private:
	std::filesystem::path MakeTextureOutputDir(const std::filesystem::path& modelOutputDir) const;

	std::vector<std::shared_ptr<CBone>> Bones;
	std::vector<std::shared_ptr<CMesh>> Meshes;
	std::vector<std::shared_ptr<CMaterial>> Materials;
	std::vector<std::shared_ptr<CAnimation>> Animations;

	std::vector<char> meshBuffer;
	std::vector<char> materialBuffer;
	std::vector<char> boneBuffer;
	std::vector<char> animBuffer;

};


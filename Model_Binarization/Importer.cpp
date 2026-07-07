
#include "Importer.h"
#include "Bone.h"
#include "Mesh.h"

CImporter::CImporter()
{
}

CImporter::~CImporter()
{
}

HRESULT CImporter::ImportFBXFolder(const std::string& strLevelName, const std::string& strSourceFolder)
{
    std::filesystem::path sourcePath(strSourceFolder);

    std::string category = sourcePath.filename().string();

    if (!std::filesystem::exists(strSourceFolder))
    {
        return E_FAIL;
    }

    for (const auto& entry : std::filesystem::recursive_directory_iterator(strSourceFolder))
    {
        if (!entry.is_regular_file())
            continue;

        const auto& path = entry.path();

        if (path.extension() != ".fbx" &&
            path.extension() != ".FBX")
            continue;

        std::string inputPath = path.string();
        std::string modelName = path.stem().string();

        std::string rootPath = strSourceFolder;

        size_t pos = rootPath.find("OriginData");

        if (pos != std::string::npos)
        {
            rootPath = rootPath.substr(0, pos);
        }

        std::string basePath = rootPath +strLevelName + "/" + category + "/";

        std::string modelDir = basePath + modelName + "/";

        std::filesystem::create_directories(modelDir);


        AssimpFBX(inputPath);
        
        std::string outputPath = modelDir + modelName + ".bin";

        ExportFBX(outputPath);


        Clear();

    }



    return S_OK;
}

HRESULT CImporter::AssimpFBX(const std::string& fbxFileName)
{
    m_index = 0;
    m_bHasAnimation = false;
    m_bHasBone = false;

    uint32_t iFlag = 0;
    iFlag |= aiProcess_ConvertToLeftHanded;
    iFlag |= aiProcess_PopulateArmatureData;
    iFlag |= aiProcess_GlobalScale;
    iFlag |= aiProcess_ImproveCacheLocality;
    iFlag |= aiProcessPreset_TargetRealtime_Fast;
//    iFlag |= aiProcess_OptimizeMeshes;

    Assimp::Importer importer;


    const aiScene* pScene = importer.ReadFile(fbxFileName, iFlag);

    if (!pScene)
        return E_FAIL;

    m_bHasAnimation = pScene->HasAnimations();

    for (uint32_t i = 0; i < pScene->mNumMeshes; ++i)
    {
        if (pScene->mMeshes[i]->HasBones())
        {
            m_bHasBone = true;
            break;
        }
    }

    if (m_bHasBone)
        Ready_Bones(pScene->mRootNode, -1);

    if (pScene->HasMeshes())
        Ready_Mesh(pScene, m_bHasBone);

    if (pScene->HasMaterials())
        Ready_Material(pScene);

    if (m_bHasAnimation)
        Ready_Animation(pScene);

    return S_OK;
}
HRESULT CImporter::ExportFBX(const std::string& outpath)
{
    std::filesystem::path path(outpath);

    std::string modelName = path.stem().string();

    std::string prefix;

    if (!m_bHasBone && !m_bHasAnimation)
        prefix = "SM_";
    else if (!m_bHasBone && m_bHasAnimation)
        prefix = "SM_";
    else if (m_bHasBone && m_bHasAnimation)
        prefix = "SK_";
    else
        prefix = "SK_";

    std::string finalPath =
        path.parent_path().string() + "/" +
        prefix + modelName + ".bin";

    fileParentName = path.parent_path().string();

    if (!m_bHasBone && !m_bHasAnimation)
    {
        ExportStatic(finalPath);
    }
    else if (!m_bHasBone && m_bHasAnimation)
    {
        //ExportStaticAnim(finalPath);
    }
    else if (m_bHasBone )
    {
        ExportSkeletal(finalPath);
    }
    else
    {
        //ExportSkeletalAnim(finalPath);
    }


    if (m_bHasAnimation) {
        ExportAnimation(finalPath);
    }
    return S_OK;
}
HRESULT CImporter::ExportStatic(const std::string& outpath)
{
    std::ofstream file(outpath, std::ios::binary);

    if (!file.is_open()) {
        return E_FAIL;
    }

    auto pushMesh = [&](const void* data, size_t size)
        {
            size_t old = meshBuffer.size();
            meshBuffer.resize(old + size);
            memcpy(meshBuffer.data() + old, data, size);
        };

    //---------------------------------------------------FILEHEADER-------------------------------------------------------------------//
    MODEL_FILE_HEADER MFH;
    MFH.bHasBone = false;
    MFH.bHasAnimation = false;
    MFH.MeshCount = (uint32_t)Meshes.size();
    MFH.AnimationCount = 0;
    MFH.MaterialCount = (uint32_t)Materials.size();
    MFH.BoneCount = (uint32_t)Bones.size();
    file.write((char*)&MFH, sizeof(MFH));

    //---------------------------------------------------------MESH-------------------------------------------------------------------//
    for (auto& mesh : Meshes)
    {
        uint32_t snameLen = (uint32_t)mesh->m_name.size();
        uint32_t svCount = (uint32_t)mesh->m_vertices->size();
        uint32_t siCount = (uint32_t)mesh->m_indices->size();

        uint32_t meshSize =
            sizeof(uint32_t) + snameLen +                    // nameLen + name
            sizeof(uint32_t) +                              // materialIndex
            sizeof(uint32_t) +                              // vCount
            sizeof(uint32_t) +                              // iCount
            sizeof(VTXMESH) * svCount +                      // vertices
            sizeof(uint32_t) * siCount;                      // indices

        // Mesh 크기 먼저 기록
        pushMesh(&meshSize, sizeof(uint32_t));


        uint32_t nameLen = (uint32_t)mesh->m_name.size();
        pushMesh(&nameLen, sizeof(uint32_t));

        pushMesh(mesh->m_name.data(), nameLen);
        
        uint32_t vMaterialIndex = mesh->m_materialIndex;
        pushMesh(&vMaterialIndex, sizeof(uint32_t));

        uint32_t vCount = mesh->m_vertices->size();
        pushMesh(&vCount, sizeof(uint32_t));

        uint32_t iCount = mesh->m_indices->size();
        pushMesh(&iCount, sizeof(uint32_t));

        pushMesh(mesh->m_vertices->data(), sizeof(VTXMESH) * vCount);
        pushMesh(mesh->m_indices->data(), sizeof(uint32_t) * iCount);
    }

    ChunkHeader chMesh;
    chMesh.type = ChunkType::CHUNK_MESH;
    chMesh.size = (uint32_t)meshBuffer.size();

    file.write((char*)&chMesh, sizeof(chMesh));
    file.write(meshBuffer.data(), meshBuffer.size());

    //--------------------------------------------------------Material-------------------------------------------------------------------//
    auto pushMaterial = [&](const void* data, size_t size)
        {
            size_t old = materialBuffer.size();
            materialBuffer.resize(old + size);
            memcpy(materialBuffer.data() + old, data, size);
        };

    for (auto& mat : Materials)
    {
        uint32_t materialSize = 0;

        // materialNum
        materialSize += sizeof(uint32_t);

        // textureTypeCount
        materialSize += sizeof(uint32_t);

        for (auto& texs : mat->m_textures)
        {
            // textureCount
            materialSize += sizeof(uint32_t);

            for (auto& tex : texs)
            {
                materialSize += sizeof(uint32_t); // m_textureType

                materialSize += sizeof(uint32_t); // File length
                materialSize += (uint32_t)tex.File.size();

                materialSize += sizeof(uint32_t); // Ext length
                materialSize += (uint32_t)tex.Ext.size();
            }
        }

        // Material 크기 먼저 기록
        pushMaterial(&materialSize, sizeof(uint32_t));


        uint32_t materialNum = mat->m_materialNum;
        pushMaterial(&materialNum, sizeof(uint32_t));

        uint32_t textureTypeCount = (uint32_t)mat->m_textures.size();
        pushMaterial(&textureTypeCount, sizeof(uint32_t));

        for (auto& texs : mat->m_textures)
        {
            uint32_t textureCount = (uint32_t)texs.size();
            pushMaterial(&textureCount, sizeof(uint32_t));

            for (auto& tex : texs)
            {
                pushMaterial(&tex.m_textureType, sizeof(uint32_t));

                uint32_t len;

                len = (uint32_t)tex.File.size();
                pushMaterial(&len, sizeof(uint32_t));
                pushMaterial(tex.File.c_str(), len);

                len = (uint32_t)tex.Ext.size();
                pushMaterial(&len, sizeof(uint32_t));
                pushMaterial(tex.Ext.c_str(), len);
            }
        }
    }

    ChunkHeader chMaterial;
    chMaterial.type = ChunkType::CHUNK_MATERIAL;
    chMaterial.size = (uint32_t)materialBuffer.size();

    file.write((char*)&chMaterial, sizeof(chMaterial));
    file.write(materialBuffer.data(), materialBuffer.size());

    file.close();
    return S_OK;
}
HRESULT CImporter::ExportSkeletal(const std::string& outpath) {
    std::ofstream file(outpath, std::ios::binary);

    if (!file.is_open()) {
        return E_FAIL;
    }

    auto pushBone = [&](const void* data, size_t size)
        {
            size_t old = boneBuffer.size();
            boneBuffer.resize(old + size);
            memcpy(boneBuffer.data() + old, data, size);
        };

    auto pushMesh = [&](const void* data, size_t size)
        {
            size_t old = meshBuffer.size();
            meshBuffer.resize(old + size);
            memcpy(meshBuffer.data() + old, data, size);
        };

    auto pushMaterial = [&](const void* data, size_t size)
        {
            size_t old = materialBuffer.size();
            materialBuffer.resize(old + size);
            memcpy(materialBuffer.data() + old, data, size);
        };


    //---------------------------------------------------FILEHEADER-------------------------------------------------------------------//
    MODEL_FILE_HEADER MFH;
    MFH.bHasBone = true;
    MFH.bHasAnimation = false;
    MFH.MeshCount = (uint32_t)Meshes.size();
    MFH.AnimationCount = 0;
    MFH.MaterialCount = (uint32_t)Materials.size();
    MFH.BoneCount = (uint32_t)Bones.size();
    file.write((char*)&MFH, sizeof(MFH));

    //-------------------------------------------------BONE-----------------------------------------------------------------------------//

    for (auto& bone : Bones)
    {
        uint32_t len = (uint32_t)bone->Bone.m_name.size();

        uint32_t boneSize =
            sizeof(uint32_t) +    // len
            len +                 // name
            sizeof(XMFLOAT4X4) +  // transform
            sizeof(uint32_t);     // parent index

        pushBone(&boneSize, sizeof(uint32_t));

        pushBone(&len, sizeof(uint32_t));
        pushBone(bone->Bone.m_name.c_str(), len);
        pushBone(&bone->Bone.m_TransformationMatrix, sizeof(XMFLOAT4X4));
        pushBone(&bone->Bone.m_patrentBoneIndex, sizeof(uint32_t));
    }

    ChunkHeader chBone;
    chBone.type = ChunkType::CHUNK_BONE;
    chBone.size = (uint32_t)boneBuffer.size();
    file.write((char*)&chBone, sizeof(chBone));
    file.write(boneBuffer.data(), boneBuffer.size());

    //---------------------------------------------------------MESH-------------------------------------------------------------------//
    for (auto& mesh : Meshes)
    {
        uint32_t vCount = (uint32_t)mesh->m_animvertices->size();
        uint32_t iCount = (uint32_t)mesh->m_indices->size();

        uint32_t BoneIndicesCount = (uint32_t)mesh->m_BoneIndices->size();
        uint32_t BoneMatricesCount = (uint32_t)mesh->m_BoneMatrices->size();
        uint32_t OffsetMatricesCount = (uint32_t)mesh->m_OffsetMatrices->size();

        uint32_t meshSize =
            sizeof(uint32_t) +                                         // MaterialIndex
            sizeof(uint32_t) +                                         // Vertex Count
            sizeof(uint32_t) +                                         // Index Count
            sizeof(VTXANIMMESH) * vCount +                             // Vertex Data
            sizeof(uint32_t) * iCount +                                // Index Data
            sizeof(uint32_t) +                                         // NumBones
            sizeof(uint32_t) +                                         // BoneIndices Count
            sizeof(uint32_t) +                                         // BoneMatrices Count
            sizeof(uint32_t) +                                         // OffsetMatrices Count
            sizeof(uint32_t) * BoneIndicesCount +                      // BoneIndices Data
            sizeof(XMFLOAT4X4) * BoneMatricesCount +                   // BoneMatrices Data
            sizeof(XMFLOAT4X4) * OffsetMatricesCount;                  // OffsetMatrices Data

        pushMesh(&meshSize, sizeof(uint32_t));

        // MaterialIndex
         pushMesh(&mesh->m_materialIndex, sizeof(uint32_t));

        // Vertex Count
        pushMesh(&vCount, sizeof(uint32_t));

        // Index Count
        pushMesh(&iCount, sizeof(uint32_t));

        // Vertex 데이터
        pushMesh(mesh->m_animvertices->data(),
            sizeof(VTXANIMMESH) * vCount);

        // Index 데이터
        pushMesh(mesh->m_indices->data(),
            sizeof(uint32_t) * iCount);

        // Mesh가 이용하는 뼈의 개수
        pushMesh(&mesh->m_iNumBones, sizeof(uint32_t));

        // BoneIndices Count
        pushMesh(&BoneIndicesCount, sizeof(uint32_t));

        // BoneMatrices Count
        pushMesh(&BoneMatricesCount, sizeof(uint32_t));

        // OffsetMatrices Count
        pushMesh(&OffsetMatricesCount, sizeof(uint32_t));

        // BoneIndices 데이터
        pushMesh(mesh->m_BoneIndices->data(),
            sizeof(uint32_t) * BoneIndicesCount);

        // BoneMatrices 데이터
        pushMesh(mesh->m_BoneMatrices->data(),
            sizeof(XMFLOAT4X4) * BoneMatricesCount);

        // OffsetMatrices 데이터
        pushMesh(mesh->m_OffsetMatrices->data(),
            sizeof(XMFLOAT4X4) * OffsetMatricesCount);
    }

    ChunkHeader chMesh;
    chMesh.type = ChunkType::CHUNK_MESH;
    chMesh.size = (uint32_t)meshBuffer.size();

    file.write((char*)&chMesh, sizeof(chMesh));
    file.write(meshBuffer.data(), meshBuffer.size());

    //--------------------------------------------------------Material-------------------------------------------------------------------//

    for (auto& mat : Materials)
    {
        uint32_t materialSize = 0;

        // materialNum
        materialSize += sizeof(uint32_t);

        // textureTypeCount
        materialSize += sizeof(uint32_t);

        for (auto& texs : mat->m_textures)
        {
            // textureCount
            materialSize += sizeof(uint32_t);

            for (auto& tex : texs)
            {
                materialSize += sizeof(uint32_t); // m_textureType

                materialSize += sizeof(uint32_t); // File length
                materialSize += (uint32_t)tex.File.size();

                materialSize += sizeof(uint32_t); // Ext length
                materialSize += (uint32_t)tex.Ext.size();
            }
        }

        // Material 크기 먼저 기록
        pushMaterial(&materialSize, sizeof(uint32_t));


        uint32_t materialNum = mat->m_materialNum;
        pushMaterial(&materialNum, sizeof(uint32_t));

        uint32_t textureTypeCount = (uint32_t)mat->m_textures.size();
        pushMaterial(&textureTypeCount, sizeof(uint32_t));

        for (auto& texs : mat->m_textures)
        {
            uint32_t textureCount = (uint32_t)texs.size();
            pushMaterial(&textureCount, sizeof(uint32_t));

            for (auto& tex : texs)
            {
                pushMaterial(&tex.m_textureType, sizeof(uint32_t));

                uint32_t len;

                len = (uint32_t)tex.File.size();
                pushMaterial(&len, sizeof(uint32_t));
                pushMaterial(tex.File.c_str(), len);

                len = (uint32_t)tex.Ext.size();
                pushMaterial(&len, sizeof(uint32_t));
                pushMaterial(tex.Ext.c_str(), len);
            }
        }
    }

    ChunkHeader chMaterial;
    chMaterial.type = ChunkType::CHUNK_MATERIAL;
    chMaterial.size = (uint32_t)materialBuffer.size();

    file.write((char*)&chMaterial, sizeof(chMaterial));
    file.write(materialBuffer.data(), materialBuffer.size());

    file.close();
    return S_OK;
}
HRESULT CImporter::ExportAnimation(const std::string& outpath)
{
    auto pushAnim = [&](const void* data, size_t size)
        {
            size_t old = animBuffer.size();
            animBuffer.resize(old + size);
            memcpy(animBuffer.data() + old, data, size);
        };
    //---------------------------------------------------Animaiton-------------------------------------------------------------------//

    for (auto& Animation : Animations)
    {
        animBuffer.clear();


        std::string animName = Animation->m_AnimationData.sAnimName;

        const std::string invalidChars = R"(< >:"/\|?*)";

        std::replace_if(
            animName.begin(),
            animName.end(),
            [](char c)
            {
                switch (c)
                {
                case '<':
                case '>':
                case ':':
                case '"':
                case '/':
                case '\\':
                case '|':
                case '?':
                case '*':
                    return true;
                default:
                    return false;
                }
            },
            '_'
        );


        std::filesystem::path animPath = fileParentName+ "/" + ( "AN_" + animName + ".bin");

        std::ofstream file(animPath, std::ios::binary);

        if (!file.is_open())
            continue;

        MODEL_FILE_HEADER MFH{};
        MFH.bHasBone = false;
        MFH.bHasAnimation = true;
        MFH.MeshCount = 0;
        MFH.AnimationCount = 1;
        MFH.MaterialCount = 0;
        MFH.BoneCount = 0;

        file.write((char*)&MFH, sizeof(MFH));


        float Duration = Animation->m_AnimationData.AnimationDuration;

        float TickPerSecond = Animation->m_AnimationData.AnimtaionTickPerSecond;

        pushAnim(&Duration, sizeof(float));
        pushAnim(&TickPerSecond, sizeof(float));

        uint32_t ChannelCount = Animation->m_AnimationData.ChannelCount;

        pushAnim(&ChannelCount, sizeof(uint32_t));
        


        for (uint32_t i = 0; i < ChannelCount; ++i)
        {



            int32_t BoneIndex = (*Animation->m_AnimationData.Channels)[i].BoneIndex;

            uint32_t KeyFrameCount = (*Animation->m_AnimationData.Channels)[i].KeyFrameCount;


            uint32_t ChannelSize =
                sizeof(int32_t) +                    // BoneIndex
                sizeof(uint32_t) +                   // KeyFrameCount
                KeyFrameCount *
                (
                    sizeof(XMFLOAT3) +               // Scale
                    sizeof(XMFLOAT4) +               // Rotation
                    sizeof(XMFLOAT3) +               // Translation
                    sizeof(float)                    // TrackPosition
                    );

            pushAnim(&ChannelSize, sizeof(uint32_t));



            pushAnim(&BoneIndex, sizeof(int32_t));
            pushAnim(&KeyFrameCount, sizeof(uint32_t));

            for (uint32_t j = 0; j < KeyFrameCount; ++j)
            {
                auto& KeyFrame = (*(*Animation->m_AnimationData.Channels)[i].KeyFrames)[j];

                pushAnim(&KeyFrame.vScale, sizeof(XMFLOAT3));
                pushAnim(&KeyFrame.vRotation, sizeof(XMFLOAT4));
                pushAnim(&KeyFrame.vTranslation, sizeof(XMFLOAT3));
                pushAnim(&KeyFrame.fTrackPosition, sizeof(float));
            }
        }

        ChunkHeader chAnim;
        chAnim.type = ChunkType::CHUNK_ANIM;
        chAnim.size = (uint32_t)animBuffer.size();

        file.write((char*)&chAnim, sizeof(chAnim));
        file.write(animBuffer.data(), animBuffer.size());

        file.close();
    }

    return S_OK;
}
    
HRESULT CImporter::Ready_Bones(const aiNode* pAINode, int32_t iParentBoneIndex) {

    auto    pBone = std::make_shared<CBone>();

    pBone->Bone.m_name= pAINode->mName.C_Str();

    memcpy(&(pBone->Bone.m_TransformationMatrix), &pAINode->mTransformation, sizeof(XMFLOAT4X4));

    pBone->Bone.m_patrentBoneIndex = iParentBoneIndex;

    Bones.push_back(pBone);

    int32_t iParentIndex = (int32_t)Bones.size() - 1;

    for (uint32_t i = 0; i < pAINode->mNumChildren; ++i)
    {
        Ready_Bones(pAINode->mChildren[i], iParentIndex);
    }

    return S_OK;
}

HRESULT CImporter::Ready_Material(const aiScene* scene) {
    uint32_t NumMaterials = scene->mNumMaterials;
    Materials.reserve(NumMaterials);

    for (uint32_t i = 0; i < NumMaterials; i++)
    {
        Load_Material(scene->mMaterials[i], i);
    }

    return S_OK;
}

void CImporter::Load_Material(aiMaterial* material, uint32_t materialNum)
{
    std::shared_ptr<CMaterial> fbxmaterial = std::make_shared<CMaterial>();
    fbxmaterial->m_materialNum = materialNum;
    for (size_t i = 0; i < AI_TEXTURE_TYPE_MAX; i++)
    {
        uint32_t		iNumTextures = material->GetTextureCount(static_cast<aiTextureType>(i));

        if (iNumTextures == 0) {
            continue;
        }

        std::vector<TEXTUREINFO> textureDummy;
        textureDummy.resize(iNumTextures);

        for (uint32_t j = 0; j < iNumTextures; j++)
        {

            char	szFileName[MAX_PATH] = { };
            char	szExt[MAX_PATH] = { };

            aiString		strTexturePath = {};

            material->GetTexture(static_cast<aiTextureType>(i), j, &strTexturePath);

            _splitpath_s(strTexturePath.C_Str(), nullptr, 0, nullptr, 0, szFileName, MAX_PATH, szExt, MAX_PATH);

            textureDummy[j].m_textureType = (uint32_t)i;
            textureDummy[j].m_textureNum = j;
            textureDummy[j].File = szFileName;
            textureDummy[j].Ext = szExt;
        }


        fbxmaterial->m_textures.push_back(textureDummy);
    }


    Materials.emplace_back(fbxmaterial);
}

HRESULT CImporter::Ready_Animation(const aiScene* scene)
{
    Animations.reserve(scene->mNumAnimations);
    for (size_t i = 0; i < scene->mNumAnimations; i++)
    {

        Animations.emplace_back(make_shared<CAnimation>());

        Load_Animaion(i, scene->mAnimations[i]);
    }

    return S_OK;
}

HRESULT CImporter::Load_Animaion(uint32_t iAnimaionCount, const aiAnimation* pAIAnimation)
{
    Animations[iAnimaionCount]->m_AnimationData.sAnimName = pAIAnimation->mName.C_Str();

    Animations[iAnimaionCount]->m_AnimationData.AnimationDuration = pAIAnimation->mDuration;
    Animations[iAnimaionCount]->m_AnimationData.AnimtaionTickPerSecond = pAIAnimation->mTicksPerSecond;
    Animations[iAnimaionCount]->m_AnimationData.ChannelCount = pAIAnimation->mNumChannels;

    Animations[iAnimaionCount]->m_AnimationData.Channels = make_shared<vector<CHANNELDATA>>();
    Animations[iAnimaionCount]->m_AnimationData.Channels->reserve(pAIAnimation->mNumChannels);


    for (size_t i = 0; i < pAIAnimation->mNumChannels; i++)
    {
        CHANNELDATA ChanelData;

        Load_Channel(ChanelData, pAIAnimation->mChannels[i]);
        Animations[iAnimaionCount]->m_AnimationData.Channels->emplace_back(ChanelData);
    }

    return S_OK;
}

HRESULT CImporter::Load_Channel(CHANNELDATA& ChannelData, const aiNodeAnim* pAIChannel)
{
    ChannelData.BoneIndex = Get_BoneIndex(pAIChannel->mNodeName.C_Str());
    if (-1 == ChannelData.BoneIndex) {
        return S_OK;
    }


    ChannelData.KeyFrameCount = max(pAIChannel->mNumScalingKeys, pAIChannel->mNumRotationKeys);
    ChannelData.KeyFrameCount = max(ChannelData.KeyFrameCount, pAIChannel->mNumPositionKeys);

    XMFLOAT3     vScale = {};
    XMFLOAT4     vRotation = {};
    XMFLOAT3     vTranslation = {};

    ChannelData.KeyFrames = make_shared<vector< KEYFRAME>>();

    ChannelData.KeyFrames->reserve(ChannelData.KeyFrameCount);

    for (size_t i = 0; i < ChannelData.KeyFrameCount; i++)
    {
        KEYFRAME            KeyFrame = {};

        if (i < pAIChannel->mNumScalingKeys) // 만약에 더 큰 값이 들어올떄 그 전의 값으로 마지막껄 채워준다.
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

        ChannelData.KeyFrames->emplace_back(KeyFrame);
    }


    return S_OK;

}

HRESULT CImporter::Ready_Mesh(const aiScene* scene, bool _bHasBone)
{
    aiNode* pNode = scene->mRootNode;
    Meshes.reserve(scene->mNumMeshes);
    if (_bHasBone)
        ProcessAnimNode(pNode, scene);
    else
        ProcessNonAnimNode(pNode, scene);

    return S_OK;
}

void CImporter::ProcessNonAnimMesh(aiMesh* mesh, const aiScene* scene)
{
    

    std::shared_ptr<std::vector<VTXMESH>> vertices = std::make_shared<std::vector<VTXMESH>>();
    std::shared_ptr<std::vector<uint32_t>> indices = std::make_shared<std::vector<uint32_t>>();


    vertices->reserve(mesh->mNumVertices);

    uint32_t indexCount = 0;
    for (UINT i = 0; i < mesh->mNumFaces; ++i)
    {
        indexCount += mesh->mFaces[i].mNumIndices;
    }
    indices->reserve(indexCount);

    for (UINT i = 0; i < mesh->mNumVertices; ++i)
    {
        VTXMESH v{};

        // Position
        v.vPosition = {
            mesh->mVertices[i].x,
            mesh->mVertices[i].y,
            mesh->mVertices[i].z
        };

        // Normal
        if (mesh->HasNormals())
        {
            v.vNormal = {
                mesh->mNormals[i].x,
                mesh->mNormals[i].y,
                mesh->mNormals[i].z
            };
        }

        // UV
        if (mesh->HasTextureCoords(0))
        {
            v.vTexcoord = {
                mesh->mTextureCoords[0][i].x,
                mesh->mTextureCoords[0][i].y
            };
        }

        // Tangent & Binormal
        if (mesh->HasTangentsAndBitangents())
        {
            v.vTangent = {
                mesh->mTangents[i].x,
                mesh->mTangents[i].y,
                mesh->mTangents[i].z
            };

            v.vBinormal = {
                mesh->mBitangents[i].x,
                mesh->mBitangents[i].y,
                mesh->mBitangents[i].z
            };
        }

        vertices->emplace_back(v);
    }

    // Index
    for (UINT i = 0; i < mesh->mNumFaces; ++i)
    {
        const aiFace& face = mesh->mFaces[i];

        for (UINT j = 0; j < face.mNumIndices; ++j)
        {
            indices->emplace_back(face.mIndices[j]);
        }
    }


    std::string _name;


    uint32_t _materialIndex;
    XMFLOAT3 _min = { FLT_MAX, FLT_MAX, FLT_MAX };
    XMFLOAT3 _max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };


    if (mesh->mName.length > 0)
        _name = mesh->mName.C_Str();
    else
        _name = "Mesh_" + std::to_string(++m_index);

    _materialIndex = mesh->mMaterialIndex;

    // boundingBox
    for (UINT i = 0; i < mesh->mNumVertices; ++i)
    {
        const aiVector3D& pos = mesh->mVertices[i];

        _min.x = min(_min.x, pos.x);
        _min.y = min(_min.y, pos.y);
        _min.z = min(_min.z, pos.z);

        _max.x = max(_max.x, pos.x);
        _max.y = max(_max.y, pos.y);
        _max.z = max(_max.z, pos.z);
    }

    std::shared_ptr<CMesh> fbxmesh = std::make_shared<CMesh>();
    
    fbxmesh->m_name = _name;
    fbxmesh->m_materialIndex = _materialIndex;
    fbxmesh->m_max = _max;
    fbxmesh->m_min = _min;
    fbxmesh->m_vertices = vertices;
    fbxmesh->m_indices = indices;



    Meshes.emplace_back(fbxmesh);

    mesh->mMaterialIndex;


}

void CImporter::ProcessNonAnimNode(aiNode* node, const aiScene* scene)
{
    for (UINT i = 0; i < node->mNumMeshes; ++i)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        ProcessNonAnimMesh(mesh, scene);
    }

    for (UINT i = 0; i < node->mNumChildren; ++i)
        ProcessNonAnimNode(node->mChildren[i], scene);
}

void CImporter::ProcessAnimMesh(aiMesh* mesh, const aiScene* scene, std::string name)
{

    std::string _name;

    _name = name;


    uint32_t _materialIndex;
    XMFLOAT3 _min = { FLT_MAX, FLT_MAX, FLT_MAX };
    XMFLOAT3 _max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

    uint32_t m_iNumBones = mesh->mNumBones;
    std::shared_ptr<std::vector<uint32_t>> m_Boneindices = std::make_shared<std::vector<uint32_t>>();
    std::shared_ptr<std::vector<XMFLOAT4X4>> m_BoneMatrices = std::make_shared<std::vector<XMFLOAT4X4>>();
    std::shared_ptr<std::vector<XMFLOAT4X4>> m_OffsetMatrices = std::make_shared<std::vector<XMFLOAT4X4>>();
    std::shared_ptr<std::vector<VTXANIMMESH>> vertices = std::make_shared<std::vector<VTXANIMMESH>>();
    std::shared_ptr<std::vector<uint32_t>> indices = std::make_shared<std::vector<uint32_t>>();
    if (m_iNumBones > 0)
    {
        m_Boneindices->resize(m_iNumBones);
        m_BoneMatrices->resize(m_iNumBones);
        m_OffsetMatrices->resize(m_iNumBones);
    }
    std::shared_ptr<CMesh> fbxmesh = std::make_shared<CMesh>();


    vertices->reserve(mesh->mNumVertices);

    uint32_t indexCount = 0;
    for (UINT i = 0; i < mesh->mNumFaces; ++i)
    {
        indexCount += mesh->mFaces[i].mNumIndices;
    }
    indices->reserve(indexCount);


    for (UINT i = 0; i < mesh->mNumVertices; ++i)
    {
        VTXANIMMESH v{};

        // Position
        v.vPosition = {
            mesh->mVertices[i].x,
            mesh->mVertices[i].y,
            mesh->mVertices[i].z
        };

        // Normal
        if (mesh->HasNormals())
        {
            v.vNormal = {
                mesh->mNormals[i].x,
                mesh->mNormals[i].y,
                mesh->mNormals[i].z
            };
        }

        // UV
        if (mesh->HasTextureCoords(0))
        {
            v.vTexcoord = {
                mesh->mTextureCoords[0][i].x,
                mesh->mTextureCoords[0][i].y
            };
        }

        // Tangent & Binormal
        if (mesh->HasTangentsAndBitangents())
        {
            v.vTangent = {
                mesh->mTangents[i].x,
                mesh->mTangents[i].y,
                mesh->mTangents[i].z
            };

            v.vBinormal = {
                mesh->mBitangents[i].x,
                mesh->mBitangents[i].y,
                mesh->mBitangents[i].z
            };
        }



        vertices->emplace_back(v);
    }



    if (0 == m_iNumBones)
    {
        m_iNumBones = 1;

        int32_t iBoneIndex = Get_BoneIndex(_name.data());

        if (-1 == iBoneIndex)
            return;

        XMFLOAT4X4 OffsetMatrix;
        XMStoreFloat4x4(&OffsetMatrix, XMMatrixIdentity());

        m_Boneindices->resize(1);
        m_BoneMatrices->resize(1);
        m_OffsetMatrices->resize(1);

        (*m_Boneindices)[0] = iBoneIndex;
        XMStoreFloat4x4(&(*m_BoneMatrices)[0], XMMatrixIdentity());
        (*m_OffsetMatrices)[0] = OffsetMatrix;
    }

    else {
        for (size_t i = 0; i < m_iNumBones; i++)
        {
            aiBone* pAIBone = mesh->mBones[i];

            XMFLOAT4X4   OffsetMatrix;
            memcpy(&OffsetMatrix, &pAIBone->mOffsetMatrix, sizeof(XMFLOAT4X4));

            XMStoreFloat4x4(&OffsetMatrix, XMMatrixTranspose(XMLoadFloat4x4(&OffsetMatrix)));
            (*m_OffsetMatrices)[i] = (OffsetMatrix);


            int32_t    iBoneIndex = Get_BoneIndex(pAIBone->mName.C_Str());
            if (-1 == iBoneIndex)
                return;

            (*m_Boneindices)[i] = (iBoneIndex);

            for (size_t j = 0; j < pAIBone->mNumWeights; j++)
            {
                if (0 == (*vertices)[pAIBone->mWeights[j].mVertexId].vBlendWeights.x)
                {
                    (*vertices)[pAIBone->mWeights[j].mVertexId].vBlendIndices.x = (uint32_t)i;
                    (*vertices)[pAIBone->mWeights[j].mVertexId].vBlendWeights.x = pAIBone->mWeights[j].mWeight;
                }

                else if (0 == (*vertices)[pAIBone->mWeights[j].mVertexId].vBlendWeights.y)
                {
                    (*vertices)[pAIBone->mWeights[j].mVertexId].vBlendIndices.y = (uint32_t)i;
                    (*vertices)[pAIBone->mWeights[j].mVertexId].vBlendWeights.y = pAIBone->mWeights[j].mWeight;
                }

                else if (0 == (*vertices)[pAIBone->mWeights[j].mVertexId].vBlendWeights.z)
                {
                    (*vertices)[pAIBone->mWeights[j].mVertexId].vBlendIndices.z = (uint32_t)i;
                    (*vertices)[pAIBone->mWeights[j].mVertexId].vBlendWeights.z = pAIBone->mWeights[j].mWeight;
                }

                else if (0 == (*vertices)[pAIBone->mWeights[j].mVertexId].vBlendWeights.w)
                {
                    (*vertices)[pAIBone->mWeights[j].mVertexId].vBlendIndices.w = (uint32_t)i;
                    (*vertices)[pAIBone->mWeights[j].mVertexId].vBlendWeights.w = pAIBone->mWeights[j].mWeight;
                }
            }
        }
    }
    // Index
    for (UINT i = 0; i < mesh->mNumFaces; ++i)
    {
        const aiFace& face = mesh->mFaces[i];

        for (UINT j = 0; j < face.mNumIndices; ++j)
        {
            indices->emplace_back(face.mIndices[j]);
        }
    }


    _materialIndex = mesh->mMaterialIndex;

    // boundingBox
    for (UINT i = 0; i < mesh->mNumVertices; ++i)
    {
        const aiVector3D& pos = mesh->mVertices[i];

        _min.x = min(_min.x, pos.x);
        _min.y = min(_min.y, pos.y);
        _min.z = min(_min.z, pos.z);

        _max.x = max(_max.x, pos.x);
        _max.y = max(_max.y, pos.y);
        _max.z = max(_max.z, pos.z);
    }


    fbxmesh->m_name = _name;
    fbxmesh->m_materialIndex = _materialIndex;
    fbxmesh->m_max = _max;
    fbxmesh->m_min = _min;
    fbxmesh->m_animvertices = vertices;
    fbxmesh->m_indices = indices;
    fbxmesh->m_iNumBones = m_iNumBones;
    fbxmesh->m_BoneIndices = m_Boneindices;
    fbxmesh->m_BoneMatrices = m_BoneMatrices;
    fbxmesh->m_OffsetMatrices = m_OffsetMatrices;


    Meshes.emplace_back(fbxmesh);


}

void CImporter::ProcessAnimNode(aiNode* node, const aiScene* scene)
{
    for (UINT i = 0; i < node->mNumMeshes; ++i)
    {
        std::string name = node->mName.C_Str();
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        ProcessAnimMesh(mesh, scene, name);
    }

    for (UINT i = 0; i < node->mNumChildren; ++i)
        ProcessAnimNode(node->mChildren[i], scene);
}

int32_t CImporter::Get_BoneIndex(const char* pBoneName)
{
    int32_t iBoneIndex = { 0 };
    auto    iter = find_if(Bones.begin(), Bones.end(), [&](std::shared_ptr<CBone> pBone)->bool
        {
            if (true == pBone->Compare_Name(pBoneName))
                return true;

            ++iBoneIndex;

            return false;
        });

    if (iter == Bones.end())
        return -1;

    return iBoneIndex;
}

void CImporter::Clear() {
    Bones.clear();
    Meshes.clear();
    Materials.clear();
    meshBuffer.clear();
    boneBuffer.clear();
    materialBuffer.clear();
    Animations.clear();

    m_bHasAnimation = false;
    m_bHasBone = false;

}

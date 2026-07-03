#include "Importer.h"

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

        std::string basePath ="./Resources/Models/" +strLevelName + "/" + category + "/";

        std::string modelDir = basePath + modelName + "/";

        std::filesystem::create_directories(modelDir);

        std::string meshBin =modelDir +modelName + ".bin";
    }

    return S_OK;
}




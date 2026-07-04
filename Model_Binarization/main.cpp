#include "pch.h"
#include "Importer.h"


using namespace std;

int main()
{
	shared_ptr<CImporter> import = make_shared<CImporter>();

//    import->ImportFBXFolder("LevelAnimEditor","./Resources/Fbx/LevelAnimEditor/Static");
    import->ImportFBXFolder("LevelAnimEditor","./Resources/Fbx/LevelAnimEditor/Skeletal");
	return 0;
}
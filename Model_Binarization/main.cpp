#include "pch.h"
#include "Importer.h"


using namespace std;

int main()
{
	shared_ptr<CImporter> import = make_shared<CImporter>();

    //import->ImportFBXFolder("LevelAnimEditor","../../JUSIN_160_FINAL_TEAM_RESOURCE/SampleClient/Models/OriginData/LevelAnimEditor/Static");
	//import->ImportFBXFolder("LevelAnimEditor","../../JUSIN_160_FINAL_TEAM_RESOURCE/SampleClient/Models/OriginData/LevelAnimEditor/Skeletal");

	import->ImportFBXFolder("Test","../../JUSIN_160_FINAL_TEAM_RESOURCE/SampleClient/Models/LightObject");

	return 0;
}
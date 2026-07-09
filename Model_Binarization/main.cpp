#include "pch.h"
#include "Importer.h"


using namespace std;

int main()
{
	shared_ptr<CImporter> import = make_shared<CImporter>();


	//import->ImportFBXFolder("","../../JUSIN_160_FINAL_TEAM_RESOURCE/SampleClient/Models/OriginData/Skeleton");
	import->ImportFBXFolder_ForMapJson("","../../JUSIN_160_FINAL_TEAM_RESOURCE/SampleClient/Models/OriginData/Static","C:/JUSIN_160_FINAL_TEAM_RESOURCE/SampleClient/Models/StaticModelJson");
	return 0;
}

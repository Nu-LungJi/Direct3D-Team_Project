#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <DirectXTex.h>
#include <directxtk/SimpleMath.h>
#include <directxtk/WICTextureLoader.h>
#include <directxtk/DDSTextureLoader.h>
#include <directxtk/SpriteFont.h>
#include <directxtk/SpriteBatch.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>
//using namespace DirectX::SimpleMath;
using namespace DirectX;
#include <wrl/client.h>
using namespace Microsoft::WRL;
#define DIRECTINPUT_VERSION	0x0800
#include <dinput.h>
#pragma warning(disable : 4251)
#include <d3dcompiler.h>

#include <shared_mutex>
#include <cstdint>
#include <stack>
#include <queue>
#include <vector>
#include <list>
#include <map>
#include <algorithm>
#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <ctime>
#include <memory>
#include <optional>
#include <future>
#include <random>
#include <limits>

// bitwise 
#pragma warning(disable: 26813)

#include "Engine_Typedef.h"
#include "Engine_Macro.h"
#include "Engine_Enum.h"
#include "Engine_Struct.h"
#include "Engine_Struct_Vertex.h"
#include "Engine_Struct_ConstantBuffer.h"
#include "Engine_Function.h"
#include "Engine_Template.h"
#include "Engine_Tag.h"
#include "Engine_Assimp_Enum.h"
#include "Engine_Base.h"
namespace E = Engine;

// for study
//#include "ExStruct.h"

#include "tracy/Tracy.hpp"

#include <imgui.h>
#include <ImGuizmo.h>

#include <nlohmann/json.hpp>


using namespace Engine;

#ifdef _DEBUG

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#ifndef DBG_NEW 

#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ ) 
#define new DBG_NEW 

#endif
#endif
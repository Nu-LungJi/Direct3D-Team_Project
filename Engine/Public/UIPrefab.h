#pragma once

#include "GameObject.h"
#include "UIObject.h"
#include "UIPrefabNode.h"

NS_BEGIN(Engine)

class CUIPrefab
{
    std::string Name;

    float Width;
    float Height;

    CUIPrefabNode RootNode;
};

NS_END
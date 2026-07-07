#pragma once
class CUIPrefabNode
{
public:

private:
    UI_TYPE Type;

    std::string Name;

    float LocalX;
    float LocalY;

    float WidthRatio;
    float HeightRatio;

    float AlphaRatio;

    int WeightOffset;

    std::string ResTag;

    std::vector<CUIPrefabNode> Children;
};


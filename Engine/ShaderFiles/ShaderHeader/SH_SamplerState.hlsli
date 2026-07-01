SamplerState SamplerWrap
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
    AddressW = Wrap;
};
SamplerState SamplerMirror
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Mirror;
    AddressV = Mirror;
    AddressW = Mirror;
};
SamplerState SamplerClamp
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Clamp;
    AddressV = Clamp;
    AddressW = Clamp;
};
SamplerState SamplerBorder
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Border;
    AddressV = Border;
    AddressW = Border;
};
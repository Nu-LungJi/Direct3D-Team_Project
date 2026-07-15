#pragma once
#include "Engine_Defines.h"

NS_BEGIN(Engine)

class CGameInstanceInitLoader
{
public:
	static HRESULT InitLoadStart();

private:
	static HRESULT LoadPrototype();
private:
	static HRESULT LoadPrototypeComponent();
	static HRESULT LoadPrototypeGameObject();

private:
	static HRESULT LoadBuffer();
private:
	static HRESULT LoadBufferConstant();
	static HRESULT LoadBufferVertexIndex();

private:
	static HRESULT LoadRenderState();
private:
	static HRESULT LoadBlendState();
	static HRESULT LoadRasterizerState();
	static HRESULT LoadDepthStencilState();
	static HRESULT LoadSamplerState();

private:
	static HRESULT LoadShader();

private:
	static HRESULT LoadTexture();

private:
	static HRESULT LoadLua();

private:
	static HRESULT LoadModel();

private:
	static HRESULT LoadAnimModel();
	static HRESULT LoadStaticModel();
};

NS_END

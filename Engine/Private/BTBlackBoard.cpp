#include "pch.h"
#include "BTBlackBoard.h"

CBTBlackBoard::CBTBlackBoard()
{
}

CBTBlackBoard::~CBTBlackBoard()
{
}
HRESULT CBTBlackBoard::Initialize()
{

	return S_OK;
}


UPtr<CBTBlackBoard> CBTBlackBoard::Create()
{
	auto pInstance = ToUPtr(new CBTBlackBoard);
	if (FAILED(pInstance->Initialize()))
	{
		return nullptr;
	}
	return pInstance;
}

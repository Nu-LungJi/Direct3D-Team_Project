#pragma once
#include "Engine_Defines.h"

NS_BEGIN(Client)

class IEditorCommand
{
public:
	virtual ~IEditorCommand() = default;
public:
	virtual _bool Execute() = 0;
	virtual _bool Undo() = 0;
};

NS_END

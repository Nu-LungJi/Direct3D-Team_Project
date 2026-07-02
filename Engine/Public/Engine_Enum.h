#pragma once

namespace Engine
{
	enum class WINMODE { FULL, WIN };
	enum class MOUSEKEYSTATE { LB, RB, MB, END };
	enum class MOUSEMOVESTATE { X, Y, Z, END };
	enum class RENDERGROUP { PRIORITY, NONBLEND, BLEND, SKYBOX, COLLIDER, UI, END };
	enum class RENDERPASS : uint32_t
	{
		DEFAULT = 1 << 0,
		SHADOW = 1 << 1,
	};
	enum class STATE { RIGHT, UP, LOOK, POSITION, END };
	enum class MODEL { NONANIM, ANIM, END };
	enum class NODETYPE {START,NODE_END ,END};
	//enum class VSYNC{ OFF, ON };



}
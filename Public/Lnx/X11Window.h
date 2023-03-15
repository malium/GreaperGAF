/***********************************************************************************
*   Copyright 2022 Marcos Sánchez Torrent.                                         *
*   All Rights Reserved.                                                           *
***********************************************************************************/

#pragma once

#ifndef GAL_X11_WINDOW_H
#define GAL_X11_WINDOW_H 1

#include "LnxWindow.h"

namespace greaper::gal
{
	struct X11WindowDesc : public LnxWindowDesc
	{
	protected:
		DisplayProtocol_t DisplayProtocol = DisplayProtocol_t::X11;
		
	public:
		StringView X11ClassName = ""sv;
		StringView X11InstanceName = ""sv;
	};
	
	class X11Window : public IWindow
	{
		
	};
}

#endif /* GAL_X11_WINDOW_H */
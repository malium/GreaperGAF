/***********************************************************************************
*   Copyright 2022 Marcos Sánchez Torrent.                                         *
*   All Rights Reserved.                                                           *
***********************************************************************************/

#pragma once

#ifndef GAL_VK_WL_WINDOW_H
#define GAL_VK_WL_WINDOW_H 1

#include "WLWindow.h"

namespace greaper::gal
{
	struct VkWLWindowDesc : public WLWindowDesc
	{
	protected:
		RenderBackend_t Backend = RenderBackend_t::Vulkan;
	public:
		
	};
	
	class VkWLWindow : public WLWindow
	{
		
	};
}

#endif /* GAL_VK_WL_WINDOW_H */
/***********************************************************************************
*   Copyright 2022 Marcos Sánchez Torrent.                                         *
*   All Rights Reserved.                                                           *
***********************************************************************************/

#pragma once

#ifndef GAL_GL_WL_WINDOW_H
#define GAL_GL_WL_WINDOW_H 1

#include "WLWindow.h"
#include "../OpenGL/GLDefines.h"
#include "../OpenGL/GALGLX.h"

namespace greaper::gal
{
	struct GLWLWindowDesc : public WLWindowDesc
	{
	protected:
		RenderBackend_t Backend = RenderBackend_t::OpenGL;
	public:
		int32 VersionMajor = -1; // Negative values selects the maximum version supported by the adapter
		int32 VersionMinor = -1; // Negative values selects the maximum version supported by the adapter
		OpenGLProfile_t Profile = OpenGLProfile_t::Core;
		bool ContextDebug = GREAPER_DEBUG; // GL_KHR_debug
		OpenGLContextRobustness_t ContextRobustness = OpenGLContextRobustness_t::NoRobustness;
		OpenGLReleaseBehaviour_t ContextReleaseBehaviour = OpenGLReleaseBehaviour_t::Flush; // GL_KHR_context_flush_control
		bool ContextGenerateErrors = true; // GL_KHR_no_error
		PWindow SharedContextWindow = PWindow();
	};
	
	class GLWLWindow : public WLWindow
	{
		
	};
}

#endif /* GAL_GL_WL_WINDOW_H */
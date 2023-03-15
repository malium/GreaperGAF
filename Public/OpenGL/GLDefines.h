/***********************************************************************************
*   Copyright 2022 Marcos Sánchez Torrent.                                         *
*   All Rights Reserved.                                                           *
***********************************************************************************/

#pragma once

#ifndef GAL_GL_DEFINES_H
#define GAL_GL_DEFINES_H 1

#include "../GALPrerequisites.h"
#include "../../../GreaperCore/Public/Enumeration.h"

ENUMERATION(OpenGLProfile, Compatibility, Core);
ENUMERATION(OpenGLContextRobustness, NoRobustness, NoResetNotification, LooseContextOnReset);
ENUMERATION(OpenGLReleaseBehaviour, Flush, None);

#endif /* GAL_GL_DEFINES_H */
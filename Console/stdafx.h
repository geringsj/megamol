/*
 * stdafx.h
 *
 * Copyright (C) 2006 - 2008 by Universitaet Stuttgart (VIS). 
 * Alle Rechte vorbehalten.
 */

#ifndef MEGAMOLCON_STDAFX_H_INCLUDED
#define MEGAMOLCON_STDAFX_H_INCLUDED
#if (defined(_MSC_VER) && (_MSC_VER > 1000))
#pragma once
#endif /* (defined(_MSC_VER) && (_MSC_VER > 1000)) */

#ifdef _WIN32
/* Windows includes */

#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers

#else /* _WIN32 */
/* Linux includes */

#endif /* _WIN32 */

/* common includes */
#include <stdio.h>
#include "vislib/tchar.h"
#include "vislib/types.h"

/**
 * Answers an ASCII string representing the trademark character.
 *
 * @return An ASCII string representing the trademark character.
 */
const char* ASCIIStringTM(void);

#endif /* MEGAMOLCON_STDAFX_H_INCLUDED */
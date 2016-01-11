/******************************************************************************/
//
// Copyright 2015 Foster T. Brereton.
// See license.md in this repository for license details.
//
/******************************************************************************/

#ifndef switches_hpp__
#define switches_hpp__

/******************************************************************************/

#include <boost/predef.h>

/******************************************************************************/

#if defined(NDEBUG)
    #define qRelease 1
#else
    #define qDebug 1
#endif

#define qDebugOff (qDebug && 0)

#if BOOST_OS_MACOS
    #define qMac 1
#endif

/******************************************************************************/

#endif // switches_hpp__

/******************************************************************************/

#pragma once
/*
 *      Copyright (C) 2005-2015 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi; see the file copying.txt. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "kodi/libXBMC_addon.h"
#include "kodi/libXBMC_pvr.h"

/**
 * Quell unused parameter warnings
 */
#ifndef _UNUSED
#if defined(__GNUC__)
#define _UNUSED(x) UNUSED_ ## x __attribute__((unused))
#elif defined(__LCLINT__)
#define _UNUSED(x) /*@unused@*/ x
#else
#define _UNUSED(x) x
#endif
#endif

/**
 * Helpers
 */
extern CHelper_libXBMC_pvr          *PVR;
extern ADDON::CHelper_libXBMC_addon *XBMC;

/**
 * Connection client settings
 */
extern std::string g_strHostname; /**< Hostname IP/URL */
extern int g_iPortWebHTTP;        /**< Hostname webinterface HTTP port */
extern int g_iPortStream;         /**< Hostname webinterface stream port */
extern bool g_bUseAuthentication; /**< Hostname use authentication */
extern std::string g_strUsername; /**< Hostname username */
extern std::string g_strPassword; /**< Hostname password */
extern bool g_bUseSecureHTTP;     /**< Hostname use HTTPS */
extern int g_iPortWebHTTPS;       /**< Hostname webinterface HTTPS port */

/**
 * Channels client settings
 */
extern bool g_bSelectTVChannelGroups;            /**< Load only selected TV channel groups */
extern int g_iNumTVChannelGroupsToLoad;          /**< Number of TV channel groups to load */
extern std::string g_strTVChannelGroupNameOne;   /**< Name of TV channel group one to load */
extern std::string g_strTVChannelGroupNameTwo;   /**< Name of TV channel group two to load */
extern std::string g_strTVChannelGroupNameThree; /**< Name of TV channel group three to load */
extern std::string g_strTVChannelGroupNameFour;  /**< Name of TV channel group four to load */
extern std::string g_strTVChannelGroupNameFive;  /**< Name of TV channel group five to load */
extern bool g_bLoadRadioChannelsGroup;           /**< Load Radio channels group */
extern bool g_bZapBeforeChannelChange;           /**< Zap before channel change */

/**
 * Recordings/Timers client settings
 */
extern bool g_bLoadRecordings;                /**< Load recordings from backend */
extern std::string g_strBackendRecordingPath; /**< Backend recording path */
extern bool g_bUseOnlyCurrentRecordingPath;   /**< Use only current recording path */
extern bool g_bAutomaticTimerlistCleanup;     /**< Automatic timer list cleanup */

/**
 * Advanced client settings
 */
extern bool g_bUseTimeshift;                 /**< Use timeshift */
extern std::string g_strTimeshiftBufferPath; /**< Timeshift buffer path */
extern bool g_bLoadWebInterfacePicons;       /**< Use hostname webinterface picons */
extern std::string g_strPiconsLocationPath;  /**< Hostname picons path */
extern int g_iClientUpdateInterval;          /**< Client update interval in minutes */
extern bool g_bSendDeepStanbyToSTB;          /**< Send deep standby command to STB */
extern bool g_bExtraDebug;                   /**< Enable extra debug mode (silence extremely verbose crap) */

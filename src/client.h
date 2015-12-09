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

/*!
 * @brief Quell unused parameter warnings
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


/*!
 * @brief Helpers
 */
extern CHelper_libXBMC_pvr          *PVR;
extern ADDON::CHelper_libXBMC_addon *XBMC;

/*!
 * @brief Connection client settings
 */
extern std::string g_strHostname; /*!< @brief Hostname IP/URL */
extern int g_iPortWebHTTP;        /*!< @brief Hostname webinterface HTTP port */
extern int g_iPortStream;         /*!< @brief Hostname webinterface stream port */
extern bool g_bUseAuthentication; /*!< @brief Hostname use authentication */
extern std::string g_strUsername; /*!< @brief Hostname username */
extern std::string g_strPassword; /*!< @brief Hostname password */
extern bool g_bUseSecureHTTP;     /*!< @brief Hostname use HTTPS */
extern int g_iPortWebHTTPS;       /*!< @brief Hostname webinterface HTTPS port */

/*!
 * @brief Channels client settings
 */
extern bool g_bSelectTVChannelGroups;            /*!< @brief Load only selected TV channel groups */
extern int g_iNumTVChannelGroupsToLoad;          /*!< @brief Number of TV channel groups to load */
extern std::string g_strTVChannelGroupNameOne;   /*!< @brief Name of TV channel group one to load */
extern std::string g_strTVChannelGroupNameTwo;   /*!< @brief Name of TV channel group two to load */
extern std::string g_strTVChannelGroupNameThree; /*!< @brief Name of TV channel group three to load */
extern std::string g_strTVChannelGroupNameFour;  /*!< @brief Name of TV channel group four to load */
extern std::string g_strTVChannelGroupNameFive;  /*!< @brief Name of TV channel group five to load */
extern bool g_bLoadRadioChannelsGroup;           /*!< @brief Load Radio channels group */
extern bool g_bZapBeforeChannelChange;           /*!< @brief Zap before channel change */

/*!
 * @brief Recordings/Timers client settings
 */
extern std::string g_strBackendRecordingPath; /*!< @brief Backend recording path */
extern bool g_bUseOnlyCurrentRecordingPath;   /*!< @brief Use only current recording path */
extern bool g_bAutomaticTimerlistCleanup;     /*!< @brief Automatic timer list cleanup */

/*!
 * @brief Advanced client settings
 */
extern bool g_bUseTimeshift;                 /*!< @brief Use timeshift */
extern std::string g_strTimeshiftBufferPath; /*!< @brief Timeshift buffer path */
extern bool g_bLoadWebInterfacePicons;       /*!< @brief Use hostname webinterface picons */
extern std::string g_strPiconsLocationPath;  /*!< @brief Hostname picons path */
extern int g_iClientUpdateInterval;          /*!< @brief Client update interval in minutes */
extern bool g_bSendDeepStanbyToSTB;          /*!< @brief Send deep standby command to STB */


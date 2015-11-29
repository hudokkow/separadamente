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

#include <string>

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
 * @brief Default client settings. Only change defaults here.
 */
#define DEFAULT_HOSTNAME                        "127.0.0.1"                                      /*!< @brief Hostname location */
#define DEFAULT_STREAM_PORT                     8001                                             /*!< @brief Stream port */
#define DEFAULT_WEB_PORT_HTTP                   80                                               /*!< @brief HTTP port */
#define DEFAULT_WEB_PORT_HTTPS                  443                                              /*!< @brief HTTPS  port */
#define DEFAULT_USE_AUTHENTICATION              false                                            /*!< @brief Use authentication */
#define DEFAULT_USERNAME                        ""                                               /*!< @brief Username */
#define DEFAULT_PASSWORD                        ""                                               /*!< @brief Password */
#define DEFAULT_USE_HTTPS                       false                                            /*!< @brief Use HTTPS */

#define DEFAULT_SELECT_TV_CHANNEL_GROUPS        false                                            /*!< @brief Select TV channel groups */
#define DEFAULT_NUM_TV_CHANNEL_GROUPS_TO_LOAD   0                                                /*!< @brief Number of TV channel groups to load */
#define DEFAULT_TV_CHANNEL_GROUP_NAME_1         ""                                               /*!< @brief TV channel group name #1 */
#define DEFAULT_TV_CHANNEL_GROUP_NAME_2         ""                                               /*!< @brief TV channel group name #2 */
#define DEFAULT_TV_CHANNEL_GROUP_NAME_3         ""                                               /*!< @brief TV channel group name #3 */
#define DEFAULT_TV_CHANNEL_GROUP_NAME_4         ""                                               /*!< @brief TV channel group name #4 */
#define DEFAULT_TV_CHANNEL_GROUP_NAME_5         ""                                               /*!< @brief TV channel group name #5 */
#define DEFAULT_LOAD_RADIO_CHANNELS_GROUP       false                                            /*!< @brief Load Radio channels group */
#define DEFAULT_ZAP_BEFORE_CHANNEL_CHANGE       false                                            /*!< @brief Zap before channel change (single tuner STB's) */

#define DEFAULT_BACKEND_RECORDING_PATH          ""                                               /*!< @brief Backend recording folder */
#define DEFAULT_USE_ONLY_CURRENT_RECORDING_PATH false                                            /*!< @brief Only record under active STB folder */
#define DEFAULT_AUTOMATIC_TIMERLIST_CLEANUP     true                                             /*!< @brief Clean completed timers automatically */

#define DEFAULT_USE_TIMESHIFT                   false                                            /*!< @brief Default use time shifting */
#define DEFAULT_TSBUFFER_PATH                   "special://userdata/addon_data/pvr.enigma2.stb"  /*!< @brief Default time shifting buffer path */
#define DEFAULT_LOAD_WEB_INTERFACE_PICONS       true                                             /*!< @brief Load picons from web interface */
#define DEFAULT_PICONS_LOCATION_PATH            ""                                               /*!< @brief Picons location folder */
#define DEFAULT_UPDATE_INTERVAL                 20                                               /*!< @brief Update interval in minutes (timers, recordings, etc.) */
#define DEFAULT_SEND_DEEP_STANBY_TO_STB         false                                            /*!< @brief Send deep standby to STB */

/*!
 * @brief Connection client settings
 */
extern std::string g_strHostname;        /*!< @brief Hostname IP/URL */
extern int         g_iPortWebHTTP;       /*!< @brief Hostname webinterface HTTP port */
extern int         g_iPortStream;        /*!< @brief Hostname webinterface stream port */
extern bool        g_bUseAuthentication; /*!< @brief Hostname use authentication */
extern std::string g_strUsername;        /*!< @brief Hostname username */
extern std::string g_strPassword;        /*!< @brief Hostname password */
extern bool        g_bUseSecureHTTP;     /*!< @brief Hostname use HTTPS */
extern int         g_iPortWebHTTPS;      /*!< @brief Hostname webinterface HTTPS port */

/*!
 * @brief Channels client settings
 */
extern bool        g_bSelectTVChannelGroups;     /*!< @brief Load only selected TV channel groups */
extern int         g_iNumTVChannelGroupsToLoad;  /*!< @brief Number of TV channel groups to load */
extern std::string g_strTVChannelGroupNameOne;   /*!< @brief Name of TV channel group one to load */
extern std::string g_strTVChannelGroupNameTwo;   /*!< @brief Name of TV channel group two to load */
extern std::string g_strTVChannelGroupNameThree; /*!< @brief Name of TV channel group three to load */
extern std::string g_strTVChannelGroupNameFour;  /*!< @brief Name of TV channel group four to load */
extern std::string g_strTVChannelGroupNameFive;  /*!< @brief Name of TV channel group five to load */
extern bool        g_bLoadRadioChannelsGroup;    /*!< @brief Load Radio channels group */
extern bool        g_bZapBeforeChannelChange;    /*!< @brief Zap before channel change */

/*!
 * @brief Recordings/Timers client settings
 */
extern std::string g_strBackendRecordingPath;      /*!< @brief Hostname recording path */
extern bool        g_bUseOnlyCurrentRecordingPath; /*!< @brief Use only current recording path */
extern bool        g_bAutomaticTimerlistCleanup;   /*!< @brief Automatic timer list cleanup */

/*!
 * @brief Advanced client settings
 */
extern bool        g_bUseTimeshift;           /*!< @brief Use timeshift */
extern std::string g_strTimeshiftBufferPath;  /*!< @brief Timeshift buffer path */
extern bool        g_bLoadWebInterfacePicons; /*!< @brief Use hostname webinterface picons */
extern std::string g_strPiconsLocationPath;   /*!< @brief Hostname picons path */
extern int         g_iClientUpdateInterval;   /*!< @brief Client update interval in minutes */
extern bool        g_bSendDeepStanbyToSTB;    /*!< @brief Send deep standby command to STB */

/*!
 * @brief Client settings
 */
extern bool        m_bCreated;     /*!< @brief Create function was successfully called */
extern std::string g_szUserPath;   /*!< @brief Path to user directory inside user profile */
extern std::string g_szClientPath; /*!< @brief Path to driver location */

extern CHelper_libXBMC_pvr          *PVR;
extern ADDON::CHelper_libXBMC_addon *XBMC;


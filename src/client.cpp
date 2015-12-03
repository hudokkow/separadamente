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

#include "client.h"

#include "E2STBData.h"

#include "kodi/libKODI_guilib.h"
#include "kodi/xbmc_pvr_dll.h"

#include <cstdlib>
#include <string>

ADDON_STATUS m_CurStatus   = ADDON_STATUS_UNKNOWN;
bool m_bCreated            = false;
std::string g_szUserPath;
std::string g_szClientPath;

std::string g_strHostname = DEFAULT_HOSTNAME;
int g_iPortStream         = DEFAULT_STREAM_PORT;
int g_iPortWebHTTP        = DEFAULT_WEB_PORT_HTTP;
bool g_bUseAuthentication = DEFAULT_USE_AUTHENTICATION;
std::string g_strUsername = DEFAULT_USERNAME;
std::string g_strPassword = DEFAULT_PASSWORD;
bool g_bUseSecureHTTP     = DEFAULT_USE_HTTPS;
int g_iPortWebHTTPS       = DEFAULT_WEB_PORT_HTTPS;

bool g_bSelectTVChannelGroups            = DEFAULT_SELECT_TV_CHANNEL_GROUPS;
int g_iNumTVChannelGroupsToLoad          = DEFAULT_NUM_TV_CHANNEL_GROUPS_TO_LOAD;
std::string g_strTVChannelGroupNameOne   = DEFAULT_TV_CHANNEL_GROUP_NAME_1;
std::string g_strTVChannelGroupNameTwo   = DEFAULT_TV_CHANNEL_GROUP_NAME_2;
std::string g_strTVChannelGroupNameThree = DEFAULT_TV_CHANNEL_GROUP_NAME_3;
std::string g_strTVChannelGroupNameFour  = DEFAULT_TV_CHANNEL_GROUP_NAME_4;
std::string g_strTVChannelGroupNameFive  = DEFAULT_TV_CHANNEL_GROUP_NAME_5;
bool g_bLoadRadioChannelsGroup           = DEFAULT_LOAD_RADIO_CHANNELS_GROUP;
bool g_bZapBeforeChannelChange           = DEFAULT_ZAP_BEFORE_CHANNEL_CHANGE;

std::string g_strBackendRecordingPath = DEFAULT_BACKEND_RECORDING_PATH;
bool g_bUseOnlyCurrentRecordingPath   = DEFAULT_USE_ONLY_CURRENT_RECORDING_PATH;
bool g_bAutomaticTimerlistCleanup     = DEFAULT_AUTOMATIC_TIMERLIST_CLEANUP;

bool g_bUseTimeshift                 = DEFAULT_USE_TIMESHIFT;
std::string g_strTimeshiftBufferPath = DEFAULT_TSBUFFER_PATH;
bool g_bLoadWebInterfacePicons       = DEFAULT_LOAD_WEB_INTERFACE_PICONS;
std::string g_strPiconsLocationPath  = DEFAULT_PICONS_LOCATION_PATH;
int g_iClientUpdateInterval          = DEFAULT_UPDATE_INTERVAL;
bool g_bSendDeepStanbyToSTB          = DEFAULT_SEND_DEEP_STANBY_TO_STB;

CE2STBData                   *E2STBData = NULL;
CHelper_libXBMC_pvr          *PVR       = NULL;
ADDON::CHelper_libXBMC_addon *XBMC      = NULL;

extern "C"
{
  /********************************************//**
   * Read settings from settings.xml, generate a
   * new file if it's not found / new install
   ***********************************************/
  void ADDON_ReadSettings(void)
  {
    /********************************************//**
     * Read Connection Settings
     ***********************************************/
    /* Read setting "hostname" from settings.xml */
    char * buffer = (char*) malloc(128);
    buffer[0] = 0; /* Set the end of string */
    if (XBMC->GetSetting("hostname", buffer))
    {
      g_strHostname = buffer;
    }
    else
    {
      XBMC->Log(ADDON::LOG_ERROR, "Couldn't get hostname setting. Default: %s", DEFAULT_HOSTNAME);
      g_strHostname = DEFAULT_HOSTNAME;
    }
    free(buffer);

    /* Read setting "webporthttp" from settings.xml */
    if (!XBMC->GetSetting("webporthttp", &g_iPortWebHTTP))
    {
      XBMC->Log(ADDON::LOG_ERROR, "Couldn't get HTTP web interface port setting. Default: %d", DEFAULT_WEB_PORT_HTTP);
      g_iPortWebHTTP = DEFAULT_WEB_PORT_HTTP;
    }

    /* Read setting "streamport" from settings.xml */
    if (!XBMC->GetSetting("streamport", &g_iPortStream))
    {
      XBMC->Log(ADDON::LOG_ERROR, "Couldn't get stream port setting. Default: %d", DEFAULT_STREAM_PORT);
      g_iPortStream = DEFAULT_STREAM_PORT;
    }

    /* Read setting "useauthentication" from settings.xml */
    if (!XBMC->GetSetting("useauthentication", &g_bUseAuthentication))
    {
      XBMC->Log(ADDON::LOG_ERROR, "Couldn't get authentication setting. Default: false");
      g_bUseAuthentication = DEFAULT_USE_AUTHENTICATION;
    }

    /* Read setting "username" from settings.xml */
    buffer = (char*) malloc(64);
    buffer[0] = 0; /* Set the end of string */
    if (XBMC->GetSetting("username", buffer))
    {
      g_strUsername = buffer;
    }
    else
    {
      XBMC->Log(ADDON::LOG_ERROR, "Couldn't get username setting. Default: (empty)");
      g_strUsername = DEFAULT_USERNAME;
    }
    free(buffer);

    /* Read setting "password" from settings.xml */
    buffer = (char*) malloc(64);
    buffer[0] = 0; /* Set the end of string */
    if (XBMC->GetSetting("password", buffer))
    {
      g_strPassword = buffer;
    }
    else
    {
      XBMC->Log(ADDON::LOG_ERROR, "Couldn't get password setting. Default: (empty)");
      g_strPassword = DEFAULT_PASSWORD;
    }
    free(buffer);

    /* Read setting "usesecurehttp" from settings.xml */
    if (!XBMC->GetSetting("usesecurehttp", &g_bUseSecureHTTP))
    {
      XBMC->Log(ADDON::LOG_ERROR, "Couldn't get secure HTTP setting. Default: false");
      g_bUseSecureHTTP = DEFAULT_USE_HTTPS;
    }

    /* Read setting "webporthttps" from settings.xml */
    if (!XBMC->GetSetting("webporthttps", &g_iPortWebHTTPS))
    {
      XBMC->Log(ADDON::LOG_ERROR, "Couldn't get HTTPS web interface port setting. Default: %d", DEFAULT_WEB_PORT_HTTPS);
      g_iPortWebHTTPS = DEFAULT_WEB_PORT_HTTPS;
    }

    /********************************************//**
     * Read Channel Settings
     ***********************************************/
    /* Read setting "selecttvchannelgroups" from settings.xml */
    if (!XBMC->GetSetting("selecttvchannelgroups", &g_bSelectTVChannelGroups))
    {
      XBMC->Log(ADDON::LOG_ERROR, "Couldn't get select TV channels setting. Default: false");
      g_bSelectTVChannelGroups = DEFAULT_SELECT_TV_CHANNEL_GROUPS;
    }

    /* Read setting "tvchannelgroupstoload" from settings.xml */
    if (!XBMC->GetSetting("tvchannelgroupstoload", &g_iNumTVChannelGroupsToLoad))
    {
      XBMC->Log(ADDON::LOG_ERROR, "Couldn't get number of TV channel groups setting. Default: %d",
          DEFAULT_NUM_TV_CHANNEL_GROUPS_TO_LOAD);
      g_iNumTVChannelGroupsToLoad = DEFAULT_NUM_TV_CHANNEL_GROUPS_TO_LOAD;
    }

    /* Read setting "tvchannelgroupone" from settings.xml */
    buffer = (char*) malloc(64);
    buffer[0] = 0; /* Set the end of string */
    if (XBMC->GetSetting("tvchannelgroupone", buffer))
    {
      g_strTVChannelGroupNameOne = buffer;
    }
    else
    {
      XBMC->Log(ADDON::LOG_ERROR, "Couldn't get TV channel group #1 setting. Default: %s",
          DEFAULT_TV_CHANNEL_GROUP_NAME_1);
      g_strTVChannelGroupNameOne = DEFAULT_TV_CHANNEL_GROUP_NAME_1;
    }
    free(buffer);

    /* Read setting "tvchannelgrouptwo" from settings.xml */
    buffer = (char*) malloc(64);
    buffer[0] = 0; /* Set the end of string */
    if (XBMC->GetSetting("tvchannelgrouptwo", buffer))
    {
      g_strTVChannelGroupNameTwo = buffer;
    }
    else
    {
      XBMC->Log(ADDON::LOG_ERROR, "Couldn't get TV channel group #2 setting. Default: %s",
          DEFAULT_TV_CHANNEL_GROUP_NAME_2);
      g_strTVChannelGroupNameTwo = DEFAULT_TV_CHANNEL_GROUP_NAME_2;
    }
    free(buffer);

    /* Read setting "tvchannelgroupthree" from settings.xml */
    buffer = (char*) malloc(64);
    buffer[0] = 0; /* Set the end of string */
    if (XBMC->GetSetting("tvchannelgroupthree", buffer))
    {
      g_strTVChannelGroupNameThree = buffer;
    }
    else
    {
      XBMC->Log(ADDON::LOG_ERROR, "Couldn't get TV channel group #3 setting. Default: %s",
          DEFAULT_TV_CHANNEL_GROUP_NAME_3);
      g_strTVChannelGroupNameThree = DEFAULT_TV_CHANNEL_GROUP_NAME_3;
    }
    free(buffer);

    /* Read setting "tvchannelgroupfour" from settings.xml */
    buffer = (char*) malloc(64);
    buffer[0] = 0; /* Set the end of string */
    if (XBMC->GetSetting("tvchannelgroupfour", buffer))
    {
      g_strTVChannelGroupNameFour = buffer;
    }
    else
    {
      XBMC->Log(ADDON::LOG_ERROR, "Couldn't get TV channel group #4 setting. Default: %s",
          DEFAULT_TV_CHANNEL_GROUP_NAME_4);
      g_strTVChannelGroupNameFour = DEFAULT_TV_CHANNEL_GROUP_NAME_4;
    }
    free(buffer);

    /* Read setting "tvchannelgroupfive" from settings.xml */
    buffer = (char*) malloc(64);
    buffer[0] = 0; /* Set the end of string */
    if (XBMC->GetSetting("tvchannelgroupfive", buffer))
    {
      g_strTVChannelGroupNameFive = buffer;
    }
    else
    {
      XBMC->Log(ADDON::LOG_ERROR, "Couldn't get TV channel group #5 setting. Default: %s",
          DEFAULT_TV_CHANNEL_GROUP_NAME_5);
      g_strTVChannelGroupNameFive = DEFAULT_TV_CHANNEL_GROUP_NAME_5;
    }
    free(buffer);

    /* Read setting "loadradiochannelsgroup" from settings.xml */
    if (!XBMC->GetSetting("loadradiochannelsgroup", &g_bLoadRadioChannelsGroup))
    {
      XBMC->Log(ADDON::LOG_ERROR, "Couldn't get load Radio channels group setting. Default: false");
      g_bLoadRadioChannelsGroup = DEFAULT_LOAD_RADIO_CHANNELS_GROUP;
    }

    /* Read setting "zap" from settings.xml */
    if (!XBMC->GetSetting("zap", &g_bZapBeforeChannelChange))
    {
      XBMC->Log(ADDON::LOG_ERROR, "Couldn't get zap before channel change setting. Default: false");
      g_bZapBeforeChannelChange = DEFAULT_ZAP_BEFORE_CHANNEL_CHANGE;
    }

    /********************************************//**
     * Read Recordings/Timers Settings
     ***********************************************/
    /* Read setting "recordingpath" from settings.xml */
    buffer = (char*) malloc(512);
    buffer[0] = 0; /* Set the end of string */
    if (XBMC->GetSetting("recordingpath", buffer))
    {
      g_strBackendRecordingPath = buffer;
    }
    else
    {
      XBMC->Log(ADDON::LOG_ERROR, "Couldn't get recording path setting. Default: %s", DEFAULT_BACKEND_RECORDING_PATH);
      g_strBackendRecordingPath = DEFAULT_BACKEND_RECORDING_PATH;
    }
    free(buffer);

    /* Read setting "onlycurrentrecordingpath" from settings.xml */
    if (!XBMC->GetSetting("onlycurrentrecordingpath", &g_bUseOnlyCurrentRecordingPath))
    {
      XBMC->Log(ADDON::LOG_ERROR, "Couldn't get use only current recording path setting. Default: false");
      g_bUseOnlyCurrentRecordingPath = DEFAULT_USE_ONLY_CURRENT_RECORDING_PATH;
    }

    /* Read setting "timerlistcleanup" from settings.xml */
    if (!XBMC->GetSetting("timerlistcleanup", &g_bAutomaticTimerlistCleanup))
    {
      XBMC->Log(ADDON::LOG_ERROR, "Couldn't get automatic timer list cleanup setting. Default: true");
      g_bAutomaticTimerlistCleanup = DEFAULT_AUTOMATIC_TIMERLIST_CLEANUP;
    }

    /********************************************//**
     * Read Advanced Settings
     ***********************************************/

    /* Read setting "usetimeshift" from settings.xml */
    if (!XBMC->GetSetting("usetimeshift", &g_bUseTimeshift))
    {
      XBMC->Log(ADDON::LOG_ERROR, "Couldn't get use only current recording path setting. Default: false");
      g_bUseTimeshift = DEFAULT_USE_TIMESHIFT;
    }

    /* Read setting "timeshiftpath" from settings.xml */
    buffer = (char*) malloc(512);
    buffer[0] = 0; /* Set the end of string */
    if (XBMC->GetSetting("timeshiftpath", buffer))
    {
      g_strTimeshiftBufferPath = buffer;
    }
    else
    {
      XBMC->Log(ADDON::LOG_ERROR, "Couldn't get time shift buffer path setting. Default: %s", DEFAULT_TSBUFFER_PATH);
      g_strTimeshiftBufferPath = DEFAULT_TSBUFFER_PATH;
    }
    free(buffer);

    /* Read setting "onlinepicons" from settings.xml */
    if (!XBMC->GetSetting("onlinepicons", &g_bLoadWebInterfacePicons))
    {
      XBMC->Log(ADDON::LOG_ERROR, "Couldn't get use online picons setting. Default: true");
      g_bLoadWebInterfacePicons = DEFAULT_LOAD_WEB_INTERFACE_PICONS;
    }

    /* Read setting "piconspath" from settings.xml */
    buffer = (char*) malloc(512);
    buffer[0] = 0; /* Set the end of string */
    if (XBMC->GetSetting("piconspath", buffer))
    {
      g_strPiconsLocationPath = buffer;
    }
    else
    {
      XBMC->Log(ADDON::LOG_ERROR, "Couldn't get picons location path setting. Default: %s",
          DEFAULT_PICONS_LOCATION_PATH);
      g_strPiconsLocationPath = DEFAULT_PICONS_LOCATION_PATH;
    }
    free(buffer);

    /* Read setting "updateinterval" from settings.xml */
    if (!XBMC->GetSetting("updateinterval", &g_iClientUpdateInterval))
    {
      XBMC->Log(ADDON::LOG_ERROR, "Couldn't get update interval setting. Default: %dm", DEFAULT_UPDATE_INTERVAL);
      g_iClientUpdateInterval = DEFAULT_UPDATE_INTERVAL;
    }

    /* Read setting "sendpowerstate" from settings.xml */
    if (!XBMC->GetSetting("sendpowerstate", &g_bSendDeepStanbyToSTB))
    {
      XBMC->Log(ADDON::LOG_ERROR, "Couldn't get send deep standby to STB setting. Default: false");
      g_bSendDeepStanbyToSTB = DEFAULT_SEND_DEEP_STANBY_TO_STB;
    }

    /********************************************//**
     * Debug block
     * Log the crap out of client settings for
     * debugging purposes
     ***********************************************/
    XBMC->Log(ADDON::LOG_DEBUG, "Configuration options");
    XBMC->Log(ADDON::LOG_DEBUG, "Hostname: %s", g_strHostname.c_str());

    if (g_bUseAuthentication && !g_strUsername.empty() && !g_strPassword.empty())
    {
      XBMC->Log(ADDON::LOG_DEBUG, "Username: %s", g_strUsername.c_str());
      XBMC->Log(ADDON::LOG_DEBUG, "Password: %s", g_strPassword.c_str());

      if ((g_strUsername.find("@") != std::string::npos))
      {
        XBMC->Log(ADDON::LOG_ERROR, "The '@' character isn't supported in the username field");
      }

      if ((g_strPassword.find("@") != std::string::npos))
      {
        XBMC->Log(ADDON::LOG_ERROR, "The '@' character isn't supported in the password field");
      }
    }

    XBMC->Log(ADDON::LOG_DEBUG, "Web interface port: %d", (g_bUseSecureHTTP) ? g_iPortWebHTTPS : g_iPortWebHTTP);
    XBMC->Log(ADDON::LOG_DEBUG, "Streaming port: %d", g_iPortStream);
    XBMC->Log(ADDON::LOG_DEBUG, "Use authentication: %s", (g_bUseAuthentication) ? "yes" : "no");
    XBMC->Log(ADDON::LOG_DEBUG, "Use HTTPS: %s", (g_bUseSecureHTTP) ? "yes" : "no");
    XBMC->Log(ADDON::LOG_DEBUG, "Select TV channel groups: %s", (g_bSelectTVChannelGroups) ? "yes" : "no");

    if (g_bSelectTVChannelGroups)
    {
      XBMC->Log(ADDON::LOG_DEBUG, "TV channel groups to load: %d", g_iNumTVChannelGroupsToLoad);

      if (!g_strTVChannelGroupNameOne.empty() && g_iNumTVChannelGroupsToLoad >= 1)
      {
        XBMC->Log(ADDON::LOG_DEBUG, "TV channel group #1: %s", g_strTVChannelGroupNameOne.c_str());
      }

      if (!g_strTVChannelGroupNameTwo.empty() && g_iNumTVChannelGroupsToLoad >= 2)
      {
        XBMC->Log(ADDON::LOG_DEBUG, "TV channel group #2: %s", g_strTVChannelGroupNameTwo.c_str());
      }

      if (!g_strTVChannelGroupNameThree.empty() && g_iNumTVChannelGroupsToLoad >= 3)
      {
        XBMC->Log(ADDON::LOG_DEBUG, "TV channel group #3: %s", g_strTVChannelGroupNameThree.c_str());
      }

      if (!g_strTVChannelGroupNameFour.empty() && g_iNumTVChannelGroupsToLoad >= 4)
      {
        XBMC->Log(ADDON::LOG_DEBUG, "TV channel group #4: %s", g_strTVChannelGroupNameFour.c_str());
      }

      if (!g_strTVChannelGroupNameFive.empty() && g_iNumTVChannelGroupsToLoad == 5)
      {
        XBMC->Log(ADDON::LOG_DEBUG, "TV channel group #5: %s", g_strTVChannelGroupNameFive.c_str());
      }
    }
    XBMC->Log(ADDON::LOG_DEBUG, "Load Radio channels group: %s", (g_bLoadRadioChannelsGroup) ? "yes" : "no");
    XBMC->Log(ADDON::LOG_DEBUG, "Use time shifting: %s", (g_bUseTimeshift) ? "yes" : "no");

    if (g_bUseTimeshift)
    {
      XBMC->Log(ADDON::LOG_DEBUG, "Time shift buffer path: %s", g_strTimeshiftBufferPath.c_str());
    }

    XBMC->Log(ADDON::LOG_DEBUG, "Use online picons: %s", (g_bLoadWebInterfacePicons) ? "yes" : "no");
    XBMC->Log(ADDON::LOG_DEBUG, "Send deep standby to STB: %s", (g_bSendDeepStanbyToSTB) ? "yes" : "no");
    XBMC->Log(ADDON::LOG_DEBUG, "Zap before channel change: %s", (g_bZapBeforeChannelChange) ? "yes" : "no");
    XBMC->Log(ADDON::LOG_DEBUG, "Automatic timer list cleanup: %s", (g_bAutomaticTimerlistCleanup) ? "yes" : "no");
    XBMC->Log(ADDON::LOG_DEBUG, "Update interval: %dm", g_iClientUpdateInterval);
  }

  /********************************************//**
   * Create
   * Called after loading. All steps to make
   * client functional must be performed here.
   ***********************************************/
  ADDON_STATUS ADDON_Create(void* hdl, void* props)
  {
    if (!hdl || !props)
      return ADDON_STATUS_UNKNOWN;

    PVR_PROPERTIES* pvrprops = (PVR_PROPERTIES*) props;

    XBMC = new ADDON::CHelper_libXBMC_addon;
    if (!XBMC->RegisterMe(hdl))
    {
      delete XBMC;
      XBMC = NULL;
      return ADDON_STATUS_PERMANENT_FAILURE;
    }

    PVR = new CHelper_libXBMC_pvr;
    if (!PVR->RegisterMe(hdl))
    {
      delete PVR;
      PVR = NULL;
      delete XBMC;
      XBMC = NULL;
      return ADDON_STATUS_PERMANENT_FAILURE;
    }

    XBMC->Log(ADDON::LOG_DEBUG, "[%s] Creating Enigma2 STB Client",
        __FUNCTION__);

    m_CurStatus = ADDON_STATUS_UNKNOWN;
    g_szUserPath = pvrprops->strUserPath;
    g_szClientPath = pvrprops->strClientPath;

    ADDON_ReadSettings();

    E2STBData = new CE2STBData;

    if (!E2STBData->Open())
    {
      delete E2STBData;
      E2STBData = NULL;
      delete PVR;
      PVR = NULL;
      delete XBMC;
      XBMC = NULL;
      m_CurStatus = ADDON_STATUS_LOST_CONNECTION;
      return m_CurStatus;
    }

    m_CurStatus = ADDON_STATUS_OK;
    m_bCreated = true;
    return m_CurStatus;
  }

  ADDON_STATUS ADDON_GetStatus()
  {
    if (m_CurStatus == ADDON_STATUS_OK && !E2STBData->IsConnected())
    {
      m_CurStatus = ADDON_STATUS_LOST_CONNECTION;
    }
    return m_CurStatus;
  }

  void ADDON_Destroy()
  {
    if (m_bCreated)
    {
      m_bCreated = false;
    }

    if (E2STBData)
    {
      E2STBData->SendPowerstate();
      delete E2STBData;
      E2STBData = NULL;
    }

    if (PVR)
    {
      delete PVR;
      PVR = NULL;
    }

    if (XBMC)
    {
      delete XBMC;
      XBMC = NULL;
    }
    m_CurStatus = ADDON_STATUS_UNKNOWN;
  }

  bool ADDON_HasSettings()
  {
    return true;
  }

  unsigned int ADDON_GetSettings(ADDON_StructSetting ***_UNUSED(sSet))
  {
    return 0;
  }

  /********************************************//**
   * Read settings from settings.xml, compare to
   * UI settings, update settings.xml accordingly
   ***********************************************/
  ADDON_STATUS ADDON_SetSetting(const char *settingName,
      const void *settingValue)
  {
    std::string str = settingName;
    if (str == "hostname")
    {
      std::string tmp_sHostname;
      XBMC->Log(ADDON::LOG_NOTICE, "[%s] Changed hostname from %s to %s", __FUNCTION__, g_strHostname.c_str(),
          (const char*) settingValue);
      tmp_sHostname = g_strHostname;
      g_strHostname = (const char*) settingValue;
      if (tmp_sHostname != g_strHostname)
      {
        return ADDON_STATUS_NEED_RESTART;
      }
    }
    else if (str == "webporthttp")
    {
      int iNewValue = *(int*) settingValue + 1;
      if (g_iPortWebHTTP != iNewValue)
      {
        XBMC->Log(ADDON::LOG_NOTICE, "[%s] Changed HTTP web interface port from %u to %u", __FUNCTION__,
            g_iPortWebHTTP, iNewValue);
        g_iPortWebHTTP = iNewValue;
        return ADDON_STATUS_OK;
      }
    }
    else if (str == "streamport")
    {
      int iNewValue = *(int*) settingValue + 1;
      if (g_iPortStream != iNewValue)
      {
        XBMC->Log(ADDON::LOG_NOTICE, "[%s] Changed streaming port from %u to %u", __FUNCTION__,
            g_iPortStream, iNewValue);
        g_iPortStream = iNewValue;
        return ADDON_STATUS_OK;
      }
    }
    else if (str == "username")
    {
      std::string tmp_sUsername = g_strUsername;
      g_strUsername = (const char*) settingValue;
      if (tmp_sUsername != g_strUsername)
      {
        XBMC->Log(ADDON::LOG_NOTICE, "[%s] Changed username ", __FUNCTION__);
        return ADDON_STATUS_NEED_RESTART;
      }
    }
    else if (str == "password")
    {
      std::string tmp_sPassword = g_strPassword;
      g_strPassword = (const char*) settingValue;
      if (tmp_sPassword != g_strPassword)
      {
        XBMC->Log(ADDON::LOG_NOTICE, "[%s] Changed password ", __FUNCTION__);
        return ADDON_STATUS_NEED_RESTART;
      }
    }
    else if (str == "usesecurehttp")
    {
      XBMC->Log(ADDON::LOG_NOTICE, "[%s] Changed use HTTPS from %u to %u",
          __FUNCTION__, g_bUseSecureHTTP, *(int*) settingValue);
      g_bUseSecureHTTP = *(bool*) settingValue;
      return ADDON_STATUS_NEED_RESTART;
    }
    else if (str == "webporthttps")
    {
      int iNewValue = *(int*) settingValue + 1;
      if (g_iPortWebHTTPS != iNewValue)
      {
        XBMC->Log(ADDON::LOG_NOTICE, "[%s] Changed HTTPS web interface port from %u to %u", __FUNCTION__,
            g_iPortWebHTTPS, iNewValue);
        g_iPortWebHTTPS = iNewValue;
        return ADDON_STATUS_OK;
      }
    }
    else if (str == "usetimeshift")
    {
      XBMC->Log(ADDON::LOG_NOTICE, "[%s] Changed use time shifting from %u to %u", __FUNCTION__,
          g_bUseTimeshift, *(int*) settingValue);
      g_bUseTimeshift = *(bool*) settingValue;
      return ADDON_STATUS_NEED_RESTART;
    }
    else if (str == "timeshiftpath")
    {
      std::string tmp_sTimeshiftBufferPath = g_strTimeshiftBufferPath;
      XBMC->Log(ADDON::LOG_NOTICE, "[%s] Changed time shifting buffer from %s to %s", __FUNCTION__,
          g_strTimeshiftBufferPath.c_str(), (const char*) settingValue);
      g_strTimeshiftBufferPath = (const char*) settingValue;
      if (tmp_sTimeshiftBufferPath != g_strTimeshiftBufferPath)
      {
        return ADDON_STATUS_NEED_RESTART;
      }
    }
    return ADDON_STATUS_OK;
  }
  void ADDON_Stop() {}
  void ADDON_FreeSettings() {}
  void ADDON_Announce(const char *_UNUSED(flag), const char *_UNUSED(sender), const char *_UNUSED(message),
      const void *_UNUSED(data)) {}

  /**************************************************************************//**
   * PVR Client Addon specific public library functions
   *****************************************************************************/
  /********************************************//**
   * Capabilities and API
   ***********************************************/
  const char* GetPVRAPIVersion(void)
  {
    static const char *strApiVersion = XBMC_PVR_API_VERSION;
    return strApiVersion;
  }

  const char* GetMininumPVRAPIVersion(void)
  {
    static const char *strMinApiVersion = XBMC_PVR_MIN_API_VERSION;
    return strMinApiVersion;
  }

  const char* GetGUIAPIVersion(void)
  {
    return KODI_GUILIB_API_VERSION;
  }

  const char* GetMininumGUIAPIVersion(void)
  {
    return KODI_GUILIB_MIN_API_VERSION;
  }

  PVR_ERROR GetAddonCapabilities(PVR_ADDON_CAPABILITIES* pCapabilities)
  {
    pCapabilities->bSupportsEPG                = true;
    pCapabilities->bSupportsTV                 = true;
    pCapabilities->bSupportsRadio              = g_bLoadRadioChannelsGroup;
    pCapabilities->bSupportsRecordings         = true;
    pCapabilities->bSupportsRecordingsUndelete = false;
    pCapabilities->bSupportsTimers             = true;
    pCapabilities->bSupportsChannelGroups      = true;
    pCapabilities->bSupportsChannelScan        = false;
    pCapabilities->bHandlesInputStream         = true;
    pCapabilities->bHandlesDemuxing            = false;
    pCapabilities->bSupportsLastPlayedPosition = false;

    return PVR_ERROR_NO_ERROR;
  }

  /********************************************//**
   * Client creation and connection
   ***********************************************/
  const char *GetBackendName(void)
  {
    static const char *strBackendName = E2STBData ? E2STBData->GetServerName() : "Unknown Backend Name";
    return strBackendName;
  }

  const char *GetBackendVersion(void)
  {
    static const char *strBackendVersion = XBMC_PVR_API_VERSION;
    return strBackendVersion;
  }

  static CStdString strConnectionString;

  const char *GetConnectionString(void)
  {
    if (E2STBData)
    {
      strConnectionString.Format("%s%s", g_strHostname.c_str(), E2STBData->IsConnected() ? "" : "Not connected!");
    }
    else
    {
      strConnectionString.Format("%s addon error!", g_strHostname.c_str());
    }
    return strConnectionString.c_str();
  }

  const char *GetBackendHostname(void)
  {
    return g_strHostname.c_str();
  }

  /********************************************//**
   * Channels
   ***********************************************/
  int GetChannelsAmount(void)
  {
    if (!E2STBData || !E2STBData->IsConnected())
    {
      return 0;
    }
    return E2STBData->GetChannelsAmount();
  }

  int GetCurrentClientChannel(void)
  {
    if (!E2STBData || !E2STBData->IsConnected())
    {
      return PVR_ERROR_SERVER_ERROR;
    }
    return E2STBData->GetCurrentClientChannel();
  }

  PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio)
  {
    if (!E2STBData || !E2STBData->IsConnected())
    {
      return PVR_ERROR_SERVER_ERROR;
    }
    return E2STBData->GetChannels(handle, bRadio);
  }

  PVR_ERROR GetChannelGroups(ADDON_HANDLE handle, bool bRadio)
  {
    if (bRadio)
    {
      return PVR_ERROR_NO_ERROR;
    }

    if (!E2STBData || !E2STBData->IsConnected())
    {
      return PVR_ERROR_SERVER_ERROR;
    }
    return E2STBData->GetChannelGroups(handle);
  }

  PVR_ERROR GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group)
  {
    if (group.bIsRadio)
    {
      return PVR_ERROR_NO_ERROR;
    }

    if (!E2STBData || !E2STBData->IsConnected())
    {
      return PVR_ERROR_SERVER_ERROR;
    }
    return E2STBData->GetChannelGroupMembers(handle, group);
  }

  int GetChannelGroupsAmount(void)
  {
    if (!E2STBData || !E2STBData->IsConnected())
    {
      return PVR_ERROR_SERVER_ERROR;
    }
    return E2STBData->GetChannelGroupsAmount();
  }

  /********************************************//**
   * EPG
   ***********************************************/
  PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
  {
    if (!E2STBData || !E2STBData->IsConnected())
    {
      return PVR_ERROR_SERVER_ERROR;
    }
    return E2STBData->GetEPGForChannel(handle, channel, iStart, iEnd);
  }

  /********************************************//**
   * Information
   ***********************************************/
  PVR_ERROR GetDriveSpace(long long *iTotal, long long *iUsed)
  {
    return E2STBData->GetDriveSpace(iTotal, iUsed);
  }

  PVR_ERROR SignalStatus(PVR_SIGNAL_STATUS &signalStatus)
  {
    return E2STBData->SignalStatus(signalStatus);
  }

  /********************************************//**
   * Recordings
   ***********************************************/
  int GetRecordingsAmount(bool deleted)
  {
    if (!E2STBData || !E2STBData->IsConnected())
    {
      return PVR_ERROR_SERVER_ERROR;
    }
    return E2STBData->GetRecordingsAmount();
  }

  PVR_ERROR GetRecordings(ADDON_HANDLE handle, bool deleted)
  {
    if (!E2STBData || !E2STBData->IsConnected())
    {
      return PVR_ERROR_SERVER_ERROR;
    }
    return E2STBData->GetRecordings(handle);
  }

  PVR_ERROR DeleteRecording(const PVR_RECORDING &recording)
  {
    if (!E2STBData || !E2STBData->IsConnected())
    {
      return PVR_ERROR_SERVER_ERROR;
    }
    return E2STBData->DeleteRecording(recording);
  }

  PVR_ERROR RenameRecording(const PVR_RECORDING &_UNUSED(recording))
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }

  /********************************************//**
   * Stream handling
   ***********************************************/
  bool OpenLiveStream(const PVR_CHANNEL &channel)
  {
    if (!E2STBData || !E2STBData->IsConnected())
    {
      return false;
    }
    return E2STBData->OpenLiveStream(channel);
  }

  bool SwitchChannel(const PVR_CHANNEL &channel)
  {
    if (!E2STBData || !E2STBData->IsConnected())
    {
      return false;
    }
    return E2STBData->SwitchChannel(channel);
  }

  void CloseLiveStream(void)
  {
    E2STBData->CloseLiveStream();
  }
  const char *GetLiveStreamURL(const PVR_CHANNEL &channel)
  {
    if (!E2STBData || !E2STBData->IsConnected())
    {
      return "";
    }
    return E2STBData->GetLiveStreamURL(channel);
  }

  /********************************************//**
   * Timers
   ***********************************************/
  PVR_ERROR GetTimerTypes(PVR_TIMER_TYPE types[], int *size)
  {
    /* TODO: Implement this to get support for the timer features introduced with PVR API 1.9.7 */
    return PVR_ERROR_NOT_IMPLEMENTED;
  }

  int GetTimersAmount(void)
  {
    if (!E2STBData || !E2STBData->IsConnected())
    {
      return 0;
    }
    return E2STBData->GetTimersAmount();
  }

  PVR_ERROR GetTimers(ADDON_HANDLE handle)
  {
    if (!E2STBData || !E2STBData->IsConnected())
    {
      return PVR_ERROR_SERVER_ERROR;
    }
    /* TODO: Change implementation to get support for the timer features introduced with PVR API 1.9.7 */
    return E2STBData->GetTimers(handle);
  }

  PVR_ERROR AddTimer(const PVR_TIMER &timer)
  {
    if (!E2STBData || !E2STBData->IsConnected())
    {
      return PVR_ERROR_SERVER_ERROR;
    }
    return E2STBData->AddTimer(timer);
  }

  PVR_ERROR DeleteTimer(const PVR_TIMER &timer, bool _UNUSED(bForceDelete))
  {
    if (!E2STBData || !E2STBData->IsConnected())
    {
      return PVR_ERROR_SERVER_ERROR;
    }
    return E2STBData->DeleteTimer(timer);
  }

  PVR_ERROR UpdateTimer(const PVR_TIMER &timer)
  {
    if (!E2STBData || !E2STBData->IsConnected())
    {
      return PVR_ERROR_SERVER_ERROR;
    }
    return E2STBData->UpdateTimer(timer);
  }

  /********************************************//**
   * Time shifting
   ***********************************************/
  bool CanPauseStream(void)
  {
    if (!E2STBData || !E2STBData->IsConnected())
    {
      return false;
    }
    return g_bUseTimeshift;
  }

  bool CanSeekStream(void)
  {
    if (!E2STBData || !E2STBData->IsConnected())
    {
      return false;
    }
    return g_bUseTimeshift;
  }

  int ReadLiveStream(unsigned char *pBuffer, unsigned int iBufferSize)
  {
    if (!E2STBData || !E2STBData->IsConnected() || !E2STBData->GetTimeshiftBuffer())
    {
      return 0;
    }
    return E2STBData->GetTimeshiftBuffer()->ReadData(pBuffer, iBufferSize);
  }

  long long SeekLiveStream(long long iPosition, int iWhence /* = SEEK_SET */)
  {
    if (!E2STBData || !E2STBData->IsConnected() || !E2STBData->GetTimeshiftBuffer())
    {
      return -1;
    }
    return E2STBData->GetTimeshiftBuffer()->Seek(iPosition, iWhence);
  }

  long long PositionLiveStream(void)
  {
    if (!E2STBData || !E2STBData->IsConnected() || !E2STBData->GetTimeshiftBuffer())
    {
      return -1;
    }
    return E2STBData->GetTimeshiftBuffer()->Position();
  }

  long long LengthLiveStream(void)
  {
    if (!E2STBData || !E2STBData->IsConnected() || !E2STBData->GetTimeshiftBuffer())
    {
      return 0;
    }
    return E2STBData->GetTimeshiftBuffer()->Length();
  }

  time_t GetBufferTimeStart()
  {
    if (!E2STBData || !E2STBData->IsConnected() || !E2STBData->GetTimeshiftBuffer())
    {
      return 0;
    }
    return E2STBData->GetTimeshiftBuffer()->TimeStart();
  }

  time_t GetBufferTimeEnd()
  {
    if (!E2STBData || !E2STBData->IsConnected() || !E2STBData->GetTimeshiftBuffer())
    {
      return 0;
    }
    return E2STBData->GetTimeshiftBuffer()->TimeEnd();
  }

  time_t GetPlayingTime()
  {
    /* FIXME: this should return the time of the *current* position */
    return GetBufferTimeEnd();
  }

  /**************************************************************************//**
   * Unused API functions
   *****************************************************************************/
  /* Channel handling */
  PVR_ERROR    OpenDialogChannelAdd(const PVR_CHANNEL &_UNUSED(channel)) { return PVR_ERROR_NOT_IMPLEMENTED; }
  PVR_ERROR    OpenDialogChannelScan(void) { return PVR_ERROR_NOT_IMPLEMENTED; }
  PVR_ERROR    OpenDialogChannelSettings(const PVR_CHANNEL &_UNUSED(channel)) { return PVR_ERROR_NOT_IMPLEMENTED; }
  PVR_ERROR    DeleteChannel(const PVR_CHANNEL &_UNUSED(channel)) { return PVR_ERROR_NOT_IMPLEMENTED; }
  PVR_ERROR    MoveChannel(const PVR_CHANNEL &_UNUSED(channel)) { return PVR_ERROR_NOT_IMPLEMENTED; }
  PVR_ERROR    RenameChannel(const PVR_CHANNEL &_UNUSED(channel)) { return PVR_ERROR_NOT_IMPLEMENTED; }
  unsigned int GetChannelSwitchDelay(void) { return 0; }

  /* Demuxer */
  bool         SeekTime(int _UNUSED(time), bool _UNUSED(backwards), double *_UNUSED(startpts)) { return false; }
  void         DemuxAbort(void) { return; }
  void         DemuxFlush(void) {}
  PVR_ERROR    GetStreamProperties(PVR_STREAM_PROPERTIES *_UNUSED(pProperties)) { return PVR_ERROR_NOT_IMPLEMENTED; }
  DemuxPacket* DemuxRead(void) { return NULL; }
  void         DemuxReset(void) {}
  void         PauseStream(bool _UNUSED(bPaused)) {}
  void         SetSpeed(int) {}

  /* Recordings */
  int       ReadRecordedStream(unsigned char *_UNUSED(pBuffer), unsigned int _UNUSED(iBufferSize)) { return 0; }
  int       GetRecordingLastPlayedPosition(const PVR_RECORDING &_UNUSED(recording)) { return PVR_ERROR_NOT_IMPLEMENTED; }
  bool      OpenRecordedStream(const PVR_RECORDING &_UNUSED(recording)) { return false; }
  void      CloseRecordedStream(void) {}
  long long LengthRecordedStream(void) { return 0; }
  long long PositionRecordedStream(void) { return -1; }
  long long SeekRecordedStream(long long _UNUSED(iPosition), int _UNUSED(iWhence) /* = SEEK_SET */) { return 0; }
  PVR_ERROR DeleteAllRecordingsFromTrash() { return PVR_ERROR_NOT_IMPLEMENTED; }
  PVR_ERROR GetRecordingEdl(const PVR_RECORDING&, PVR_EDL_ENTRY[], int*) { return PVR_ERROR_NOT_IMPLEMENTED; }
  PVR_ERROR SetRecordingPlayCount(const PVR_RECORDING &_UNUSED(recording), int _UNUSED(count)) { return PVR_ERROR_NOT_IMPLEMENTED; }
  PVR_ERROR SetRecordingLastPlayedPosition(const PVR_RECORDING &_UNUSED(recording), int _UNUSED(lastplayedposition)) { return PVR_ERROR_NOT_IMPLEMENTED; }
  PVR_ERROR UndeleteRecording(const PVR_RECORDING& recording) { return PVR_ERROR_NOT_IMPLEMENTED; }

  /* Time shifting */
  /* TODO: implement */
  bool IsTimeshifting(void) { return false; }

  /* Menu hook */
  PVR_ERROR CallMenuHook(const PVR_MENUHOOK &_UNUSED(menuhook), const PVR_MENUHOOK_DATA &_UNUSED(item)) { return PVR_ERROR_NOT_IMPLEMENTED; }
}

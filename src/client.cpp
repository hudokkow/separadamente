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

#include "E2STBChannels.h"
#include "E2STBConnection.h"
#include "E2STBData.h"
#include "E2STBRecordings.h"
#include "E2STBVersion.h"

#include "kodi/libXBMC_addon.h"
#include "kodi/libXBMC_pvr.h"
#include "kodi/xbmc_addon_types.h"
#include "kodi/xbmc_pvr_dll.h"
#include "kodi/xbmc_pvr_types.h"
#include "p8-platform/util/util.h"

#include <ctime>
#include <cstdlib>
#include <string>

using namespace e2stb;

/*!
 * @brief Initialize helpers
 */
CHelper_libXBMC_pvr          *PVR  = NULL;
ADDON::CHelper_libXBMC_addon *XBMC = NULL;

/*!
 * @brief Initialize globals
 */
ADDON_STATUS      g_currentStatus   = ADDON_STATUS_UNKNOWN;
CE2STBChannels   *g_E2STBChannels   = nullptr;
CE2STBConnection *g_E2STBConnection = nullptr;
CE2STBData       *g_E2STBData       = nullptr;
CE2STBRecordings *g_E2STBRecordings = nullptr;

/*!
 * @brief Connection client settings
 */
std::string g_strHostname  = "127.0.0.1";
int g_iPortWebHTTP         = 80;
int g_iPortStream          = 8001;
bool g_bUseAuthentication  = false;
std::string g_strUsername;
std::string g_strPassword;
bool g_bUseSecureHTTP      = false;
int g_iPortWebHTTPS        = 443;

/*!
 * @brief Channels client settings
 */
bool g_bSelectTVChannelGroups             = false;
int g_iNumTVChannelGroupsToLoad           = 0;
std::string g_strTVChannelGroupNameOne;
std::string g_strTVChannelGroupNameTwo;
std::string g_strTVChannelGroupNameThree;
std::string g_strTVChannelGroupNameFour;
std::string g_strTVChannelGroupNameFive;
bool g_bLoadRadioChannelsGroup            = false;
bool g_bZapBeforeChannelChange            = false;

/*!
 * @brief Recordings/Timers client settings
 */
std::string g_strBackendRecordingPath;
bool g_bUseOnlyCurrentRecordingPath    = false;
bool g_bAutomaticTimerlistCleanup      = true;

/*!
 * @brief Advanced client settings
 */
bool g_bUseTimeshift                 = false;
std::string g_strTimeshiftBufferPath = "special://userdata/addon_data/pvr.enigma2.stb";
bool g_bLoadWebInterfacePicons       = true;
std::string g_strPiconsLocationPath;
int g_iClientUpdateInterval          = 120;
bool g_bSendDeepStanbyToSTB          = false;
/* TODO: Implement setting on UI options */
bool g_bExtraDebug                   = false;

extern "C"
{
void ADDON_ReadSettings(void)
{
  /*!
   * @brief Read settings from settings.xml. Generate a new file if it's not found / new install
   */
  char * buffer = (char*) malloc(1024);
  if (XBMC->GetSetting("hostname", buffer))
    g_strHostname = buffer;

  if (!XBMC->GetSetting("webporthttp", &g_iPortWebHTTP))
    g_iPortWebHTTP = 80;

  if (!XBMC->GetSetting("streamport", &g_iPortStream))
    g_iPortStream = 8001;

  if (!XBMC->GetSetting("useauthentication", &g_bUseAuthentication))
    g_bUseAuthentication = false;

  if (XBMC->GetSetting("username", buffer))
    g_strUsername = buffer;

  if (XBMC->GetSetting("password", buffer))
    g_strPassword = buffer;

  if (!XBMC->GetSetting("usesecurehttp", &g_bUseSecureHTTP))
    g_bUseSecureHTTP = false;

  if (!XBMC->GetSetting("webporthttps", &g_iPortWebHTTPS))
    g_iPortWebHTTPS = 443;

  if (!XBMC->GetSetting("selecttvchannelgroups", &g_bSelectTVChannelGroups))
    g_bSelectTVChannelGroups = false;

  if (!XBMC->GetSetting("tvchannelgroupstoload", &g_iNumTVChannelGroupsToLoad))
    g_iNumTVChannelGroupsToLoad = 0;

  if (XBMC->GetSetting("tvchannelgroupone", buffer))
    g_strTVChannelGroupNameOne = buffer;

  if (XBMC->GetSetting("tvchannelgrouptwo", buffer))
    g_strTVChannelGroupNameTwo = buffer;

  if (XBMC->GetSetting("tvchannelgroupthree", buffer))
    g_strTVChannelGroupNameThree = buffer;

  if (XBMC->GetSetting("tvchannelgroupfour", buffer))
    g_strTVChannelGroupNameFour = buffer;

  if (XBMC->GetSetting("tvchannelgroupfive", buffer))
    g_strTVChannelGroupNameFive = buffer;

  if (!XBMC->GetSetting("loadradiochannelsgroup", &g_bLoadRadioChannelsGroup))
    g_bLoadRadioChannelsGroup = false;

  if (!XBMC->GetSetting("zap", &g_bZapBeforeChannelChange))
    g_bZapBeforeChannelChange = false;

  if (XBMC->GetSetting("recordingpath", buffer))
    g_strBackendRecordingPath = buffer;

  if (!XBMC->GetSetting("onlycurrentrecordingpath", &g_bUseOnlyCurrentRecordingPath))
    g_bUseOnlyCurrentRecordingPath = false;

  if (!XBMC->GetSetting("timerlistcleanup", &g_bAutomaticTimerlistCleanup))
    g_bAutomaticTimerlistCleanup = true;

  if (!XBMC->GetSetting("usetimeshift", &g_bUseTimeshift))
    g_bUseTimeshift = false;

  if (XBMC->GetSetting("timeshiftpath", buffer))
    g_strTimeshiftBufferPath = buffer;

  if (!XBMC->GetSetting("onlinepicons", &g_bLoadWebInterfacePicons))
    g_bLoadWebInterfacePicons = true;

  if (XBMC->GetSetting("piconspath", buffer))
    g_strPiconsLocationPath = buffer;

  if (!XBMC->GetSetting("updateinterval", &g_iClientUpdateInterval))
    g_iClientUpdateInterval = 120;

  if (!XBMC->GetSetting("sendpowerstate", &g_bSendDeepStanbyToSTB))
    g_bSendDeepStanbyToSTB = false;

  free(buffer);

  /*!
   * @brief Log the crap out of client settings for debugging purposes
   */
  XBMC->Log(ADDON::LOG_DEBUG, "Version: %s", g_strAddonVersion.c_str());
  XBMC->Log(ADDON::LOG_DEBUG, "Hostname: %s", g_strHostname.c_str());

  if (g_bUseAuthentication && !g_strUsername.empty() && !g_strPassword.empty())
  {
    XBMC->Log(ADDON::LOG_DEBUG, "Username: %s", g_strUsername.c_str());
    XBMC->Log(ADDON::LOG_DEBUG, "Password: %s", g_strPassword.c_str());

    if ((g_strUsername.find("@") != std::string::npos))
      XBMC->Log(ADDON::LOG_ERROR, "The '@' character isn't supported in the username field");

    if ((g_strPassword.find("@") != std::string::npos))
      XBMC->Log(ADDON::LOG_ERROR, "The '@' character isn't supported in the password field");
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
      XBMC->Log(ADDON::LOG_DEBUG, "TV channel group #1: %s", g_strTVChannelGroupNameOne.c_str());

    if (!g_strTVChannelGroupNameTwo.empty() && g_iNumTVChannelGroupsToLoad >= 2)
      XBMC->Log(ADDON::LOG_DEBUG, "TV channel group #2: %s", g_strTVChannelGroupNameTwo.c_str());

    if (!g_strTVChannelGroupNameThree.empty() && g_iNumTVChannelGroupsToLoad >= 3)
      XBMC->Log(ADDON::LOG_DEBUG, "TV channel group #3: %s", g_strTVChannelGroupNameThree.c_str());

    if (!g_strTVChannelGroupNameFour.empty() && g_iNumTVChannelGroupsToLoad >= 4)
      XBMC->Log(ADDON::LOG_DEBUG, "TV channel group #4: %s", g_strTVChannelGroupNameFour.c_str());

    if (!g_strTVChannelGroupNameFive.empty() && g_iNumTVChannelGroupsToLoad == 5)
      XBMC->Log(ADDON::LOG_DEBUG, "TV channel group #5: %s", g_strTVChannelGroupNameFive.c_str());
  }

  XBMC->Log(ADDON::LOG_DEBUG, "Load Radio channels group: %s", (g_bLoadRadioChannelsGroup) ? "yes" : "no");
  XBMC->Log(ADDON::LOG_DEBUG, "Use time shifting: %s", (g_bUseTimeshift) ? "yes" : "no");

  if (g_bUseTimeshift)
    XBMC->Log(ADDON::LOG_DEBUG, "Time shift buffer located at: %s", g_strTimeshiftBufferPath.c_str());

  XBMC->Log(ADDON::LOG_DEBUG, "Use online picons: %s", (g_bLoadWebInterfacePicons) ? "yes" : "no");
  XBMC->Log(ADDON::LOG_DEBUG, "Send deep standby to STB: %s", (g_bSendDeepStanbyToSTB) ? "yes" : "no");
  XBMC->Log(ADDON::LOG_DEBUG, "Zap before channel change: %s", (g_bZapBeforeChannelChange) ? "yes" : "no");
  XBMC->Log(ADDON::LOG_DEBUG, "Automatic timer list cleanup: %s", (g_bAutomaticTimerlistCleanup) ? "yes" : "no");
  XBMC->Log(ADDON::LOG_DEBUG, "Update interval: %dm", g_iClientUpdateInterval);
}

/*!
 * @brief Called after loading. All steps to make client functional must be performed here
 */
ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
  if (!hdl || !props)
    return ADDON_STATUS_UNKNOWN;

  /* Instantiate helpers */
  PVR  = new CHelper_libXBMC_pvr;
  XBMC = new ADDON::CHelper_libXBMC_addon;

  if (!PVR->RegisterMe(hdl) || !XBMC->RegisterMe(hdl))
  {
    SAFE_DELETE(PVR);
    SAFE_DELETE(XBMC);
    return ADDON_STATUS_PERMANENT_FAILURE;
  }

  XBMC->Log(ADDON::LOG_DEBUG, "[%s] Creating Enigma2 STB Client", __FUNCTION__);
  g_currentStatus = ADDON_STATUS_UNKNOWN;

  ADDON_ReadSettings();

  /* Instantiate globals */
  g_E2STBChannels   = new CE2STBChannels;
  g_E2STBConnection = new CE2STBConnection;
  g_E2STBData       = new CE2STBData;
  g_E2STBRecordings = new CE2STBRecordings;

  /* TODO: reorganize calls */
  if (!g_E2STBConnection->Initialize())
  {
    SAFE_DELETE(g_E2STBChannels);
    SAFE_DELETE(g_E2STBConnection);
    SAFE_DELETE(g_E2STBData);
    SAFE_DELETE(g_E2STBRecordings);
    SAFE_DELETE(PVR);
    SAFE_DELETE(XBMC);
    g_currentStatus = ADDON_STATUS_LOST_CONNECTION;
    return g_currentStatus;
  }
  g_currentStatus = ADDON_STATUS_OK;
  return g_currentStatus;
}

ADDON_STATUS ADDON_GetStatus()
{
  return g_currentStatus;
}

void ADDON_Destroy()
{
  g_E2STBConnection->SendPowerstate();
  SAFE_DELETE(g_E2STBChannels);
  SAFE_DELETE(g_E2STBConnection);
  SAFE_DELETE(g_E2STBData);
  SAFE_DELETE(g_E2STBRecordings);
  SAFE_DELETE(PVR);
  SAFE_DELETE(XBMC);
  g_currentStatus = ADDON_STATUS_UNKNOWN;
}

bool ADDON_HasSettings()
{
  return true;
}

unsigned int ADDON_GetSettings(ADDON_StructSetting ***_UNUSED(sSet))
{
  return 0;
}

/*!
 * @brief  Read settings from settings.xml, compare to UI settings, update settings.xml accordingly
 */
ADDON_STATUS ADDON_SetSetting(const char *settingName,
    const void *settingValue)
{
  std::string str = settingName;
  if (str == "hostname")
  {
    std::string tmp_sHostname;
    XBMC->Log(ADDON::LOG_DEBUG, "[%s] Changed hostname from %s to %s", __FUNCTION__, g_strHostname.c_str(),
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
      XBMC->Log(ADDON::LOG_DEBUG, "[%s] Changed HTTP web interface port from %u to %u", __FUNCTION__,
          g_iPortWebHTTP, iNewValue);
      g_iPortWebHTTP = iNewValue;
      return ADDON_STATUS_NEED_RESTART;
    }
  }
  else if (str == "streamport")
  {
    int iNewValue = *(int*) settingValue + 1;
    if (g_iPortStream != iNewValue)
    {
      XBMC->Log(ADDON::LOG_DEBUG, "[%s] Changed streaming port from %u to %u", __FUNCTION__,
          g_iPortStream, iNewValue);
      g_iPortStream = iNewValue;
      return ADDON_STATUS_NEED_RESTART;
    }
  }
  else if (str == "username")
  {
    std::string tmp_sUsername = g_strUsername;
    g_strUsername = (const char*) settingValue;
    if (tmp_sUsername != g_strUsername)
    {
      XBMC->Log(ADDON::LOG_DEBUG, "[%s] Changed username ", __FUNCTION__);
      return ADDON_STATUS_NEED_RESTART;
    }
  }
  else if (str == "password")
  {
    std::string tmp_sPassword = g_strPassword;
    g_strPassword = (const char*) settingValue;
    if (tmp_sPassword != g_strPassword)
    {
      XBMC->Log(ADDON::LOG_DEBUG, "[%s] Changed password ", __FUNCTION__);
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
      XBMC->Log(ADDON::LOG_DEBUG, "[%s] Changed HTTPS web interface port from %u to %u", __FUNCTION__,
          g_iPortWebHTTPS, iNewValue);
      g_iPortWebHTTPS = iNewValue;
      return ADDON_STATUS_NEED_RESTART;
    }
  }
  else if (str == "usetimeshift")
  {
    XBMC->Log(ADDON::LOG_DEBUG, "[%s] Changed use time shifting from %u to %u", __FUNCTION__,
        g_bUseTimeshift, *(int*) settingValue);
    g_bUseTimeshift = *(bool*) settingValue;
    return ADDON_STATUS_NEED_RESTART;
  }
  else if (str == "timeshiftpath")
  {
    std::string tmp_sTimeshiftBufferPath = g_strTimeshiftBufferPath;
    XBMC->Log(ADDON::LOG_DEBUG, "[%s] Changed time shifting buffer from %s to %s", __FUNCTION__,
        g_strTimeshiftBufferPath.c_str(), (const char*) settingValue);
    g_strTimeshiftBufferPath = (const char*) settingValue;
    if (tmp_sTimeshiftBufferPath != g_strTimeshiftBufferPath)
    {
      return ADDON_STATUS_NEED_RESTART;
    }
  }
  return ADDON_STATUS_OK;
}

void ADDON_Stop()
{
}

void ADDON_FreeSettings()
{
}

void ADDON_Announce(const char *_UNUSED(flag), const char *_UNUSED(sender), const char *_UNUSED(message),
    const void *_UNUSED(data))
{
}

/*!
 * @brief PVR client addon specific public library functions (API and capabilities)
 */
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
  /* GUI API is not used */
  return "";
}

const char* GetMininumGUIAPIVersion(void)
{
  /* GUI API is not used */
  return "";
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

/*!
 * @brief PVR client addon backend identification
 */
const char *GetBackendName(void)
{
  static std::string strBackendName = g_E2STBConnection->GetBackendName();
  return strBackendName.c_str();
}

const char *GetBackendVersion(void)
{
  static std::string strBackendVersion = g_E2STBConnection->GetBackendVersion();
  return strBackendVersion.c_str();
}

const char *GetConnectionString(void)
{
  static std::string strBackendURLWeb = g_E2STBConnection->GetBackendURLWeb();
  return strBackendURLWeb.c_str();
}

const char *GetBackendHostname(void)
{
  return g_strHostname.c_str();
}

/*!
 * @brief PVR client addon channels handling
 */
int GetChannelsAmount(void)
{
  return g_E2STBChannels->GetChannelsVector().size();
}

PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio)
{
  return g_E2STBChannels->GetChannels(handle, bRadio);
}

PVR_ERROR GetChannelGroups(ADDON_HANDLE handle, bool bRadio)
{
  if (bRadio)
    return PVR_ERROR_NO_ERROR;

  return g_E2STBChannels->GetChannelGroups(handle);
}

PVR_ERROR GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group)
{
  if (group.bIsRadio)
    return PVR_ERROR_NO_ERROR;

  return g_E2STBChannels->GetChannelGroupMembers(handle, group);
}

int GetChannelGroupsAmount(void)
{
  return g_E2STBChannels->GetChannelGroupsAmount();
}

/*!
 * @brief PVR client addon EPG handling
 */
PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
{
  return g_E2STBChannels->GetEPGForChannel(handle, channel, iStart, iEnd);
}

/*!
 * @brief PVR client addon information handling
 */
PVR_ERROR GetDriveSpace(long long *iTotal, long long *iUsed)
{
  return g_E2STBData->GetDriveSpace(iTotal, iUsed);
}

PVR_ERROR SignalStatus(PVR_SIGNAL_STATUS &signalStatus)
{
  return g_E2STBData->SignalStatus(signalStatus);
}

/*!
 * @brief PVR client addon recordings handling
 */
int GetRecordingsAmount(bool deleted)
{
  return g_E2STBRecordings->GetRecordingsAmount();
}

PVR_ERROR GetRecordings(ADDON_HANDLE handle, bool deleted)
{
  return g_E2STBRecordings->GetRecordings(handle);
}

PVR_ERROR DeleteRecording(const PVR_RECORDING &recording)
{
  return g_E2STBRecordings->DeleteRecording(recording);
}

PVR_ERROR RenameRecording(const PVR_RECORDING &_UNUSED(recording))
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

/*!
 * @brief PVR client addon stream handling
 */
bool OpenLiveStream(const PVR_CHANNEL &channel)
{
  return g_E2STBData->OpenLiveStream(channel);
}

bool SwitchChannel(const PVR_CHANNEL &channel)
{
  return g_E2STBData->SwitchChannel(channel);
}

void CloseLiveStream(void)
{
  g_E2STBData->CloseLiveStream();
}

const char *GetLiveStreamURL(const PVR_CHANNEL &channel)
{
  /* TODO: revisit */
  // return g_E2STBChannels->m_channels.at(channel.iUniqueId - 1).strStreamURL.c_str();
  return g_E2STBChannels->GetLiveStreamURL(channel);
}

/*!
 * @brief PVR client addon timers handling
 */
PVR_ERROR GetTimerTypes(PVR_TIMER_TYPE types[], int *size)
{
  /* TODO: Implement this to get support for the timer features introduced with PVR API 1.9.7 */
  return PVR_ERROR_NOT_IMPLEMENTED;
}

int GetTimersAmount(void)
{
  return g_E2STBData->GetTimersAmount();
}

PVR_ERROR GetTimers(ADDON_HANDLE handle)
{
  /* TODO: Change implementation to get support for the timer features introduced with PVR API 1.9.7 */
  return g_E2STBData->GetTimers(handle);
}

PVR_ERROR AddTimer(const PVR_TIMER &timer)
{
  return g_E2STBData->AddTimer(timer);
}

PVR_ERROR DeleteTimer(const PVR_TIMER &timer, bool _UNUSED(bForceDelete))
{
  return g_E2STBData->DeleteTimer(timer);
}

PVR_ERROR UpdateTimer(const PVR_TIMER &timer)
{
  return g_E2STBData->UpdateTimer(timer);
}

/*!
 * @brief PVR client addon time shifting handling
 */
bool CanPauseStream(void)
{
  return g_bUseTimeshift;
}

bool CanSeekStream(void)
{
  return g_bUseTimeshift;
}

int ReadLiveStream(unsigned char *pBuffer, unsigned int iBufferSize)
{
  if (!g_E2STBData->GetTimeshiftBuffer())
    return 0;

  return g_E2STBData->GetTimeshiftBuffer()->ReadData(pBuffer, iBufferSize);
}

long long SeekLiveStream(long long iPosition, int iWhence /* = SEEK_SET */)
{
  if (!g_E2STBData->GetTimeshiftBuffer())
    return -1;

  return g_E2STBData->GetTimeshiftBuffer()->Seek(iPosition, iWhence);
}

long long PositionLiveStream(void)
{
  if (!g_E2STBData->GetTimeshiftBuffer())
    return -1;

  return g_E2STBData->GetTimeshiftBuffer()->Position();
}

long long LengthLiveStream(void)
{
  if (!g_E2STBData->GetTimeshiftBuffer())
    return 0;

  return g_E2STBData->GetTimeshiftBuffer()->Length();
}

time_t GetBufferTimeStart()
{
  if (!g_E2STBData->GetTimeshiftBuffer())
    return 0;

  return g_E2STBData->GetTimeshiftBuffer()->TimeStart();
}

time_t GetBufferTimeEnd()
{
  if (!g_E2STBData->GetTimeshiftBuffer())
    return 0;

  return g_E2STBData->GetTimeshiftBuffer()->TimeEnd();
}

time_t GetPlayingTime()
{
  /* FIXME: this should return the time of the *current* position */
  return GetBufferTimeEnd();
}

/*!
 * @brief Unused API functions
 */

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

/* EPG */
PVR_ERROR SetEPGTimeFrame(int) { return PVR_ERROR_NOT_IMPLEMENTED; }

/* Time shifting */
/* TODO: implement */
bool IsTimeshifting(void) { return false; }
bool IsRealTimeStream() { return true; }

/* Menu hook */
PVR_ERROR CallMenuHook(const PVR_MENUHOOK &_UNUSED(menuhook), const PVR_MENUHOOK_DATA &_UNUSED(item)) { return PVR_ERROR_NOT_IMPLEMENTED; }
}

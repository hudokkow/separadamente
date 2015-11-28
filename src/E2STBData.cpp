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

#include "E2STBData.h"

#include "client.h"

#include "tinyxml.h"
#include "E2STBXMLUtils.h"

#include <algorithm>  /* std::transform for GetDeviceInfo() */
#include <cctype>     /* std::toupper for GetDeviceInfo() */
#include <string>
#include <vector>

/**************************************************************************//**
 * General // General // General // General // General // General // General
 *****************************************************************************/
/********************************************//**
 * Constructor
 ***********************************************/
CE2STBData::CE2STBData()
: m_bIsConnected{false}
, m_strBackendBaseURLWeb{}
, m_strBackendBaseURLStream{}
, m_strEnigmaVersion{}
, m_strImageVersion{}
, m_strWebIfVersion{}
, m_strServerName{"Enigma2 STB"}
, m_iTimersIndexCounter{1}
, m_iUpdateIntervalTimer{0}
, m_iCurrentChannel{-1}
, m_iNumChannelGroups{0}
, m_iNumRecordings{0}
, m_tsBuffer{nullptr}
{
  std::string strURLAuthentication;

  if (g_bUseAuthentication && !g_strUsername.empty() && !g_strPassword.empty())
  {
    strURLAuthentication = g_strUsername + ":" + g_strPassword + "@";
  }

  if (!g_bUseSecureHTTP)
  {
    m_strBackendBaseURLWeb = "http://" + strURLAuthentication + g_strHostname + ":"
        + m_e2stbutils.IntToString(g_iPortWebHTTP) + "/";
    m_strBackendBaseURLStream = "http://" + strURLAuthentication + g_strHostname + ":"
        + m_e2stbutils.IntToString(g_iPortStream) + "/";
  }
  else
  {
    m_strBackendBaseURLWeb = "https://" + strURLAuthentication + g_strHostname + ":"
        + m_e2stbutils.IntToString(g_iPortWebHTTPS) + "/";
    m_strBackendBaseURLStream = "https://" + strURLAuthentication + g_strHostname + ":"
        + m_e2stbutils.IntToString(g_iPortStream) + "/";
  }
}

/********************************************//**
 * Destructor
 ***********************************************/
CE2STBData::~CE2STBData()
{
  PLATFORM::CLockObject lock(m_mutex);
  XBMC->Log(ADDON::LOG_DEBUG, "[%s] Stopping update thread", __FUNCTION__);
  StopThread();

  XBMC->Log(ADDON::LOG_DEBUG, "[%s] Removing internal channels list", __FUNCTION__);
  m_channels.clear();

  XBMC->Log(ADDON::LOG_DEBUG, "[%s] Removing internal timers list", __FUNCTION__);
  m_timers.clear();

  XBMC->Log(ADDON::LOG_DEBUG, "[%s] Removing internal recordings list", __FUNCTION__);
  m_recordings.clear();

  XBMC->Log(ADDON::LOG_DEBUG, "[%s] Removing internal channels groups list", __FUNCTION__);
  m_channelsGroups.clear();

  if (m_tsBuffer)
  {
    XBMC->Log(ADDON::LOG_DEBUG, "[%s] Removing internal time shifting buffer", __FUNCTION__);
    delete m_tsBuffer;
  }

  m_bIsConnected = false;
}

/********************************************//**
 * Open client
 ***********************************************/
bool CE2STBData::Open()
{
  PLATFORM::CLockObject lock(m_mutex);
  m_bIsConnected = GetDeviceInfo();

  if (!m_bIsConnected)
  {
    XBMC->Log(ADDON::LOG_ERROR, "[%s] Web interface can't be reached. Make sure connection options are correct",
        __FUNCTION__);
    return false;
  }
  LoadRecordingLocations();

  if (m_channels.size() == 0)
  {
    if (!LoadChannelGroups())
    {
      return false;
    }

    if (!LoadChannels())
    {
      return false;
    }
  }
  TimerUpdates();
  XBMC->Log(ADDON::LOG_NOTICE, "[%s] Starting Enigma2 STB Client update thread", __FUNCTION__);
  CreateThread();
  return IsRunning();
}

/********************************************//**
 * Check client is connected
 ***********************************************/
bool CE2STBData::IsConnected()
{
  return m_bIsConnected;
}

/********************************************//**
 * Send deep standby command to STB if Enigma2
 * STB client is deactivated or Kodi is closed
 ***********************************************/
void CE2STBData::SendPowerstate()
{
  if (!g_bSendDeepStanbyToSTB)
  {
    return;
  }
  PLATFORM::CLockObject lock(m_mutex);

  /* TODO: Review power states functionality
  http://wiki.dbox2-tuning.net/wiki/Enigma2:WebInterface
   */
  std::string strTemp = "web/powerstate?newstate=1";

  std::string strResult;
  SendCommandToSTB(strTemp, strResult, true);
}

/********************************************//**
 * Get STB backend information. Image version,
 * Enigma version, WebIf version, Device name
 ***********************************************/
bool CE2STBData::GetDeviceInfo()
{
  std::string strURL = m_strBackendBaseURLWeb + "web/deviceinfo";
  std::string strXML = m_e2stbutils.ConnectToBackend(strURL);

  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(strXML.c_str()))
  {
    XBMC->Log(ADDON::LOG_DEBUG, "[%s] Unable to parse XML %s at line %d", __FUNCTION__, xmlDoc.ErrorDesc(),
        xmlDoc.ErrorRow());
    return false;
  }

  TiXmlHandle hDoc(&xmlDoc);
  TiXmlElement* pElement;
  TiXmlHandle hRoot(0);

  pElement = hDoc.FirstChildElement("e2deviceinfo").Element();

  if (!pElement)
  {
    XBMC->Log(ADDON::LOG_ERROR, "[%s] Couldn't find <e2deviceinfo> element", __FUNCTION__);
    return false;
  }

  std::string strTemp;

  if (!XMLUtils::GetString(pElement, "e2enigmaversion", strTemp))
  {
    XBMC->Log(ADDON::LOG_ERROR, "[%s] Couldn't parse <e2enigmaversion> from result", __FUNCTION__);
    return false;
  }
  m_strEnigmaVersion = strTemp;
  XBMC->Log(ADDON::LOG_NOTICE, "[%s] Enigma2 version is %s", __FUNCTION__, m_strEnigmaVersion.c_str());

  if (!XMLUtils::GetString(pElement, "e2imageversion", strTemp))
  {
    XBMC->Log(ADDON::LOG_ERROR, "[%s] Couldn't parse <e2imageversion> from result", __FUNCTION__);
    return false;
  }
  m_strImageVersion = strTemp;
  XBMC->Log(ADDON::LOG_NOTICE, "[%s] Enigma2 image version is %s", __FUNCTION__, m_strImageVersion.c_str());

  if (!XMLUtils::GetString(pElement, "e2webifversion", strTemp))
  {
    XBMC->Log(ADDON::LOG_ERROR, "[%s] Couldn't parse <e2webifversion> from result", __FUNCTION__);
    return false;
  }
  m_strWebIfVersion = strTemp;
  XBMC->Log(ADDON::LOG_NOTICE, "[%s] Enigma2 web interface version is %s", __FUNCTION__, m_strWebIfVersion.c_str());

  if (!XMLUtils::GetString(pElement, "e2devicename", strTemp))
  {
    XBMC->Log(ADDON::LOG_ERROR, "[%s] Couldn't parse <e2devicename> from result", __FUNCTION__);
    return false;
  }
  /* http://stackoverflow.com/questions/7131858/stdtransform-and-toupper-no-matching-function */
  std::transform(strTemp.begin(), strTemp.end(), strTemp.begin(), (int (*)(int))std::toupper);
  m_strServerName += " " + strTemp;
  XBMC->Log(ADDON::LOG_NOTICE, "[%s] Enigma2 device name is %s", __FUNCTION__, m_strServerName.c_str());
  return true;
}

/********************************************//**
 * Send command to backend STB
 ***********************************************/
bool CE2STBData::SendCommandToSTB(const std::string& strCommandURL, std::string& strResultText, bool bIgnoreResult)
{
  /* std::string is needed to quell warning */
  /* ISO C++ says that these are ambiguous, even though the worst conversion */
  /* for the first is better than the worst conversion for the second */
  std::string strURL = m_strBackendBaseURLWeb + std::string(strCommandURL);
  std::string strXML = m_e2stbutils.ConnectToBackend(strURL);

  if (!bIgnoreResult)
  {
    TiXmlDocument xmlDoc;
    if (!xmlDoc.Parse(strXML.c_str()))
    {
      XBMC->Log(ADDON::LOG_DEBUG, "[%s] Unable to parse XML %s at line %d", __FUNCTION__, xmlDoc.ErrorDesc(),
          xmlDoc.ErrorRow());
      return false;
    }

    TiXmlHandle hDoc(&xmlDoc);
    TiXmlElement* pElement;
    TiXmlHandle hRoot(0);

    pElement = hDoc.FirstChildElement("e2simplexmlresult").Element();
    if (!pElement)
    {
      XBMC->Log(ADDON::LOG_DEBUG, "[%s] Couldn't find <e2simplexmlresult> element", __FUNCTION__);
      return false;
    }

    bool bTmp;
    if (!XMLUtils::GetBoolean(pElement, "e2state", bTmp))
    {
      XBMC->Log(ADDON::LOG_ERROR, "[%s] Couldn't parse <e2state> from result", __FUNCTION__);
      strResultText = "Could not parse e2state!";
      return false;
    }

    if (!XMLUtils::GetString(pElement, "e2statetext", strResultText))
    {
      XBMC->Log(ADDON::LOG_ERROR, "[%s] Could not parse <e2state> from result!", __FUNCTION__);
      return false;
    }

    if (!bTmp)
    {
      XBMC->Log(ADDON::LOG_ERROR, "[%s] Backend sent error message %s", __FUNCTION__, strResultText.c_str());
    }
    return bTmp;
  }
  return true;
}

/********************************************//**
 * Process
 ***********************************************/
void *CE2STBData::Process()
{
  XBMC->Log(ADDON::LOG_DEBUG, "[%s] Starting", __FUNCTION__);

  for (unsigned int iChannelPtr = 0; iChannelPtr < m_channels.size();
      iChannelPtr++)
  {
    XBMC->Log(ADDON::LOG_DEBUG, "[%s] Triggering EPG update for channel %d", __FUNCTION__, iChannelPtr);
    PVR->TriggerEpgUpdate(m_channels.at(iChannelPtr).iUniqueId);
  }

  while (!IsStopped())
  {
    Sleep(5 * 1000);
    m_iUpdateIntervalTimer += 5;

    if (static_cast<int>(m_iUpdateIntervalTimer) > (g_iClientUpdateInterval * 60))
    {
      m_iUpdateIntervalTimer = 0;

      /* Trigger Timer and Recording updates according to client settings */
      PLATFORM::CLockObject lock(m_mutex);
      XBMC->Log(ADDON::LOG_NOTICE, "[%s] Updating timers and recordings", __FUNCTION__);

      if (g_bAutomaticTimerlistCleanup)
      {
        std::string strTemp = "web/timercleanup?cleanup=true";

        std::string strResult;
        if (!SendCommandToSTB(strTemp, strResult))
        {
          XBMC->Log(ADDON::LOG_ERROR, "[%s] Automatic timer list cleanup failed", __FUNCTION__);
        }
      }
      TimerUpdates();
      PVR->TriggerRecordingUpdate();
    }
  }
  m_started.Broadcast();
  return NULL;
}

/**************************************************************************//**
 * Channels // Channels // Channels // Channels // Channels // Channels
 *****************************************************************************/
/********************************************//**
 * Load channels
 ***********************************************/
PVR_ERROR CE2STBData::GetChannels(ADDON_HANDLE handle, bool bRadio)
{
  for (unsigned int iChannelPtr = 0; iChannelPtr < m_channels.size(); iChannelPtr++)
  {
    SE2STBChannel &channel = m_channels.at(iChannelPtr);
    if (channel.bRadio == bRadio)
    {
      PVR_CHANNEL xbmcChannel;
      memset(&xbmcChannel, 0, sizeof(PVR_CHANNEL));

      xbmcChannel.iUniqueId = channel.iUniqueId;
      xbmcChannel.bIsRadio = channel.bRadio;
      xbmcChannel.iChannelNumber = channel.iChannelNumber;
      PVR_STRCPY(xbmcChannel.strChannelName, channel.strChannelName.c_str());
      PVR_STRCPY(xbmcChannel.strInputFormat, ""); /* Unused */

      xbmcChannel.iEncryptionSystem = 0;
      xbmcChannel.bIsHidden = false;

      PVR_STRCPY(xbmcChannel.strIconPath, channel.strIconPath.c_str());

      if (!g_bUseTimeshift)
      {
        std::string strStream = "pvr://stream/tv/" + m_e2stbutils.IntToString(channel.iUniqueId) + ".ts";
        PVR_STRCPY(xbmcChannel.strStreamURL, strStream.c_str());
      }
      PVR->TransferChannelEntry(handle, &xbmcChannel);
    }
  }
  return PVR_ERROR_NO_ERROR;
}

/********************************************//**
 * Load channel groups
 ***********************************************/
PVR_ERROR CE2STBData::GetChannelGroups(ADDON_HANDLE handle)
{
  for (unsigned int iTagPtr = 0; iTagPtr < m_channelsGroups.size(); iTagPtr++)
  {
    PVR_CHANNEL_GROUP tag;
    memset(&tag, 0, sizeof(PVR_CHANNEL_GROUP));

    tag.bIsRadio  = false;
    tag.iPosition = 0; /* groups default order, unused */
    PVR_STRCPY(tag.strGroupName, m_channelsGroups[iTagPtr].strGroupName.c_str());
    PVR->TransferChannelGroup(handle, &tag);
  }
  return PVR_ERROR_NO_ERROR;
}

/********************************************//**
 * Get channel groups members
 ***********************************************/
PVR_ERROR CE2STBData::GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group)
{
  XBMC->Log(ADDON::LOG_DEBUG, "[%s] Adding channels from group %s", __FUNCTION__, group.strGroupName);
  std::string strTemp = group.strGroupName;
  for (unsigned int i = 0; i < m_channels.size(); i++)
  {
    SE2STBChannel &myChannel = m_channels.at(i);
    if (!strTemp.compare(myChannel.strGroupName))
    {
      PVR_CHANNEL_GROUP_MEMBER tag;
      memset(&tag, 0, sizeof(PVR_CHANNEL_GROUP_MEMBER));

      PVR_STRCPY(tag.strGroupName, group.strGroupName);
      tag.iChannelUniqueId = myChannel.iUniqueId;
      tag.iChannelNumber = myChannel.iChannelNumber;

      XBMC->Log(ADDON::LOG_DEBUG, "[%s] Added channel %s with unique ID %d to group %s and channel number %d",
          __FUNCTION__, myChannel.strChannelName.c_str(), tag.iChannelUniqueId, group.strGroupName,
          myChannel.iChannelNumber);
      PVR->TransferChannelGroupMember(handle, &tag);
    }
  }
  return PVR_ERROR_NO_ERROR;
}

/********************************************//**
 * Get number of channels
 ***********************************************/
int CE2STBData::GetTotalChannelNumber(std::string strServiceReference)
{
  for (unsigned int i = 0; i < m_channels.size(); i++)
  {
    if (!strServiceReference.compare(m_channels[i].strServiceReference))
    {
      return i + 1;
    }
  }
  return -1;
}

/********************************************//**
 * Load channels from backend
 ***********************************************/
bool CE2STBData::LoadChannels(std::string strServiceReference, std::string strGroupName)
{
  XBMC->Log(ADDON::LOG_DEBUG, "[%s] Loading channel group %s", __FUNCTION__, strGroupName.c_str());

  std::string strURL = m_strBackendBaseURLWeb + "web/getservices?sRef="
      + m_e2stbutils.URLEncode(strServiceReference);
  std::string strXML = m_e2stbutils.ConnectToBackend(strURL);

  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(strXML.c_str()))
  {
    XBMC->Log(ADDON::LOG_DEBUG, "[%s] Unable to parse XML %s at line %d", __FUNCTION__, xmlDoc.ErrorDesc(),
        xmlDoc.ErrorRow());
    return false;
  }

  TiXmlHandle hDoc(&xmlDoc);
  TiXmlElement* pElement;
  TiXmlHandle hRoot(0);

  pElement = hDoc.FirstChildElement("e2servicelist").Element();

  if (!pElement)
  {
    XBMC->Log(ADDON::LOG_DEBUG, "[%s] Couldn't find <e2servicelist> element", __FUNCTION__);
    return false;
  }

  hRoot = TiXmlHandle(pElement);

  TiXmlElement* pNode = hRoot.FirstChildElement("e2service").Element();

  if (!pNode)
  {
    XBMC->Log(ADDON::LOG_DEBUG, "[%s] Couldn't find <e2service> element", __FUNCTION__);
    return false;
  }

  bool bRadio;

  bRadio = !strGroupName.compare("radio");

  for (; pNode != NULL; pNode = pNode->NextSiblingElement("e2service"))
  {
    std::string strTemp;

    if (!XMLUtils::GetString(pNode, "e2servicereference", strTemp))
    {
      continue;
    }

    /* Discard label elements */
    if (strTemp.compare(0, 5, "1:64:") == 0)
    {
      continue;
    }

    SE2STBChannel newChannel;
    newChannel.bRadio = bRadio;
    /* TODO: Delete */
    // newChannel.bInitialEPG = true;
    newChannel.strGroupName = strGroupName;
    newChannel.iUniqueId = m_channels.size() + 1;
    newChannel.iChannelNumber = m_channels.size() + 1;
    newChannel.strServiceReference = strTemp;

    if (!XMLUtils::GetString(pNode, "e2servicename", strTemp))
    {
      continue;
    }

    newChannel.strChannelName = strTemp;

    std::string strPicon;
    strPicon = newChannel.strServiceReference;

    int j = 0;
    std::string::iterator it = strPicon.begin();

    while (j < 10 && it != strPicon.end())
    {
      if (*it == ':')
      {
        j++;
      }
      it++;
    }
    std::string::size_type index = it - strPicon.begin();

    strPicon = strPicon.substr(0, index);

    it = strPicon.end() - 1;
    if (*it == ':')
    {
      strPicon.erase(it);
    }
    std::string strTemp2 = strPicon;

    std::replace(strPicon.begin(), strPicon.end(), ':', '_');
    strPicon = g_strPiconsLocationPath + strPicon + ".png";

    newChannel.strIconPath = strPicon;
    /* Stop changing this! It is the STREAM port, dumbass */
    std::string strURL = m_strBackendBaseURLStream + strTemp2;

    newChannel.strStreamURL = strURL;

    if (g_bLoadWebInterfacePicons)
    {
      std::replace(strTemp2.begin(), strTemp2.end(), ':', '_');
      strURL = m_strBackendBaseURLWeb + "picon/" + strTemp2 + ".png";
      newChannel.strIconPath = strURL;
    }
    m_channels.push_back(newChannel);
    XBMC->Log(ADDON::LOG_DEBUG, "[%s] Loaded channel %s with picon %s", __FUNCTION__,
        newChannel.strChannelName.c_str(), newChannel.strIconPath.c_str());
  }
  XBMC->Log(ADDON::LOG_NOTICE, "[%s] Loaded %d channels", __FUNCTION__, m_channels.size());
  return true;
}

/********************************************//**
 * Load channels
 ***********************************************/
bool CE2STBData::LoadChannels()
{
  bool bOk = false;
  m_channels.clear();
  for (int i = 0; i < m_iNumChannelGroups; i++)
  {
    SE2STBChannelGroup &myGroup = m_channelsGroups.at(i);
    if (LoadChannels(myGroup.strServiceReference, myGroup.strGroupName))
    {
      bOk = true;
    }
  }
  /* TODO: Check another way to load Radio channels in API. Currently there's
  no way one can request a Radio bouquets list, like we do for TV bouquets.
   */
  if (g_bLoadRadioChannelsGroup)
  {
    std::string strTemp = "1:7:1:0:0:0:0:0:0:0:FROM BOUQUET \"userbouquet.favourites.radio\" ORDER BY bouquet";
    LoadChannels(strTemp, "radio");
  }
  return bOk;
}

/********************************************//**
 * Load channel groups from backend
 ***********************************************/
bool CE2STBData::LoadChannelGroups()
{
  std::string strURL = m_strBackendBaseURLWeb + "web/getservices";
  std::string strXML = m_e2stbutils.ConnectToBackend(strURL);

  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(strXML.c_str()))
  {
    XBMC->Log(ADDON::LOG_DEBUG, "[%s] Unable to parse XML %s at line %d", __FUNCTION__, xmlDoc.ErrorDesc(),
        xmlDoc.ErrorRow());
    return false;
  }

  TiXmlHandle hDoc(&xmlDoc);
  TiXmlElement* pElement;
  TiXmlHandle hRoot(0);

  pElement = hDoc.FirstChildElement("e2servicelist").Element();

  if (!pElement)
  {
    XBMC->Log(ADDON::LOG_DEBUG, "[%s] Couldn't find <e2servicelist> element", __FUNCTION__);
    return false;
  }

  hRoot = TiXmlHandle(pElement);

  TiXmlElement* pNode = hRoot.FirstChildElement("e2service").Element();

  if (!pNode)
  {
    XBMC->Log(ADDON::LOG_DEBUG, "[%s] Couldn't find <e2service> element", __FUNCTION__);
    return false;
  }

  m_channelsGroups.clear();
  m_iNumChannelGroups = 0;

  for (; pNode != NULL; pNode = pNode->NextSiblingElement("e2service"))
  {
    std::string strTemp;

    if (!XMLUtils::GetString(pNode, "e2servicereference", strTemp))
    {
      continue;
    }

    SE2STBChannelGroup newGroup;
    newGroup.strServiceReference = strTemp;

    if (!XMLUtils::GetString(pNode, "e2servicename", strTemp))
    {
      continue;
    }

    if (strTemp.compare(0, 3, "---") == 0)
    {
      continue;
    }

    newGroup.strGroupName = strTemp;

    if (g_bSelectTVChannelGroups)
    {
      if (!g_strTVChannelGroupNameOne.empty() && g_strTVChannelGroupNameOne.compare(strTemp) == 0
          && g_iNumTVChannelGroupsToLoad >= 1)
      {
        XBMC->Log(ADDON::LOG_DEBUG, "[%s] %s matches requested TV channel group #1 %s", __FUNCTION__, strTemp.c_str(),
            g_strTVChannelGroupNameOne.c_str());
      }
      else if (!g_strTVChannelGroupNameTwo.empty() && g_strTVChannelGroupNameTwo.compare(strTemp.c_str()) == 0
          && g_iNumTVChannelGroupsToLoad >= 2)
      {
        XBMC->Log(ADDON::LOG_DEBUG, "[%s] %s matches requested TV channel group #2 %s", __FUNCTION__, strTemp.c_str(),
            g_strTVChannelGroupNameTwo.c_str());
      }
      else if (!g_strTVChannelGroupNameThree.empty() && g_strTVChannelGroupNameThree.compare(strTemp.c_str()) == 0
          && g_iNumTVChannelGroupsToLoad >= 3)
      {
        XBMC->Log(ADDON::LOG_DEBUG, "[%s] %s matches requested TV channel group #3 %s", __FUNCTION__, strTemp.c_str(),
            g_strTVChannelGroupNameThree.c_str());
      }
      else if (!g_strTVChannelGroupNameFour.empty() && g_strTVChannelGroupNameFour.compare(strTemp.c_str()) == 0
          && g_iNumTVChannelGroupsToLoad >= 4)
      {
        XBMC->Log(ADDON::LOG_DEBUG, "[%s] %s matches requested TV channel group #4 %s", __FUNCTION__, strTemp.c_str(),
            g_strTVChannelGroupNameFour.c_str());
      }
      else if (!g_strTVChannelGroupNameFive.empty() && g_strTVChannelGroupNameFive.compare(strTemp.c_str()) == 0
          && g_iNumTVChannelGroupsToLoad == 5)
      {
        XBMC->Log(ADDON::LOG_DEBUG, "[%s] %s matches requested TV channel group #5 %s", __FUNCTION__, strTemp.c_str(),
            g_strTVChannelGroupNameFive.c_str());
      }
      else
      {
        XBMC->Log(ADDON::LOG_DEBUG, "[%s] TV channel group %s doesn't match any requested group", __FUNCTION__,
            strTemp.c_str());
        continue;
      }
    }
    m_channelsGroups.push_back(newGroup);
    XBMC->Log(ADDON::LOG_NOTICE, "[%s] Loaded TV channel group %s", __FUNCTION__, newGroup.strGroupName.c_str());
    m_iNumChannelGroups++;
  }
  XBMC->Log(ADDON::LOG_NOTICE, "[%s] Loaded %d TV channel groups", __FUNCTION__, m_iNumChannelGroups);
  return true;
}

/**************************************************************************//**
 * EPG // EPG // EPG // EPG // EPG // EPG // EPG // EPG // EPG // EPG // EPG
 *****************************************************************************/
/********************************************//**
 * Get EPG for channel
 ***********************************************/
PVR_ERROR CE2STBData::GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
{
  if (channel.iUniqueId - 1 > m_channels.size())
  {
    XBMC->Log(ADDON::LOG_ERROR, "[%s] Couldn't fetch EPG for channel with unique ID %d", __FUNCTION__,
        channel.iUniqueId);
    return PVR_ERROR_NO_ERROR;
  }

  SE2STBChannel myChannel = m_channels.at(channel.iUniqueId - 1);

  std::string strURL = m_strBackendBaseURLWeb + "web/epgservice?sRef="
      + m_e2stbutils.URLEncode(myChannel.strServiceReference);
  std::string strXML = m_e2stbutils.ConnectToBackend(strURL);

  int iNumEPG = 0;

  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(strXML.c_str()))
  {
    XBMC->Log(ADDON::LOG_DEBUG, "[%s] Unable to parse XML %s at line %d", __FUNCTION__, xmlDoc.ErrorDesc(),
        xmlDoc.ErrorRow());
    return PVR_ERROR_SERVER_ERROR;
  }

  TiXmlHandle hDoc(&xmlDoc);
  TiXmlElement* pElement;
  TiXmlHandle hRoot(0);

  pElement = hDoc.FirstChildElement("e2eventlist").Element();

  if (!pElement)
  {
    XBMC->Log(ADDON::LOG_DEBUG, "[%s] Couldn't find <e2eventlist> element", __FUNCTION__);
    /* EPG could be empty for this channel. Return "NO_ERROR" */
    return PVR_ERROR_NO_ERROR;
  }

  hRoot = TiXmlHandle(pElement);

  TiXmlElement* pNode = hRoot.FirstChildElement("e2event").Element();

  if (!pNode)
  {
    XBMC->Log(ADDON::LOG_DEBUG, "[%s] Couldn't find <e2event> element", __FUNCTION__);
    return PVR_ERROR_SERVER_ERROR;
  }

  for (; pNode != NULL; pNode = pNode->NextSiblingElement("e2event"))
  {
    std::string strTemp;

    int iTmpStart;
    int iTmp;

    if (!XMLUtils::GetInt(pNode, "e2eventstart", iTmpStart))
    {
      continue;
    }

    /*  Skip unnecessary events */
    if (iStart > iTmpStart)
    {
      continue;
    }

    if (!XMLUtils::GetInt(pNode, "e2eventduration", iTmp))
    {
      continue;
    }

    if ((iEnd > 1) && (iEnd < (iTmpStart + iTmp)))
    {
      continue;
    }

    SE2STBEPG entry;
    entry.startTime = iTmpStart;
    entry.endTime = iTmpStart + iTmp;

    if (!XMLUtils::GetInt(pNode, "e2eventid", entry.iEventId))
    {
      continue;
    }

    entry.iChannelId = channel.iUniqueId;

    if (!XMLUtils::GetString(pNode, "e2eventtitle", strTemp))
    {
      continue;
    }

    entry.strTitle = strTemp;

    entry.strServiceReference = myChannel.strServiceReference;

    if (XMLUtils::GetString(pNode, "e2eventdescriptionextended", strTemp))
    {
      entry.strPlot = strTemp;
    }

    if (XMLUtils::GetString(pNode, "e2eventdescription", strTemp))
    {
      entry.strPlotOutline = strTemp;
    }

    EPG_TAG tag;
    memset(&tag, 0, sizeof(EPG_TAG));

    tag.iUniqueBroadcastId  = entry.iEventId;
    tag.strTitle            = entry.strTitle.c_str();
    tag.iChannelNumber      = channel.iChannelNumber;
    tag.startTime           = entry.startTime;
    tag.endTime             = entry.endTime;
    tag.strPlotOutline      = entry.strPlotOutline.c_str();
    tag.strPlot             = entry.strPlot.c_str();
    tag.strOriginalTitle    = "";    /* Unused */
    tag.strCast             = "";    /* Unused */
    tag.strDirector         = "";    /* Unused */
    tag.strWriter           = "";    /* Unused */
    tag.iYear               = 0;     /* Unused */
    tag.strIMDBNumber       = "";    /* Unused */
    tag.strIconPath         = "";    /* Unused */
    tag.iGenreType          = 0;     /* Unused */
    tag.iGenreSubType       = 0;     /* Unused */
    tag.strGenreDescription = "";    /* Unused */
    tag.firstAired          = 0;     /* Unused */
    tag.iParentalRating     = 0;     /* Unused */
    tag.iStarRating         = 0;     /* Unused */
    tag.bNotify             = false; /* Unused */
    tag.iSeriesNumber       = 0;     /* Unused */
    tag.iEpisodeNumber      = 0;     /* Unused */
    tag.iEpisodePartNumber  = 0;     /* Unused */
    tag.strEpisodeName      = "";    /* Unused */
    tag.iFlags              = EPG_TAG_FLAG_UNDEFINED;

    PVR->TransferEpgEntry(handle, &tag);

    iNumEPG++;

    XBMC->Log(ADDON::LOG_DEBUG, "[%s] Loaded EPG entry %d - %s for channel %d starting at %d and ending at %d",
        __FUNCTION__, tag.iUniqueBroadcastId, tag.strTitle, entry.iChannelId, entry.startTime,
        entry.endTime);
  }
  XBMC->Log(ADDON::LOG_DEBUG, "[%s] Loaded %u EPG entries for channel %s", __FUNCTION__, iNumEPG,
      channel.strChannelName);
  return PVR_ERROR_NO_ERROR;
}

/**************************************************************************//**
 * Information // Information // Information // Information // Information
 *****************************************************************************/
/********************************************//**
 * Get backend system HDD total and free space
 ***********************************************/
PVR_ERROR CE2STBData::GetDriveSpace(long long *iTotal, long long *iUsed)
{
  std::string strURL = m_strBackendBaseURLWeb + "web/deviceinfo";
  std::string strXML = m_e2stbutils.ConnectToBackend(strURL);

  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(strXML.c_str()))
  {
    XBMC->Log(ADDON::LOG_DEBUG, "[%s] Unable to parse XML %s at line %d", __FUNCTION__, xmlDoc.ErrorDesc(),
        xmlDoc.ErrorRow());
  }

  TiXmlHandle hDoc(&xmlDoc);
  TiXmlElement* pElement;
  TiXmlHandle hRoot(0);

  pElement = hDoc.FirstChildElement("e2deviceinfo").FirstChildElement("e2hdds").FirstChildElement("e2hdd").Element();

  if (!pElement)
  {
    XBMC->Log(ADDON::LOG_ERROR, "[%s] Couldn't find <e2hdd> element", __FUNCTION__);
    return PVR_ERROR_SERVER_ERROR;
  }

  std::string strTemp;
  *iTotal = 0;
  *iUsed = 0;

  if (!XMLUtils::GetString(pElement, "e2capacity", strTemp))
  {
    XBMC->Log(ADDON::LOG_ERROR, "[%s] Couldn't parse <e2capacity> from result", __FUNCTION__);
  }

  double totalHDDSpace = 0;
  totalHDDSpace = std::atof(strTemp.c_str()) * 1024 * 1024;

  if (!XMLUtils::GetString(pElement, "e2free", strTemp))
  {
    XBMC->Log(ADDON::LOG_ERROR, "[%s] Couldn't parse <e2free> from result", __FUNCTION__);
  }
  double freeHDDSpace = 0;
  freeHDDSpace = std::atof(strTemp.c_str());

  std::string strSizeModifier;
  strSizeModifier = strTemp.substr(strTemp.length() - 2);
  XBMC->Log(ADDON::LOG_DEBUG, "[%s] Size modifier is %s", __FUNCTION__, strSizeModifier.c_str());

  if (strSizeModifier == "MB")
  {/* Result might be in MB */
    freeHDDSpace *= 1024;
  }
  else if (strSizeModifier == "GB")
  {/* Result might be in GB */
    freeHDDSpace *= (1024 * 1024);
  }
  else
  {/* Result might be in TB */
    freeHDDSpace *= (1024 * 1024 * 1024);
  }
  *iTotal = (long long) totalHDDSpace;
  *iUsed = (long long) (totalHDDSpace - freeHDDSpace);
  XBMC->Log(ADDON::LOG_NOTICE, "[%s] Backend HDD has %lldKB used of %lldKB total space", __FUNCTION__, *iUsed, *iTotal);
  return PVR_ERROR_NO_ERROR;
}

/********************************************//**
 * Get backend tuner information (SNR, BER, AGC)
 ***********************************************/
PVR_ERROR CE2STBData::SignalStatus(PVR_SIGNAL_STATUS &signalStatus)
{
  static PVR_SIGNAL_STATUS tag;
  memset(&tag, 0, sizeof(tag));

  std::string strURL = m_strBackendBaseURLWeb + "web/signal";
  std::string strXML = m_e2stbutils.ConnectToBackend(strURL);

  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(strXML.c_str()))
  {
    XBMC->Log(ADDON::LOG_DEBUG, "[%s] Unable to parse XML %s at line %d", __FUNCTION__, xmlDoc.ErrorDesc(),
        xmlDoc.ErrorRow());
  }

  TiXmlHandle hDoc(&xmlDoc);
  TiXmlElement* pElement;
  TiXmlHandle hRoot(0);

  pElement = hDoc.FirstChildElement("e2frontendstatus").Element();
  if (!pElement)
  {
    XBMC->Log(ADDON::LOG_ERROR, "[%s] Couldn't find <e2frontendstatus> element", __FUNCTION__);
    return PVR_ERROR_SERVER_ERROR;
  }

  std::string strTemp;
  if (!XMLUtils::GetString(pElement, "e2snrdb", strTemp))
  {
    XBMC->Log(ADDON::LOG_ERROR, "[%s] Couldn't parse <e2snrdb> from result", __FUNCTION__);
  }
  std::string strSNRDB = strTemp;
  XBMC->Log(ADDON::LOG_DEBUG, "[%s] SNRDB is %s", __FUNCTION__, strSNRDB.c_str());
  /* STB's API 100% = 17.00dB, hence SNRDB * 5.88235 */
  tag.iSNR = (std::stof(strTemp.c_str()) * 5.88235 * 655.35 + 0.5);

  if (!XMLUtils::GetString(pElement, "e2snr", strTemp))
  {
    XBMC->Log(ADDON::LOG_ERROR, "[%s] Couldn't parse <e2snr> from result", __FUNCTION__);
  }
  std::string strSNR = strTemp;
  XBMC->Log(ADDON::LOG_DEBUG, "[%s] SNR is %s", __FUNCTION__, strSNR.c_str());
  tag.iSignal = (std::stoi(strTemp.c_str()) * 655.35);

  if (!XMLUtils::GetString(pElement, "e2ber", strTemp))
  {
    XBMC->Log(ADDON::LOG_ERROR, "[%s] Couldn't parse <e2ber> from result", __FUNCTION__);
  }
  std::string strBER = strTemp;
  XBMC->Log(ADDON::LOG_DEBUG, "[%s] BER is %s", __FUNCTION__, strBER.c_str());
  tag.iBER = std::stol(strTemp.c_str());

  signalStatus = tag;
  return PVR_ERROR_NO_ERROR;
}

/**************************************************************************//**
 * Recordings // Recordings // Recordings // Recordings // Recordings
 *****************************************************************************/
/********************************************//**
 * Delete recording
 ***********************************************/
PVR_ERROR CE2STBData::DeleteRecording(const PVR_RECORDING &recinfo)
{
  std::string strTemp = "web/moviedelete?sRef=" + m_e2stbutils.URLEncode(recinfo.strRecordingId);

  std::string strResult;
  if (!SendCommandToSTB(strTemp, strResult))
  {
    return PVR_ERROR_FAILED;
  }
  PVR->TriggerRecordingUpdate();
  return PVR_ERROR_NO_ERROR;
}

/********************************************//**
 * Get recordings
 ***********************************************/
PVR_ERROR CE2STBData::GetRecordings(ADDON_HANDLE handle)
{
  m_iNumRecordings = 0;
  m_recordings.clear();

  for (unsigned int i = 0; i < m_recordingsLocations.size(); i++)
  {
    if (!GetRecordingFromLocation(m_recordingsLocations[i]))
    {
      XBMC->Log(ADDON::LOG_ERROR, "[%s] Error fetching recordings list from folder %s", __FUNCTION__,
          m_recordingsLocations[i].c_str());
    }
  }
  TransferRecordings(handle);
  return PVR_ERROR_NO_ERROR;
}

/********************************************//**
 * Load recording locations from backend
 ***********************************************/
bool CE2STBData::LoadRecordingLocations()
{
  std::string strURL;
  if (g_bUseOnlyCurrentRecordingPath)
  {
    strURL = m_strBackendBaseURLWeb + "web/getcurrlocation";
  }
  else
  {
    strURL = m_strBackendBaseURLWeb + "web/getlocations";
  }

  std::string strXML = m_e2stbutils.ConnectToBackend(strURL);

  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(strXML.c_str()))
  {
    XBMC->Log(ADDON::LOG_DEBUG, "[%s] Unable to parse XML %s at line %d", __FUNCTION__, xmlDoc.ErrorDesc(),
        xmlDoc.ErrorRow());
    return false;
  }

  TiXmlHandle hDoc(&xmlDoc);
  TiXmlElement* pElement;
  TiXmlHandle hRoot(0);

  pElement = hDoc.FirstChildElement("e2locations").Element();

  if (!pElement)
  {
    XBMC->Log(ADDON::LOG_DEBUG, "[%s] Couldn't find <e2locations> element", __FUNCTION__);
    return false;
  }

  hRoot = TiXmlHandle(pElement);

  TiXmlElement* pNode = hRoot.FirstChildElement("e2location").Element();

  if (!pNode)
  {
    XBMC->Log(ADDON::LOG_DEBUG, "[%s] Couldn't find <e2location> element", __FUNCTION__);
    return false;
  }

  int iNumLocations = 0;

  for (; pNode != NULL; pNode = pNode->NextSiblingElement("e2location"))
  {
    std::string strTemp = pNode->GetText();
    m_recordingsLocations.push_back(strTemp);
    iNumLocations++;
    XBMC->Log(ADDON::LOG_NOTICE, "[%s] Added %s as a recording location", __FUNCTION__, strTemp.c_str());
  }
  XBMC->Log(ADDON::LOG_NOTICE, "[%s] Loaded %d recording locations", __FUNCTION__, iNumLocations);
  return true;
}

/********************************************//**
 * Check if recording exists
 ***********************************************/
bool CE2STBData::IsInRecordingFolder(std::string strRecordingFolder)
{
  int iMatches = 0;
  for (unsigned int i = 0; i < m_recordings.size(); i++)
  {
    if (strRecordingFolder.compare(m_recordings.at(i).strTitle) == 0)
    {
      iMatches++;
      XBMC->Log(ADDON::LOG_DEBUG, "[%s] Found recording title %s in recordings vector", __FUNCTION__,
          strRecordingFolder.c_str());
      if (iMatches > 1)
      {
        XBMC->Log(ADDON::LOG_DEBUG, "[%s] Found recording title %s more than once in recordings vector", __FUNCTION__,
            strRecordingFolder.c_str());
        return true;
      }
    }
  }
  return false;
}

/********************************************//**
 * Get recordings from loaded location(s)
 ***********************************************/
bool CE2STBData::GetRecordingFromLocation(std::string strRecordingFolder)
{
  std::string strURL;
  if (!strRecordingFolder.compare("default"))
  {
    strURL = m_strBackendBaseURLWeb + "web/movielist";
  }
  else
  {
    strURL = m_strBackendBaseURLWeb + "web/movielist" + "?dirname=" + m_e2stbutils.URLEncode(strRecordingFolder);
  }

  std::string strXML = m_e2stbutils.ConnectToBackend(strURL);

  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(strXML.c_str()))
  {
    XBMC->Log(ADDON::LOG_DEBUG, "[%s] Unable to parse XML %s at line %d", __FUNCTION__, xmlDoc.ErrorDesc(),
        xmlDoc.ErrorRow());
    return false;
  }

  TiXmlHandle hDoc(&xmlDoc);
  TiXmlElement* pElement;
  TiXmlHandle hRoot(0);

  pElement = hDoc.FirstChildElement("e2movielist").Element();

  if (!pElement)
  {
    XBMC->Log(ADDON::LOG_DEBUG, "[%s] Couldn't find <e2movielist> element", __FUNCTION__);
    return false;
  }

  hRoot = TiXmlHandle(pElement);

  TiXmlElement* pNode = hRoot.FirstChildElement("e2movie").Element();

  if (!pNode)
  {
    XBMC->Log(ADDON::LOG_DEBUG, "[%s] Couldn't find <e2movie> element", __FUNCTION__);
    return false;
  }

  int iNumRecording = 0;

  for (; pNode != NULL; pNode = pNode->NextSiblingElement("e2movie"))
  {
    std::string strTemp;
    int iTmp;

    SE2STBRecording recording;

    recording.iLastPlayedPosition = 0;
    if (XMLUtils::GetString(pNode, "e2servicereference", strTemp))
    {
      recording.strRecordingId = strTemp;
    }

    if (XMLUtils::GetString(pNode, "e2title", strTemp))
    {
      recording.strTitle = strTemp;
    }

    if (XMLUtils::GetString(pNode, "e2description", strTemp))
    {
      recording.strPlotOutline = strTemp;
    }

    if (XMLUtils::GetString(pNode, "e2descriptionextended", strTemp))
    {
      recording.strPlot = strTemp;
    }

    if (XMLUtils::GetString(pNode, "e2servicename", strTemp))
    {
      recording.strChannelName = strTemp;
    }

    recording.strIconPath = GetChannelPiconPath(strTemp);

    if (XMLUtils::GetInt(pNode, "e2time", iTmp))
    {
      recording.startTime = iTmp;
    }

    if (XMLUtils::GetString(pNode, "e2length", strTemp))
    {
      recording.iDuration = m_e2stbutils.TimeStringToSeconds(strTemp);
    }
    else
    {
      recording.iDuration = 0;
    }

    if (XMLUtils::GetString(pNode, "e2filename", strTemp))
    {
      recording.strStreamURL = m_strBackendBaseURLWeb + "file?file=" + m_e2stbutils.URLEncode(strTemp);
    }
    m_iNumRecordings++;
    iNumRecording++;
    m_recordings.push_back(recording);
    XBMC->Log(ADDON::LOG_DEBUG, "[%s] Loaded recording %s starting at %d with length %d", __FUNCTION__,
        recording.strTitle.c_str(), recording.startTime, recording.iDuration);
  }
  XBMC->Log(ADDON::LOG_NOTICE, "[%s] Loaded %u recording entries from folder %s", __FUNCTION__, iNumRecording,
      strRecordingFolder.c_str());
  return true;
}

/********************************************//**
 * Transfer recordings
 ***********************************************/
void CE2STBData::TransferRecordings(ADDON_HANDLE handle)
{
  for (unsigned int i = 0; i < m_recordings.size(); i++)
  {
    SE2STBRecording &recording = m_recordings.at(i);
    PVR_RECORDING tag;
    memset(&tag, 0, sizeof(PVR_RECORDING));
    PVR_STRCPY(tag.strRecordingId, recording.strRecordingId.c_str());
    PVR_STRCPY(tag.strTitle, recording.strTitle.c_str());
    PVR_STRCPY(tag.strStreamURL, recording.strStreamURL.c_str());
    PVR_STRCPY(tag.strPlotOutline, recording.strPlotOutline.c_str());
    PVR_STRCPY(tag.strPlot, recording.strPlot.c_str());
    PVR_STRCPY(tag.strChannelName, recording.strChannelName.c_str());
    PVR_STRCPY(tag.strIconPath, recording.strIconPath.c_str());

    std::string strTemp;

    if (IsInRecordingFolder(recording.strTitle))
    {
      strTemp = "/" + recording.strTitle + "/";
    }
    else
    {
      strTemp = "/";
    }
    recording.strDirectory = strTemp;
    PVR_STRCPY(tag.strDirectory, recording.strDirectory.c_str());
    tag.recordingTime = recording.startTime;
    tag.iDuration = recording.iDuration;
    PVR->TransferRecordingEntry(handle, &tag);
  }
}

/********************************************//**
 * Get Picons location for recordings
 ***********************************************/
std::string CE2STBData::GetChannelPiconPath(std::string strChannelName)
{
  for (unsigned int i = 0; i < m_channels.size(); i++)
  {
    if (!strChannelName.compare(m_channels[i].strChannelName))
    {
      return m_channels[i].strIconPath;
    }
  }
  return "";
}

/**************************************************************************//**
 * Stream Handling // Stream Handling // Stream Handling // Stream Handling
 *****************************************************************************/
/********************************************//**
 * Open channel live stream
 ***********************************************/
bool CE2STBData::OpenLiveStream(const PVR_CHANNEL &channel)
{
  XBMC->Log(ADDON::LOG_NOTICE, "[%s] Opening channel %u", __FUNCTION__, channel.iUniqueId);

  if (static_cast<int>(channel.iUniqueId) == m_iCurrentChannel)
  {
    return true;
  }
  SwitchChannel(channel);
  return true;
}

/********************************************//**
 * Switch channel
 ***********************************************/
bool CE2STBData::SwitchChannel(const PVR_CHANNEL &channel)
{
  XBMC->Log(ADDON::LOG_DEBUG, "[%s] Switching to channel %d", __FUNCTION__, channel.iUniqueId);

  if (static_cast<int>(channel.iUniqueId) == m_iCurrentChannel)
  {
    return true;
  }
  CloseLiveStream();
  m_iCurrentChannel = static_cast<int>(channel.iUniqueId);


  /* TODO: Check it works with single tuner STB. It doesn't for
  tuner number > 1 and it shouldnt't(?) unless all tuners are busy?
   */
  if (g_bZapBeforeChannelChange)
  {
    std::string strServiceReference = m_channels.at(channel.iUniqueId - 1).strServiceReference;
    std::string strTemp = "web/zap?sRef=" + m_e2stbutils.URLEncode(strServiceReference);
    XBMC->Log(ADDON::LOG_DEBUG, "[%s] Zap command sent to box %s", __FUNCTION__, strTemp.c_str());

    std::string strResult;
    if (!SendCommandToSTB(strTemp, strResult))
    {
      return false;
    }

    if (!g_bUseTimeshift)
    {
      return true;
    }
  }

  if (m_tsBuffer)
  {
    delete m_tsBuffer;
  }

  XBMC->Log(ADDON::LOG_NOTICE, "[%s] Starting time shift buffer for channel %s", __FUNCTION__, GetLiveStreamURL(channel));
  m_tsBuffer = new CE2STBTimeshift(GetLiveStreamURL(channel), g_strTimeshiftBufferPath);
  return m_tsBuffer->IsValid();
}
/********************************************//**
 * Close channel live stream
 ***********************************************/
void CE2STBData::CloseLiveStream(void)
{
  m_iCurrentChannel = -1;

  if (m_tsBuffer)
  {
    delete m_tsBuffer;
  }
}

/**************************************************************************//**
 * Timers // Timers // Timers // Timers // Timers // Timers // Timers // Timers
 *****************************************************************************/
/********************************************//**
 * Add timer
 ***********************************************/
PVR_ERROR CE2STBData::AddTimer(const PVR_TIMER &timer)
{
  XBMC->Log(ADDON::LOG_DEBUG, "[%s] Channel UID is %d with title %s and EPG ID %d", __FUNCTION__,
      timer.iClientChannelUid, timer.strTitle, timer.iEpgUid);

  std::string strServiceReference = m_channels.at(timer.iClientChannelUid - 1).strServiceReference;
  std::string strTemp = "web/timeradd?sRef="
      + m_e2stbutils.URLEncode(strServiceReference) + "&repeated="
      + m_e2stbutils.IntToString(timer.iWeekdays) + "&begin="
      + m_e2stbutils.IntToString(timer.startTime) + "&end="
      + m_e2stbutils.IntToString(timer.endTime) + "&name="
      + m_e2stbutils.URLEncode(timer.strTitle) + "&description="
      + m_e2stbutils.URLEncode(timer.strSummary) + "&eit="
      + m_e2stbutils.IntToString(timer.iEpgUid);

  if (!g_strBackendRecordingPath.empty())
  {
    strTemp += "&dirname=&" + m_e2stbutils.URLEncode(g_strBackendRecordingPath);
  }

  std::string strResult;
  if (!SendCommandToSTB(strTemp, strResult))
  {
    return PVR_ERROR_SERVER_ERROR;
  }
  TimerUpdates();
  return PVR_ERROR_NO_ERROR;
}

/********************************************//**
 * Delete timer
 ***********************************************/
PVR_ERROR CE2STBData::DeleteTimer(const PVR_TIMER &timer)
{
  std::string strServiceReference = m_channels.at(timer.iClientChannelUid - 1).strServiceReference;
  std::string strTemp = "web/timerdelete?sRef="
      + m_e2stbutils.URLEncode(strServiceReference) + "&begin="
      + m_e2stbutils.IntToString(timer.startTime) + "&end="
      + m_e2stbutils.IntToString(timer.endTime);

  std::string strResult;
  if (!SendCommandToSTB(strTemp, strResult))
  {
    return PVR_ERROR_SERVER_ERROR;
  }

  if (timer.state == PVR_TIMER_STATE_RECORDING)
  {
    PVR->TriggerRecordingUpdate();
  }
  TimerUpdates();
  return PVR_ERROR_NO_ERROR;
}

/********************************************//**
 * Get timers
 ***********************************************/
PVR_ERROR CE2STBData::GetTimers(ADDON_HANDLE handle)
{
  XBMC->Log(ADDON::LOG_NOTICE, "[%s] Number of timers is %d", __FUNCTION__, m_timers.size());
  for (unsigned int i = 0; i < m_timers.size(); i++)
  {
    SE2STBTimer &timer = m_timers.at(i);
    XBMC->Log(ADDON::LOG_DEBUG, "[%s] Transferring timer %s with client index %d", __FUNCTION__,
        timer.strTitle.c_str(), timer.iClientIndex);

    PVR_TIMER tag;
    memset(&tag, 0, sizeof(PVR_TIMER));

    /* TODO: Implement own timer types to get support for the timer features introduced with PVR API 1.9.7 */
    tag.iTimerType = PVR_TIMER_TYPE_NONE;

    tag.iClientChannelUid = timer.iChannelId;
    tag.startTime         = timer.startTime;
    tag.endTime           = timer.endTime;
    PVR_STRCPY(tag.strTitle, timer.strTitle.c_str());
    PVR_STRCPY(tag.strDirectory, "/");                  /* Unused */
    PVR_STRCPY(tag.strSummary, timer.strPlot.c_str());
    tag.state         = timer.state;
    tag.iPriority     = 0;                              /* Unused */
    tag.iLifetime     = 0;                              /* Unused */
    tag.firstDay      = 0;                              /* Unused */
    tag.iWeekdays     = timer.iWeekdays;
    tag.iEpgUid       = timer.iEpgID;
    tag.iMarginStart  = 0;                              /* Unused */
    tag.iMarginEnd    = 0;                              /* Unused */
    tag.iGenreType    = 0;                              /* Unused */
    tag.iGenreSubType = 0;                              /* Unused */
    tag.iClientIndex  = timer.iClientIndex;

    PVR->TransferTimerEntry(handle, &tag);
  }
  return PVR_ERROR_NO_ERROR;
}

/********************************************//**
 * Update timer
 ***********************************************/
PVR_ERROR CE2STBData::UpdateTimer(const PVR_TIMER &timer)
{
  /* TODO Check it works */
  XBMC->Log(ADDON::LOG_DEBUG, "[%s] Timer channel ID %d", __FUNCTION__, timer.iClientChannelUid);

  std::string strServiceReference = m_channels.at(timer.iClientChannelUid - 1).strServiceReference;

  unsigned int i = 0;
  while (i < m_timers.size())
  {
    if (m_timers.at(i).iClientIndex == timer.iClientIndex)
    {
      break;
    }
    else
    {
      i++;
    }
  }
  SE2STBTimer &oldTimer = m_timers.at(i);
  std::string strOldServiceReference = m_channels.at(oldTimer.iChannelId - 1).strServiceReference;
  XBMC->Log(ADDON::LOG_DEBUG, "[%s] Old timer channel ID %d", __FUNCTION__, oldTimer.iChannelId);

  int iDisabled = 0;
  if (timer.state == PVR_TIMER_STATE_CANCELLED)
  {
    iDisabled = 1;
  }
  std::string strTemp = "web/timerchange?sRef="
      + m_e2stbutils.URLEncode(strServiceReference) + "&begin="
      + m_e2stbutils.IntToString(timer.startTime) + "&end="
      + m_e2stbutils.IntToString(timer.endTime) + "&name="
      + m_e2stbutils.URLEncode(timer.strTitle) + "&eventID=&description="
      + m_e2stbutils.URLEncode(timer.strSummary) + "&tags=&afterevent=3&eit=0&disabled="
      + m_e2stbutils.IntToString(iDisabled) + "&justplay=0&repeated="
      + m_e2stbutils.IntToString(timer.iWeekdays) + "&channelOld="
      + m_e2stbutils.URLEncode(strOldServiceReference) + "&beginOld="
      + m_e2stbutils.IntToString(oldTimer.startTime) + "&endOld="
      + m_e2stbutils.IntToString(oldTimer.endTime) + "&deleteOldOnSave=1";

  std::string strResult;
  if (!SendCommandToSTB(strTemp, strResult))
  {
    return PVR_ERROR_SERVER_ERROR;
  }
  TimerUpdates();
  return PVR_ERROR_NO_ERROR;
}

/********************************************//**
 * Update timers
 ***********************************************/
void CE2STBData::TimerUpdates()
{
  std::vector<SE2STBTimer> newtimer = LoadTimers();

  for (unsigned int i = 0; i < m_timers.size(); i++)
  {
    m_timers[i].iUpdateState = E2STB_UPDATE_STATE_NONE;
  }

  unsigned int iUpdated = 0;
  unsigned int iUnchanged = 0;

  for (unsigned int j = 0; j < newtimer.size(); j++)
  {
    for (unsigned int i = 0; i < m_timers.size(); i++)
    {
      if (m_timers[i].like(newtimer[j]))
      {
        if (m_timers[i] == newtimer[j])
        {
          m_timers[i].iUpdateState = E2STB_UPDATE_STATE_FOUND;
          newtimer[j].iUpdateState = E2STB_UPDATE_STATE_FOUND;
          iUnchanged++;
        }
        else
        {
          newtimer[j].iUpdateState = E2STB_UPDATE_STATE_UPDATED;
          m_timers[i].iUpdateState = E2STB_UPDATE_STATE_UPDATED;
          m_timers[i].strTitle     = newtimer[j].strTitle;
          m_timers[i].strPlot      = newtimer[j].strPlot;
          m_timers[i].iChannelId   = newtimer[j].iChannelId;
          m_timers[i].startTime    = newtimer[j].startTime;
          m_timers[i].endTime      = newtimer[j].endTime;
          m_timers[i].iWeekdays    = newtimer[j].iWeekdays;
          m_timers[i].iEpgID       = newtimer[j].iEpgID;
          iUpdated++;
        }
      }
    }
  }

  unsigned int iRemoved = 0;

  for (unsigned int i = 0; i < m_timers.size(); i++)
  {
    if (m_timers.at(i).iUpdateState == E2STB_UPDATE_STATE_NONE)
    {
      XBMC->Log(ADDON::LOG_NOTICE, "[%s] Removed timer %s with client index %d", __FUNCTION__,
          m_timers.at(i).strTitle.c_str(), m_timers.at(i).iClientIndex);
      m_timers.erase(m_timers.begin() + i);
      i = 0;
      iRemoved++;
    }
  }
  unsigned int iNew = 0;

  for (unsigned int i = 0; i < newtimer.size(); i++)
  {
    if (newtimer.at(i).iUpdateState == E2STB_UPDATE_STATE_NEW)
    {
      SE2STBTimer &timer = newtimer.at(i);
      timer.iClientIndex = m_iTimersIndexCounter;
      XBMC->Log(ADDON::LOG_NOTICE, "[%s] New timer %s with client index %d", __FUNCTION__, timer.strTitle.c_str(),
          m_iTimersIndexCounter);
      m_timers.push_back(timer);
      m_iTimersIndexCounter++;
      iNew++;
    }
  }
  XBMC->Log(ADDON::LOG_NOTICE, "[%s] %d timers removed, %d untouched, %d,updated and %d new", __FUNCTION__,
      iRemoved, iUnchanged, iUpdated, iNew);

  if (iRemoved != 0 || iUpdated != 0 || iNew != 0)
  {
    XBMC->Log(ADDON::LOG_NOTICE, "[%s] Timers list changes detected, triggering an update", __FUNCTION__);
    PVR->TriggerTimerUpdate();
  }
}

/********************************************//**
 * Load timers from backend
 ***********************************************/
std::vector<SE2STBTimer> CE2STBData::LoadTimers()
{
  std::vector<SE2STBTimer> timers;

  std::string strURL = m_strBackendBaseURLWeb + "web/timerlist";
  std::string strXML = m_e2stbutils.ConnectToBackend(strURL);

  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse(strXML.c_str()))
  {
    XBMC->Log(ADDON::LOG_DEBUG, "[%s] Unable to parse XML %s at line %d", __FUNCTION__, xmlDoc.ErrorDesc(),
        xmlDoc.ErrorRow());
    return timers;
  }

  TiXmlHandle hDoc(&xmlDoc);
  TiXmlElement* pElement;
  TiXmlHandle hRoot(0);

  pElement = hDoc.FirstChildElement("e2timerlist").Element();

  if (!pElement)
  {
    XBMC->Log(ADDON::LOG_DEBUG, "[%s] Couldn't find <e2timerlist> element", __FUNCTION__);
    return timers;
  }

  hRoot = TiXmlHandle(pElement);

  TiXmlElement* pNode = hRoot.FirstChildElement("e2timer").Element();

  if (!pNode)
  {
    XBMC->Log(ADDON::LOG_DEBUG, "[%s] Couldn't find <e2timer> element", __FUNCTION__);
    return timers;
  }

  for (; pNode != NULL; pNode = pNode->NextSiblingElement("e2timer"))
  {
    std::string strTemp;
    /* TODO Check if's */
    int iTmp;
    bool bTmp;
    int iDisabled;

    if (XMLUtils::GetString(pNode, "e2name", strTemp))
    {
      XBMC->Log(ADDON::LOG_DEBUG, "[%s] Processing timer %s", __FUNCTION__, strTemp.c_str());
    }

    if (!XMLUtils::GetInt(pNode, "e2state", iTmp))
    {
      continue;
    }

    if (!XMLUtils::GetInt(pNode, "e2disabled", iDisabled))
    {
      continue;
    }

    SE2STBTimer timer;

    timer.strTitle = strTemp;

    if (XMLUtils::GetString(pNode, "e2servicereference", strTemp))
    {
      timer.iChannelId = GetTotalChannelNumber(strTemp);
    }

    if (!XMLUtils::GetInt(pNode, "e2timebegin", iTmp))
    {
      continue;
    }

    timer.startTime = iTmp;

    if (!XMLUtils::GetInt(pNode, "e2timeend", iTmp))
    {
      continue;
    }

    timer.endTime = iTmp;

    if (XMLUtils::GetString(pNode, "e2description", strTemp))
    {
      timer.strPlot = strTemp;
    }

    if (XMLUtils::GetInt(pNode, "e2repeated", iTmp))
    {
      timer.iWeekdays = iTmp;
    }
    else
    {
      timer.iWeekdays = 0;
    }

    if (XMLUtils::GetInt(pNode, "e2eit", iTmp))
    {
      timer.iEpgID = iTmp;
    }
    else
    {
      timer.iEpgID = 0;
    }

    timer.state = PVR_TIMER_STATE_NEW;

    if (!XMLUtils::GetInt(pNode, "e2state", iTmp))
    {
      continue;
    }

    XBMC->Log(ADDON::LOG_DEBUG, "[%s] e2state is %d", __FUNCTION__, iTmp);

    if (iTmp == 0)
    {
      timer.state = PVR_TIMER_STATE_SCHEDULED;
      XBMC->Log(ADDON::LOG_DEBUG, "[%s] Timer state is scheduled", __FUNCTION__);
    }

    if (iTmp == 2)
    {
      timer.state = PVR_TIMER_STATE_RECORDING;
      XBMC->Log(ADDON::LOG_DEBUG, "[%s] Timer state is recording", __FUNCTION__);
    }

    if (iTmp == 3 && iDisabled == 0)
    {
      timer.state = PVR_TIMER_STATE_COMPLETED;
      XBMC->Log(ADDON::LOG_DEBUG, "[%s] Timer state is completed", __FUNCTION__);
    }

    if (XMLUtils::GetBoolean(pNode, "e2cancled", bTmp))
    {
      if (bTmp)
      {
        timer.state = PVR_TIMER_STATE_ABORTED;
        XBMC->Log(ADDON::LOG_DEBUG, "[%s] Timer state is aborted", __FUNCTION__);
      }
    }

    if (iDisabled == 1)
    {
      timer.state = PVR_TIMER_STATE_CANCELLED;
      XBMC->Log(ADDON::LOG_DEBUG, "[%s] Timer state is cancelled", __FUNCTION__);
    }

    if (timer.state == PVR_TIMER_STATE_NEW)
    {
      XBMC->Log(ADDON::LOG_DEBUG, "[%s] Timer state is new", __FUNCTION__);
    }

    timers.push_back(timer);
    XBMC->Log(ADDON::LOG_NOTICE, "[%s] Fetched timer %s beginning at %d and ending at %d", __FUNCTION__,
        timer.strTitle.c_str(), timer.startTime, timer.endTime);
  }
  XBMC->Log(ADDON::LOG_NOTICE, "[%s] Fetched %u timer entries", __FUNCTION__, timers.size());
  return timers;
}


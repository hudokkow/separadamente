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

#include <mutex>
#include <string>
#include <thread>
#include <vector>

/**************************************************************************//**
 * General // General // General // General // General // General // General
 *****************************************************************************/
/********************************************//**
 * Constructor
 ***********************************************/
CE2STBData::CE2STBData()
: m_iTimersIndexCounter{1}
, m_iCurrentChannel{-1}
, m_iNumChannelGroups{0}
, m_iNumRecordings{0}
, m_tsBuffer{nullptr}
{
  /* Start the background update thread */
  m_active = true;
  m_backgroundThread = std::thread([this]()
  {
    BackgroundUpdate();
  });
}

/********************************************//**
 * Destructor
 ***********************************************/
CE2STBData::~CE2STBData()
{
  XBMC->Log(ADDON::LOG_DEBUG, "[%s] Stopping background update thread", __FUNCTION__);
  /* Signal the background thread to stop */
  m_active = false;
  if (m_backgroundThread.joinable())
    m_backgroundThread.join();

  std::unique_lock<std::mutex> lock(m_mutex);
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
    m_tsBuffer = nullptr;
  }
}

/********************************************//**
 * Open client
 ***********************************************/
bool CE2STBData::Open()
{
  std::unique_lock<std::mutex> lock(m_mutex);
  if (!m_e2stbconnection.m_bIsConnected)
  {
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
  return true;
}

/********************************************//**
 * Process
 ***********************************************/
void CE2STBData::BackgroundUpdate()
{
  /* Keep count of how many times the loop has run so we can trigger
   * updates according to g_iClientUpdateInterval client setting */
  static unsigned int lapCounter = 1;

  /* g_iClientUpdateInterval is set in minutes and there are 12 loops in each minute */
  g_iClientUpdateInterval *= 12;

  XBMC->Log(ADDON::LOG_DEBUG, "[%s] Starting background update thread", __FUNCTION__);

  for (unsigned int iChannelPtr = 0; iChannelPtr < m_channels.size(); iChannelPtr++)
  {
    XBMC->Log(ADDON::LOG_DEBUG, "[%s] Triggering EPG update for channel %d", __FUNCTION__, iChannelPtr);
    PVR->TriggerEpgUpdate(m_channels.at(iChannelPtr).iUniqueId);
  }

  while (m_active)
  {
    if (lapCounter % g_iClientUpdateInterval == 0)
    {
      std::unique_lock<std::mutex> lock(m_mutex);
      XBMC->Log(ADDON::LOG_DEBUG, "[%s] Updating timers and recordings", __FUNCTION__);

      if (g_bAutomaticTimerlistCleanup)
      {
        std::string strTemp = "web/timercleanup?cleanup=true";

        std::string strResult;
        if (!m_e2stbconnection.SendCommandToSTB(strTemp, strResult))
        {
          XBMC->Log(ADDON::LOG_ERROR, "[%s] Automatic timer list cleanup failed", __FUNCTION__);
        }
      }
      TimerUpdates();
      PVR->TriggerRecordingUpdate();
    }
    lapCounter++;
    usleep(5000 * 1000);
  }
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
      strncpy(xbmcChannel.strChannelName, channel.strChannelName.c_str(), sizeof(xbmcChannel.strChannelName) - 1);
      strncpy(xbmcChannel.strInputFormat, "", 0); /* Unused */

      xbmcChannel.iEncryptionSystem = 0;
      xbmcChannel.bIsHidden = false;

      strncpy(xbmcChannel.strIconPath, channel.strIconPath.c_str(), sizeof(xbmcChannel.strIconPath) - 1);

      if (!g_bUseTimeshift)
      {
        std::string strStream = "pvr://stream/tv/" + m_e2stbutils.IntToString(channel.iUniqueId) + ".ts";
        strncpy(xbmcChannel.strStreamURL, strStream.c_str(), sizeof(xbmcChannel.strStreamURL) - 1);
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
    PVR_CHANNEL_GROUP channelsGroups;
    memset(&channelsGroups, 0, sizeof(PVR_CHANNEL_GROUP));

    channelsGroups.bIsRadio  = false;
    channelsGroups.iPosition = 0; /* groups default order, unused */
    strncpy(channelsGroups.strGroupName, m_channelsGroups[iTagPtr].strGroupName.c_str(), sizeof(channelsGroups.strGroupName) - 1);
    PVR->TransferChannelGroup(handle, &channelsGroups);
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
      PVR_CHANNEL_GROUP_MEMBER channelGroupMembers;
      memset(&channelGroupMembers, 0, sizeof(PVR_CHANNEL_GROUP_MEMBER));

      strncpy(channelGroupMembers.strGroupName, group.strGroupName, sizeof(channelGroupMembers.strGroupName) - 1);
      channelGroupMembers.iChannelUniqueId = myChannel.iUniqueId;
      channelGroupMembers.iChannelNumber = myChannel.iChannelNumber;

      XBMC->Log(ADDON::LOG_DEBUG, "[%s] Added channel %s with unique ID %d to group %s and channel number %d",
          __FUNCTION__, myChannel.strChannelName.c_str(), channelGroupMembers.iChannelUniqueId, group.strGroupName,
          myChannel.iChannelNumber);
      PVR->TransferChannelGroupMember(handle, &channelGroupMembers);
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

  std::string strURL = m_e2stbconnection.m_strBackendBaseURLWeb + "web/getservices?sRef="
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
    std::string strURL = m_e2stbconnection.m_strBackendBaseURLStream + strTemp2;

    newChannel.strStreamURL = strURL;

    if (g_bLoadWebInterfacePicons)
    {
      std::replace(strTemp2.begin(), strTemp2.end(), ':', '_');
      strURL = m_e2stbconnection.m_strBackendBaseURLWeb + "picon/" + strTemp2 + ".png";
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
  std::string strURL = m_e2stbconnection.m_strBackendBaseURLWeb + "web/getservices";
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

  std::string strURL = m_e2stbconnection.m_strBackendBaseURLWeb + "web/epgservice?sRef="
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

    EPG_TAG channelEPG;
    memset(&channelEPG, 0, sizeof(EPG_TAG));

    channelEPG.iUniqueBroadcastId  = entry.iEventId;
    channelEPG.strTitle            = entry.strTitle.c_str();
    channelEPG.iChannelNumber      = channel.iChannelNumber;
    channelEPG.startTime           = entry.startTime;
    channelEPG.endTime             = entry.endTime;
    channelEPG.strPlotOutline      = entry.strPlotOutline.c_str();
    channelEPG.strPlot             = entry.strPlot.c_str();
    channelEPG.strOriginalTitle    = "";    /* Unused */
    channelEPG.strCast             = "";    /* Unused */
    channelEPG.strDirector         = "";    /* Unused */
    channelEPG.strWriter           = "";    /* Unused */
    channelEPG.iYear               = 0;     /* Unused */
    channelEPG.strIMDBNumber       = "";    /* Unused */
    channelEPG.strIconPath         = "";    /* Unused */
    channelEPG.iGenreType          = 0;     /* Unused */
    channelEPG.iGenreSubType       = 0;     /* Unused */
    channelEPG.strGenreDescription = "";    /* Unused */
    channelEPG.firstAired          = 0;     /* Unused */
    channelEPG.iParentalRating     = 0;     /* Unused */
    channelEPG.iStarRating         = 0;     /* Unused */
    channelEPG.bNotify             = false; /* Unused */
    channelEPG.iSeriesNumber       = 0;     /* Unused */
    channelEPG.iEpisodeNumber      = 0;     /* Unused */
    channelEPG.iEpisodePartNumber  = 0;     /* Unused */
    channelEPG.strEpisodeName      = "";    /* Unused */
    channelEPG.iFlags              = EPG_TAG_FLAG_UNDEFINED;

    PVR->TransferEpgEntry(handle, &channelEPG);

    iNumEPG++;

    XBMC->Log(ADDON::LOG_DEBUG, "[%s] Loaded EPG entry %d - %s for channel %d starting at %d and ending at %d",
        __FUNCTION__, channelEPG.iUniqueBroadcastId, channelEPG.strTitle, entry.iChannelId, entry.startTime,
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
  std::string strURL = m_e2stbconnection.m_strBackendBaseURLWeb + "web/deviceinfo";
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
  static PVR_SIGNAL_STATUS signalStat;
  memset(&signalStat, 0, sizeof(signalStat));

  std::string strURL = m_e2stbconnection.m_strBackendBaseURLWeb + "web/signal";
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
  signalStat.iSNR = (std::stof(strTemp.c_str()) * 5.88235 * 655.35 + 0.5);

  if (!XMLUtils::GetString(pElement, "e2snr", strTemp))
  {
    XBMC->Log(ADDON::LOG_ERROR, "[%s] Couldn't parse <e2snr> from result", __FUNCTION__);
  }
  std::string strSNR = strTemp;
  XBMC->Log(ADDON::LOG_DEBUG, "[%s] SNR is %s", __FUNCTION__, strSNR.c_str());
  signalStat.iSignal = (std::stoi(strTemp.c_str()) * 655.35);

  if (!XMLUtils::GetString(pElement, "e2ber", strTemp))
  {
    XBMC->Log(ADDON::LOG_ERROR, "[%s] Couldn't parse <e2ber> from result", __FUNCTION__);
  }
  std::string strBER = strTemp;
  XBMC->Log(ADDON::LOG_DEBUG, "[%s] BER is %s", __FUNCTION__, strBER.c_str());
  signalStat.iBER = std::stol(strTemp.c_str());

  signalStatus = signalStat;
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
  if (!m_e2stbconnection.SendCommandToSTB(strTemp, strResult))
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
    strURL = m_e2stbconnection.m_strBackendBaseURLWeb + "web/getcurrlocation";
  }
  else
  {
    strURL = m_e2stbconnection.m_strBackendBaseURLWeb + "web/getlocations";
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
    strURL = m_e2stbconnection.m_strBackendBaseURLWeb + "web/movielist";
  }
  else
  {
    strURL = m_e2stbconnection.m_strBackendBaseURLWeb + "web/movielist" + "?dirname=" + m_e2stbutils.URLEncode(strRecordingFolder);
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
      recording.strStreamURL = m_e2stbconnection.m_strBackendBaseURLWeb + "file?file=" + m_e2stbutils.URLEncode(strTemp);
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
    PVR_RECORDING recordings;
    memset(&recordings, 0, sizeof(PVR_RECORDING));
    strncpy(recordings.strRecordingId, recording.strRecordingId.c_str(), sizeof(recordings.strRecordingId) - 1);
    strncpy(recordings.strTitle, recording.strTitle.c_str(), sizeof(recordings.strTitle) - 1);
    strncpy(recordings.strStreamURL, recording.strStreamURL.c_str(), sizeof(recordings.strStreamURL) - 1);
    strncpy(recordings.strPlotOutline, recording.strPlotOutline.c_str(), sizeof(recordings.strPlotOutline) - 1);
    strncpy(recordings.strPlot, recording.strPlot.c_str(), sizeof(recordings.strPlot) - 1);
    strncpy(recordings.strChannelName, recording.strChannelName.c_str(), sizeof(recordings.strChannelName) - 1);
    strncpy(recordings.strIconPath, recording.strIconPath.c_str(), sizeof(recordings.strIconPath) - 1);

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
    strncpy(recordings.strDirectory, recording.strDirectory.c_str(), sizeof(recordings.strDirectory) - 1);
    recordings.recordingTime = recording.startTime;
    recordings.iDuration = recording.iDuration;
    PVR->TransferRecordingEntry(handle, &recordings);
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
    if (!m_e2stbconnection.SendCommandToSTB(strTemp, strResult))
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
    m_tsBuffer = nullptr;
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
    m_tsBuffer = nullptr;
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

  timer_t startTime = timer.startTime - (timer.iMarginStart * 60);
  timer_t endTime = timer.endTime + (timer.iMarginEnd * 60);

  std::string strServiceReference = m_channels.at(timer.iClientChannelUid - 1).strServiceReference;
  std::string strTemp = "web/timeradd?sRef=" + m_e2stbutils.URLEncode(strServiceReference)
      + "&repeated=" + m_e2stbutils.IntToString(timer.iWeekdays)
      + "&begin=" + startTime
      + "&end=" + endTime
      + "&name=" + m_e2stbutils.URLEncode(timer.strTitle)
      + "&description=" + m_e2stbutils.URLEncode(timer.strSummary)
      + "&eit=" + m_e2stbutils.IntToString(timer.iEpgUid);

  if (!g_strBackendRecordingPath.empty())
  {
    strTemp += "&dirname=&" + m_e2stbutils.URLEncode(g_strBackendRecordingPath);
  }

  std::string strResult;
  if (!m_e2stbconnection.SendCommandToSTB(strTemp, strResult))
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
  timer_t startTime = timer.startTime - (timer.iMarginStart * 60);
  timer_t endTime = timer.endTime + (timer.iMarginEnd * 60);

  std::string strServiceReference = m_channels.at(timer.iClientChannelUid - 1).strServiceReference;
  std::string strTemp = "web/timerdelete?sRef=" + m_e2stbutils.URLEncode(strServiceReference)
      + "&begin=" + startTime
      + "&end=" + endTime;

  std::string strResult;
  if (!m_e2stbconnection.SendCommandToSTB(strTemp, strResult))
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

    PVR_TIMER timers;
    memset(&timers, 0, sizeof(PVR_TIMER));

    /* TODO: Implement own timer types to get support for the timer features introduced with PVR API 1.9.7 */
    timers.iTimerType = PVR_TIMER_TYPE_NONE;

    timers.iClientChannelUid = timer.iChannelId;
    timers.startTime         = timer.startTime;
    timers.endTime           = timer.endTime;
    strncpy(timers.strTitle, timer.strTitle.c_str(), sizeof(timers.strTitle) - 1);
    strncpy(timers.strDirectory, "/", sizeof(timers.strDirectory) - 1);
    strncpy(timers.strSummary, timer.strPlot.c_str(), sizeof(timers.strSummary) - 1);
    timers.state         = timer.state;
    timers.iPriority     = 0;                              /* Unused */
    timers.iLifetime     = 0;                              /* Unused */
    timers.firstDay      = 0;                              /* Unused */
    timers.iWeekdays     = timer.iWeekdays;
    timers.iEpgUid       = timer.iEpgID;
    timers.iMarginStart  = 0;                              /* Unused */
    timers.iMarginEnd    = 0;                              /* Unused */
    timers.iGenreType    = 0;                              /* Unused */
    timers.iGenreSubType = 0;                              /* Unused */
    timers.iClientIndex  = timer.iClientIndex;

    PVR->TransferTimerEntry(handle, &timers);
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
  if (!m_e2stbconnection.SendCommandToSTB(strTemp, strResult))
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

  std::string strURL = m_e2stbconnection.m_strBackendBaseURLWeb + "web/timerlist";
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


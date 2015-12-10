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

#include "E2STBChannels.h"

#include "client.h"

#include "tinyxml.h"
#include "E2STBXMLUtils.h"

#include <algorithm>
#include <string>
#include <vector>

CE2STBChannels::CE2STBChannels()
: m_iNumChannelGroups{0}
{
}

CE2STBChannels::~CE2STBChannels()
{
  XBMC->Log(ADDON::LOG_DEBUG, "[%s] Removing internal channels list", __FUNCTION__);
  m_channels.clear();

  XBMC->Log(ADDON::LOG_DEBUG, "[%s] Removing internal channels groups list", __FUNCTION__);
  m_channelsGroups.clear();
}

bool CE2STBChannels::Open()
{
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
  return true;
}

PVR_ERROR CE2STBChannels::GetChannels(ADDON_HANDLE handle, bool bRadio)
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

PVR_ERROR CE2STBChannels::GetChannelGroups(ADDON_HANDLE handle)
{
  for (unsigned int iTagPtr = 0; iTagPtr < m_channelsGroups.size(); iTagPtr++)
  {
    PVR_CHANNEL_GROUP channelsGroups;
    memset(&channelsGroups, 0, sizeof(PVR_CHANNEL_GROUP));

    channelsGroups.bIsRadio = false;
    channelsGroups.iPosition = 0; /* groups default order, unused */
    strncpy(channelsGroups.strGroupName, m_channelsGroups[iTagPtr].strGroupName.c_str(),
        sizeof(channelsGroups.strGroupName) - 1);
    PVR->TransferChannelGroup(handle, &channelsGroups);
  }
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR CE2STBChannels::GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group)
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

int CE2STBChannels::GetTotalChannelNumber(std::string strServiceReference)
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

PVR_ERROR CE2STBChannels::GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
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

bool CE2STBChannels::LoadChannels(std::string strServiceReference, std::string strGroupName)
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

bool CE2STBChannels::LoadChannels()
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

bool CE2STBChannels::LoadChannelGroups()
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

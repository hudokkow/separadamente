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

#include "E2STBConnection.h"

#include "kodi/xbmc_addon_types.h"
#include "kodi/xbmc_pvr_types.h"

#include <ctime>
#include <string>
#include <vector>

namespace e2stb
{
struct SE2STBEPG
{
  int         iEventId;
  std::string strServiceReference;
  std::string strTitle;
  int         iChannelId;
  time_t      startTime;
  time_t      endTime;
  std::string strPlotOutline;
  std::string strPlot;
};

struct SE2STBChannelGroup
{
  std::string strServiceReference;
  std::string strGroupName;
};

struct SE2STBChannel
{
  bool        bRadio;
  int         iUniqueId;
  int         iChannelNumber;
  std::string strGroupName;
  std::string strChannelName;
  std::string strServiceReference;
  std::string strStreamURL;
  std::string strIconPath;
};

class CE2STBChannels
{
public:
  CE2STBChannels();
  ~CE2STBChannels();

  std::vector<SE2STBChannel> m_channels;
  int GetChannelsAmount(void) { return m_channels.size(); }
  PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio);
  PVR_ERROR GetChannelGroups(ADDON_HANDLE handle);
  PVR_ERROR GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group);
  unsigned int GetChannelGroupsAmount(void) { return m_iNumChannelGroups; }
  int GetTotalChannelNumber(std::string strServiceReference);
  const char* GetLiveStreamURL(const PVR_CHANNEL &channel);
  PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd);

private:
  int m_iNumChannelGroups;
  std::vector<SE2STBChannelGroup> m_channelsGroups;

  bool LoadChannels(std::string strServerReference, std::string strGroupName);
  bool LoadChannels();
  bool LoadChannelGroups();

  CE2STBConnection m_e2stbconnection; /*!< @brief CE2STBConnection class handler */
};
} /* namespace e2stb */

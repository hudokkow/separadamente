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
#include "compat.h"
#include "E2STBXMLUtils.h"

#include "kodi/xbmc_addon_types.h"
#include "kodi/xbmc_pvr_types.h"
#include "p8-platform/util/util.h"

#include "tinyxml.h"
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using namespace e2stb;

CE2STBData::CE2STBData()
: m_iTimersIndexCounter{1}
, m_iCurrentChannel{-1}
, m_tsBuffer{nullptr}
{
  TimerUpdates();
  /* Start the background update thread */
  m_active = true;
  m_backgroundThread = std::thread([this]()
    {
      BackgroundUpdate();
    });
}

CE2STBData::~CE2STBData()
{
  XBMC->Log(ADDON::LOG_DEBUG, "[%s] Stopping background update thread", __FUNCTION__);
  /* Signal the background thread to stop */
  m_active = false;
  if (m_backgroundThread.joinable())
    m_backgroundThread.join();

  std::unique_lock<std::mutex> lock(m_mutex);
  XBMC->Log(ADDON::LOG_DEBUG, "[%s] Removing internal timers list", __FUNCTION__);
  m_timers.clear();

  if (m_tsBuffer)
  {
    XBMC->Log(ADDON::LOG_DEBUG, "[%s] Removing internal time shifting buffer", __FUNCTION__);
    SAFE_DELETE(m_tsBuffer);
  }
}

void CE2STBData::BackgroundUpdate()
{
  /* Keep count of how many times the loop has run so we can trigger
   * updates according to g_iClientUpdateInterval client setting */
  static unsigned int lapCounter = 1;

  /* g_iClientUpdateInterval is set in minutes and there are 12 loops in each minute */
  g_iClientUpdateInterval *= 12;

  XBMC->Log(ADDON::LOG_DEBUG, "[%s] Starting background update thread", __FUNCTION__);

  for (unsigned int iChannelPtr = 0; iChannelPtr < m_e2stbchannels.m_channels.size(); iChannelPtr++)
  {
    XBMC->Log(ADDON::LOG_DEBUG, "[%s] Triggering EPG update for channel %d", __FUNCTION__, iChannelPtr);
    PVR->TriggerEpgUpdate(m_e2stbchannels.m_channels.at(iChannelPtr).iUniqueId);
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

bool CE2STBData::OpenLiveStream(const PVR_CHANNEL &channel)
{
  XBMC->Log(ADDON::LOG_NOTICE, "[%s] Opening channel %u", __FUNCTION__, channel.iUniqueId);

  if (static_cast<int>(channel.iUniqueId) == m_iCurrentChannel)
    return true;

  SwitchChannel(channel);
  return true;
}

bool CE2STBData::SwitchChannel(const PVR_CHANNEL &channel)
{
  XBMC->Log(ADDON::LOG_DEBUG, "[%s] Switching to channel %d", __FUNCTION__, channel.iUniqueId);

  if (static_cast<int>(channel.iUniqueId) == m_iCurrentChannel)
    return true;

  CloseLiveStream();
  m_iCurrentChannel = static_cast<int>(channel.iUniqueId);


  /* TODO: Check it works with single tuner STB. It doesn't for
  tuner number > 1 and it shouldnt't(?) unless all tuners are busy? */
  if (g_bZapBeforeChannelChange)
  {
    std::string strServiceReference = m_e2stbchannels.m_channels.at(channel.iUniqueId - 1).strServiceReference;
    std::string strTemp = "web/zap?sRef=" + m_e2stbconnection.URLEncode(strServiceReference);
    XBMC->Log(ADDON::LOG_DEBUG, "[%s] Zap command sent to box %s", __FUNCTION__, strTemp.c_str());

    std::string strResult;
    if (!m_e2stbconnection.SendCommandToSTB(strTemp, strResult))
      return false;

    if (!g_bUseTimeshift)
      return true;
  }

  if (m_tsBuffer)
    SAFE_DELETE(m_tsBuffer);

  XBMC->Log(ADDON::LOG_NOTICE, "[%s] Starting time shift buffer for channel %s", __FUNCTION__, m_e2stbchannels.GetLiveStreamURL(channel));
  m_tsBuffer = new CE2STBTimeshift(m_e2stbchannels.GetLiveStreamURL(channel), g_strTimeshiftBufferPath);
  return m_tsBuffer->IsValid();
}

void CE2STBData::CloseLiveStream(void)
{
  m_iCurrentChannel = -1;

  if (m_tsBuffer)
    SAFE_DELETE(m_tsBuffer);
}

PVR_ERROR CE2STBData::AddTimer(const PVR_TIMER &timer)
{
  XBMC->Log(ADDON::LOG_DEBUG, "[%s] Channel UID is %d with title %s and EPG ID %d", __FUNCTION__,
      timer.iClientChannelUid, timer.strTitle, timer.iEpgUid);

  unsigned int marginBefore = timer.startTime - (timer.iMarginStart * 60);
  unsigned int marginAfter = timer.endTime + (timer.iMarginEnd * 60);

  std::string strServiceReference = m_e2stbchannels.m_channels.at(timer.iClientChannelUid - 1).strServiceReference;
  std::string strTemp = "web/timeradd?sRef=" + m_e2stbconnection.URLEncode(strServiceReference) +
      "&repeated=" + compat::to_string(timer.iWeekdays) +
      "&begin=" + compat::to_string(marginBefore) +
      "&end=" + compat::to_string(marginAfter) +
      "&name=" + m_e2stbconnection.URLEncode(timer.strTitle)+
      "&description=" + m_e2stbconnection.URLEncode(timer.strSummary)+
      "&eit=" + compat::to_string(timer.iEpgUid);

  if (!g_strBackendRecordingPath.empty())
    strTemp += "&dirname=&" + m_e2stbconnection.URLEncode(g_strBackendRecordingPath);

  XBMC->Log(ADDON::LOG_NOTICE, "[%s] Added timer %s", __FUNCTION__, strTemp.c_str());

  std::string strResult;
  if (!m_e2stbconnection.SendCommandToSTB(strTemp, strResult))
    return PVR_ERROR_SERVER_ERROR;

  TimerUpdates();
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR CE2STBData::DeleteTimer(const PVR_TIMER &timer)
{
  unsigned int marginBefore = timer.startTime - (timer.iMarginStart * 60);
  unsigned int marginAfter = timer.endTime + (timer.iMarginEnd * 60);

  /* TODO: test this */
  std::string strServiceReference = m_e2stbchannels.m_channels.at(timer.iClientChannelUid - 1).strServiceReference;
  std::string strTemp = "web/timerdelete?sRef=" + m_e2stbconnection.URLEncode(strServiceReference) +
      "&begin=" + compat::to_string(marginBefore) +
      "&end=" + compat::to_string(marginAfter);

  std::string strResult;
  if (!m_e2stbconnection.SendCommandToSTB(strTemp, strResult))
    return PVR_ERROR_SERVER_ERROR;

  XBMC->Log(ADDON::LOG_NOTICE, "[%s] Deleted timer %s", __FUNCTION__, strTemp.c_str());

  if (timer.state == PVR_TIMER_STATE_RECORDING)
    PVR->TriggerRecordingUpdate();

  TimerUpdates();
  return PVR_ERROR_NO_ERROR;
}

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
    timers.iPriority     = 0; /* Unused */
    timers.iLifetime     = 0; /* Unused */
    timers.firstDay      = 0; /* Unused */
    timers.iWeekdays     = timer.iWeekdays;
    timers.iEpgUid       = timer.iEpgID;
    timers.iMarginStart  = 0; /* Unused */
    timers.iMarginEnd    = 0; /* Unused */
    timers.iGenreType    = 0; /* Unused */
    timers.iGenreSubType = 0; /* Unused */
    timers.iClientIndex  = timer.iClientIndex;

    PVR->TransferTimerEntry(handle, &timers);
  }
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR CE2STBData::UpdateTimer(const PVR_TIMER &timer)
{
  /* TODO Check it works */
  XBMC->Log(ADDON::LOG_DEBUG, "[%s] Timer channel ID %d", __FUNCTION__, timer.iClientChannelUid);

  std::string strServiceReference = m_e2stbchannels.m_channels.at(timer.iClientChannelUid - 1).strServiceReference;

  unsigned int i = 0;
  while (i < m_timers.size())
  {
    if (m_timers.at(i).iClientIndex == timer.iClientIndex)
      break;
    else
      i++;
  }
  SE2STBTimer &oldTimer = m_timers.at(i);
  std::string strOldServiceReference = m_e2stbchannels.m_channels.at(oldTimer.iChannelId - 1).strServiceReference;
  XBMC->Log(ADDON::LOG_DEBUG, "[%s] Old timer channel ID %d", __FUNCTION__, oldTimer.iChannelId);

  int iDisabled = 0;
  if (timer.state == PVR_TIMER_STATE_CANCELLED)
    iDisabled = 1;

  std::string strTemp = "web/timerchange?sRef=" +
      m_e2stbconnection.URLEncode(strServiceReference) + "&begin=" +
      compat::to_string(timer.startTime) + "&end=" +
      compat::to_string(timer.endTime) + "&name=" +
      m_e2stbconnection.URLEncode(timer.strTitle) + "&eventID=&description=" +
      m_e2stbconnection.URLEncode(timer.strSummary) + "&tags=&afterevent=3&eit=0&disabled=" +
      compat::to_string(iDisabled) + "&justplay=0&repeated=" +
      compat::to_string(timer.iWeekdays) + "&channelOld=" +
      m_e2stbconnection.URLEncode(strOldServiceReference) + "&beginOld=" +
      compat::to_string(oldTimer.startTime) + "&endOld=" +
      compat::to_string(oldTimer.endTime) + "&deleteOldOnSave=1";

  std::string strResult;
  if (!m_e2stbconnection.SendCommandToSTB(strTemp, strResult))
    return PVR_ERROR_SERVER_ERROR;

  TimerUpdates();
  return PVR_ERROR_NO_ERROR;
}

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

std::vector<SE2STBTimer> CE2STBData::LoadTimers()
{
  std::vector<SE2STBTimer> timers;

  std::string strURL = m_e2stbconnection.GetBackendURLWeb() + "web/timerlist";
  std::string strXML = m_e2stbconnection.ConnectToBackend(strURL);

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
      XBMC->Log(ADDON::LOG_DEBUG, "[%s] Processing timer %s", __FUNCTION__, strTemp.c_str());

    if (!XMLUtils::GetInt(pNode, "e2state", iTmp))
      continue;

    if (!XMLUtils::GetInt(pNode, "e2disabled", iDisabled))
      continue;

    SE2STBTimer timer;

    timer.strTitle = strTemp;

    if (XMLUtils::GetString(pNode, "e2servicereference", strTemp))
      timer.iChannelId = m_e2stbchannels.GetTotalChannelNumber(strTemp);

    if (!XMLUtils::GetInt(pNode, "e2timebegin", iTmp))
      continue;

    timer.startTime = iTmp;

    if (!XMLUtils::GetInt(pNode, "e2timeend", iTmp))
      continue;

    timer.endTime = iTmp;

    if (XMLUtils::GetString(pNode, "e2description", strTemp))
      timer.strPlot = strTemp;

    if (XMLUtils::GetInt(pNode, "e2repeated", iTmp))
      timer.iWeekdays = iTmp;
    else
      timer.iWeekdays = 0;

    if (XMLUtils::GetInt(pNode, "e2eit", iTmp))
      timer.iEpgID = iTmp;
    else
      timer.iEpgID = 0;

    timer.state = PVR_TIMER_STATE_NEW;

    if (!XMLUtils::GetInt(pNode, "e2state", iTmp))
      continue;

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
      XBMC->Log(ADDON::LOG_DEBUG, "[%s] Timer state is new", __FUNCTION__);

    timers.push_back(timer);
    XBMC->Log(ADDON::LOG_NOTICE, "[%s] Fetched timer %s beginning at %d and ending at %d", __FUNCTION__,
        timer.strTitle.c_str(), timer.startTime, timer.endTime);
  }
  XBMC->Log(ADDON::LOG_NOTICE, "[%s] Fetched %u timer entries", __FUNCTION__, timers.size());
  return timers;
}


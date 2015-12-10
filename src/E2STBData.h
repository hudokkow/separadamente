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

#include "client.h"

#include "E2STBChannels.h"
#include "E2STBConnection.h"
#include "E2STBTimeshift.h"
#include "E2STBUtils.h"

#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

typedef enum E2STB_UPDATE_STATE
{
  E2STB_UPDATE_STATE_NONE,
  E2STB_UPDATE_STATE_FOUND,
  E2STB_UPDATE_STATE_UPDATED,
  E2STB_UPDATE_STATE_NEW
} E2STB_UPDATE_STATE;

struct SE2STBTimer
{
  std::string     strTitle;
  std::string     strPlot;
  int             iChannelId;
  time_t          startTime;
  time_t          endTime;
  int             iWeekdays;
  unsigned int    iEpgID;
  PVR_TIMER_STATE state;
  int             iUpdateState;
  unsigned int    iClientIndex;

  bool like(const SE2STBTimer &right) const
  {
    bool bChanged = true;
    bChanged = bChanged && (startTime  == right.startTime);
    bChanged = bChanged && (endTime    == right.endTime);
    bChanged = bChanged && (iChannelId == right.iChannelId);
    bChanged = bChanged && (iWeekdays  == right.iWeekdays);
    bChanged = bChanged && (iEpgID     == right.iEpgID);

    return bChanged;
  }

  bool operator ==(const SE2STBTimer &right) const
  {
    bool bChanged = true;
    bChanged = bChanged && (startTime  == right.startTime);
    bChanged = bChanged && (endTime    == right.endTime);
    bChanged = bChanged && (iChannelId == right.iChannelId);
    bChanged = bChanged && (iWeekdays  == right.iWeekdays);
    bChanged = bChanged && (iEpgID     == right.iEpgID);
    bChanged = bChanged && (state      == right.state);
    bChanged = bChanged && (!strTitle.compare(right.strTitle));
    bChanged = bChanged && (!strPlot.compare(right.strPlot));

    return bChanged;
  }
};

class CE2STBData
{
  public:
    CE2STBData(void);
    ~CE2STBData();

    /* Client creation and connection */
    bool        Open();

    /* Channels */
    int          GetCurrentClientChannel(void) { return m_iCurrentChannel; }

    /* Information */
    PVR_ERROR GetDriveSpace(long long *iTotal, long long *iUsed); /*!< @brief Backend Interface */
    PVR_ERROR SignalStatus(PVR_SIGNAL_STATUS &signalStatus);      /*!< @brief Backend Interface */

    /* Stream handling */
    bool        OpenLiveStream(const PVR_CHANNEL &channel);
    bool        SwitchChannel(const PVR_CHANNEL &channel);
    void        CloseLiveStream();
    const char* GetLiveStreamURL(const PVR_CHANNEL &channel) { return m_e2stbchannels.m_channels.at(channel.iUniqueId - 1).strStreamURL.c_str(); }

    /* Timers */
    int       GetTimersAmount(void) { return m_timers.size(); }
    PVR_ERROR AddTimer(const PVR_TIMER &timer);    /*!< @brief Backend Interface */
    PVR_ERROR DeleteTimer(const PVR_TIMER &timer); /*!< @brief Backend Interface */
    PVR_ERROR GetTimers(ADDON_HANDLE handle);
    PVR_ERROR UpdateTimer(const PVR_TIMER &timer); /*!< @brief Backend Interface */

    /* Time shifting */
    CE2STBTimeshift *GetTimeshiftBuffer() { return m_tsBuffer; }

  private:
    /********************************************//**
     * Members
     ***********************************************/
    unsigned int m_iTimersIndexCounter;    /*!< @brief Timers counter */

    /* Channels */
    int                             m_iCurrentChannel;   /*!< @brief Current channel uniqueID */

    /* Timers */
    std::vector<SE2STBTimer> m_timers; /*!< @brief Backend timers */

    /**
     * Controls whether the background update thread should keep running or not
     */
    std::atomic<bool> m_active;

    /**
     * The background update thread
     */
    std::thread m_backgroundThread;


    /********************************************//**
     * Functions
     ***********************************************/
    void BackgroundUpdate();

    /* Timers */
    void TimerUpdates();
    std::vector<SE2STBTimer> LoadTimers(); /*!< @brief Backend Interface */

    /* Time shifting */
    CE2STBTimeshift *m_tsBuffer; /*!< @brief Time shifting class handler */

    mutable std::mutex m_mutex;         /*!< @brief mutex class handler */
    CE2STBChannels   m_e2stbchannels; /*!< @brief CE2STBChannels class handler */
    CE2STBConnection m_e2stbconnection; /*!< @brief CE2STBConnection class handler */
    CE2STBUtils      m_e2stbutils;      /*!< @brief CE2STBUtils class handler */
};


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

#include "E2STBTimeshift.h"
#include "E2STBUtils.h"

#include "platform/util/StdString.h"
#include "platform/threads/threads.h"

#include <string>
#include <vector>

typedef enum E2STB_UPDATE_STATE
{
  E2STB_UPDATE_STATE_NONE,
  E2STB_UPDATE_STATE_FOUND,
  E2STB_UPDATE_STATE_UPDATED,
  E2STB_UPDATE_STATE_NEW
} E2STB_UPDATE_STATE;

struct SE2STBEPG
{
    int iEventId;
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
    int         iGroupState;
    std::vector<SE2STBEPG> EPG;
};

struct SE2STBChannel
{
    bool        bRadio;
    /* TODO: Delete */
    // bool        bInitialEPG;
    int         iUniqueId;
    int         iChannelNumber;
    std::string strGroupName;
    std::string strChannelName;
    std::string strServiceReference;
    std::string strStreamURL;
    std::string strIconPath;
};

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

    SE2STBTimer()
    {
      iUpdateState = E2STB_UPDATE_STATE_NEW;
    }

    bool like (const SE2STBTimer &right) const
    {
      bool bChanged = true;
      bChanged = bChanged && (startTime  == right.startTime);
      bChanged = bChanged && (endTime    == right.endTime);
      bChanged = bChanged && (iChannelId == right.iChannelId);
      bChanged = bChanged && (iWeekdays  == right.iWeekdays);
      bChanged = bChanged && (iEpgID     == right.iEpgID);

      return bChanged;
    }

    bool operator == (const SE2STBTimer &right) const
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

struct SE2STBRecording
{
    std::string strRecordingId;
    time_t      startTime;
    int         iDuration;
    int         iLastPlayedPosition;
    std::string strTitle;
    std::string strStreamURL;
    std::string strPlot;
    std::string strPlotOutline;
    std::string strChannelName;
    std::string strDirectory;
    std::string strIconPath;
};

class CE2STBData: public PLATFORM::CThread
{
  public:
    CE2STBData(void);
    ~CE2STBData();

    /* Client creation and connection */
    bool        Open();
    bool        IsConnected();
    void        SendPowerstate();
    const char* GetServerName() { return m_strServerName.c_str(); }

    /* Channels */
    int          GetChannelsAmount(void) { return m_channels.size(); }
    int          GetCurrentClientChannel(void) { return m_iCurrentChannel; }
    PVR_ERROR    GetChannels(ADDON_HANDLE handle, bool bRadio);
    PVR_ERROR    GetChannelGroups(ADDON_HANDLE handle);
    PVR_ERROR    GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group);
    unsigned int GetChannelGroupsAmount(void) { return m_iNumChannelGroups; }

    /* EPG */
    PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd); /*!< @brief Backend Interface */

    /* Information */
    PVR_ERROR GetDriveSpace(long long *iTotal, long long *iUsed); /*!< @brief Backend Interface */
    PVR_ERROR SignalStatus(PVR_SIGNAL_STATUS &signalStatus);      /*!< @brief Backend Interface */

    /* Recordings */
    PVR_ERROR    DeleteRecording(const PVR_RECORDING &recinfo);
    PVR_ERROR    GetRecordings(ADDON_HANDLE handle);
    unsigned int GetRecordingsAmount() { return m_iNumRecordings; }

    /* Stream handling */
    bool        OpenLiveStream(const PVR_CHANNEL &channel);
    bool        SwitchChannel(const PVR_CHANNEL &channel);
    void        CloseLiveStream();
    const char* GetLiveStreamURL(const PVR_CHANNEL &channel) { return m_channels.at(channel.iUniqueId - 1).strStreamURL.c_str(); }

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
    /* Client creation and connection */
    bool        m_bIsConnected;            /*!< @brief Backend connection check */
    std::string m_strBackendBaseURLWeb;    /*!< @brief Backend base URL Web */
    std::string m_strBackendBaseURLStream; /*!< @brief Backend base URL Stream */
    std::string m_strEnigmaVersion;        /*!< @brief Backend Enigma2 version */
    std::string m_strImageVersion;         /*!< @brief Backend Image version */
    std::string m_strWebIfVersion;         /*!< @brief Backend web interface version */
    std::string m_strServerName;           /*!< @brief Backend name */

    unsigned int m_iTimersIndexCounter;    /*!< @brief Timers counter */
    unsigned int m_iUpdateIntervalTimer;   /*!< @brief Client update interval */

    /* Channels */
    int                             m_iCurrentChannel;   /*!< @brief Current channel uniqueID */
    int                             m_iNumChannelGroups; /*!< @brief Number of channel groups */
    std::vector<SE2STBChannel>      m_channels;          /*!< @brief Backend channels */
    std::vector<SE2STBChannelGroup> m_channelsGroups;    /*!< @brief Backend channel groups */

    /* Recordings */
    int                          m_iNumRecordings;
    std::vector<std::string>     m_recordingsLocations;
    std::vector<SE2STBRecording> m_recordings;

    /* Timers */
    std::vector<SE2STBTimer> m_timers; /*!< @brief Backend timers */

    PLATFORM::CCondition<bool> m_started;


    /********************************************//**
     * Functions
     ***********************************************/
    /* Client creation and connection */
    bool GetDeviceInfo(); /*!< @brief Backend Interface */
    bool SendCommandToSTB(const CStdString& strCommandURL, CStdString& strResult, bool bIgnoreResult = false); /*!< @brief Backend Interface */
    virtual void *Process(void);

    /* Channels */
    int         GetTotalChannelNumber(std::string strServiceReference);
    bool        LoadChannels(std::string strServerReference, std::string strGroupName); /*!< @brief Backend Interface */
    bool        LoadChannels();
    bool        LoadChannelGroups(); /*!< @brief Backend Interface */

    /* Recordings */
    bool        LoadRecordingLocations(); /*!< @brief Backend Interface */
    bool        IsInRecordingFolder(std::string);
    bool        GetRecordingFromLocation(std::string strRecordingFolder);
    void        TransferRecordings(ADDON_HANDLE handle);
    std::string GetChannelPiconPath(std::string strChannelName);

    /* Timers */
    void TimerUpdates();
    std::vector<SE2STBTimer> LoadTimers(); /*!< @brief Backend Interface */

    /* Lock */
    PLATFORM::CMutex m_mutex;    /*!< @brief CMutex class handler */

    /* Time shifting */
    CE2STBTimeshift *m_tsBuffer; /*!< @brief Time shifting class handler */

    /* Utils */
    CE2STBUtils *m_e2stbutils; /*!< @brief Utils */
};


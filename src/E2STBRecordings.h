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

#include "E2STBConnection.h"
#include "E2STBUtils.h"

#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

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

class CE2STBRecordings
{
  public:
    CE2STBRecordings(void);
    ~CE2STBRecordings(void);

    /* Recordings */
    PVR_ERROR    DeleteRecording(const PVR_RECORDING &recinfo);
    PVR_ERROR    GetRecordings(ADDON_HANDLE handle);
    unsigned int GetRecordingsAmount() { return m_iNumRecordings; }

  private:
    /* Recordings */
    int                          m_iNumRecordings;
    std::vector<std::string>     m_recordingsLocations;
    std::vector<SE2STBRecording> m_recordings;

    /* Recordings */
    bool        LoadRecordingLocations(); /*!< @brief Backend Interface */
    bool        IsInRecordingFolder(std::string);
    bool        GetRecordingFromLocation(std::string strRecordingFolder);
    void        TransferRecordings(ADDON_HANDLE handle);
    std::string GetChannelPiconPath(std::string strChannelName);

    CE2STBConnection m_e2stbconnection; /*!< @brief CE2STBConnection class handler */
    CE2STBUtils      m_e2stbutils;      /*!< @brief CE2STBUtils class handler */
};

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

#include "E2STBUtils.h"

#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class CE2STBConnection
{
  public:
    CE2STBConnection(void);
    ~CE2STBConnection();

    std::string m_strBackendBaseURLWeb;    /*!< @brief Backend base URL Web */
    std::string m_strBackendBaseURLStream; /*!< @brief Backend base URL Stream */

    /* Client creation and connection */
    bool        Open();
    bool        IsConnected();
    void        SendPowerstate();
    const char* GetServerName() { return m_strServerName.c_str(); }

    bool GetDeviceInfo(); /*!< @brief Backend Interface */
    bool SendCommandToSTB(const std::string& strCommandURL, std::string& strResult, bool bIgnoreResult = false); /*!< @brief Backend Interface */

  private:
    /********************************************//**
     * Members
     ***********************************************/
    /* Client creation and connection */
    bool        m_bIsConnected;            /*!< @brief Backend connection check */
    std::string m_strEnigmaVersion;        /*!< @brief Backend Enigma2 version */
    std::string m_strImageVersion;         /*!< @brief Backend Image version */
    std::string m_strWebIfVersion;         /*!< @brief Backend web interface version */
    std::string m_strServerName;           /*!< @brief Backend name */

    /* Lock */
    mutable std::mutex m_mutex;    /*!< @brief mutex class handler */

    /* Utils */
    CE2STBUtils m_e2stbutils; /*!< @brief Utils */
};

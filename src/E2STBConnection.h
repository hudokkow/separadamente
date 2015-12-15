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

#include "kodi/xbmc_pvr_types.h"

#include <mutex>
#include <string>

namespace e2stb
{
class CE2STBConnection
{
public:
  CE2STBConnection();
  ~CE2STBConnection();

  std::string m_strBackendBaseURLWeb;    /*!< @brief Backend base URL Web */
  std::string m_strBackendBaseURLStream; /*!< @brief Backend base URL Stream */
  std::string m_strWebIfVersion;         /*!< @brief Backend web interface version */
  std::string m_strServerName;           /*!< @brief Backend name */

  /*!
   * @brief Returns backend connection status
   */
  bool Open();
  /*!
   * @brief Signal backend to shutdown
   */
  void SendPowerstate();
  /*!
   * @brief Get backend device info
   */
  bool GetDeviceInfo();
  /*!
   * @brief Send command to backend STB
   */
  bool SendCommandToSTB(const std::string& strCommandURL, std::string& strResult, bool bIgnoreResult = false);
  /*!
   * @brief Safe encode URL
   */
  std::string URLEncode(const std::string& strURL);
  /*!
   * @brief Connect to backend
   */
  std::string ConnectToBackend(std::string& strURL);
  /*!
   * @brief Backend HDD information
   */
  PVR_ERROR GetDriveSpace(long long *iTotal, long long *iUsed);
  /*!
   * @brief Backend signal status
   */
  PVR_ERROR SignalStatus(PVR_SIGNAL_STATUS &signalStatus);

private:
  bool        m_bIsConnected;     /*!< @brief Backend connection check */
  std::string m_strEnigmaVersion; /*!< @brief Backend Enigma2 version */
  std::string m_strImageVersion;  /*!< @brief Backend Image version */

  mutable std::mutex m_mutex;     /*!< @brief mutex class handler */
};
} /* namespace e2stb */

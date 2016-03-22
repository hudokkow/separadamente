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

#include <string>

namespace e2stb
{
class CE2STBConnection
{
public:
  CE2STBConnection();
  ~CE2STBConnection();

  /*!
   * @brief Backend connection status
   * return True if backend is available, false otherwise
   */
  bool Initialize();
  /*!
   * @brief Initialize backend connection strings (web and stream)
   */
  void ConnectionStrings();
  /*!
   * @brief Get backend device info
   * return True if backend is available, false otherwise
   */
  bool GetDeviceInfo();
  /*!
   * @brief Backend connection status
   * return Backend name
   */
  std::string GetBackendName() const;
  /*!
   * @brief Backend connection status
   * return Backend Web API version
   */
  std::string GetBackendVersion() const;
  /*!
   * @brief Backend connection status
   * return Backend Web API URL
   */
  std::string GetBackendURLWeb() const;
  /*!
   * @brief Backend connection status
   * return Backend stream URL
   */
  std::string GetBackendURLStream() const;
  /*!
   * @brief Signal backend to shutdown if defined in GUI settings
   */
  void SendPowerstate();
  /*!
   * @brief Send command/info to backend
   */
  bool SendCommandToSTB(const std::string& strCommandURL, std::string& strResult, bool bIgnoreResult = false);
  /*!
   * @brief Safe encode URL
   * param[in] strCommnandURL URL string to safe encode
   * param[in] strResult String with result
   * param[in]  bIgnoreResult Ignore result
   * return True if successful, false otherwise
   */
  std::string URLEncode(const std::string& strURL);
  /*!
   * @brief Connect to backend
   * param[in] strURL URL string to connect to backend
   * return Safe encoded URL
   */
  std::string ConnectToBackend(const std::string& strPath);

private:
  std::string m_strBackendURLWeb;    /*!< @brief Backend base URL Web */
  std::string m_strBackendURLStream; /*!< @brief Backend base URL Stream */
  bool        m_bIsConnected;        /*!< @brief Backend connection check */
  std::string m_strEnigmaVersion;    /*!< @brief Backend Enigma2 version */
  std::string m_strImageVersion;     /*!< @brief Backend Image version */
  std::string m_strWebIfVersion;     /*!< @brief Backend web interface version */
  std::string m_strServerName;       /*!< @brief Backend name */
};
} /* namespace e2stb */

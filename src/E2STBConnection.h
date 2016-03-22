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

  /**
   * Initialize backend connection
   * @@return True if successful, false otherwise
   */
  bool Initialize();
  /**
   * Initialize backend connection strings (web and stream)
   */
  void ConnectionStrings();
  /**
   * Backend connection status
   * @@return True if connected, false otherwise
   */
  bool GetDeviceInfo();
  /**
   * Get backend name
   * @@return Backend name
   */
  std::string GetBackendName() const;
  /**
   * Get backend API version
   * @@return Backend API version
   */
  std::string GetBackendVersion() const;
  /**
   * Get backend web API URL
   * @return Backend web API URL
   */
  std::string GetBackendURLWeb() const;
  /**
   * Get backend stream API URL
   * @return Backend stream API URL
   */
  std::string GetBackendURLStream() const;
  /**
   * Signal backend to shutdown if defined in GUI settings
   */
  void SendPowerstate();
  /**
   * Send command/info to backend
   * @param strCommandURL
   * @param strResult
   * @param bIgnoreResult
   * @return
   */
  bool SendCommandToSTB(const std::string& strCommandURL, std::string& strResult, bool bIgnoreResult = false);
  /**
   * Safe encode URL
   * @param strURL URL string to safe encode
   * @return Safe encoded string
   */
  std::string URLEncode(const std::string& strURL);
  /**
   * Connect to backend
   * @param[in] strURL URL string to connect to backend
   * @return Safe encoded URL
   */
  /**
   * Connect to backend
   * @param strPath Path on backend
   * @return Parsed data on backend path
   */
  std::string GetBackendData(const std::string& strPath);

private:
  std::string m_strBackendURLWeb;    /**< Backend base URL Web */
  std::string m_strBackendURLStream; /**< Backend base URL Stream */
  bool        m_bIsConnected;        /**< Backend connection check */
  std::string m_strEnigmaVersion;    /**< Backend Enigma2 version */
  std::string m_strImageVersion;     /**< Backend Image version */
  std::string m_strWebIfVersion;     /**< Backend web API version */
  std::string m_strServerName;       /**< Backend name */
};
} /* namespace e2stb */

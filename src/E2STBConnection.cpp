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

#include "client.h"
#include "compat.h"
#include "E2STBXMLUtils.h"

#include "kodi/xbmc_pvr_types.h"
#include "p8-platform/util/StringUtils.h" /* ToUpper for GetDeviceInfo() */

#include "tinyxml.h"
#include <iomanip>    /* std::setw for URLEncode() */
#include <string>
#include <sstream>    /* std::ostringstream for URLEncode() */

using namespace e2stb;

CE2STBConnection::CE2STBConnection()
: m_strBackendURLWeb{}
, m_strBackendURLStream{}
, m_bIsConnected{false}
, m_strEnigmaVersion{}
, m_strImageVersion{}
, m_strWebIfVersion{}
, m_strServerName{"Enigma2 STB"}
{
  ConnectionStrings();
  XBMC->Log(ADDON::LOG_DEBUG, "[%s] hudosky CE2STBConnection ctor", __FUNCTION__);
}

CE2STBConnection::~CE2STBConnection()
{
  XBMC->Log(ADDON::LOG_DEBUG, "[%s] hudosky CE2STBConnection dtor", __FUNCTION__);
  m_bIsConnected = false;
}

bool CE2STBConnection::Initialize()
{
  m_bIsConnected = GetDeviceInfo();

  if (!m_bIsConnected)
  {
    XBMC->Log(ADDON::LOG_ERROR, "[%s] Web interface can't be reached. Wrong connection settings?",  __FUNCTION__);
    return false;
  }
  return true;
}

void CE2STBConnection::ConnectionStrings()
{
  std::string strURLAuth;

  if (g_bUseAuthentication && !g_strUsername.empty() && !g_strPassword.empty())
    strURLAuth = g_strUsername + ":" + g_strPassword + "@";

  if (!g_bUseSecureHTTP)
  {
    m_strBackendURLWeb = "http://" + strURLAuth + g_strHostname + ":" + compat::to_string(g_iPortWebHTTP) + "/";
    m_strBackendURLStream = "http://" + strURLAuth + g_strHostname + ":" + compat::to_string(g_iPortStream) + "/";
  }
  else
  {
    m_strBackendURLWeb = "https://" + strURLAuth + g_strHostname + ":" + compat::to_string(g_iPortWebHTTPS) + "/";
    m_strBackendURLStream = "https://" + strURLAuth + g_strHostname + ":" + compat::to_string(g_iPortStream) + "/";
  }
}

bool CE2STBConnection::GetDeviceInfo()
{
  std::string strURL = m_strBackendURLWeb + "web/deviceinfo";
  std::string strXML = ConnectToBackend(strURL);

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

  pElement = hDoc.FirstChildElement("e2deviceinfo").Element();

  if (!pElement)
  {
    XBMC->Log(ADDON::LOG_ERROR, "[%s] Couldn't find <e2deviceinfo> element", __FUNCTION__);
    return false;
  }

  std::string strTemp;

  if (!XMLUtils::GetString(pElement, "e2enigmaversion", strTemp))
  {
    XBMC->Log(ADDON::LOG_ERROR, "[%s] Couldn't parse <e2enigmaversion> from result", __FUNCTION__);
    return false;
  }
  m_strEnigmaVersion = strTemp;
  XBMC->Log(ADDON::LOG_DEBUG, "[%s] Enigma2 version is %s", __FUNCTION__, m_strEnigmaVersion.c_str());

  if (!XMLUtils::GetString(pElement, "e2imageversion", strTemp))
  {
    XBMC->Log(ADDON::LOG_ERROR, "[%s] Couldn't parse <e2imageversion> from result", __FUNCTION__);
    return false;
  }
  m_strImageVersion = strTemp;
  XBMC->Log(ADDON::LOG_DEBUG, "[%s] Enigma2 image version is %s", __FUNCTION__, m_strImageVersion.c_str());

  if (!XMLUtils::GetString(pElement, "e2webifversion", strTemp))
  {
    XBMC->Log(ADDON::LOG_ERROR, "[%s] Couldn't parse <e2webifversion> from result", __FUNCTION__);
    return false;
  }
  m_strWebIfVersion = strTemp;
  XBMC->Log(ADDON::LOG_DEBUG, "[%s] Enigma2 web interface version is %s", __FUNCTION__, m_strWebIfVersion.c_str());

  if (!XMLUtils::GetString(pElement, "e2devicename", strTemp))
  {
    XBMC->Log(ADDON::LOG_ERROR, "[%s] Couldn't parse <e2devicename> from result", __FUNCTION__);
    return false;
  }
  StringUtils::ToUpper(strTemp);
  m_strServerName += " " + strTemp;
  XBMC->Log(ADDON::LOG_DEBUG, "[%s] Enigma2 device name is %s", __FUNCTION__, m_strServerName.c_str());
  return true;
}

std::string CE2STBConnection::GetBackendName() const
{
  return m_strServerName;
}

std::string CE2STBConnection::GetBackendVersion() const
{
  return m_strWebIfVersion;
}

std::string CE2STBConnection::GetBackendURLWeb() const
{
  return m_strBackendURLWeb;
}

std::string CE2STBConnection::GetBackendURLStream() const
{
  return m_strBackendURLStream;
}

void CE2STBConnection::SendPowerstate()
{
  if (!g_bSendDeepStanbyToSTB)
    return;
  /* TODO: Review power states functionality - http://dream.reichholf.net/wiki/Enigma2:WebInterface
    0 = Toogle Standby
    1 = Deepstandby
    2 = Reboot
    3 = Restart Enigma2
    4 = Wakeup form Standby
    5 = Standby
   */
  std::string strTemp = "web/powerstate?newstate=1";

  std::string strResult;
  SendCommandToSTB(strTemp, strResult, true);
}

bool CE2STBConnection::SendCommandToSTB(const std::string& strCommandURL, std::string& strResultText, bool bIgnoreResult)
{
  std::string strURL = m_strBackendURLWeb + std::string(strCommandURL);
  std::string strXML = ConnectToBackend(strURL);

  if (!bIgnoreResult)
  {
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

    pElement = hDoc.FirstChildElement("e2simplexmlresult").Element();
    if (!pElement)
    {
      XBMC->Log(ADDON::LOG_DEBUG, "[%s] Couldn't find <e2simplexmlresult> element", __FUNCTION__);
      return false;
    }

    bool bTmp;
    if (!XMLUtils::GetBoolean(pElement, "e2state", bTmp))
    {
      XBMC->Log(ADDON::LOG_ERROR, "[%s] Couldn't parse <e2state> from result", __FUNCTION__);
      strResultText = "Could not parse e2state!";
      return false;
    }

    if (!XMLUtils::GetString(pElement, "e2statetext", strResultText))
    {
      XBMC->Log(ADDON::LOG_ERROR, "[%s] Could not parse <e2state> from result!", __FUNCTION__);
      return false;
    }

    if (!bTmp)
      XBMC->Log(ADDON::LOG_ERROR, "[%s] Backend sent error message %s", __FUNCTION__, strResultText.c_str());

    return bTmp;
  }
  return true;
}

/* Adapted from http://stackoverflow.com/a/17708801 / stolen from pvr.vbox. Thanks Jalle19 */
std::string CE2STBConnection::URLEncode(const std::string& strURL)
{
  std::ostringstream escaped;
  escaped.fill('0');
  escaped << std::hex;

  for (auto i = strURL.cbegin(), n = strURL.cend(); i != n; ++i)
  {
    std::string::value_type c = (*i);

    /* Keep alphanumeric and other accepted characters intact */
    if (c < 0 || isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
    {
      escaped << c;
      continue;
    }

    /* Any other characters are percent-encoded */
    escaped << '%' << std::setw(2) << int(static_cast<unsigned char>(c));
  }
  return escaped.str();
}

std::string CE2STBConnection::ConnectToBackend(std::string& strURL)
{
  std::string strResult;
  void* fileHandle = XBMC->OpenFile(strURL.c_str(), 0);
  if (fileHandle)
  {
    char buffer[1024];
    while (XBMC->ReadFileString(fileHandle, buffer, 1024))
    {
      strResult.append(buffer);
    }
    XBMC->CloseFile(fileHandle);
    XBMC->Log(ADDON::LOG_DEBUG, "[%s] Got result with length %u", __FUNCTION__, strResult.length());
  }
  else
    XBMC->Log(ADDON::LOG_DEBUG, "[%s] Couldn't open web interface.", __FUNCTION__);

  return strResult;
}

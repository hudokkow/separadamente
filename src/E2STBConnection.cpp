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

#include "tinyxml.h"
#include "E2STBXMLUtils.h"

#include <algorithm>  /* std::transform for GetDeviceInfo() */
#include <cctype>     /* std::toupper for GetDeviceInfo() */
#include <mutex>
#include <string>

using namespace e2stb;

CE2STBConnection::CE2STBConnection()
: m_strBackendBaseURLWeb{}
, m_strBackendBaseURLStream{}
, m_bIsConnected{false}
, m_strEnigmaVersion{}
, m_strImageVersion{}
, m_strWebIfVersion{}
, m_strServerName{"Enigma2 STB"}
{
  std::string strURLAuthentication;

  if (g_bUseAuthentication && !g_strUsername.empty() && !g_strPassword.empty())
  {
    strURLAuthentication = g_strUsername + ":" + g_strPassword + "@";
  }

  if (!g_bUseSecureHTTP)
  {
    m_strBackendBaseURLWeb = "http://" + strURLAuthentication + g_strHostname + ":"
        + m_e2stbutils.IntToString(g_iPortWebHTTP) + "/";
    m_strBackendBaseURLStream = "http://" + strURLAuthentication + g_strHostname + ":"
        + m_e2stbutils.IntToString(g_iPortStream) + "/";
  }
  else
  {
    m_strBackendBaseURLWeb = "https://" + strURLAuthentication + g_strHostname + ":"
        + m_e2stbutils.IntToString(g_iPortWebHTTPS) + "/";
    m_strBackendBaseURLStream = "https://" + strURLAuthentication + g_strHostname + ":"
        + m_e2stbutils.IntToString(g_iPortStream) + "/";
  }
}

CE2STBConnection::~CE2STBConnection()
{
  m_bIsConnected = false;
}

bool CE2STBConnection::Open()
{
  std::unique_lock<std::mutex> lock(m_mutex);
  m_bIsConnected = GetDeviceInfo();

  if (!m_bIsConnected)
  {
    XBMC->Log(ADDON::LOG_ERROR, "[%s] Web interface can't be reached. Make sure connection options are correct",
        __FUNCTION__);
    return false;
  }
  return true;
}

bool CE2STBConnection::IsConnected()
{
  return m_bIsConnected;
}

void CE2STBConnection::SendPowerstate()
{
  if (!g_bSendDeepStanbyToSTB)
  {
    return;
  }
  std::unique_lock<std::mutex> lock(m_mutex);

  /* TODO: Review power states functionality
  http://wiki.dbox2-tuning.net/wiki/Enigma2:WebInterface
   */
  std::string strTemp = "web/powerstate?newstate=1";

  std::string strResult;
  SendCommandToSTB(strTemp, strResult, true);
}

bool CE2STBConnection::GetDeviceInfo()
{
  std::string strURL = m_strBackendBaseURLWeb + "web/deviceinfo";
  std::string strXML = m_e2stbutils.ConnectToBackend(strURL);

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
  XBMC->Log(ADDON::LOG_NOTICE, "[%s] Enigma2 version is %s", __FUNCTION__, m_strEnigmaVersion.c_str());

  if (!XMLUtils::GetString(pElement, "e2imageversion", strTemp))
  {
    XBMC->Log(ADDON::LOG_ERROR, "[%s] Couldn't parse <e2imageversion> from result", __FUNCTION__);
    return false;
  }
  m_strImageVersion = strTemp;
  XBMC->Log(ADDON::LOG_NOTICE, "[%s] Enigma2 image version is %s", __FUNCTION__, m_strImageVersion.c_str());

  if (!XMLUtils::GetString(pElement, "e2webifversion", strTemp))
  {
    XBMC->Log(ADDON::LOG_ERROR, "[%s] Couldn't parse <e2webifversion> from result", __FUNCTION__);
    return false;
  }
  m_strWebIfVersion = strTemp;
  XBMC->Log(ADDON::LOG_NOTICE, "[%s] Enigma2 web interface version is %s", __FUNCTION__, m_strWebIfVersion.c_str());

  if (!XMLUtils::GetString(pElement, "e2devicename", strTemp))
  {
    XBMC->Log(ADDON::LOG_ERROR, "[%s] Couldn't parse <e2devicename> from result", __FUNCTION__);
    return false;
  }
  /* http://stackoverflow.com/questions/7131858/stdtransform-and-toupper-no-matching-function */
  std::transform(strTemp.begin(), strTemp.end(), strTemp.begin(), (int (*)(int))std::toupper);
  m_strServerName += " " + strTemp;
  XBMC->Log(ADDON::LOG_NOTICE, "[%s] Enigma2 device name is %s", __FUNCTION__, m_strServerName.c_str());
  return true;
}

bool CE2STBConnection::SendCommandToSTB(const std::string& strCommandURL, std::string& strResultText, bool bIgnoreResult)
{
  /* std::string is needed to quell warning */
  /* ISO C++ says that these are ambiguous, even though the worst conversion */
  /* for the first is better than the worst conversion for the second */
  std::string strURL = m_strBackendBaseURLWeb + std::string(strCommandURL);
  std::string strXML = m_e2stbutils.ConnectToBackend(strURL);

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
    {
      XBMC->Log(ADDON::LOG_ERROR, "[%s] Backend sent error message %s", __FUNCTION__, strResultText.c_str());
    }
    return bTmp;
  }
  return true;
}

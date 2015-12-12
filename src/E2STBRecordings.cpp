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

#include "E2STBRecordings.h"

#include "client.h"

#include "tinyxml.h"
#include "E2STBXMLUtils.h"

#include <string>
#include <vector>

using namespace e2stb;

CE2STBRecordings::CE2STBRecordings()
: m_iNumRecordings{0}
{
}

CE2STBRecordings::~CE2STBRecordings()
{
  XBMC->Log(ADDON::LOG_DEBUG, "[%s] Removing internal recordings list", __FUNCTION__);
  m_recordings.clear();
}

bool CE2STBRecordings::Open()
{
  if (!LoadRecordingLocations())
    return false;

  return true;
}

PVR_ERROR CE2STBRecordings::GetRecordings(ADDON_HANDLE handle)
{
  m_iNumRecordings = 0;
  m_recordings.clear();

  for (unsigned int i = 0; i < m_recordingsLocations.size(); i++)
  {
    if (!GetRecordingFromLocation(m_recordingsLocations[i]))
    {
      XBMC->Log(ADDON::LOG_ERROR, "[%s] Error fetching recordings list from folder %s", __FUNCTION__,
          m_recordingsLocations[i].c_str());
    }
  }
  TransferRecordings(handle);
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR CE2STBRecordings::DeleteRecording(const PVR_RECORDING &recinfo)
{
  std::string strTemp = "web/moviedelete?sRef=" + m_e2stbconnection.URLEncode(recinfo.strRecordingId);

  std::string strResult;
  if (!m_e2stbconnection.SendCommandToSTB(strTemp, strResult))
  {
    return PVR_ERROR_FAILED;
  }
  PVR->TriggerRecordingUpdate();
  return PVR_ERROR_NO_ERROR;
}

bool CE2STBRecordings::LoadRecordingLocations()
{
  std::string strURL;
  if (g_bUseOnlyCurrentRecordingPath)
  {
    strURL = m_e2stbconnection.m_strBackendBaseURLWeb + "web/getcurrlocation";
  }
  else
  {
    strURL = m_e2stbconnection.m_strBackendBaseURLWeb + "web/getlocations";
  }

  std::string strXML = m_e2stbconnection.ConnectToBackend(strURL);

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

  pElement = hDoc.FirstChildElement("e2locations").Element();

  if (!pElement)
  {
    XBMC->Log(ADDON::LOG_DEBUG, "[%s] Couldn't find <e2locations> element", __FUNCTION__);
    return false;
  }

  hRoot = TiXmlHandle(pElement);

  TiXmlElement* pNode = hRoot.FirstChildElement("e2location").Element();

  if (!pNode)
  {
    XBMC->Log(ADDON::LOG_DEBUG, "[%s] Couldn't find <e2location> element", __FUNCTION__);
    return false;
  }

  int iNumLocations = 0;

  for (; pNode != NULL; pNode = pNode->NextSiblingElement("e2location"))
  {
    std::string strTemp = pNode->GetText();
    m_recordingsLocations.push_back(strTemp);
    iNumLocations++;
    XBMC->Log(ADDON::LOG_NOTICE, "[%s] Added %s as a recording location", __FUNCTION__, strTemp.c_str());
  }
  XBMC->Log(ADDON::LOG_NOTICE, "[%s] Loaded %d recording locations", __FUNCTION__, iNumLocations);
  return true;
}

bool CE2STBRecordings::IsInRecordingFolder(std::string strRecordingFolder)
{
  int iMatches = 0;
  for (unsigned int i = 0; i < m_recordings.size(); i++)
  {
    if (strRecordingFolder.compare(m_recordings.at(i).strTitle) == 0)
    {
      iMatches++;
      XBMC->Log(ADDON::LOG_DEBUG, "[%s] Found recording title %s in recordings vector", __FUNCTION__,
          strRecordingFolder.c_str());
      if (iMatches > 1)
      {
        XBMC->Log(ADDON::LOG_DEBUG, "[%s] Found recording title %s more than once in recordings vector", __FUNCTION__,
            strRecordingFolder.c_str());
        return true;
      }
    }
  }
  return false;
}

bool CE2STBRecordings::GetRecordingFromLocation(std::string strRecordingFolder)
{
  std::string strURL;
  if (!strRecordingFolder.compare("default"))
  {
    strURL = m_e2stbconnection.m_strBackendBaseURLWeb + "web/movielist";
  }
  else
  {
    strURL = m_e2stbconnection.m_strBackendBaseURLWeb + "web/movielist" + "?dirname=" + m_e2stbconnection.URLEncode(strRecordingFolder);
  }

  std::string strXML = m_e2stbconnection.ConnectToBackend(strURL);

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

  pElement = hDoc.FirstChildElement("e2movielist").Element();

  if (!pElement)
  {
    XBMC->Log(ADDON::LOG_DEBUG, "[%s] Couldn't find <e2movielist> element", __FUNCTION__);
    return false;
  }

  hRoot = TiXmlHandle(pElement);

  TiXmlElement* pNode = hRoot.FirstChildElement("e2movie").Element();

  if (!pNode)
  {
    XBMC->Log(ADDON::LOG_DEBUG, "[%s] Couldn't find <e2movie> element", __FUNCTION__);
    return false;
  }

  int iNumRecording = 0;

  for (; pNode != NULL; pNode = pNode->NextSiblingElement("e2movie"))
  {
    std::string strTemp;
    int iTmp;

    SE2STBRecording recording;

    recording.iLastPlayedPosition = 0;
    if (XMLUtils::GetString(pNode, "e2servicereference", strTemp))
    {
      recording.strRecordingId = strTemp;
    }

    if (XMLUtils::GetString(pNode, "e2title", strTemp))
    {
      recording.strTitle = strTemp;
    }

    if (XMLUtils::GetString(pNode, "e2description", strTemp))
    {
      recording.strPlotOutline = strTemp;
    }

    if (XMLUtils::GetString(pNode, "e2descriptionextended", strTemp))
    {
      recording.strPlot = strTemp;
    }

    if (XMLUtils::GetString(pNode, "e2servicename", strTemp))
    {
      recording.strChannelName = strTemp;
    }

    recording.strIconPath = GetChannelPiconPath(strTemp);

    if (XMLUtils::GetInt(pNode, "e2time", iTmp))
    {
      recording.startTime = iTmp;
    }

    if (XMLUtils::GetString(pNode, "e2length", strTemp))
    {
      recording.iDuration = m_e2stbutils.TimeStringToSeconds(strTemp);
    }
    else
    {
      recording.iDuration = 0;
    }

    if (XMLUtils::GetString(pNode, "e2filename", strTemp))
    {
      recording.strStreamURL = m_e2stbconnection.m_strBackendBaseURLWeb + "file?file=" + m_e2stbconnection.URLEncode(strTemp);
    }
    m_iNumRecordings++;
    iNumRecording++;
    m_recordings.push_back(recording);
    XBMC->Log(ADDON::LOG_DEBUG, "[%s] Loaded recording %s starting at %d with length %d", __FUNCTION__,
        recording.strTitle.c_str(), recording.startTime, recording.iDuration);
  }
  XBMC->Log(ADDON::LOG_NOTICE, "[%s] Loaded %u recording entries from folder %s", __FUNCTION__, iNumRecording,
      strRecordingFolder.c_str());
  return true;
}

void CE2STBRecordings::TransferRecordings(ADDON_HANDLE handle)
{
  for (unsigned int i = 0; i < m_recordings.size(); i++)
  {
    SE2STBRecording &recording = m_recordings.at(i);
    PVR_RECORDING recordings;
    memset(&recordings, 0, sizeof(PVR_RECORDING));
    strncpy(recordings.strRecordingId, recording.strRecordingId.c_str(), sizeof(recordings.strRecordingId) - 1);
    strncpy(recordings.strTitle, recording.strTitle.c_str(), sizeof(recordings.strTitle) - 1);
    strncpy(recordings.strStreamURL, recording.strStreamURL.c_str(), sizeof(recordings.strStreamURL) - 1);
    strncpy(recordings.strPlotOutline, recording.strPlotOutline.c_str(), sizeof(recordings.strPlotOutline) - 1);
    strncpy(recordings.strPlot, recording.strPlot.c_str(), sizeof(recordings.strPlot) - 1);
    strncpy(recordings.strChannelName, recording.strChannelName.c_str(), sizeof(recordings.strChannelName) - 1);
    strncpy(recordings.strIconPath, recording.strIconPath.c_str(), sizeof(recordings.strIconPath) - 1);

    std::string strTemp;

    if (IsInRecordingFolder(recording.strTitle))
    {
      strTemp = "/" + recording.strTitle + "/";
    }
    else
    {
      strTemp = "/";
    }
    recording.strDirectory = strTemp;
    strncpy(recordings.strDirectory, recording.strDirectory.c_str(), sizeof(recordings.strDirectory) - 1);
    recordings.recordingTime = recording.startTime;
    recordings.iDuration = recording.iDuration;
    PVR->TransferRecordingEntry(handle, &recordings);
  }
}

std::string CE2STBRecordings::GetChannelPiconPath(std::string strChannelName)
{
  for (unsigned int i = 0; i < m_e2stbchannels.m_channels.size(); i++)
  {
    if (!strChannelName.compare(m_e2stbchannels.m_channels[i].strChannelName))
    {
      return m_e2stbchannels.m_channels[i].strIconPath;
    }
  }
  return "";
}

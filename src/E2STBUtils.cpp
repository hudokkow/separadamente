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

#include "E2STBUtils.h"

#include "client.h"

#include "platform/util/StdString.h"

#include <iterator>
#include <iomanip>
#include <string>
#include <sstream>
#include <vector>

// some implementations don't support std::to_string... I'm looking at you Android
// taken from http://stackoverflow.com/questions/5590381/easiest-way-to-convert-int-to-string-in-c
std::string CE2STBUtils::IntToString (int a)
{
    std::ostringstream temp;
    temp << a;
    return temp.str();
}

/********************************************//**
 * Convert time string to seconds
 ***********************************************/
long CE2STBUtils::TimeStringToSeconds(const CStdString &timeString)
{
  std::vector<CStdString> secs;
  SplitString(timeString, ":", secs);
  int timeInSecs = 0;
  for (unsigned int i = 0; i < secs.size(); i++)
  {
    timeInSecs *= 60;
    timeInSecs += std::stoi(secs[i]);
  }
  return timeInSecs;
}

/********************************************//**
 * Safe encode URL
 ***********************************************/
// Adapted from http://stackoverflow.com/a/17708801 / stolen from pvr.vbox. Thanks Jalle19
std::string CE2STBUtils::URLEncode(const std::string& strURL)
{
  std::ostringstream escaped;
  escaped.fill('0');
  escaped << std::hex;

  for (auto i = strURL.cbegin(), n = strURL.cend(); i != n; ++i)
  {
    std::string::value_type c = (*i);

    // Keep alphanumeric and other accepted characters intact
    if (c < 0 || isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
    {
      escaped << c;
      continue;
    }

    // Any other characters are percent-encoded
    escaped << '%' << std::setw(2) << int(static_cast<unsigned char>(c));
  }

  return escaped.str();
}

/********************************************//**
 * Generate URL
 ***********************************************/
std::string CE2STBUtils::BackendConnection(std::string& url)
{
  XBMC->Log(ADDON::LOG_NOTICE, "[%s] Opening web interface with URL %s", __FUNCTION__, url.c_str());

  std::string strTemp;
  if (!GetXMLFromHTTP(url, strTemp))
  {
    XBMC->Log(ADDON::LOG_DEBUG, "[%s] Couldn't open web interface.", __FUNCTION__);
    return "";
  }
  XBMC->Log(ADDON::LOG_DEBUG, "[%s] Got result with length %u", __FUNCTION__, strTemp.length());
  return strTemp;
}

/********************************************//**
 * Split string using delimiter
 ***********************************************/
int CE2STBUtils::SplitString(const CStdString& input, const CStdString& delimiter, std::vector<CStdString>& results,
    unsigned int iMaxStrings)
{
  int iPos = -1;
  int newPos = -1;
  int sizeS2 = delimiter.GetLength();
  int isize = input.GetLength();

  results.clear();
  std::vector<unsigned int> positions;

  newPos = input.Find(delimiter, 0);

  if (newPos < 0)
  {
    results.push_back(input);
    return 1;
  }

  while (newPos > iPos)
  {
    positions.push_back(newPos);
    iPos = newPos;
    newPos = input.Find(delimiter, iPos + sizeS2);
  }

  // numFound equals number of delimiters, one less than the number of substrings
  unsigned int numFound = positions.size();
  if (iMaxStrings > 0 && numFound >= iMaxStrings)
  {
    numFound = iMaxStrings - 1;
  }

  for (unsigned int i = 0; i <= numFound; i++)
  {
    std::string s;
    if (i == 0)
    {
      if (i == numFound)
      {
        s = input;
      }
      else
      {
        s = input.Mid(i, positions[i]);
      }
    }
    else
    {
      int offset = positions[i - 1] + sizeS2;
      if (offset < isize)
      {
        if (i == numFound)
        {
          s = input.Mid(offset);
        }
        else if (i > 0)
        {
          s = input.Mid(positions[i - 1] + sizeS2,
              positions[i] - positions[i - 1] - sizeS2);
        }
      }
    }
    results.push_back(s);
  }
  return results.size();
}

/********************************************//**
 * Read backend data
 ***********************************************/
bool CE2STBUtils::GetXMLFromHTTP(const std::string &strURL, std::string &strResult)
{
  void* fileHandle = XBMC->OpenFile(strURL.c_str(), 0);
  if (fileHandle)
  {
    char buffer[1024];
    while (XBMC->ReadFileString(fileHandle, buffer, 1024))
    {
      strResult.append(buffer);
    }
    XBMC->CloseFile(fileHandle);
    return true;
  }
  return false;
}

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

#include <iterator>
#include <iomanip>
#include <string>
#include <sstream>
#include <vector>

using namespace e2stb;

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
long CE2STBUtils::TimeStringToSeconds(const std::string &timeString)
{
  std::vector<std::string> secs;
  TokenizeString(timeString, ":", secs);
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

std::string CE2STBUtils::ConnectToBackend(std::string& strURL)
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
  {
    XBMC->Log(ADDON::LOG_DEBUG, "[%s] Couldn't open web interface.", __FUNCTION__);
  }
  return strResult;
}

// adapted from http://stackoverflow.com/questions/53849/how-do-i-tokenize-a-string-in-c
int CE2STBUtils::TokenizeString(const std::string& str, const std::string& delimiter, std::vector<std::string>& results)
{
  std::string::size_type start_pos = 0;
  std::string::size_type delim_pos = 0;

  while (std::string::npos != delim_pos)
  {
    delim_pos = str.find_first_of(delimiter, start_pos);
    results.push_back(str.substr(start_pos, delim_pos - start_pos));
    start_pos = delim_pos + 1;
  }
  return results.size();
}

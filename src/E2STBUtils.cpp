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
std::string CE2STBUtils::URLEncodeInline(const std::string& strURL)
{
  const char SAFE[256] =
  {
  /*     0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
  /* 0 */0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  /* 1 */0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  /* 2 */0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  /* 3 */1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,

  /* 4 */0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  /* 5 */1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
  /* 6 */0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  /* 7 */1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,

  /* 8 */0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  /* 9 */0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  /* A */0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  /* B */0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

  /* C */0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  /* D */0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  /* E */0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  /* F */0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

  const char DEC2HEX[16 + 1] = "0123456789ABCDEF";
  const unsigned char * pSrc = (const unsigned char *) strURL.c_str();
  const int SRC_LEN = strURL.length();
  unsigned char * const pStart = new unsigned char[SRC_LEN * 3];
  unsigned char * pEnd = pStart;
  const unsigned char * const SRC_END = pSrc + SRC_LEN;

  for (; pSrc < SRC_END; ++pSrc)
  {
    if (SAFE[*pSrc])
    {
      *pEnd++ = *pSrc;
    }
    else
    {
      // escape this char
      *pEnd++ = '%';
      *pEnd++ = DEC2HEX[*pSrc >> 4];
      *pEnd++ = DEC2HEX[*pSrc & 0x0F];
    }
  }

  std::string sResult((char *) pStart, (char *) pEnd);
  delete[] pStart;
  return sResult;
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

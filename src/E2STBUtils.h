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

#include "platform/util/StdString.h"

#include <string>
#include <vector>

class CE2STBUtils
{
  public:
    CE2STBUtils(void) {};
    ~CE2STBUtils(void) {};

    std::string IntToString (int a);
    static long TimeStringToSeconds(const CStdString& timeString);
    std::string URLEncode(const std::string& strURL);
    std::string ConnectToBackend(std::string& strURL);

  private:
    static int SplitString(const CStdString& input, const CStdString& delimiter, std::vector<CStdString>& results,
        unsigned int iMaxStrings = 0);
};


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
#include <string>
#include <sstream>
#include <vector>

using namespace e2stb;

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

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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "E2STBXMLUtils.h"
#include "platform/util/StringUtils.h"
#include "tinyxml2/tinyxml2.h"

#include <cstdlib>
#include <string>

bool XMLUtils::GetInt(const tinyxml2::XMLNode* pRootNode, const char* strTag, int& iIntValue)
{
  const tinyxml2::XMLNode* pNode = pRootNode->FirstChildElement(strTag);
  if (!pNode || !pNode->FirstChild())
    return false;
  iIntValue = atoi(pNode->FirstChild()->Value());
  return true;
}

bool XMLUtils::GetBoolean(const tinyxml2::XMLNode* pRootNode, const char* strTag, bool& bBoolValue)
{
  const tinyxml2::XMLNode* pNode = pRootNode->FirstChildElement(strTag);
  if (!pNode || !pNode->FirstChild())
    return false;
  std::string strEnabled = pNode->FirstChild();
  StringUtils::ToLower(strEnabled);
  if (strEnabled == "off" || strEnabled == "no" || strEnabled == "disabled" || strEnabled == "false"
      || strEnabled == "0")
    bBoolValue = false;
  else
  {
    bBoolValue = true;
    if (strEnabled != "on" && strEnabled != "yes" && strEnabled != "enabled" && strEnabled != "true")
      return false;  // invalid bool switch - it's probably some other string.
  }
  return true;
}

bool XMLUtils::GetString(const tinyxml2::XMLNode* pRootNode, const char* strTag, std::string& strStringValue)
{
  const tinyxml2::XMLElement* pElement = pRootNode->FirstChildElement(strTag);
  if (!pElement)
    return false;
  const tinyxml2::XMLNode* pNode = pElement->FirstChild();
  if (pNode != NULL)
  {
    strStringValue = pNode;
    return true;
  }
  strStringValue.clear();
  return true;
}

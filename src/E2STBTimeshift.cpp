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

#include "E2STBTimeshift.h"

#include "client.h"

#include "p8-platform/threads/threads.h"
#include "p8-platform/util/StdString.h"

#include <ctime>
#include <cstdint>

using namespace e2stb;

CE2STBTimeshift::CE2STBTimeshift(CStdString streamURL, CStdString bufferPath)
  : m_bufferPath(bufferPath)
{
  m_streamHandle = XBMC->OpenFile(streamURL, READ_NO_CACHE);
  m_bufferPath += "/tsbuffer.ts";
  m_filebufferWriteHandle = XBMC->OpenFileForWrite(m_bufferPath, true);
#ifndef TARGET_POSIX
  m_writePos = 0;
#endif
  Sleep(100);
  m_filebufferReadHandle = XBMC->OpenFile(m_bufferPath, READ_NO_CACHE);
  m_start = time(NULL);
  CreateThread();
}

CE2STBTimeshift::~CE2STBTimeshift(void)
{
  Stop();
  if (IsRunning())
    StopThread();

  if (m_filebufferWriteHandle)
    XBMC->CloseFile(m_filebufferWriteHandle);
  if (m_filebufferReadHandle)
    XBMC->CloseFile(m_filebufferReadHandle);
  if (m_streamHandle)
    XBMC->CloseFile(m_streamHandle);
}

bool CE2STBTimeshift::IsValid()
{
  return (m_streamHandle != NULL
      && m_filebufferWriteHandle != NULL
      && m_filebufferReadHandle != NULL);
}

void CE2STBTimeshift::Stop()
{
  m_start = 0;
}

void *CE2STBTimeshift::Process()
{
  XBMC->Log(ADDON::LOG_DEBUG, "Timeshift: Thread started");
  byte buffer[STREAM_READ_BUFFER_SIZE];

  while (m_start)
  {
    unsigned int read = XBMC->ReadFile(m_streamHandle, buffer, sizeof(buffer));
    XBMC->WriteFile(m_filebufferWriteHandle, buffer, read);

#ifndef TARGET_POSIX
    m_mutex.Lock();
    m_writePos += read;
    m_mutex.Unlock();
#endif
  }
  XBMC->Log(ADDON::LOG_DEBUG, "Timeshift: Thread stopped");
  return NULL;
}

long long CE2STBTimeshift::Seek(long long position, int whence)
{
  return XBMC->SeekFile(m_filebufferReadHandle, position, whence);
}

long long CE2STBTimeshift::Position()
{
  return XBMC->GetFilePosition(m_filebufferReadHandle);
}

long long CE2STBTimeshift::Length()
{
  // We can't use GetFileLength here as it's value will be cached
  // by XBMC until we read or seek above it.
  // see xbmc/xbmc/filesystem/HDFile.cpp CHDFile::GetLength()
  //return XBMC->GetFileLength(m_filebufferReadHandle);

  int64_t writePos = 0;
#ifdef TARGET_POSIX
  /* refresh write position */
  XBMC->SeekFile(m_filebufferWriteHandle, 0L, SEEK_CUR);
  writePos = XBMC->GetFilePosition(m_filebufferWriteHandle);
#else
  m_mutex.Lock();
  writePos = m_writePos;
  m_mutex.Unlock();
#endif
  return writePos;
}

int CE2STBTimeshift::ReadData(unsigned char *buffer, unsigned int size)
{
  /* make sure we never read above the current write position */
  int64_t readPos = XBMC->GetFilePosition(m_filebufferReadHandle);
  unsigned int timeWaited = 0;
  while (readPos + size > Length())
  {
    if (timeWaited > BUFFER_READ_TIMEOUT)
    {
      XBMC->Log(ADDON::LOG_DEBUG, "Timeshift: Read timed out; waited %u", timeWaited);
      return -1;
    }
    Sleep(BUFFER_READ_WAITTIME);
    timeWaited += BUFFER_READ_WAITTIME;
  }

  return XBMC->ReadFile(m_filebufferReadHandle, buffer, size);
}

time_t CE2STBTimeshift::TimeStart()
{
  return m_start;
}

time_t CE2STBTimeshift::TimeEnd()
{
  return (m_start) ? time(NULL) : 0;
}

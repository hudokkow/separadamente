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

#include "p8-platform/threads/threads.h"
#include "p8-platform/util/StdString.h"

#include <ctime>
#include <cstdint>

namespace e2stb
{
/* indicate that caller can handle truncated reads, where function returns before entire buffer has been filled */
#define READ_TRUNCATED 0x01
/* indicate that that caller support read in the minimum defined chunk size, this disables internal cache then */
#define READ_CHUNKED 0x02
/* use cache to access this file */
#define READ_CACHED 0x04
/* open without caching regardless to file type. */
#define READ_NO_CACHE 0x08
/* calculate bitrate for file while reading */
#define READ_BITRATE 0x10

#define STREAM_READ_BUFFER_SIZE   32768
#define BUFFER_READ_TIMEOUT       10000
#define BUFFER_READ_WAITTIME      50

class CE2STBTimeshift: public P8PLATFORM::CThread
{
public:
  CE2STBTimeshift(CStdString streamURL, CStdString bufferPath);
  ~CE2STBTimeshift(void);
  bool IsValid();
  int ReadData(unsigned char *buffer, unsigned int size);
  long long Seek(long long position, int whence);
  long long Position();
  long long Length();
  void Stop(void);
  time_t TimeStart();
  time_t TimeEnd();

private:
  virtual void *Process(void);

  CStdString m_bufferPath;
  void *m_streamHandle;
  void *m_filebufferReadHandle;
  void *m_filebufferWriteHandle;
  time_t m_start;
#ifndef TARGET_POSIX
  P8PLATFORM::CMutex m_mutex;
  uint64_t m_writePos;
#endif
};
} /* namespace e2stb */

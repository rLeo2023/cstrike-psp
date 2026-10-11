/*
*
*   This program is free software; you can redistribute it and/or modify it
*   under the terms of the GNU General Public License as published by the
*   Free Software Foundation; either version 2 of the License, or (at
*   your option) any later version.
*
*   This program is distributed in the hope that it will be useful, but
*   WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*   General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software Foundation,
*   Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
*   In addition, as a special exception, the author gives permission to
*   link the code of this program with the Half-Life Game Engine ("HL
*   Engine") and Modified Game Libraries ("MODs") developed by Valve,
*   L.L.C ("Valve").  You must obey the GNU General Public License in all
*   respects for all of the code used other than the HL Engine and MODs
*   from Valve.  If you modify this file, you may extend this exception
*   to your version of the file, but you are not obligated to do so.  If
*   you do not wish to do so, delete this exception statement from your
*   version.
*
*/

#pragma once

#ifdef __PSP__

// Navigation files are a few MB and parsing them allocates about as much
// again. Loading the whole file first doubled the peak and ran the PSP out
// of memory on bigger maps, so it is streamed through a small buffer instead.
class SteamFile
{
public:
	SteamFile(const char *filename);
	~SteamFile();

	bool IsValid() const { return m_file != nullptr || m_fileData != nullptr; }
	bool Read(void *data, int length);

private:
	enum { BUFFER_SIZE = 8192 };

	void *m_file;	// stdio FILE, opened from the game directory
	byte *m_buffer;	// on the heap, these live on the stack
	int m_bufferPos;
	int m_bufferLen;

	// fallback when the file can't be opened directly: whole file in memory
	byte *m_fileData;
	byte *m_cursor;
	int m_bytesLeft;
};

// implemented in nav_file.cpp

#else

class SteamFile
{
public:
	SteamFile(const char *filename);
	~SteamFile();

	bool IsValid() const { return (m_fileData) ? true : false; }
	bool Read(void *data, int length);

private:
	byte *m_fileData;
	int m_fileDataLength;

	byte *m_cursor;
	int m_bytesLeft;
};

inline SteamFile::SteamFile(const char *filename)
{
	m_fileData = (byte *)LOAD_FILE_FOR_ME(const_cast<char *>(filename), &m_fileDataLength);
	m_cursor = m_fileData;
	m_bytesLeft = m_fileDataLength;
}

inline SteamFile::~SteamFile()
{
	if (m_fileData)
	{
		FREE_FILE(m_fileData);
		m_fileData = nullptr;
	}
}

inline bool SteamFile::Read(void *data, int length)
{
	if (length > m_bytesLeft || !m_cursor || m_bytesLeft <= 0)
		return false;

	byte *readCursor = static_cast<byte *>(data);
	for (int i = 0; i < length; i++)
	{
		*readCursor++ = *m_cursor++;
		m_bytesLeft--;
	}

	return true;
}

#endif // __PSP__

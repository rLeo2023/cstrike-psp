/*
PSP file system shim for the game library.

On desktop this file loads filesystem_stdio through the dynamic loader and
grabs its CreateInterface. PSP has no dlopen, but the game library is linked
into the engine binary, so the engine filesystem is already right here: this
adapter just forwards to it.

Only FileExists is actually used by the game library (hostage rescue models
and bot chatter files). The rest exists because IFileSystem is all pure
virtuals, and they forward to the equivalent engine entry points so the
interface stays usable instead of crashing if something else starts calling it.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
*/

#include "precompiled.h"
// FS_FileSize() and COM_ParseFile() come from the engine, which we are
// linked into. Declared by hand here because the game's own engine/ headers
// are the rehlds ones, not xash3d's. They are C functions in the engine, so
// they must be declared extern "C" or they get a C++ mangled name.
extern "C" {
	extern int   FS_FileSize( const char *path, int onlyingamedir );
	extern char *COM_ParseFile( char *data, char *token );
}

IFileSystem *g_pFileSystem = nullptr;
CSysModule *g_pFileSystemModule = nullptr;

#if defined(__PSP__)

class CEngineFileSystem : public IFileSystem
{
public:
	void Mount() override {}
	void Unmount() override {}
	void RemoveAllSearchPaths() override {}
	void AddSearchPath(const char *, const char *) override {}
	bool RemoveSearchPath(const char *) override { return false; }
	void RemoveFile(const char *, const char *) override {}
	void CreateDirHierarchy(const char *, const char *) override {}

	bool FileExists(const char *pFileName) override
	{
		return pFileName && *pFileName && FS_FileSize(pFileName, false) >= 0;
	}

	bool IsDirectory(const char *) override { return false; }
	FileHandle_t Open(const char *, const char *, const char * = 0L) override { return FILESYSTEM_INVALID_HANDLE; }
	void Close(FileHandle_t) override {}
	void Seek(FileHandle_t, int, FileSystemSeek_t) override {}
	unsigned int Tell(FileHandle_t) override { return 0; }
	unsigned int Size(FileHandle_t) override { return 0; }
	unsigned int Size(const char *pFileName) override
	{
		int size = pFileName ? FS_FileSize(pFileName, false) : -1;
		return size < 0 ? 0 : (unsigned int)size;
	}
	long GetFileTime(const char *) override { return 0; }
	void FileTimeToString(char *, int, long) override {}
	bool IsOk(FileHandle_t) override { return false; }
	void Flush(FileHandle_t) override {}
	bool EndOfFile(FileHandle_t) override { return true; }
	int Read(void *, int, FileHandle_t) override { return 0; }
	int Write(const void *, int, FileHandle_t) override { return 0; }
	char *ReadLine(char *, int, FileHandle_t) override { return NULL; }
	int FPrintf(FileHandle_t, char *, ...) override { return 0; }
	void *GetReadBuffer(FileHandle_t, int *, bool) override { return NULL; }
	void ReleaseReadBuffer(FileHandle_t, void *) override {}
	const char *FindFirst(const char *, FileFindHandle_t *, const char * = 0L) override { return NULL; }
	const char *FindNext(FileFindHandle_t) override { return NULL; }
	bool FindIsDirectory(FileFindHandle_t) override { return false; }
	void FindClose(FileFindHandle_t) override {}
	void GetLocalCopy(const char *) override {}
	const char *GetLocalPath(const char *, char *, int) override { return NULL; }
	char *ParseFile(char *pFileBytes, char *pToken, bool *pWasQuoted) override
	{
		return COM_ParseFile(pFileBytes, pToken);
	}
	bool FullPathToRelativePath(const char *, char *) override { return false; }
	bool GetCurrentDirectory(char *, int) override { return false; }
	void PrintOpenedFiles() override {}
	void SetWarningFunc(WarningFunc_t) override {}
	void SetWarningLevel(FileWarningLevel_t) override {}
	void LogLevelLoadStarted(const char *) override {}
	void LogLevelLoadFinished(const char *) override {}

	// xash3d has no pack mounting command, the game data ships as loose wads
	bool AddPackFile(const char *, const char *) override { return false; }

	FileHandle_t OpenFromCacheForRead(const char *, const char *, const char * = 0L) override { return FILESYSTEM_INVALID_HANDLE; }
	void AddSearchPathNoWrite(const char *, const char *) override {}
	int PauseResourcePreloading() override { return 0; }
	int ResumeResourcePreloading() override { return 0; }
	int SetVBuf(FileHandle_t, char *, int, long) override { return 0; }
	void GetInterfaceVersion(char *, int) override {}
	bool IsFileImmediatelyAvailable(const char *) override { return false; }
	WaitForResourcesHandle_t WaitForResources(const char *) override { return 0; }
	bool GetWaitForResourcesProgress(WaitForResourcesHandle_t, float *, bool *) override { return false; }
	void CancelWaitForResources(WaitForResourcesHandle_t) override {}
	bool IsAppReadyForOfflinePlay(int) override { return true; }
	int HintResourceNeed(const char *, int) override { return 0; }
};

static CEngineFileSystem s_EngineFileSystem;

bool FileSystem_Init()
{
	// nothing to load, the engine filesystem is linked in already
	g_pFileSystem = &s_EngineFileSystem;
	return true;
}

void FileSystem_Shutdown()
{
	g_pFileSystem = nullptr;
}

#else

bool FileSystem_Init()
{
	g_pFileSystemModule = Sys_LoadModule(STDIO_FILESYSTEM_LIB);
	if (!g_pFileSystemModule)
	{
		return false;
	}

	// Get FileSystem interface
	CreateInterfaceFn filesystemFactoryFn = Sys_GetFactory(g_pFileSystemModule);
	if (!filesystemFactoryFn)
	{
		Sys_Error("Unable to get filesystem factory.");
	}

	g_pFileSystem = (IFileSystem *)filesystemFactoryFn(FILESYSTEM_INTERFACE_VERSION, nullptr);
	if (!g_pFileSystem)
	{
		Sys_Error("Can not retrive filesystem interface version '" FILESYSTEM_INTERFACE_VERSION "'.");
	}

	return true;
}

void FileSystem_Shutdown()
{
	if (g_pFileSystemModule)
	{
		Sys_UnloadModule(g_pFileSystemModule);
		g_pFileSystemModule = nullptr;
	}
}

#endif // __PSP__
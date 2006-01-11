/*
** 2004 May 22
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
******************************************************************************
**
** This file contains code that is specific to windows.
*/
#include "sqliteInt.h"
#include "os.h"
#if OS_WIN               /* This file is used for windows only */

#include <winbase.h>

#ifdef __CYGWIN__
# include <sys/cygwin.h>
#endif

/*
** Macros used to determine whether or not to use threads.
*/
#if defined(THREADSAFE) && THREADSAFE
# define SQLITE_W32_THREADS 1
#endif

/*
** Include code that is common to all os_*.c files
*/
#include "os_common.h"

/*
** Determine if we are dealing with WindowsCE - which has a much
** reduced API.
*/
#if defined(_WIN32_WCE)
# define OS_WINCE 1
#else
# define OS_WINCE 0
#endif

#if OS_WINCE
typedef struct _LOCKDATA
{
  long nReaders;
  long nPending;
  long nReserved;
  long nExclusive;
} LOCKDATA;
#endif

/*
** The winFile structure is a subclass of OsFile specific to the win32
** portability layer.
*/
typedef struct winFile winFile;
struct winFile {
  IoMethod const *pMethod;/* Must be first */
  HANDLE h;               /* Handle for accessing the file */
  unsigned char locktype; /* Type of lock currently held on this file */
  short sharedLockByte;   /* Randomly chosen byte used as a shared lock */
#if OS_WINCE
  WCHAR *zDeleteOnClose;   /* Name of file to delete when closing */

#ifndef SQLITE_OMIT_WIN_LOCKS
  HANDLE hMutex;
  HANDLE hShared;
  LOCKDATA local;
  LOCKDATA *shared;
#endif 

#endif
};


/*
** Do not include any of the File I/O interface procedures if the
** SQLITE_OMIT_DISKIO macro is defined (indicating that there database
** will be in-memory only)
*/
#ifndef SQLITE_OMIT_DISKIO

/*
** The following variable is (normally) set once and never changes
** thereafter.  It records whether the operating system is Win95
** or WinNT.
**
** 0:   Operating system unknown.
** 1:   Operating system is Win95.
** 2:   Operating system is WinNT.
**
** In order to facilitate testing on a WinNT system, the test fixture
** can manually set this value to 1 to emulate Win98 behavior.
*/
int sqlite3_os_type = 0;

/*
** Return true (non-zero) if we are running under WinNT, Win2K, WinXP,
** or WinCE.  Return false (zero) for Win95, Win98, or WinME.
**
** Here is an interesting observation:  Win95, Win98, and WinME lack
** the LockFileEx() API.  But we can still statically link against that
** API as long as we don't call it win running Win95/98/ME.  A call to
** this routine is used to determine if the host is Win95/98/ME or
** WinNT/2K/XP so that we will know whether or not we can safely call
** the LockFileEx() API.
*/
#if OS_WINCE
# define isNT()  (1)
#else
  static int isNT(void){
    if( sqlite3_os_type==0 ){
      OSVERSIONINFO sInfo;
      sInfo.dwOSVersionInfoSize = sizeof(sInfo);
      GetVersionEx(&sInfo);
      sqlite3_os_type = sInfo.dwPlatformId==VER_PLATFORM_WIN32_NT ? 2 : 1;
    }
    return sqlite3_os_type==2;
  }
#endif /* OS_WINCE */

#if OS_WINCE
/*
** WindowsCE does not have a localtime() function.  So create a
** substitute.
*/
#include <time.h>
struct tm *__cdecl localtime(const time_t *t)
{
  static struct tm y;
  FILETIME uTm, lTm;
  SYSTEMTIME pTm;
  i64 t64;
  t64 = *t;
  t64 = (t64 + 11644473600)*10000000;
  uTm.dwLowDateTime = t64 & 0xFFFFFFFF;
  uTm.dwHighDateTime= t64 >> 32;
  FileTimeToLocalFileTime(&uTm,&lTm);
  FileTimeToSystemTime(&lTm,&pTm);
  y.tm_year = pTm.wYear - 1900;
  y.tm_mon = pTm.wMonth - 1;
  y.tm_wday = pTm.wDayOfWeek;
  y.tm_mday = pTm.wDay;
  y.tm_hour = pTm.wHour;
  y.tm_min = pTm.wMinute;
  y.tm_sec = pTm.wSecond;
  return &y;
}

/* This will never be called, but defined to make the code compile */
#define GetTempPathA(a,b)

#endif

/*
** Compile with -DSQLITE_OMIT_WIN_LOCKS to disable file locking on
** windows.  If you do this and two or more connections attempt to
** write the database at the same time, the database file will be
** corrupted.  But some versions of WindowsCE do not support locking,
** in which case compiling with this option is required just to get
** it to work at all.
*/
#ifdef SQLITE_OMIT_WIN_LOCKS
# define LockFile(a,b,c,d,e) (1)
# define LockFileEx(a,b,c,d,e,f) (1)
# define UnlockFile(a,b,c,d,e) (1)
# define UnlockFileEx(a,b,c,d,e) (1)
#endif

/*
** Convert a UTF-8 string to UTF-32.  Space to hold the returned string
** is obtained from sqliteMalloc.
*/
static WCHAR *utf8ToUnicode(const char *zFilename){
  int nByte;
  WCHAR *zWideFilename;

  if( !isNT() ){
    return 0;
  }
  nByte = MultiByteToWideChar(CP_UTF8, 0, zFilename, -1, NULL, 0)*sizeof(WCHAR);
  zWideFilename = sqliteMalloc( nByte*sizeof(zWideFilename[0]) );
  if( zWideFilename==0 ){
    return 0;
  }
  nByte = MultiByteToWideChar(CP_UTF8, 0, zFilename, -1, zWideFilename, nByte);
  if( nByte==0 ){
    sqliteFree(zWideFilename);
    zWideFilename = 0;
  }
  return zWideFilename;
}

/*
** Convert UTF-32 to UTF-8.  Space to hold the returned string is
** obtained from sqliteMalloc().
*/
static char *unicodeToUtf8(const WCHAR *zWideFilename){
  int nByte;
  char *zFilename;

  nByte = WideCharToMultiByte(CP_UTF8, 0, zWideFilename, -1, 0, 0, 0, 0);
  zFilename = sqliteMalloc( nByte );
  if( zFilename==0 ){
    return 0;
  }
  nByte = WideCharToMultiByte(CP_UTF8, 0, zWideFilename, -1, zFilename, nByte,
                              0, 0);
  if( nByte == 0 ){
    sqliteFree(zFilename);
    zFilename = 0;
  }
  return zFilename;
}

#if OS_WINCE && !SQLITE_OMIT_WIN_LOCKS
#define LockFile(a, b, c, d, e) pseudoLockFile(&a, b, c, d, e)
#define UnlockFile(a, b, c, d, e) pseudoUnlockFile(&a, b, c, d, e)
#define LockFileEx(a, b, c, d, e, f) pseudoLockFileEx(&a, b, c, d, e, f)

#define LOCKSTRUCT winFile

#ifndef PtrToInt
#define PtrToInt( p )  ((INT)(INT_PTR) (p) )
#endif

#define HANDLE_TO_LOCKSTRUCT(a) (LOCKSTRUCT *)&((LPBYTE)a)[-PtrToInt((&((LOCKSTRUCT *)0)->h))]

#define MUTEX_ACQUIRE(h) { DWORD dwErr; do { dwErr = WaitForSingleObject(h, INFINITE); } while (dwErr != WAIT_OBJECT_0 && dwErr != WAIT_ABANDONED); }
#define MUTEX_RELEASE(h) ReleaseMutex(h)

static BOOL CreateLockStruct(const char *pszFilename, LOCKSTRUCT *pLocks)
{
  WCHAR *pszTok;
  WCHAR *pszName = utf8ToUnicode(pszFilename);
  BOOL bInit = TRUE;

  /* Initialize the local lockdata */
  ZeroMemory(&pLocks->local, sizeof(LOCKDATA));

  /* Create a unique global name for the mutex and subsequently the shared memory */
  CharLowerW(pszName);
  while (pszTok = wcschr(pszName, '\\'))
  {
    *pszTok = '_';
  }

  /* Create/open the named mutex */
  pLocks->hMutex = CreateMutexW(NULL, FALSE, pszName);
  if (!pLocks->hMutex)
  {
    sqliteFree(pszName);
    return FALSE;
  }

  /* Acquire the mutex before continuing */
  MUTEX_ACQUIRE(pLocks->hMutex);
  
  /* Create/open the shared memory */
  CharUpperW(pszName);
  pLocks->hShared = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(LOCKDATA), pszName);  

  /* Set a flag that indicates we're the first to create the memory so it must be zero-initialized */
  if (GetLastError() == ERROR_ALREADY_EXISTS)
  {
    bInit = FALSE;
  }

  sqliteFree(pszName);

  /* If we succeeded in making the shared memory handle, map it. */
  if (pLocks->hShared)
  {
    pLocks->shared = (LOCKDATA *)MapViewOfFile(pLocks->hShared, FILE_MAP_READ|FILE_MAP_WRITE, 0, 0, sizeof(LOCKDATA));
    /* If mapping failed, close the shared memory handle and erase it */
    if (!pLocks->shared)
    {
      CloseHandle(pLocks->hShared);
      pLocks->hShared = NULL;
    }
  }

  /* If shared memory could not be created, then close the mutex and fail */
  if (pLocks->hShared == NULL)
  {
    MUTEX_RELEASE(pLocks->hMutex);
    CloseHandle(pLocks->hMutex);
    pLocks->hMutex = NULL;
    return FALSE;
  }
  
  /* Initialize the shared memory if we're supposed to */
  if (bInit)
  {
    ZeroMemory(pLocks->shared, sizeof(LOCKDATA));
  }

  MUTEX_RELEASE(pLocks->hMutex);

  return TRUE;
}

static void DestroyLockStruct(LOCKSTRUCT *pLocks)
{
  if (pLocks->hMutex)
  {
    /* Acquire the mutex */
    MUTEX_ACQUIRE(pLocks->hMutex);

    /* The following blocks should probably assert in debug mode, but they
       are to cleanup in case any locks remained open */
    if (pLocks->local.nReaders)
    {
      pLocks->shared->nReaders --;
    }

    if (pLocks->local.nReserved)
    {
      pLocks->shared->nReserved = 0;
    }

    if (pLocks->local.nPending)
    {
      pLocks->shared->nPending = 0;
    }

    if (pLocks->local.nExclusive)
    {
      pLocks->shared->nExclusive = 0;
    }

    /* De-reference and close our copy of the shared memory handle */
    UnmapViewOfFile(pLocks->shared);
    CloseHandle(pLocks->hShared);

    /* Done with the mutex */
    MUTEX_RELEASE(pLocks->hMutex);    
    CloseHandle(pLocks->hMutex);
    pLocks->hMutex = NULL;
  }
}

/* Custom pseudo file locking support specifically for SQLite */
BOOL pseudoLockFile(HANDLE *phFile, DWORD dwFileOffsetLow, DWORD dwFileOffsetHigh, DWORD nNumberOfBytesToLockLow, DWORD nNumberOfBytesToLockHigh)
{
  LOCKSTRUCT *pls = HANDLE_TO_LOCKSTRUCT(phFile);
  BOOL bReturn = FALSE;

  if (!pls->hMutex) return TRUE;

  MUTEX_ACQUIRE(pls->hMutex);

  /* Wanting an exclusive lock? */
  if (dwFileOffsetLow == SHARED_FIRST && nNumberOfBytesToLockLow == SHARED_SIZE)
  {
    if (pls->shared->nReaders == 0 && pls->shared->nExclusive == 0)
    {
        pls->shared->nExclusive = 1;
        pls->local.nExclusive = 1;
        bReturn = TRUE;
    }
  }
  /* Want a read-only lock? */
  else if ((dwFileOffsetLow >= SHARED_FIRST && dwFileOffsetLow < SHARED_FIRST + SHARED_SIZE) && nNumberOfBytesToLockLow == 1)
  {
    if (pls->shared->nExclusive == 0)
    {
      pls->local.nReaders ++;
      if (pls->local.nReaders == 1)
      {
        pls->shared->nReaders ++;
      }
      bReturn = TRUE;
    }
  }
  /* Want a pending lock? */
  else if (dwFileOffsetLow == PENDING_BYTE && nNumberOfBytesToLockLow == 1)
  {
    /* If no pending lock has been acquired, then acquire it */
    if (pls->shared->nPending == 0)
    {
      pls->shared->nPending = 1;
      pls->local.nPending = 1;
      bReturn = TRUE;
    }
  }
  /* Want a reserved lock? */
  else if (dwFileOffsetLow == RESERVED_BYTE && nNumberOfBytesToLockLow == 1)
  {
    if (pls->shared->nReserved == 0)
    {
      pls->shared->nReserved = 1;
      pls->local.nReserved = 1;
      bReturn = TRUE;
    }
  }

  MUTEX_RELEASE(pls->hMutex);

  return bReturn;
}

BOOL pseudoUnlockFile(HANDLE *phFile, DWORD dwFileOffsetLow, DWORD dwFileOffsetHigh, DWORD nNumberOfBytesToUnlockLow, DWORD nNumberOfBytesToUnlockHigh)
{
  LOCKSTRUCT *pls = HANDLE_TO_LOCKSTRUCT(phFile);
  BOOL bReturn = FALSE;

  if (!pls->hMutex) return TRUE;

  MUTEX_ACQUIRE(pls->hMutex);

  /* Releasing a reader lock or an exclusive lock */
  if (dwFileOffsetLow >= SHARED_FIRST && dwFileOffsetLow < SHARED_FIRST + SHARED_SIZE)
  {
    /* Did we have an exclusive lock? */
    if (pls->local.nExclusive)
    {
      pls->local.nExclusive = 0;
      pls->shared->nExclusive = 0;
      bReturn = TRUE;
    }
    /* Did we just have a reader lock? */
    else if (pls->local.nReaders)
    {
      pls->local.nReaders --;
      if (pls->local.nReaders == 0)
      {
        pls->shared->nReaders --;
      }
      bReturn = TRUE;
    }
  }
  /* Releasing a pending lock */
  else if (dwFileOffsetLow == PENDING_BYTE && nNumberOfBytesToUnlockLow == 1)
  {
    if (pls->local.nPending)
    {
      pls->local.nPending = 0;
      pls->shared->nPending = 0;
      bReturn = TRUE;
    }
  }
  /* Releasing a reserved lock */
  else if (dwFileOffsetLow == RESERVED_BYTE && nNumberOfBytesToUnlockLow == 1)
  {
    if (pls->local.nReserved)
    {
      pls->local.nReserved = 0;
      pls->shared->nReserved = 0;
      bReturn = TRUE;
    }
  }

  MUTEX_RELEASE(pls->hMutex);

  return bReturn;
}

BOOL pseudoLockFileEx(HANDLE *phFile, DWORD dwFlags, DWORD dwReserved, DWORD nNumberOfBytesToLockLow, DWORD nNumberOfBytesToLockHigh, LPOVERLAPPED lpOverlapped)
{
  /* If the caller wants a shared read lock, forward this call to pseudoLockFile */
  if (lpOverlapped->Offset == SHARED_FIRST && dwFlags == 1 && nNumberOfBytesToLockLow == SHARED_SIZE)
    return pseudoLockFile(phFile, SHARED_FIRST, 0, 1, 0);

  return FALSE;
}

#endif /* OS_WINCE && !SQLITE_OMIT_WIN_LOCKS */

/*
** Delete the named file
*/
int sqlite3WinDelete(const char *zFilename){
  WCHAR *zWide = utf8ToUnicode(zFilename);
  if( zWide ){
    DeleteFileW(zWide);
    sqliteFree(zWide);
  }else{
#if OS_WINCE
    return SQLITE_NOMEM;
#else
    DeleteFileA(zFilename);
#endif
  }
  TRACE2("DELETE \"%s\"\n", zFilename);
  return SQLITE_OK;
}

/*
** Return TRUE if the named file exists.
*/
int sqlite3WinFileExists(const char *zFilename){
  int exists = 0;
  WCHAR *zWide = utf8ToUnicode(zFilename);
  if( zWide ){
    exists = GetFileAttributesW(zWide) != 0xffffffff;
    sqliteFree(zWide);
  }else{
#if OS_WINCE
    return SQLITE_NOMEM;
#else
    exists = GetFileAttributesA(zFilename) != 0xffffffff;
#endif
  }
  return exists;
}

/* Forward declaration */
int allocateWinFile(winFile *pInit, OsFile **pId);

/*
** Attempt to open a file for both reading and writing.  If that
** fails, try opening it read-only.  If the file does not exist,
** try to create it.
**
** On success, a handle for the open file is written to *id
** and *pReadonly is set to 0 if the file was opened for reading and
** writing or 1 if the file was opened read-only.  The function returns
** SQLITE_OK.
**
** On failure, the function returns SQLITE_CANTOPEN and leaves
** *id and *pReadonly unchanged.
*/
int sqlite3WinOpenReadWrite(
  const char *zFilename,
  OsFile **pId,
  int *pReadonly
){
  winFile f;
  HANDLE h;
  WCHAR *zWide = utf8ToUnicode(zFilename);
  assert( *pId==0 );
  if( zWide ){
    h = CreateFileW(zWide,
       GENERIC_READ | GENERIC_WRITE,
       FILE_SHARE_READ | FILE_SHARE_WRITE,
       NULL,
       OPEN_ALWAYS,
       FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
       NULL
    );
    if( h==INVALID_HANDLE_VALUE ){
      h = CreateFileW(zWide,
         GENERIC_READ,
         FILE_SHARE_READ,
         NULL,
         OPEN_ALWAYS,
         FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
         NULL
      );
      if( h==INVALID_HANDLE_VALUE ){
        sqliteFree(zWide);
        return SQLITE_CANTOPEN;
      }
      *pReadonly = 1;
    }else{
      *pReadonly = 0;
    }
#if OS_WINCE && !SQLITE_OMIT_WIN_LOCKS
    if (!CreateLockStruct(zFilename, &f)){
      CloseHandle(h);
      sqliteFree(zWide);
      return SQLITE_CANTOPEN;
    }
#endif
    sqliteFree(zWide);
  }else{
#if OS_WINCE
    return SQLITE_NOMEM;
#else
    h = CreateFileA(zFilename,
       GENERIC_READ | GENERIC_WRITE,
       FILE_SHARE_READ | FILE_SHARE_WRITE,
       NULL,
       OPEN_ALWAYS,
       FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
       NULL
    );
    if( h==INVALID_HANDLE_VALUE ){
      h = CreateFileA(zFilename,
         GENERIC_READ,
         FILE_SHARE_READ,
         NULL,
         OPEN_ALWAYS,
         FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
         NULL
      );
      if( h==INVALID_HANDLE_VALUE ){
        return SQLITE_CANTOPEN;
      }
      *pReadonly = 1;
    }else{
      *pReadonly = 0;
    }
#endif /* OS_WINCE */
  }
  f.h = h;
  f.locktype = NO_LOCK;
  f.sharedLockByte = 0;
#if OS_WINCE
  f.zDeleteOnClose = 0;
#endif
  TRACE3("OPEN R/W %d \"%s\"\n", h, zFilename);
  return allocateWinFile(&f, pId);
}


/*
** Attempt to open a new file for exclusive access by this process.
** The file will be opened for both reading and writing.  To avoid
** a potential security problem, we do not allow the file to have
** previously existed.  Nor do we allow the file to be a symbolic
** link.
**
** If delFlag is true, then make arrangements to automatically delete
** the file when it is closed.
**
** On success, write the file handle into *id and return SQLITE_OK.
**
** On failure, return SQLITE_CANTOPEN.
*/
int sqlite3WinOpenExclusive(const char *zFilename, OsFile **pId, int delFlag){
  winFile f;
  HANDLE h;
  int fileflags;
  WCHAR *zWide = utf8ToUnicode(zFilename);
  assert( *pId == 0 );
  fileflags = FILE_FLAG_RANDOM_ACCESS;
#if !OS_WINCE
  if( delFlag ){
    fileflags |= FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE;
  }
#endif
  if( zWide ){
    h = CreateFileW(zWide,
       GENERIC_READ | GENERIC_WRITE,
       0,
       NULL,
       CREATE_ALWAYS,
       fileflags,
       NULL
    );
    sqliteFree(zWide);
  }else{
#if OS_WINCE
    return SQLITE_NOMEM;
#else
    h = CreateFileA(zFilename,
       GENERIC_READ | GENERIC_WRITE,
       0,
       NULL,
       CREATE_ALWAYS,
       fileflags,
       NULL
    );
#endif /* OS_WINCE */
  }
  if( h==INVALID_HANDLE_VALUE ){
    return SQLITE_CANTOPEN;
  }
  f.h = h;
  f.locktype = NO_LOCK;
  f.sharedLockByte = 0;
#if OS_WINCE
  f.zDeleteOnClose = delFlag ? utf8ToUnicode(zFilename) : 0;
#ifndef SQLITE_OMIT_WIN_LOCKS
  f.hMutex = NULL;
#endif
#endif
  TRACE3("OPEN EX %d \"%s\"\n", h, zFilename);
  return allocateWinFile(&f, pId);
}

/*
** Attempt to open a new file for read-only access.
**
** On success, write the file handle into *id and return SQLITE_OK.
**
** On failure, return SQLITE_CANTOPEN.
*/
int sqlite3WinOpenReadOnly(const char *zFilename, OsFile **pId){
  winFile f;
  HANDLE h;
  WCHAR *zWide = utf8ToUnicode(zFilename);
  assert( *pId==0 );
  if( zWide ){
    h = CreateFileW(zWide,
       GENERIC_READ,
       0,
       NULL,
       OPEN_EXISTING,
       FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
       NULL
    );
    sqliteFree(zWide);
  }else{
#if OS_WINCE
    return SQLITE_NOMEM;
#else
    h = CreateFileA(zFilename,
       GENERIC_READ,
       0,
       NULL,
       OPEN_EXISTING,
       FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
       NULL
    );
#endif
  }
  if( h==INVALID_HANDLE_VALUE ){
    return SQLITE_CANTOPEN;
  }
  f.h = h;
  f.locktype = NO_LOCK;
  f.sharedLockByte = 0;
#if OS_WINCE
  f.zDeleteOnClose = 0;
#ifndef SQLITE_OMIT_WIN_LOCKS
  f.hMutex = NULL;
#endif
#endif
  TRACE3("OPEN RO %d \"%s\"\n", h, zFilename);
  return allocateWinFile(&f, pId);
}

/*
** Attempt to open a file descriptor for the directory that contains a
** file.  This file descriptor can be used to fsync() the directory
** in order to make sure the creation of a new file is actually written
** to disk.
**
** This routine is only meaningful for Unix.  It is a no-op under
** windows since windows does not support hard links.
**
** On success, a handle for a previously open file is at *id is
** updated with the new directory file descriptor and SQLITE_OK is
** returned.
**
** On failure, the function returns SQLITE_CANTOPEN and leaves
** *id unchanged.
*/
static int winOpenDirectory(
  OsFile *id,
  const char *zDirname
){
  return SQLITE_OK;
}

/*
** If the following global variable points to a string which is the
** name of a directory, then that directory will be used to store
** temporary files.
*/
char *sqlite3_temp_directory = 0;

/*
** Create a temporary file name in zBuf.  zBuf must be big enough to
** hold at least SQLITE_TEMPNAME_SIZE characters.
*/
int sqlite3WinTempFileName(char *zBuf){
  static char zChars[] =
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "0123456789";
  int i, j;
  char zTempPath[SQLITE_TEMPNAME_SIZE];
  if( sqlite3_temp_directory ){
    strncpy(zTempPath, sqlite3_temp_directory, SQLITE_TEMPNAME_SIZE-30);
    zTempPath[SQLITE_TEMPNAME_SIZE-30] = 0;
  }else if( isNT() ){
    char *zMulti;
    WCHAR zWidePath[SQLITE_TEMPNAME_SIZE];
    GetTempPathW(SQLITE_TEMPNAME_SIZE-30, zWidePath);
    zMulti = unicodeToUtf8(zWidePath);
    if( zMulti ){
      strncpy(zTempPath, zMulti, SQLITE_TEMPNAME_SIZE-30);
      zTempPath[SQLITE_TEMPNAME_SIZE-30] = 0;
      sqliteFree(zMulti);
    }
  }else{
    GetTempPathA(SQLITE_TEMPNAME_SIZE-30, zTempPath);
  }
  for(i=strlen(zTempPath); i>0 && zTempPath[i-1]=='\\'; i--){}
  zTempPath[i] = 0;
  for(;;){
    sprintf(zBuf, "%s\\"TEMP_FILE_PREFIX, zTempPath);
    j = strlen(zBuf);
    sqlite3Randomness(15, &zBuf[j]);
    for(i=0; i<15; i++, j++){
      zBuf[j] = (char)zChars[ ((unsigned char)zBuf[j])%(sizeof(zChars)-1) ];
    }
    zBuf[j] = 0;
    if( !sqlite3OsFileExists(zBuf) ) break;
  }
  TRACE2("TEMP FILENAME: %s\n", zBuf);
  return SQLITE_OK; 
}

/*
** Close a file.
*/
static int winClose(OsFile **pId){
  winFile *pFile;
  if( pId && (pFile = (winFile*)*pId)!=0 ){
    TRACE2("CLOSE %d\n", pFile->h);
    CloseHandle(pFile->h);
#if OS_WINCE
#ifndef SQLITE_OMIT_WIN_LOCKS
    DestroyLockStruct(pFile);
#endif
    if( pFile->zDeleteOnClose ){
      DeleteFileW(pFile->zDeleteOnClose);
      sqliteFree(pFile->zDeleteOnClose);
    }
#endif
    OpenCounter(-1);
    sqliteFree(pFile);
    *pId = 0;
  }
  return SQLITE_OK;
}

/*
** Read data from a file into a buffer.  Return SQLITE_OK if all
** bytes were read successfully and SQLITE_IOERR if anything goes
** wrong.
*/
static int winRead(OsFile *id, void *pBuf, int amt){
  DWORD got;
  assert( id!=0 );
  SimulateIOError(SQLITE_IOERR);
  TRACE3("READ %d lock=%d\n", ((winFile*)id)->h, ((winFile*)id)->locktype);
  if( !ReadFile(((winFile*)id)->h, pBuf, amt, &got, 0) ){
    got = 0;
  }
  if( got==(DWORD)amt ){
    return SQLITE_OK;
  }else{
    return SQLITE_IOERR;
  }
}

/*
** Write data from a buffer into a file.  Return SQLITE_OK on success
** or some other error code on failure.
*/
static int winWrite(OsFile *id, const void *pBuf, int amt){
  int rc = 0;
  DWORD wrote;
  assert( id!=0 );
  SimulateIOError(SQLITE_IOERR);
  SimulateDiskfullError;
  TRACE3("WRITE %d lock=%d\n", ((winFile*)id)->h, ((winFile*)id)->locktype);
  assert( amt>0 );
  while( amt>0 && (rc = WriteFile(((winFile*)id)->h, pBuf, amt, &wrote, 0))!=0
         && wrote>0 ){
    amt -= wrote;
    pBuf = &((char*)pBuf)[wrote];
  }
  if( !rc || amt>(int)wrote ){
    return SQLITE_FULL;
  }
  return SQLITE_OK;
}

/*
** Some microsoft compilers lack this definition.
*/
#ifndef INVALID_SET_FILE_POINTER
# define INVALID_SET_FILE_POINTER ((DWORD)-1)
#endif

/*
** Move the read/write pointer in a file.
*/
static int winSeek(OsFile *id, i64 offset){
  LONG upperBits = offset>>32;
  LONG lowerBits = offset & 0xffffffff;
  DWORD rc;
  assert( id!=0 );
#ifdef SQLITE_TEST
  if( offset ) SimulateDiskfullError
#endif
  SEEK(offset/1024 + 1);
  rc = SetFilePointer(((winFile*)id)->h, lowerBits, &upperBits, FILE_BEGIN);
  TRACE3("SEEK %d %lld\n", ((winFile*)id)->h, offset);
  if( rc==INVALID_SET_FILE_POINTER && GetLastError()!=NO_ERROR ){
    return SQLITE_FULL;
  }
  return SQLITE_OK;
}

/*
** Make sure all writes to a particular file are committed to disk.
*/
static int winSync(OsFile *id, int dataOnly){
  assert( id!=0 );
  TRACE3("SYNC %d lock=%d\n", ((winFile*)id)->h, ((winFile*)id)->locktype);
  if( FlushFileBuffers(((winFile*)id)->h) ){
    return SQLITE_OK;
  }else{
    return SQLITE_IOERR;
  }
}

/*
** Sync the directory zDirname. This is a no-op on operating systems other
** than UNIX.
*/
int sqlite3WinSyncDirectory(const char *zDirname){
  SimulateIOError(SQLITE_IOERR);
  return SQLITE_OK;
}

/*
** Truncate an open file to a specified size
*/
static int winTruncate(OsFile *id, i64 nByte){
  LONG upperBits = nByte>>32;
  assert( id!=0 );
  TRACE3("TRUNCATE %d %lld\n", ((winFile*)id)->h, nByte);
  SimulateIOError(SQLITE_IOERR);
  SetFilePointer(((winFile*)id)->h, nByte, &upperBits, FILE_BEGIN);
  SetEndOfFile(((winFile*)id)->h);
  return SQLITE_OK;
}

/*
** Determine the current size of a file in bytes
*/
static int winFileSize(OsFile *id, i64 *pSize){
  DWORD upperBits, lowerBits;
  assert( id!=0 );
  SimulateIOError(SQLITE_IOERR);
  lowerBits = GetFileSize(((winFile*)id)->h, &upperBits);
  *pSize = (((i64)upperBits)<<32) + lowerBits;
  return SQLITE_OK;
}

/*
** LOCKFILE_FAIL_IMMEDIATELY is undefined on some Windows systems.
*/
#ifndef LOCKFILE_FAIL_IMMEDIATELY
# define LOCKFILE_FAIL_IMMEDIATELY 1
#endif

/*
** Acquire a reader lock.
** Different API routines are called depending on whether or not this
** is Win95 or WinNT.
*/
static int getReadLock(winFile *id){
  int res;
  if( isNT() ){
    OVERLAPPED ovlp;
    ovlp.Offset = SHARED_FIRST;
    ovlp.OffsetHigh = 0;
    ovlp.hEvent = 0;
    res = LockFileEx(id->h, LOCKFILE_FAIL_IMMEDIATELY, 0, SHARED_SIZE,0,&ovlp);
  }else{
    int lk;
    sqlite3Randomness(sizeof(lk), &lk);
    id->sharedLockByte = (lk & 0x7fffffff)%(SHARED_SIZE - 1);
    res = LockFile(id->h, SHARED_FIRST+id->sharedLockByte, 0, 1, 0);
  }
  return res;
}

/*
** Undo a readlock
*/
static int unlockReadLock(winFile *pFile){
  int res;
  if( isNT() ){
    res = UnlockFile(pFile->h, SHARED_FIRST, 0, SHARED_SIZE, 0);
  }else{
    res = UnlockFile(pFile->h, SHARED_FIRST + pFile->sharedLockByte, 0, 1, 0);
  }
  return res;
}

#ifndef SQLITE_OMIT_PAGER_PRAGMAS
/*
** Check that a given pathname is a directory and is writable 
**
*/
int sqlite3WinIsDirWritable(char *zDirname){
  int fileAttr;
  WCHAR *zWide;
  if( zDirname==0 ) return 0;
  if( !isNT() && strlen(zDirname)>MAX_PATH ) return 0;
  zWide = utf8ToUnicode(zDirname);
  if( zWide ){
    fileAttr = GetFileAttributesW(zWide);
    sqliteFree(zWide);
  }else{
#if OS_WINCE
    return 0;
#else
    fileAttr = GetFileAttributesA(zDirname);
#endif
  }
  if( fileAttr == 0xffffffff ) return 0;
  if( (fileAttr & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY ){
    return 0;
  }
  return 1;
}
#endif /* SQLITE_OMIT_PAGER_PRAGMAS */

/*
** Lock the file with the lock specified by parameter locktype - one
** of the following:
**
**     (1) SHARED_LOCK
**     (2) RESERVED_LOCK
**     (3) PENDING_LOCK
**     (4) EXCLUSIVE_LOCK
**
** Sometimes when requesting one lock state, additional lock states
** are inserted in between.  The locking might fail on one of the later
** transitions leaving the lock state different from what it started but
** still short of its goal.  The following chart shows the allowed
** transitions and the inserted intermediate states:
**
**    UNLOCKED -> SHARED
**    SHARED -> RESERVED
**    SHARED -> (PENDING) -> EXCLUSIVE
**    RESERVED -> (PENDING) -> EXCLUSIVE
**    PENDING -> EXCLUSIVE
**
** This routine will only increase a lock.  The winUnlock() routine
** erases all locks at once and returns us immediately to locking level 0.
** It is not possible to lower the locking level one step at a time.  You
** must go straight to locking level 0.
*/
static int winLock(OsFile *id, int locktype){
  int rc = SQLITE_OK;    /* Return code from subroutines */
  int res = 1;           /* Result of a windows lock call */
  int newLocktype;       /* Set id->locktype to this value before exiting */
  int gotPendingLock = 0;/* True if we acquired a PENDING lock this time */
  winFile *pFile = (winFile*)id;

  assert( pFile!=0 );
  TRACE5("LOCK %d %d was %d(%d)\n",
          pFile->h, locktype, pFile->locktype, pFile->sharedLockByte);

  /* If there is already a lock of this type or more restrictive on the
  ** OsFile, do nothing. Don't use the end_lock: exit path, as
  ** sqlite3OsEnterMutex() hasn't been called yet.
  */
  if( pFile->locktype>=locktype ){
    return SQLITE_OK;
  }

  /* Make sure the locking sequence is correct
  */
  assert( pFile->locktype!=NO_LOCK || locktype==SHARED_LOCK );
  assert( locktype!=PENDING_LOCK );
  assert( locktype!=RESERVED_LOCK || pFile->locktype==SHARED_LOCK );

  /* Lock the PENDING_LOCK byte if we need to acquire a PENDING lock or
  ** a SHARED lock.  If we are acquiring a SHARED lock, the acquisition of
  ** the PENDING_LOCK byte is temporary.
  */
  newLocktype = pFile->locktype;
  if( pFile->locktype==NO_LOCK
   || (locktype==EXCLUSIVE_LOCK && pFile->locktype==RESERVED_LOCK)
  ){
    int cnt = 3;
    while( cnt-->0 && (res = LockFile(pFile->h, PENDING_BYTE, 0, 1, 0))==0 ){
      /* Try 3 times to get the pending lock.  The pending lock might be
      ** held by another reader process who will release it momentarily.
      */
      TRACE2("could not get a PENDING lock. cnt=%d\n", cnt);
      Sleep(1);
    }
    gotPendingLock = res;
  }

  /* Acquire a shared lock
  */
  if( locktype==SHARED_LOCK && res ){
    assert( pFile->locktype==NO_LOCK );
    res = getReadLock(pFile);
    if( res ){
      newLocktype = SHARED_LOCK;
    }
  }

  /* Acquire a RESERVED lock
  */
  if( locktype==RESERVED_LOCK && res ){
    assert( pFile->locktype==SHARED_LOCK );
    res = LockFile(pFile->h, RESERVED_BYTE, 0, 1, 0);
    if( res ){
      newLocktype = RESERVED_LOCK;
    }
  }

  /* Acquire a PENDING lock
  */
  if( locktype==EXCLUSIVE_LOCK && res ){
    newLocktype = PENDING_LOCK;
    gotPendingLock = 0;
  }

  /* Acquire an EXCLUSIVE lock
  */
  if( locktype==EXCLUSIVE_LOCK && res ){
    assert( pFile->locktype>=SHARED_LOCK );
    res = unlockReadLock(pFile);
    TRACE2("unreadlock = %d\n", res);
    res = LockFile(pFile->h, SHARED_FIRST, 0, SHARED_SIZE, 0);
    if( res ){
      newLocktype = EXCLUSIVE_LOCK;
    }else{
      TRACE2("error-code = %d\n", GetLastError());
    }
  }

  /* If we are holding a PENDING lock that ought to be released, then
  ** release it now.
  */
  if( gotPendingLock && locktype==SHARED_LOCK ){
    UnlockFile(pFile->h, PENDING_BYTE, 0, 1, 0);
  }

  /* Update the state of the lock has held in the file descriptor then
  ** return the appropriate result code.
  */
  if( res ){
    rc = SQLITE_OK;
  }else{
    TRACE4("LOCK FAILED %d trying for %d but got %d\n", pFile->h,
           locktype, newLocktype);
    rc = SQLITE_BUSY;
  }
  pFile->locktype = newLocktype;
  return rc;
}

/*
** This routine checks if there is a RESERVED lock held on the specified
** file by this or any other process. If such a lock is held, return
** non-zero, otherwise zero.
*/
static int winCheckReservedLock(OsFile *id){
  int rc;
  winFile *pFile = (winFile*)id;
  assert( pFile!=0 );
  if( pFile->locktype>=RESERVED_LOCK ){
    rc = 1;
    TRACE3("TEST WR-LOCK %d %d (local)\n", pFile->h, rc);
  }else{
    rc = LockFile(pFile->h, RESERVED_BYTE, 0, 1, 0);
    if( rc ){
      UnlockFile(pFile->h, RESERVED_BYTE, 0, 1, 0);
    }
    rc = !rc;
    TRACE3("TEST WR-LOCK %d %d (remote)\n", pFile->h, rc);
  }
  return rc;
}

/*
** Lower the locking level on file descriptor id to locktype.  locktype
** must be either NO_LOCK or SHARED_LOCK.
**
** If the locking level of the file descriptor is already at or below
** the requested locking level, this routine is a no-op.
**
** It is not possible for this routine to fail if the second argument
** is NO_LOCK.  If the second argument is SHARED_LOCK then this routine
** might return SQLITE_IOERR;
*/
static int winUnlock(OsFile *id, int locktype){
  int type;
  int rc = SQLITE_OK;
  winFile *pFile = (winFile*)id;
  assert( pFile!=0 );
  assert( locktype<=SHARED_LOCK );
  TRACE5("UNLOCK %d to %d was %d(%d)\n", pFile->h, locktype,
          pFile->locktype, pFile->sharedLockByte);
  type = pFile->locktype;
  if( type>=EXCLUSIVE_LOCK ){
    UnlockFile(pFile->h, SHARED_FIRST, 0, SHARED_SIZE, 0);
    if( locktype==SHARED_LOCK && !getReadLock(pFile) ){
      /* This should never happen.  We should always be able to
      ** reacquire the read lock */
      rc = SQLITE_IOERR;
    }
  }
  if( type>=RESERVED_LOCK ){
    UnlockFile(pFile->h, RESERVED_BYTE, 0, 1, 0);
  }
  if( locktype==NO_LOCK && type>=SHARED_LOCK ){
    unlockReadLock(pFile);
  }
  if( type>=PENDING_LOCK ){
    UnlockFile(pFile->h, PENDING_BYTE, 0, 1, 0);
  }
  pFile->locktype = locktype;
  return rc;
}

/*
** Turn a relative pathname into a full pathname.  Return a pointer
** to the full pathname stored in space obtained from sqliteMalloc().
** The calling function is responsible for freeing this space once it
** is no longer needed.
*/
char *sqlite3WinFullPathname(const char *zRelative){
  char *zFull;
#if defined(__CYGWIN__)
  int nByte;
  nByte = strlen(zRelative) + MAX_PATH + 1001;
  zFull = sqliteMalloc( nByte );
  if( zFull==0 ) return 0;
  if( cygwin_conv_to_full_win32_path(zRelative, zFull) ) return 0;
#elif OS_WINCE
  /* WinCE has no concept of a relative pathname, or so I am told. */
  zFull = sqlite3StrDup(zRelative);
#else
  char *zNotUsed;
  WCHAR *zWide;
  int nByte;
  zWide = utf8ToUnicode(zRelative);
  if( zWide ){
    WCHAR *zTemp, *zNotUsedW;
    nByte = GetFullPathNameW(zWide, 0, 0, &zNotUsedW) + 1;
    zTemp = sqliteMalloc( nByte*sizeof(zTemp[0]) );
    if( zTemp==0 ) return 0;
    GetFullPathNameW(zWide, nByte, zTemp, &zNotUsedW);
    sqliteFree(zWide);
    zFull = unicodeToUtf8(zTemp);
    sqliteFree(zTemp);
  }else{
    nByte = GetFullPathNameA(zRelative, 0, 0, &zNotUsed) + 1;
    zFull = sqliteMalloc( nByte*sizeof(zFull[0]) );
    if( zFull==0 ) return 0;
    GetFullPathNameA(zRelative, nByte, zFull, &zNotUsed);
  }
#endif
  return zFull;
}

/*
** The fullSync option is meaningless on windows.   This is a no-op.
*/
static void winSetFullSync(OsFile *id, int v){
  return;
}

/*
** Return the underlying file handle for an OsFile
*/
static int winFileHandle(OsFile *id){
  return (int)((winFile*)id)->h;
}

/*
** Return an integer that indices the type of lock currently held
** by this handle.  (Used for testing and analysis only.)
*/
static int winLockState(OsFile *id){
  return ((winFile*)id)->locktype;
}

/*
** This vector defines all the methods that can operate on an OsFile
** for win32.
*/
static const IoMethod sqlite3WinIoMethod = {
  winClose,
  winOpenDirectory,
  winRead,
  winWrite,
  winSeek,
  winTruncate,
  winSync,
  winSetFullSync,
  winFileHandle,
  winFileSize,
  winLock,
  winUnlock,
  winLockState,
  winCheckReservedLock,
};

/*
** Allocate memory for an OsFile.  Initialize the new OsFile
** to the value given in pInit and return a pointer to the new
** OsFile.  If we run out of memory, close the file and return NULL.
*/
int allocateWinFile(winFile *pInit, OsFile **pId){
  winFile *pNew;
  pNew = sqliteMalloc( sizeof(*pNew) );
  if( pNew==0 ){
    CloseHandle(pInit->h);
#if OS_WINCE
    sqliteFree(pInit->zDeleteOnClose);
#endif
    *pId = 0;
    return SQLITE_NOMEM;
  }else{
    *pNew = *pInit;
    pNew->pMethod = &sqlite3WinIoMethod;
    *pId = (OsFile*)pNew;
    return SQLITE_OK;
  }
}


#endif /* SQLITE_OMIT_DISKIO */
/***************************************************************************
** Everything above deals with file I/O.  Everything that follows deals
** with other miscellanous aspects of the operating system interface
****************************************************************************/

/*
** Get information to seed the random number generator.  The seed
** is written into the buffer zBuf[256].  The calling function must
** supply a sufficiently large buffer.
*/
int sqlite3WinRandomSeed(char *zBuf){
  /* We have to initialize zBuf to prevent valgrind from reporting
  ** errors.  The reports issued by valgrind are incorrect - we would
  ** prefer that the randomness be increased by making use of the
  ** uninitialized space in zBuf - but valgrind errors tend to worry
  ** some users.  Rather than argue, it seems easier just to initialize
  ** the whole array and silence valgrind, even if that means less randomness
  ** in the random seed.
  **
  ** When testing, initializing zBuf[] to zero is all we do.  That means
  ** that we always use the same random number sequence.* This makes the
  ** tests repeatable.
  */
  memset(zBuf, 0, 256);
  GetSystemTime((LPSYSTEMTIME)zBuf);
  return SQLITE_OK;
}

/*
** Sleep for a little while.  Return the amount of time slept.
*/
int sqlite3WinSleep(int ms){
  Sleep(ms);
  return ms;
}

/*
** Static variables used for thread synchronization
*/
static int inMutex = 0;
#ifdef SQLITE_W32_THREADS
  static CRITICAL_SECTION cs;
#endif

/*
** The following pair of routine implement mutual exclusion for
** multi-threaded processes.  Only a single thread is allowed to
** executed code that is surrounded by EnterMutex() and LeaveMutex().
**
** SQLite uses only a single Mutex.  There is not much critical
** code and what little there is executes quickly and without blocking.
*/
void sqlite3WinEnterMutex(){
#ifdef SQLITE_W32_THREADS
  static int isInit = 0;
  while( !isInit ){
    static long lock = 0;
    if( InterlockedIncrement(&lock)==1 ){
      InitializeCriticalSection(&cs);
      isInit = 1;
    }else{
      Sleep(1);
    }
  }
  EnterCriticalSection(&cs);
#endif
  assert( !inMutex );
  inMutex = 1;
}
void sqlite3WinLeaveMutex(){
  assert( inMutex );
  inMutex = 0;
#ifdef SQLITE_W32_THREADS
  LeaveCriticalSection(&cs);
#endif
}

/*
** Return TRUE if we are currently within the mutex and FALSE if not.
*/
int sqlite3WinInMutex(){
  return inMutex;
}


/*
** The following variable, if set to a non-zero value, becomes the result
** returned from sqlite3OsCurrentTime().  This is used for testing.
*/
#ifdef SQLITE_TEST
int sqlite3_current_time = 0;
#endif

/*
** Find the current time (in Universal Coordinated Time).  Write the
** current time and date as a Julian Day number into *prNow and
** return 0.  Return 1 if the time and date cannot be found.
*/
int sqlite3WinCurrentTime(double *prNow){
  FILETIME ft;
  /* FILETIME structure is a 64-bit value representing the number of 
     100-nanosecond intervals since January 1, 1601 (= JD 2305813.5). 
  */
  double now;
#if OS_WINCE
  SYSTEMTIME time;
  GetSystemTime(&time);
  SystemTimeToFileTime(&time,&ft);
#else
  GetSystemTimeAsFileTime( &ft );
#endif
  now = ((double)ft.dwHighDateTime) * 4294967296.0; 
  *prNow = (now + ft.dwLowDateTime)/864000000000.0 + 2305813.5;
#ifdef SQLITE_TEST
  if( sqlite3_current_time ){
    *prNow = sqlite3_current_time/86400.0 + 2440587.5;
  }
#endif
  return 0;
}

/* 
** The first time this function is called from a specific thread, nByte 
** bytes of data area are allocated and zeroed. A pointer to the new 
** allocation is returned to the caller. 
**
** Each subsequent call to this function from the thread returns the same
** pointer. The argument is ignored in this case.
*/
void *sqlite3WinThreadSpecificData(int nByte){
  static void *pTsd = 0;
  static int key;
  static int keyInit = 0;

  if( !keyInit ){
    sqlite3OsEnterMutex();
    if( !keyInit ){
      key = TlsAlloc();
      if( key==0xffffffff ){
        sqlite3OsLeaveMutex();
        return 0;
      }
      keyInit = 1;
    }
    sqlite3OsLeaveMutex();
  }
  pTsd = TlsGetValue(key);
  if( !pTsd ){
    pTsd = sqlite3OsMalloc(nByte);
    if( pTsd ){
      memset(pTsd, 0, nByte);
      TlsSetValue(key, pTsd);
    }
  }
  return pTsd;
}
#endif /* OS_WIN */

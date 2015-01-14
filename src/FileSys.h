/* XDS v2.51: Copyright (c) 1999-2003 Excelsior, LLC. All Rights Reserved. */
/* Generated by XDS Modula-2 to ANSI C v4.20 translator */

#ifndef FileSys_H_
#define FileSys_H_
#ifndef X2C_H_
#include "X2C.h"
#endif
#ifndef SysClock_H_
#include "SysClock.h"
#endif

extern X2C_BOOLEAN FileSys_Exists(X2C_CHAR [], X2C_CARD32);

extern void FileSys_ModifyTime(X2C_CHAR [], X2C_CARD32, X2C_CARD32 *,
                X2C_BOOLEAN *);

extern void FileSys_SetFileTime(X2C_CHAR [], X2C_CARD32, X2C_CARD32);

extern void FileSys_Rename(X2C_CHAR [], X2C_CARD32, X2C_CHAR [], X2C_CARD32,
                X2C_BOOLEAN *);

extern void FileSys_Remove(X2C_CHAR [], X2C_CARD32, X2C_BOOLEAN *);

extern void FileSys_FullName(X2C_CHAR [], X2C_CARD32, X2C_CHAR [],
                X2C_CARD32);

extern X2C_CARD32 FileSys_GetCDNameLength(void);

extern void FileSys_GetCDName(X2C_CHAR [], X2C_CARD32);

extern X2C_BOOLEAN FileSys_SetCD(X2C_CHAR [], X2C_CARD32);

typedef void *FileSys_Directory;

struct FileSys_Entry;


struct FileSys_Entry {
   X2C_CARD32 fileSize;
   struct SysClock_DateTime creaTime;
   struct SysClock_DateTime modfTime;
   X2C_CARD32 nameSize;
   X2C_BOOLEAN isDir;
   X2C_BOOLEAN done;
};

extern void FileSys_OpenDir(FileSys_Directory *, X2C_CHAR [], X2C_CARD32,
                struct FileSys_Entry *);

extern void FileSys_NextDirEntry(FileSys_Directory, struct FileSys_Entry *);

extern void FileSys_CloseDir(FileSys_Directory *);

extern void FileSys_GetName(FileSys_Directory, X2C_CHAR [], X2C_CARD32);

extern X2C_BOOLEAN FileSys_CreateDirectory(X2C_CHAR [], X2C_CARD32);

extern X2C_BOOLEAN FileSys_RemoveDirectory(X2C_CHAR [], X2C_CARD32);

extern X2C_BOOLEAN FileSys_GetDrive(X2C_CHAR *);

extern X2C_BOOLEAN FileSys_SetDrive(X2C_CHAR);

extern X2C_BOOLEAN FileSys_GetDriveCDNameLength(X2C_CHAR, X2C_CARD32 *);

extern X2C_BOOLEAN FileSys_GetDriveCDName(X2C_CHAR, X2C_CHAR [], X2C_CARD32);

extern X2C_BOOLEAN FileSys_GetLabel(X2C_CHAR, X2C_CHAR [], X2C_CARD32);


extern void FileSys_BEGIN(void);


#endif /* FileSys_H_ */
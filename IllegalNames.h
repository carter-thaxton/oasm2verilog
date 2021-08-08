
#ifndef ILLEGAL_NAMES_H
#define ILLEGAL_NAMES_H

extern void InitializeIllegalNames();
extern bool IsReservedVerilogName(const char *name);
extern bool IsIllegalSignalName(const char *name);
extern bool IsIllegalModuleName(const char *name);

#endif


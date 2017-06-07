#ifndef PROM_IF_IFC_H
#define PROM_IF_IFC_H

extern void PromIfInit(HWND hwnd, void (*rx)(HWND, BYTE*, DWORD));
extern BYTE PromIfReadData(BYTE *data, DWORD length);
extern BOOL PromIfWriteData(HANDLE hFile, BYTE *data, DWORD length, LPOVERLAPPED lpOverlapped);

#endif


#ifndef INFO_BUFFER_IFC_H
#define INFO_BUFFER_IFC_H

extern void InfoBufferInit(void);
extern void InfoBufferAdd(LPCTSTR szStr, int iMaxLength);
extern int InfoBufferGetEntryCounter(void);
extern TCHAR *InfoBufferGetString(int iNum);

#endif


#include "InfoBuffer.h"
#include "InfoBuffer_par.h"

TCHAR szText[MAX_LINES][MAX_TEXT_LENGTH];

int LineCnt = 0;

void InfoBufferInit(void)
{
    memset(&szText[0][0], 0, sizeof(szText));
    LineCnt = 0;
    return;
}

void InfoBufferAdd(LPCTSTR szStr, int iMaxLength)
{
    if ((NULL == szStr) || (0 == iMaxLength))
    {
        /* data not valid */
    }
    else
    {
        if (iMaxLength <= MAX_TEXT_LENGTH)
        {
            memcpy(&szText[LineCnt][0], &szStr[0], (iMaxLength * sizeof(TCHAR)));
        }
        else
        {
            memcpy(&szText[LineCnt][0], &szStr[0], MAX_TEXT_LENGTH);
        }

        ++LineCnt;

        if (LineCnt < MAX_LINES)
        {
            /* no overflow */
        }
        else
        {
            memcpy(&szText[0][0], &szText[MAX_LINES - 1][0], (MAX_TEXT_LENGTH * sizeof(TCHAR)));
            LineCnt = 1;
        }
    }

    return;
}

int InfoBufferGetEntryCounter(void)
{
    return LineCnt;
}

TCHAR *InfoBufferGetString(int iNum)
{
    TCHAR *szStr = NULL;

    if ((iNum < MAX_LINES) && (iNum < LineCnt))
    {
        szStr = &szText[iNum][0];
    }
    else
    {
        szStr = NULL;
    }

    return &szStr[0];
}


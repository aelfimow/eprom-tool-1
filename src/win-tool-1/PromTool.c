#include <windows.h>
#include <process.h>
#include "PromIf_ifc.h"
#include "InfoBuffer_ifc.h"

#define MAX_POST_RETRIES    (3u)
#define MAX_RETRY_DELAY     (100u)  /* in ms */

#define MAX_DATA_TO_WRITE_LENGTH    (256u)

#define MAX_DISPLAY_TEXT    (1024)

#define WM_UART_RX_VALID_PACKET     (WM_USER + 1u)

struct tThreadParams
{
    HANDLE event;
    HWND hwnd;
};

enum tPromCommand
{
    E_CMD_NOP = 0u,
    E_CMD_READ_OUT = 1u,
    E_CMD_READ_OUT_READY = 2u,
    E_CMD_WRITE = 3u,
    E_CMD_WRITE_READY = 4u
};

enum tIdmItem
{
    IDM_BASE = 40000u,
    IDM_APP_EXIT,
    IDM_PROM_READ_OUT,
    IDM_PROM_WRITE
};

static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
static HMENU PromToolMenu(void);

static VOID InitUartDCB(DCB *dcb);
static DWORD WINAPI UartThread(LPVOID pvoid);
static void UartRxData(HWND hwnd, BYTE *data, DWORD length);
static void UartReadLoop(HANDLE hFile, enum tPromCommand ExpectedAns);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
    static const TCHAR szAppName[] = TEXT("PromTool");
    HWND hwnd = NULL;
    MSG msg;
    WNDCLASS wndclass;

    if (NULL == hPrevInstance)
    {
    }
    else
    {
    }

    if (NULL == szCmdLine)
    {
    }
    else
    {
    }

    wndclass.style = (CS_HREDRAW | CS_VREDRAW);
    wndclass.lpfnWndProc = &WndProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.hInstance = hInstance;
    wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wndclass.lpszMenuName = NULL;
    wndclass.lpszClassName = &szAppName[0];

    if (0 == RegisterClass(&wndclass))
    {
        (void)MessageBox(NULL, TEXT("Error in RegisterClass."), &szAppName[0], MB_ICONERROR);
        return 0;
    }
    else
    {
        /* RegisterClass's return value is ok */
    }

    hwnd = CreateWindow(
            &szAppName[0],
            &szAppName[0],
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            NULL,
            PromToolMenu(),
            hInstance,
            NULL);

    if (NULL == hwnd)
    {
        (void)MessageBox(NULL, TEXT("Error in CreateWindow."), &szAppName[0], MB_ICONERROR);
        return 0;
    }
    else
    {
        /* CreateWindow's return value is ok */
    }

    ShowWindow(hwnd, iCmdShow);

    if (FALSE == UpdateWindow(hwnd))
    {
        (void)MessageBox(NULL, TEXT("Error in UpdateWindow."), &szAppName[0], MB_ICONERROR);
        return 0;
    }
    else
    {
        /* UpdateWindow's return value is ok */
    }

    while (0 != GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return msg.wParam;
}

static HMENU PromToolMenu(void)
{
    HMENU hMenu = CreateMenu();
    HMENU hMenuPopup = CreateMenu();

    (void)AppendMenu(hMenuPopup, MF_SEPARATOR, 0, NULL);
    (void)AppendMenu(hMenuPopup, MF_STRING, IDM_APP_EXIT, TEXT("&Exit"));

    (void)AppendMenu(hMenu, MF_POPUP, (UINT)hMenuPopup, TEXT("&File"));

    hMenuPopup = CreateMenu();

    (void)AppendMenu(hMenuPopup, MF_SEPARATOR, 0, NULL);

    (void)AppendMenu(hMenu, MF_POPUP, (UINT)hMenuPopup, TEXT("&Edit"));

    hMenuPopup = CreateMenu();

    (void)AppendMenu(hMenuPopup, MF_STRING, IDM_PROM_READ_OUT, TEXT("&Read out"));
    (void)AppendMenu(hMenuPopup, MF_STRING, IDM_PROM_WRITE, TEXT("&Write"));

    (void)AppendMenu(hMenu, MF_POPUP, (UINT)hMenuPopup, TEXT("&Command"));

    return hMenu;
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static const TCHAR szDataToWriteFileName[] = TEXT("DataToWrite.bin");
    static HANDLE hFileLog = INVALID_HANDLE_VALUE;
    static HANDLE hDataToWrite = INVALID_HANDLE_VALUE;
    static struct tThreadParams ThreadParams;
    static DWORD ValidPacketCnt = 0u;
    static int cxChar = 0;
    static int cyChar = 0;
    static int cxCaps = 0;
    static DWORD UartThreadId = 0;
    static HANDLE hUartThread = NULL;
    static BYTE DataToWrite[MAX_DATA_TO_WRITE_LENGTH];
    static DWORD cbWrittenPacketCnt = 0UL;

    TCHAR Text[MAX_DISPLAY_TEXT];

    PAINTSTRUCT ps;
    HDC hdc = NULL;
    TEXTMETRIC tm;
    RECT rect;
    int y = 0;
    int iLines = 0;
    int iLength = 0;
    int iStart = 0;
    TCHAR *szStr = NULL;
    LRESULT result = 0;
    DWORD Length = 0;
    BYTE *DataPacket = NULL;
    DWORD WrittenResult = 0u;
    enum tPromCommand ans = E_CMD_NOP;
    HMENU hMenu = NULL;
    DWORD cbReadBytes = 0;

    if (WM_CREATE == message)
    {
        InfoBufferInit();

        PromIfInit(hwnd, &UartRxData);
        ValidPacketCnt = 0u;

        ThreadParams.hwnd = hwnd;
        ThreadParams.event = CreateEvent(NULL, FALSE, FALSE, NULL);

        hUartThread = CreateThread(NULL, 0, &UartThread, (LPVOID)&ThreadParams, 0, &UartThreadId);

        WaitForSingleObject(ThreadParams.event, INFINITE);

        hdc = GetDC(hwnd);
        GetTextMetrics(hdc, &tm);
        ReleaseDC(hwnd, hdc);

        cxChar = tm.tmAveCharWidth;
        cyChar = (tm.tmHeight + tm.tmExternalLeading);

        if (0 == (tm.tmPitchAndFamily & 1u))
        {
            cxCaps = cxChar;
        }
        else
        {
            cxCaps = ((3 * cxChar) / 2);
        }

        result = 0;
    }
    else if (WM_SETFOCUS == message)
    {
        result = 0;
    }
    else if (WM_SIZE == message)
    {
        result = 0;
    }
    else if (WM_UART_RX_VALID_PACKET == message)
    {
        Length = (DWORD)wParam;
        DataPacket = (BYTE*)lParam;

        ans = (enum tPromCommand)DataPacket[0];

        if (Length > 0)
        {
            --Length;
        }
        else
        {
        }

        switch (ans)
        {
            case E_CMD_READ_OUT:
                {
                    if ((0 == Length) || (INVALID_HANDLE_VALUE == hFileLog))
                    {
                        /* no data */
                    }
                    else
                    {
                        WriteFile(hFileLog, (LPVOID)&DataPacket[1u], Length, &WrittenResult, NULL);

                        ++ValidPacketCnt;

                        (void)wsprintf(&Text[0], TEXT("Valid packet counter: %u, data length is %u"), ValidPacketCnt, Length);
                        InfoBufferAdd(&Text[0], MAX_DISPLAY_TEXT);

                        InvalidateRect(hwnd, NULL, TRUE);
                    }
                    break;
                }

            case E_CMD_READ_OUT_READY:
                {
                    if (INVALID_HANDLE_VALUE == hFileLog)
                    {
                    }
                    else
                    {
                        CloseHandle(hFileLog);
                        hFileLog = INVALID_HANDLE_VALUE;
                    }

                    (void)wsprintf(&Text[0], TEXT("Read out complete. %u valid packets."), ValidPacketCnt);
                    ValidPacketCnt = 0;
                    InfoBufferAdd(&Text[0], MAX_DISPLAY_TEXT);
                    InvalidateRect(hwnd, NULL, TRUE);

                    hMenu = GetMenu(hwnd);
                    EnableMenuItem(hMenu, IDM_PROM_READ_OUT, MF_ENABLED);
                    EnableMenuItem(hMenu, IDM_PROM_WRITE, MF_ENABLED);

                    break;
                }

            case E_CMD_WRITE:
                {
                    break;
                }

            case E_CMD_WRITE_READY:
                {
                    if (INVALID_HANDLE_VALUE == hDataToWrite)
                    {
                        (void)wsprintf(
                                &Text[0],
                                TEXT("Data file %s is not opened. Write process finished."),
                                &szDataToWriteFileName[0]);
                        InfoBufferAdd(&Text[0], MAX_DISPLAY_TEXT);
                        InvalidateRect(hwnd, NULL, TRUE);
                    }
                    else
                    {
                        ++cbWrittenPacketCnt;

                        (void)wsprintf(
                                &Text[0],
                                TEXT("Written data packet counter %u."),
                                cbWrittenPacketCnt);

                        InfoBufferAdd(&Text[0], MAX_DISPLAY_TEXT);
                        InvalidateRect(hwnd, NULL, TRUE);

                        if (FALSE == ReadFile(
                                    hDataToWrite,
                                    &DataToWrite[0],
                                    MAX_DATA_TO_WRITE_LENGTH,
                                    &cbReadBytes, NULL))
                        {
                            (void)wsprintf(
                                    &Text[0],
                                    TEXT("Error. Could not read file %s."),
                                    &szDataToWriteFileName[0]);
                            InfoBufferAdd(&Text[0], MAX_DISPLAY_TEXT);
                            InvalidateRect(hwnd, NULL, TRUE);
                            CloseHandle(hDataToWrite);
                            hDataToWrite = INVALID_HANDLE_VALUE;
                        }
                        else
                        {
                            if (0 == cbReadBytes)
                            {
                                (void)wsprintf(
                                        &Text[0],
                                        TEXT("Ready writing data from %s."),
                                        &szDataToWriteFileName[0]);
                                InfoBufferAdd(&Text[0], MAX_DISPLAY_TEXT);
                                InvalidateRect(hwnd, NULL, TRUE);
                                CloseHandle(hDataToWrite);
                                hDataToWrite = INVALID_HANDLE_VALUE;

                                hMenu = GetMenu(hwnd);
                                EnableMenuItem(hMenu, IDM_PROM_READ_OUT, MF_ENABLED);
                                EnableMenuItem(hMenu, IDM_PROM_WRITE, MF_ENABLED);
                            }
                            else if (FALSE == PostThreadMessage(
                                        UartThreadId,
                                        (UINT)IDM_PROM_WRITE,
                                        (WPARAM)cbReadBytes,
                                        (LPARAM)&DataToWrite[0]))
                            {
                                (void)wsprintf(
                                        &Text[0],
                                        TEXT("Could not send WRITE command to PROM device."));
                                InfoBufferAdd(&Text[0], MAX_DISPLAY_TEXT);
                                InvalidateRect(hwnd, NULL, TRUE);
                                CloseHandle(hDataToWrite);
                                hDataToWrite = INVALID_HANDLE_VALUE;
                            }
                            else
                            {
                                /* the write command successfully posted to UART thread */
                                (void)wsprintf(
                                        &Text[0],
                                        TEXT("%u Bytes. Wait."),
                                        cbReadBytes);
                                InfoBufferAdd(&Text[0], MAX_DISPLAY_TEXT);
                                InvalidateRect(hwnd, NULL, TRUE);
                            }
                        }
                    }
                    break;
                }

            default:
                {
                    break;
                }
        }

        result = 0;
    }
    else if (WM_COMMAND == message)
    {
        enum tIdmItem IdmItem = (enum tIdmItem)LOWORD(wParam);

        switch (IdmItem)
        {
            case IDM_BASE:
                {
                    break;
                }

            case IDM_APP_EXIT:
                {
                    PostMessage(hwnd, WM_CLOSE, 0, 0);
                    break;
                }

            case IDM_PROM_READ_OUT:
                {
                    ValidPacketCnt = 0;

                    if (INVALID_HANDLE_VALUE == hFileLog)
                    {
                    }
                    else
                    {
                        CloseHandle(hFileLog);
                    }

                    hFileLog = CreateFile(
                            TEXT("log.txt"),
                            GENERIC_WRITE,
                            FILE_SHARE_WRITE,
                            NULL,
                            CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);

                    if (FALSE == PostThreadMessage(UartThreadId, (UINT)IDM_PROM_READ_OUT, 0, 0))
                    {
                        (void)wsprintf(&Text[0], TEXT("Could not send READ OUT command to PROM device."));
                        InfoBufferAdd(&Text[0], MAX_DISPLAY_TEXT);
                        InvalidateRect(hwnd, NULL, TRUE);
                    }
                    else
                    {
                        hMenu = GetMenu(hwnd);
                        EnableMenuItem(hMenu, IDM_PROM_READ_OUT, MF_GRAYED);
                        EnableMenuItem(hMenu, IDM_PROM_WRITE, MF_GRAYED);
                    }
                    break;
                }

            case IDM_PROM_WRITE:
                {
                    cbWrittenPacketCnt = 0UL;

                    if (INVALID_HANDLE_VALUE == hDataToWrite)
                    {
                    }
                    else
                    {
                        CloseHandle(hDataToWrite);
                    }

                    hDataToWrite = CreateFile(
                            &szDataToWriteFileName[0],
                            GENERIC_READ,
                            FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);

                    if (INVALID_HANDLE_VALUE == hDataToWrite)
                    {
                        (void)wsprintf(
                                &Text[0],
                                TEXT("Error. Could not open file %s."),
                                &szDataToWriteFileName[0]);
                        InfoBufferAdd(&Text[0], MAX_DISPLAY_TEXT);
                        InvalidateRect(hwnd, NULL, TRUE);
                    }
                    else if (FALSE == PostThreadMessage(UartThreadId, (UINT)IDM_PROM_WRITE, 0, 0))
                    {
                        (void)wsprintf(
                                &Text[0],
                                TEXT("Could not send WRITE command to PROM device."));
                        InfoBufferAdd(&Text[0], MAX_DISPLAY_TEXT);
                        InvalidateRect(hwnd, NULL, TRUE);
                        CloseHandle(hDataToWrite);
                        hDataToWrite = INVALID_HANDLE_VALUE;
                    }
                    else
                    {
                        hMenu = GetMenu(hwnd);
                        EnableMenuItem(hMenu, IDM_PROM_READ_OUT, MF_GRAYED);
                        EnableMenuItem(hMenu, IDM_PROM_WRITE, MF_GRAYED);

                        (void)wsprintf(
                                &Text[0],
                                TEXT("File %s opened. Writing data. "),
                                &szDataToWriteFileName[0]);
                        InfoBufferAdd(&Text[0], MAX_DISPLAY_TEXT);
                        InvalidateRect(hwnd, NULL, TRUE);
                    }
                    break;
                }

            default:
                {
                    break;
                }
        }

        result = 0;
    }
    else if (WM_PAINT == message)
    {
        hdc = BeginPaint(hwnd, &ps);

        if (FALSE == GetClientRect(hwnd, &rect))
        {
        }
        else
        {
            if ((0 == cxChar) || (0 == cyChar))
            {
                /* font width and height is invalid */
            }
            else
            {
                if (rect.bottom <= rect.top)
                {
                    /* the rectangular area is somehow not big enough? */
                }
                else
                {
                    iLines = ((rect.bottom - rect.top) / cyChar);

                    if (iLines < InfoBufferGetEntryCounter())
                    {
                        iStart = (InfoBufferGetEntryCounter() - iLines);
                    }
                    else
                    {
                        iStart = 0;
                    }

                    for (y = 0; y < rect.bottom; y += cyChar)
                    {
                        szStr = InfoBufferGetString(iStart);
                        if (NULL == szStr)
                        {
                        }
                        else
                        {
                            iLength = lstrlen(&szStr[0]);
                            TextOut(hdc, cxCaps, y, &szStr[0], iLength);
                        }
                        ++iStart;
                    }
                }
            }
        }

        EndPaint(hwnd, &ps);

        result = 0;
    }
    else if (WM_DESTROY == message)
    {
        if (INVALID_HANDLE_VALUE == hFileLog)
        {
            /* Log-file already closed */
        }
        else
        {
            CloseHandle(hFileLog);
            hFileLog = INVALID_HANDLE_VALUE;
        }

        if (INVALID_HANDLE_VALUE == hDataToWrite)
        {
            /* data to write file already closed */
        }
        else
        {
            CloseHandle(hDataToWrite);
            hDataToWrite = INVALID_HANDLE_VALUE;
        }

        PostQuitMessage(0);

        result = 0;
    }
    else
    {
        result = DefWindowProc(hwnd, message, wParam, lParam);
    }

    return result;
}

static VOID InitUartDCB(DCB *dcb)
{
    dcb[0].DCBlength = sizeof(DCB);
    dcb[0].BaudRate = CBR_115200;
    dcb[0].fBinary = TRUE;
    dcb[0].fParity = FALSE;
    dcb[0].fOutxCtsFlow = FALSE;
    dcb[0].fOutxDsrFlow = FALSE;
    dcb[0].fDtrControl = DTR_CONTROL_DISABLE;
    dcb[0].fDsrSensitivity = FALSE;
    dcb[0].fTXContinueOnXoff = FALSE;
    dcb[0].fOutX = FALSE;
    dcb[0].fInX = FALSE;
    dcb[0].fErrorChar = FALSE;
    dcb[0].fNull = FALSE;
    dcb[0].fRtsControl = RTS_CONTROL_DISABLE;
    dcb[0].fAbortOnError = FALSE;
    dcb[0].fDummy2 = 0;
    dcb[0].wReserved = 0;
    dcb[0].XonLim = 0;
    dcb[0].XoffLim = 0;
    dcb[0].ByteSize = 8u;
    dcb[0].Parity = NOPARITY;
    dcb[0].StopBits = ONESTOPBIT;
    dcb[0].XonChar = 0;
    dcb[0].XoffChar = 0;
    dcb[0].ErrorChar = 0;
    dcb[0].EofChar = 0;
    dcb[0].EvtChar = 0;
    dcb[0].wReserved1 = 0;

    return;
}

static DWORD WINAPI UartThread(LPVOID pvoid)
{
    static const TCHAR szThreadName[] = TEXT("UartThread");
    static const TCHAR szPortName[] = TEXT("\\\\.\\COM4");
    DCB dcb;
    struct tThreadParams *params = (struct tThreadParams*)pvoid;
    BYTE TxData[16384u];
    DWORD cbTxDataIndex = 0;
    HANDLE hFileCom = NULL;
    MSG msg;
    enum tIdmItem PromCmd = IDM_BASE;
    BOOL done = FALSE;
    BYTE *pDataToWrite = NULL;
    DWORD cbDataToWrite = 0;
    DWORD i = 0;

    PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

    if (NULL == params)
    {
    }
    else
    {
        SetEvent(params[0].event);
    }

    InitUartDCB(&dcb);

    hFileCom = CreateFile(
            &szPortName[0],
            (GENERIC_WRITE | GENERIC_READ),
            FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            0,
            NULL);

    if (INVALID_HANDLE_VALUE == hFileCom)
    {
        (void)MessageBox(NULL, TEXT("Could not open serial port."), &szThreadName[0], MB_ICONERROR);
        ExitThread(0);
    }
    else
    {
        /* CreateFile's return value is ok */
    }

    if (0 == SetCommState(hFileCom, &dcb))
    {
        (void)MessageBox(NULL, TEXT("Could not initialize serial port."), &szThreadName[0], MB_ICONERROR);
        ExitThread(0);
    }
    else
    {
        /* SetCommState's return value is ok */
    }

    while ((NULL != params) && (FALSE == done))
    {
        GetMessage(&msg, NULL, 0, 0);

        PromCmd = (enum tIdmItem)(msg.message);

        switch (PromCmd)
        {
            case IDM_BASE:
                {
                    break;
                }

            case IDM_PROM_READ_OUT:
                {
                    TxData[0] = (BYTE)E_CMD_READ_OUT;
                    if (FALSE == PromIfWriteData(hFileCom, &TxData[0], 1u))
                    {
                        (void)MessageBox(NULL, TEXT("Could not write READ OUT command."), &szThreadName[0], MB_ICONERROR);
                    }
                    else
                    {
                        UartReadLoop(hFileCom, E_CMD_READ_OUT_READY);
                    }
                    break;
                }

            case IDM_PROM_WRITE:
                {
                    cbDataToWrite = (DWORD)msg.wParam;
                    pDataToWrite = (BYTE*)msg.lParam;
                    cbTxDataIndex = 0;
                    TxData[cbTxDataIndex] = (BYTE)E_CMD_WRITE;
                    ++cbTxDataIndex;
                    if ((0 == cbDataToWrite) || (NULL == pDataToWrite))
                    {
                        if (FALSE == PromIfWriteData(hFileCom, &TxData[0], 1u))
                        {
                            (void)MessageBox(
                                    NULL,
                                    TEXT("Could not send WRITE command."),
                                    &szThreadName[0],
                                    MB_ICONERROR);
                        }
                        else
                        {
                            UartReadLoop(hFileCom, E_CMD_WRITE_READY);
                        }
                    }
                    else
                    {
                        for (i = 0; i < cbDataToWrite; ++i)
                        {
                            TxData[cbTxDataIndex] = pDataToWrite[i];
                            ++cbTxDataIndex;
                        }

                        if (FALSE == PromIfWriteData(hFileCom, &TxData[0], cbTxDataIndex))
                        {
                            (void)MessageBox(
                                    NULL,
                                    TEXT("Could not send WRITE command."),
                                    &szThreadName[0],
                                    MB_ICONERROR);
                        }
                        else
                        {
                            UartReadLoop(hFileCom, E_CMD_WRITE_READY);
                        }
                    }
                    break;
                }

            default:
                {
                    (void)MessageBox(NULL, TEXT("Unknown command!"), &szThreadName[0], MB_ICONINFORMATION);
                    break;
                }
        }
    }

    (void)MessageBox(NULL, TEXT("Exit."), &szThreadName[0], MB_ICONINFORMATION);

    CloseHandle(hFileCom);

    ExitThread(0);
    return 0;
}

static void UartReadLoop(HANDLE hFile, enum tPromCommand ExpectedAns)
{
    static BYTE RxData[16384u];
    static DWORD RxDataCnt = 0u;

    DWORD BytesRead = 0u;
    BOOL result = FALSE;
    enum tPromCommand ans = E_CMD_NOP;

    while (ExpectedAns != ans)
    {
        result = ReadFile(
                hFile,
                (LPVOID)&RxData[RxDataCnt],
                1u,
                &BytesRead,
                NULL);

        if ((FALSE != result) && (0 != BytesRead))
        {
            ans = (enum tPromCommand)PromIfReadData(&RxData[RxDataCnt], BytesRead);

            RxDataCnt += BytesRead;

            if (RxDataCnt >= ((sizeof(RxData)) / (sizeof(RxData[0]))))
            {
                RxDataCnt = 0u;
            }
            else
            {
                /* RxDataCnt range is ok */
            }
        }
        else
        {
            /* ReadFile error */
            (void)MessageBox(
                    NULL,
                    TEXT("Could not read serial port."),
                    TEXT("UartReadLoop"),
                    MB_ICONERROR);

            ans = ExpectedAns;
        }
    }

    return;
}

static void UartRxData(HWND hwnd, BYTE *data, DWORD length)
{
    if ((NULL == data) || (0 == length))
    {
        /* data is not valid */
    }
    else
    {
        PostMessage(hwnd, WM_UART_RX_VALID_PACKET, (WPARAM)length, (LPARAM)&data[0]);
    }
    return;
}


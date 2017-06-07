#include <windows.h>
#include <process.h>
#include "PromIf_ifc.h"

#define ID_EDIT     (1)

#define MAX_POST_RETRIES    (3u)
#define MAX_RETRY_DELAY     (100u)  /* in ms */

#define MAX_DATA_TO_WRITE_LENGTH    (256u)

#define MAX_UART_COM_PORTS      (16u)
#define MAX_UART_THREAD_ERR     (16u)

#define WM_UART_ERR     (WM_USER + 1u)
#define WM_UART_RX_CALLBACK     (WM_USER + 2u)

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
static void UartCallback(HWND hwnd, BYTE *pData, DWORD length);
static void UartReport(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
static void UartReadLoop(HANDLE hFile, enum tPromCommand ExpectedAns);
static void EditPrintf(HWND hwndEdit, TCHAR *szFormat, ...);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
    static const TCHAR szAppName[] = TEXT("PromTool");
    HWND hwnd = NULL;
    MSG msg;
    WNDCLASS wndclass;

    hPrevInstance = hPrevInstance;
    szCmdLine = szCmdLine;

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
    static DWORD ValidPacketCnt = 0u;
    static DWORD UartThreadId = 0;
    static HANDLE hUartThread = NULL;
    static BYTE DataToWrite[MAX_DATA_TO_WRITE_LENGTH];
    static DWORD cbWrittenPacketCnt = 0UL;

    static HWND hwndEdit = NULL;

    struct tThreadParams ThreadParams = { 0, 0 };
    DWORD Length = 0;
    BYTE *pDataPacket = NULL;
    DWORD WrittenResult = 0u;
    enum tPromCommand ans = E_CMD_NOP;
    HMENU hMenu = NULL;
    DWORD cbReadBytes = 0;

    switch (message)
    {
        case WM_CREATE:
            {
                PromIfInit(hwnd, &UartCallback);
                ValidPacketCnt = 0u;

                ThreadParams.hwnd = hwnd;
                ThreadParams.event = CreateEvent(NULL, FALSE, FALSE, NULL);

                hUartThread = CreateThread(NULL, 0, &UartThread, (LPVOID)&ThreadParams, 0, &UartThreadId);

                if (NULL == hUartThread)
                {
                    /* failed creating thread */
                }
                else
                {
                    /* thread created, wait for thread creating message queue */
                    WaitForSingleObject(ThreadParams.event, INFINITE);
                }

                hwndEdit = CreateWindow(
                        TEXT("edit"),
                        NULL,
                        (WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | WS_BORDER | \
                         ES_READONLY | ES_LEFT | ES_MULTILINE | ES_AUTOHSCROLL | ES_AUTOVSCROLL),
                        0,
                        0,
                        0,
                        0,
                        hwnd,
                        (HMENU)ID_EDIT,
                        ((LPCREATESTRUCT)lParam)->hInstance,
                        NULL);

                if (NULL == hwndEdit)
                {
                    (void)MessageBox(NULL, TEXT("Error: hwndEdit == NULL."), TEXT("WndProc"), MB_ICONERROR);
                }

                return 0;
            }

        case WM_SETFOCUS:
            {
                if (NULL != hwndEdit)
                {
                    SetFocus(hwndEdit);
                }
                return 0;
            }

        case WM_SIZE:
            {
                if (NULL != hwndEdit)
                {
                    MoveWindow(hwndEdit, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
                }
                return 0;
            }

        case WM_UART_ERR:
            {
                BOOL err = (BOOL)wParam;
                TCHAR *pName = (TCHAR *)lParam;

                if (FALSE == err)
                {
                    /* no error occured */
                    if (NULL == pName)
                    {
                        /* no message from communication thread */
                    }
                    else
                    {
                        EditPrintf(hwndEdit, TEXT("Communication message: %s"), pName);
                    }
                }
                else
                {
                    /* error occured */
                    if (NULL == pName)
                    {
                        EditPrintf(hwndEdit, TEXT("Communication error."));
                    }
                    else
                    {
                        EditPrintf(hwndEdit, TEXT("Communication error: %s"), pName);
                    }
                }

                return 0;
            }

        case WM_UART_RX_CALLBACK:
            {
                Length = (DWORD)wParam;
                pDataPacket = (BYTE*)lParam;

                /* check, if there are data available */
                if (NULL == pDataPacket)
                {
                    ans = E_CMD_NOP;
                }
                else
                {
                    ans = (enum tPromCommand)pDataPacket[0];
                }

                if (Length > 0)
                {
                    --Length;
                }
                else
                {
                    /* Length is somehow 0, do nothing here */
                }

                switch (ans)
                {
                    case E_CMD_NOP:
                        {
                            hMenu = GetMenu(hwnd);
                            EnableMenuItem(hMenu, IDM_PROM_READ_OUT, MF_ENABLED);
                            EnableMenuItem(hMenu, IDM_PROM_WRITE, MF_ENABLED);
                            break;
                        }

                    case E_CMD_READ_OUT:
                        {
                            if (((0 == Length) || (NULL == pDataPacket)) || (INVALID_HANDLE_VALUE == hFileLog))
                            {
                                /* no data */
                            }
                            else
                            {
                                WriteFile(hFileLog, (LPVOID)&pDataPacket[1u], Length, &WrittenResult, NULL);

                                ++ValidPacketCnt;

                                EditPrintf(hwndEdit, TEXT("Valid packet counter: %u, data length is %u"), ValidPacketCnt, Length);
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

                            EditPrintf(hwndEdit, TEXT("Read out complete. %u valid packets."), ValidPacketCnt);

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
                                EditPrintf(hwndEdit, TEXT("Data file %s is not opened. Write process finished."), &szDataToWriteFileName[0]);
                            }
                            else
                            {
                                ++cbWrittenPacketCnt;

                                EditPrintf(hwndEdit, TEXT("Written data packet counter %u."), cbWrittenPacketCnt);

                                if (FALSE == ReadFile(
                                            hDataToWrite,
                                            &DataToWrite[0],
                                            MAX_DATA_TO_WRITE_LENGTH,
                                            &cbReadBytes, NULL))
                                {
                                    EditPrintf(hwndEdit, TEXT("Error. Could not read file %s."), &szDataToWriteFileName[0]);

                                    CloseHandle(hDataToWrite);
                                    hDataToWrite = INVALID_HANDLE_VALUE;
                                }
                                else
                                {
                                    if (0 == cbReadBytes)
                                    {
                                        EditPrintf(hwndEdit, TEXT("Ready writing data from %s."), &szDataToWriteFileName[0]);

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
                                        EditPrintf(hwndEdit, TEXT("Could not send WRITE command to PROM device."));

                                        CloseHandle(hDataToWrite);
                                        hDataToWrite = INVALID_HANDLE_VALUE;
                                    }
                                    else
                                    {
                                        /* the write command successfully posted to UART thread */
                                        EditPrintf(hwndEdit, TEXT("%u Bytes. Wait."), cbReadBytes);
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

                return 0;
            }

        case WM_COMMAND:
            {
                enum tIdmItem IdmItem = (enum tIdmItem)LOWORD(wParam);

                /* check for errors from edit field */
                if (ID_EDIT == LOWORD(wParam))
                {
                    if ((EN_ERRSPACE == HIWORD(wParam)) || (EN_MAXTEXT == HIWORD(wParam)))
                    {
                        /* edit window has no RAM and reports error */
                        /* do nothing here */
                    }
                }

                /* check, what command is selected by user */
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

                            EditPrintf(hwndEdit, TEXT("Selected Command: Read out contents of ROM."));

                            if (INVALID_HANDLE_VALUE == hFileLog)
                            {
                                /* handle already invalid, do nothing */
                            }
                            else
                            {
                                CloseHandle(hFileLog);
                            }

                            hFileLog = CreateFile(
                                    TEXT("readout.bin"),
                                    GENERIC_WRITE,
                                    FILE_SHARE_WRITE,
                                    NULL,
                                    CREATE_ALWAYS,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL);

                            if (INVALID_HANDLE_VALUE == hFileLog)
                            {
                                EditPrintf(hwndEdit, TEXT("Could not create file for writing read out data."));
                            }
                            else if (FALSE == PostThreadMessage(UartThreadId, (UINT)IDM_PROM_READ_OUT, 0, 0))
                            {
                                EditPrintf(hwndEdit, TEXT("Could not send READ OUT command to PROM device."));
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

                            EditPrintf(hwndEdit, TEXT("Selected command: Write contents of file %s to the EPROM."), &szDataToWriteFileName[0]);

                            if (INVALID_HANDLE_VALUE == hDataToWrite)
                            {
                                /* handle already invalid, do nothing */
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
                                EditPrintf(hwndEdit, TEXT("Error. Could not open file %s."), &szDataToWriteFileName[0]);
                            }
                            else if (FALSE == PostThreadMessage(UartThreadId, (UINT)IDM_PROM_WRITE, 0, 0))
                            {
                                EditPrintf(hwndEdit, TEXT("Could not send WRITE command to PROM device."));

                                CloseHandle(hDataToWrite);
                                hDataToWrite = INVALID_HANDLE_VALUE;
                            }
                            else
                            {
                                hMenu = GetMenu(hwnd);
                                EnableMenuItem(hMenu, IDM_PROM_READ_OUT, MF_GRAYED);
                                EnableMenuItem(hMenu, IDM_PROM_WRITE, MF_GRAYED);

                                EditPrintf(hwndEdit, TEXT("File %s opened. Writing data. "), &szDataToWriteFileName[0]);
                            }
                            break;
                        }

                    default:
                        {
                            break;
                        }
                }

                return 0;
            }

        case WM_DESTROY:
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

                return 0;
            }

        default:
            {
                /* no action here */
                break;
            }
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
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
    TCHAR szPortName[256u] = { 0 };
    DCB dcb;
    struct tThreadParams params = { 0, 0 };
    BYTE TxData[16384u];
    DWORD cbTxDataIndex = 0;
    HANDLE hFileCom = INVALID_HANDLE_VALUE;
    MSG msg;
    enum tIdmItem PromCmd = IDM_BASE;
    BOOL done = FALSE;
    BYTE *pDataToWrite = NULL;
    DWORD cbDataToWrite = 0;
    DWORD i = 0;
    TCHAR errorMsg[MAX_UART_THREAD_ERR][256u];
    DWORD comCnt = 1u;
    DWORD errorCnt = 0u;

    memcpy(&params, pvoid, sizeof(struct tThreadParams));

    PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

    SetEvent(params.event);

    InitUartDCB(&dcb);

    /* This loop tries to open COM port, which is found first */
    for (i = 0u; i < MAX_UART_COM_PORTS; ++i)
    {
        if (INVALID_HANDLE_VALUE == hFileCom)
        {
            /* generate COM device name */
            memset(&szPortName[0], 0, sizeof(szPortName));
            wsprintf(&szPortName[0], TEXT("\\\\.\\COM%u"), comCnt);

            /* count next COM port */
            ++comCnt;

            /* try open it */
            hFileCom = CreateFile(
                    &szPortName[0],
                    (GENERIC_WRITE | GENERIC_READ),
                    0,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL);

            if (INVALID_HANDLE_VALUE == hFileCom)
            {
                memset(&errorMsg[errorCnt][0], 0, sizeof(errorMsg[errorCnt]));
                wsprintf(&errorMsg[errorCnt][0], TEXT("Could not open COM port %s."), &szPortName[0]);

                UartReport(params.hwnd, WM_UART_ERR, TRUE, (LPARAM)&errorMsg[errorCnt][0]);

                ++errorCnt;
                errorCnt %= MAX_UART_THREAD_ERR;
            }
            else if (0 == SetCommState(hFileCom, &dcb))
            {
                CloseHandle(hFileCom);
                hFileCom = INVALID_HANDLE_VALUE;

                memset(&errorMsg[errorCnt][0], 0, sizeof(errorMsg[errorCnt]));
                wsprintf(&errorMsg[errorCnt][0], TEXT("Could not initialize COM port %s."), &szPortName[0]);

                UartReport(params.hwnd, WM_UART_ERR, TRUE, (LPARAM)&errorMsg[errorCnt][0]);

                ++errorCnt;
                errorCnt %= MAX_UART_THREAD_ERR;
            }
            else
            {
                /* SetCommState's return value is ok */
                memset(&errorMsg[errorCnt][0], 0, sizeof(errorMsg[errorCnt]));
                wsprintf(&errorMsg[errorCnt][0], TEXT("Initialization successfull for %s."), &szPortName[0]);

                UartReport(params.hwnd, WM_UART_ERR, FALSE, (LPARAM)&errorMsg[errorCnt][0]);

                ++errorCnt;
                errorCnt %= MAX_UART_THREAD_ERR;
            }
        }
    }

    while (FALSE == done)
    {
        if (FALSE == PeekMessage(&msg, (HWND)-1, 0, 0, PM_REMOVE))
        {
            /* no message available, do nothing */
            Sleep(10);
        }
        else
        {
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
                        if (FALSE == PromIfWriteData(hFileCom, &TxData[0], 1u, NULL))
                        {
                            /* Report error message */
                            memset(&errorMsg[errorCnt][0], 0, sizeof(errorMsg[errorCnt]));
                            wsprintf(&errorMsg[errorCnt][0], TEXT("Could not send READ OUT command."));

                            UartReport(params.hwnd, WM_UART_ERR, TRUE, (LPARAM)&errorMsg[errorCnt][0]);

                            ++errorCnt;
                            errorCnt %= MAX_UART_THREAD_ERR;

                            UartCallback(params.hwnd, NULL, 0);
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
                            if (FALSE == PromIfWriteData(hFileCom, &TxData[0], 1u, NULL))
                            {
                                /* Report error message */
                                memset(&errorMsg[errorCnt][0], 0, sizeof(errorMsg[errorCnt]));
                                wsprintf(&errorMsg[errorCnt][0], TEXT("Could not send WRITE command."));

                                UartReport(params.hwnd, WM_UART_ERR, TRUE, (LPARAM)&errorMsg[errorCnt][0]);

                                ++errorCnt;
                                errorCnt %= MAX_UART_THREAD_ERR;

                                UartCallback(params.hwnd, NULL, 0);
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

                            if (FALSE == PromIfWriteData(hFileCom, &TxData[0], cbTxDataIndex, NULL))
                            {
                                /* Report error message */
                                memset(&errorMsg[errorCnt][0], 0, sizeof(errorMsg[errorCnt]));
                                wsprintf(&errorMsg[errorCnt][0], TEXT("Could not send WRITE command."));

                                UartReport(params.hwnd, WM_UART_ERR, TRUE, (LPARAM)&errorMsg[errorCnt][0]);

                                ++errorCnt;
                                errorCnt %= MAX_UART_THREAD_ERR;

                                UartCallback(params.hwnd, NULL, 0);
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
                        /* Report error message */
                        memset(&errorMsg[errorCnt][0], 0, sizeof(errorMsg[errorCnt]));
                        wsprintf(&errorMsg[errorCnt][0], TEXT("Unknown command received."));

                        UartReport(params.hwnd, WM_UART_ERR, TRUE, (LPARAM)&errorMsg[errorCnt][0]);

                        ++errorCnt;
                        errorCnt %= MAX_UART_THREAD_ERR;
                        break;
                    }
            }
        }
    }

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
            RxDataCnt %= ((sizeof(RxData)) / (sizeof(RxData[0])));
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

static void UartCallback(HWND hwnd, BYTE *pData, DWORD length)
{
    static const DWORD maxRetry = 3u;
    DWORD i = 0u;
    BOOL result = FALSE;

    for (i = 0u; i < maxRetry; ++i)
    {
        if (FALSE == result)
        {
            result = PostMessage(hwnd, WM_UART_RX_CALLBACK, (WPARAM)length, (LPARAM)pData);

            /* if posting failed, wait a bit before proceeding */
            if (FALSE == result)
            {
                Sleep(100);
            }
        }
    }

    return;
}

static void UartReport(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static const DWORD maxRetry = 3u;
    DWORD i = 0u;
    BOOL result = FALSE;

    for (i = 0u; i < maxRetry; ++i)
    {
        if (FALSE == result)
        {
            result = PostMessage(hwnd, message, wParam, lParam);

            /* if posting failed, wait a bit before proceeding */
            if (FALSE == result)
            {
                Sleep(100);
            }
        }
    }

    return;
}

static void EditPrintf(HWND hwndEdit, TCHAR *szFormat, ...)
{
    static TCHAR szEndLine[] = TEXT("\r\n");
    TCHAR szBuffer[1024u];
    va_list pArgList;

    if (NULL != hwndEdit)
    {
        va_start(pArgList, szFormat);
        memset(&szBuffer[0], 0, sizeof(szBuffer));
        wvsprintf(&szBuffer[0], szFormat, pArgList);
        va_end(pArgList);

        SendMessage(hwndEdit, EM_SETSEL, (WPARAM)-1, (WPARAM)-1);
        SendMessage(hwndEdit, EM_REPLACESEL, FALSE, (LPARAM)&szBuffer[0]);
        SendMessage(hwndEdit, EM_SCROLLCARET, 0, 0);

        SendMessage(hwndEdit, EM_SETSEL, (WPARAM)-1, (WPARAM)-1);
        SendMessage(hwndEdit, EM_REPLACESEL, FALSE, (LPARAM)&szEndLine[0]);
        SendMessage(hwndEdit, EM_SCROLLCARET, 0, 0);
    }
}


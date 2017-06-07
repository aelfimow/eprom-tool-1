#include "PromIf.h"
#include "PromIf_par.h"

#define DEBUG_PRINT(str)    (void)MessageBox(NULL, (str), TEXT("PromIf FSM"), MB_ICONERROR)

static void (*RxFunc)(HWND, BYTE *, DWORD) = NULL;
static volatile HWND RxHwnd = NULL;

static BYTE RxData[MAX_RX_PACKETS][MAX_RX_PACKET_SIZE];
static volatile DWORD RxPacketCnt = 0;
static volatile DWORD RxDataCnt = 0;

static BYTE TxData[MAX_TX_PACKET_SIZE];

static volatile enum tPromIfRxState RxState = E_RX_START;

static volatile BOOL fMagicValid = FALSE;
static volatile DWORD RxLengthCnt = 0u;
static volatile DWORD RxLength = 0u;

static BYTE RxCheckSum[MAX_CHECKSUM_LENGTH];
static volatile DWORD RxCheckSumCnt = 0;

void PromIfInit(HWND hwnd, void (*rx)(HWND, BYTE*, DWORD))
{
    RxHwnd = hwnd;
    RxFunc = rx;
    RxState = E_RX_START;
    memset(&RxData[0][0], 0, sizeof(RxData));
    RxPacketCnt = 0;
    RxDataCnt = 0;
    fMagicValid = FALSE;
    RxLengthCnt = 0u;
    RxLength = 0u;
    memset(&RxCheckSum[0], 0, sizeof(RxCheckSum));
    RxCheckSumCnt = 0;
    return;
}

BYTE PromIfReadData(BYTE *data, DWORD length)
{
    DWORD i = 0;
    BYTE ret = 0;

    for (i = 0; i < length; ++i)
    {
        switch (RxState)
        {
            case E_RX_START:
                {
                    if (FALSE == fMagicValid)
                    {
                        if (MAGIC_START_A == data[i])
                        {
                            fMagicValid = TRUE;
                        }
                        else
                        {
#if 0
                            TCHAR Buffer[256u];
                            wsprintf(&Buffer[0], TEXT("MAGIC_START_A is invalid. data[%u] = %X."), i, data[i]);
                            DEBUG_PRINT(&Buffer[0]);
#endif
                            fMagicValid = FALSE;
                        }
                    }
                    else
                    {
                        fMagicValid = FALSE;

                        if (MAGIC_START_B == data[i])
                        {
                            RxLengthCnt = 0u;
                            RxLength = 0u;
                            RxState = E_RX_LENGTH;
                        }
                        else
                        {
                            TCHAR Buffer[256u];
                            (void)wsprintf(&Buffer[0], TEXT("MAGIC_START_B is invalid. data[%u] = %X."), i, data[i]);
                            DEBUG_PRINT(&Buffer[0]);
                        }
                    }
                    break;
                }

            case E_RX_LENGTH:
                {
                    DWORD val = (DWORD)data[i];

                    ++RxLengthCnt;

                    if (1u == RxLengthCnt)
                    {
                        RxLength = (DWORD)(val * 16777216UL);
                    }
                    else if (2u == RxLengthCnt)
                    {
                        RxLength += (DWORD)(val * 65536UL);
                    }
                    else if (3u == RxLengthCnt)
                    {
                        RxLength += (DWORD)(val * 256UL);
                    }
                    else if (4u == RxLengthCnt)
                    {
                        RxLength += val;

                        if ((RxLength < MAX_RX_PACKET_SIZE) && (0 != RxLength))
                        {
                            RxDataCnt = 0;
                            RxState = E_RX_DATA;
                        }
                        else
                        {
                            DEBUG_PRINT(TEXT("RxLength is 0 or greater than MAX_RX_PACKET_SIZE."));
                            RxState = E_RX_START;
                        }
                    }
                    else
                    {
                        DEBUG_PRINT(TEXT("RxLengthCnt is invalid."));
                        RxState = E_RX_START;
                    }
                    
                    break;
                }

            case E_RX_DATA:
                {
                    RxData[RxPacketCnt][RxDataCnt] = data[i];

                    ++RxDataCnt;

                    if (RxDataCnt >= RxLength)
                    {
                        RxCheckSumCnt = 0;
                        RxState = E_RX_CHECKSUM;
                    }
                    else
                    {
                    }

                    break;
                }

            case E_RX_CHECKSUM:
                {
                    RxCheckSum[RxCheckSumCnt] = data[i];

                    ++RxCheckSumCnt;

                    if (RxCheckSumCnt >= MAX_CHECKSUM_LENGTH)
                    {
                        fMagicValid = FALSE;
                        RxState = E_RX_END;
                    }
                    else
                    {
                        /* no overflow of the checksum counter */
                    }

                    break;
                }

            case E_RX_END:
                {
                    if (FALSE == fMagicValid)
                    {
                        if (MAGIC_END_A == data[i])
                        {
                            fMagicValid = TRUE;
                        }
                        else
                        {
                            TCHAR Buffer[256];
                            (void)wsprintf(&Buffer[0], TEXT("MAGIC_END_A is invalid. data[%u]: %X"), i, data[i]);
                            DEBUG_PRINT(&Buffer[0]);
                            fMagicValid = FALSE;
                            RxState = E_RX_START;
                        }
                    }
                    else
                    {
                        fMagicValid = FALSE;
                        RxState = E_RX_START;

                        if (MAGIC_END_B == data[i])
                        {
                            ret = RxData[RxPacketCnt][0];

                            if (NULL == RxFunc)
                            {
                                /* rx function is not valid */
                            }
                            else
                            {
                                if (RxDataCnt < MAX_RX_PACKET_SIZE)
                                {
                                    RxFunc(RxHwnd, &RxData[RxPacketCnt][0], RxDataCnt);

                                    ++RxPacketCnt;

                                    if (RxPacketCnt < MAX_RX_PACKETS)
                                    {
                                        /* no overflow of the packet counter */
                                    }
                                    else
                                    {
                                        RxPacketCnt = 0;
                                    }
                                }
                                else
                                {
                                    DEBUG_PRINT(TEXT("RxPacketCnt is invalid."));
                                    /* received data count is somehow too large */
                                }
                            }

                        }
                        else
                        {
                            DEBUG_PRINT(TEXT("MAGIC_END_B is invalid."));
                            /* received magic word is not valid */
                        }
                    }

                    break;
                }

            default:
                {
                    DEBUG_PRINT(TEXT("Invalid RxState."));
                    fMagicValid = FALSE;
                    RxState = E_RX_START;
                    break;
                }
        }
    }
    
    return ret;
}

BOOL PromIfWriteData(HANDLE hFile, BYTE *data, DWORD length)
{
    DWORD cbIndex = 0;
    DWORD cbWritten = 0;
    DWORD i = 0;

    TxData[cbIndex] = MAGIC_START_A;
    ++cbIndex;
    TxData[cbIndex] = MAGIC_START_B;
    ++cbIndex;

    TxData[cbIndex] = (BYTE)((length / 16777216UL) & LENGTH_MASK);
    ++cbIndex;
    TxData[cbIndex] = (BYTE)((length / 65536UL) & LENGTH_MASK);
    ++cbIndex;
    TxData[cbIndex] = (BYTE)((length / 256UL) & LENGTH_MASK);
    ++cbIndex;
    TxData[cbIndex] = (BYTE)((length / 1UL) & LENGTH_MASK);
    ++cbIndex;

    for (i = 0u; i < length; ++i)
    {
        TxData[cbIndex] = data[i];
        ++cbIndex;
    }

    TxData[cbIndex] = 0;
    ++cbIndex;
    TxData[cbIndex] = 0;
    ++cbIndex;
    TxData[cbIndex] = 0;
    ++cbIndex;
    TxData[cbIndex] = 0;
    ++cbIndex;

    TxData[cbIndex] = MAGIC_END_A;
    ++cbIndex;
    TxData[cbIndex] = MAGIC_END_B;
    ++cbIndex;

    return WriteFile(hFile, &TxData[0], cbIndex, &cbWritten, NULL);
}


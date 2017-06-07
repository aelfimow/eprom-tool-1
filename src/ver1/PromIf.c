#include "PromIf.h"
#include "PromIf_par.h"

enum tPromIfRxState RxState = E_RX_START;
void (*RxCallback)(INT8U *, INT32U) = NULL;
BOOLEAN fMagicValid = FALSE;
INT8U RxLengthCnt = 0u;
INT32U RxLength = 0UL;
INT32U RxDataCnt = 0UL;
INT8U RxCheckSumCnt = 0u;
INT8U RxData[MAX_RX_PACKET_SIZE];
INT8U RxCheckSum[MAX_CHECK_SUM_LENGTH];

INT8U LenField[MAX_DATA_LENGTH];
INT8U CheckSumField[MAX_CHECK_SUM_LENGTH];

void PromIfInit(void (*Callback)(INT8U *, INT32U))
{
    RxState = E_RX_START;
    fMagicValid = FALSE;
    RxLengthCnt = 0u;
    RxLength = 0UL;
    RxDataCnt = 0UL;
    RxCheckSumCnt = 0u;
    RxCallback = Callback;
    return;
}

void PromIfWrite(const INT8U *data, const INT32U length)
{
    INT32U i = 0UL;
    INT8U val = 0u;

    UsartTx(MAGIC_START_A);
    UsartTx(MAGIC_START_B);

    for (i = 0u; i < MAX_DATA_LENGTH; ++i)
    {
        LenField[i] = 0;
    }

    i = 0;
    LenField[i] = (INT8U)((length / 16777216UL) & BYTE_MASK);
    ++i;
    LenField[i] = (INT8U)((length / 65536UL) & BYTE_MASK);
    ++i;
    LenField[i] = (INT8U)((length / 256UL) & BYTE_MASK);
    ++i;
    LenField[i] = (INT8U)(length & BYTE_MASK);

    UsartTx(LenField[0u]);
    UsartTx(LenField[1u]);
    UsartTx(LenField[2u]);
    UsartTx(LenField[3u]);

    for (i = 0u; i < length; ++i)
    {
        val = data[i];
        UsartTx(val);
    }

    for (i = 0u; i < MAX_CHECK_SUM_LENGTH; ++i)
    {
        CheckSumField[i] = 0;
    }

    UsartTx(CheckSumField[0u]);
    UsartTx(CheckSumField[1u]);
    UsartTx(CheckSumField[2u]);
    UsartTx(CheckSumField[3u]);

    UsartTx(MAGIC_END_A);
    UsartTx(MAGIC_END_B);

    return;
}

void PromIfRead(const INT8U *data, const INT32U length)
{
    INT32U i = 0;

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
                            fMagicValid = FALSE;
                        }
                    }
                    else
                    {
                        fMagicValid = FALSE;

                        if (MAGIC_START_B == data[i])
                        {
                            RxLengthCnt = 0u;
                            RxLength = 0UL;
                            RxState = E_RX_LENGTH;
                        }
                        else
                        {
                        }
                    }
                    break;
                }

            case E_RX_LENGTH:
                {
                    INT32U val = (INT32U)data[i];

                    ++RxLengthCnt;

                    if (1u == RxLengthCnt)
                    {
                        RxLength = (INT32U)(val * 16777216UL);
                    }
                    else if (2u == RxLengthCnt)
                    {
                        RxLength += (INT32U)(val * 65536UL);
                    }
                    else if (3u == RxLengthCnt)
                    {
                        RxLength += (INT32U)(val * 256UL);
                    }
                    else if (4u == RxLengthCnt)
                    {
                        RxLength += val;

                        if ((RxLength < MAX_RX_PACKET_SIZE) && (0UL != RxLength))
                        {
                            RxDataCnt = 0UL;
                            RxState = E_RX_DATA;
                        }
                        else
                        {
                            RxState = E_RX_START;
                        }
                    }
                    else
                    {
                        RxState = E_RX_START;
                    }
                    break;
                }

            case E_RX_DATA:
                {
                    RxData[RxDataCnt] = data[i];

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

                    if (RxCheckSumCnt >= MAX_CHECK_SUM_LENGTH)
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
                            fMagicValid = FALSE;
                        }
                    }
                    else
                    {
                        fMagicValid = FALSE;
                        RxState = E_RX_START;

                        if (MAGIC_END_B == data[i])
                        {
                            if (NULL == RxCallback)
                            {
                                /* callback function is not valid */
                            }
                            else
                            {
                                RxCallback(&RxData[0], RxLength);
                            }
                        }
                        else
                        {
                            /* magic word is not valid */
                        }
                    }

                    break;
                }

            default:
                {
                    fMagicValid = FALSE;
                    RxState = E_RX_START;
                    break;
                }
        }
    }

    return;
}


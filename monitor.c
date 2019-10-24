/* --COPYRIGHT--,BSD
 * Copyright (c) 2014, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/

/*******************************************************************************
 *
 *  monitor.c - c file for serial command monitor. Interprets and sends
 *            - data to/from the host PC
 *
 ******************************************************************************/

#include "uart.h"
#include "monitor.h"
#include "utility.h"

#define RW_CMD              0x80
#define TRANSFER_SIZE_MASK  0x3f
#define BYTE_MASK           0xff
#define RW_MASK             0x40

// RW CMD TYPE
#define READ                1
#define WRITE               0
// Override these depends on target:
// TODO Replace hard-coded number
// CMD_BUFFER_SIZE =  5 + sizeOfMauIn8BitByte * 63
#define CMD_BUFFER_SIZE     68 // 1 + 4 + 63 = 68

unsigned char           gInCmdBuffer[CMD_BUFFER_SIZE];
unsigned short          gInCmdBufferIdx = 0;
volatile unsigned short gInCmdSkipCount;

void ClearBufferRelatedParam ();

void
WriteVar (int            id,
          unsigned char *data,
          int            size) 
{
    unsigned char *addr = 0;

    switch (id) {
        // GUI variables
    case 1:
        addr = &G_FIRMWARE_VERSION;
        break;
    case 2:
        addr = &G_FULL_SCALE_CURRENT;
        break;
    case 3:
        addr = &G_TORQUE_OLD;
        break;
    case 4:
        addr = &G_ISGAIN_OLD;
        break;
    case 5:
        addr = &G_BYPASS_INDEXER;
        break;
    case 6:
        addr = &G_BYPASS_INDEXER_OLD;
        break;
    case 7:
        addr = &G_WRITE_ALL_REG;
        break;
    case 8:
        addr = &G_READ_ALL_REG;
        break;
    case 9:
        addr = &G_RESET_FAULTS;
        break;
    case 10:
        addr = &G_MANUAL_WRITE;
        break;
    case 11:
        addr = &G_WRITE_ADDR;
        break;
    case 12:
        addr = &G_WRITE_DATA;
        break;
    case 13:
        addr = &G_MANUAL_READ;
        break;
    case 14:
        addr = &G_READ_ADDR;
        break;
    case 15:
        addr = &G_READ_DATA;
        break;

        // Stepper motion profile
    case 16:
        addr = &G_START_STOP_SPEED;
        break;
    case 17:
        addr = &G_TARGET_SPEED;
        break;
    case 18:
        addr = &G_ACCEL_RATE;
        break;
    case 19:
        addr = &G_TOTAL_NUM_STEPS;
        break;
    case 20:
        addr = &G_STEPS_TO_ACCEL;
        break;
    case 21:
        addr = &G_MOTOR_STATE;
        break;
    case 22:
        addr = &G_SPEED_PROFILE;
        break;
    case 23:
        addr = &G_SPEED_PROFILE_LOCK;
        break;
    case 24:
        addr = &G_STEP_PROFILE;
        break;
    case 25:
        addr = &G_STEP_PROFILE_LOCK;
        break;

        // Motor status
    case 26:
        addr = &G_CUR_NUM_STEPS;
        break;
    case 27:
        addr = &G_CUR_SPEED;
        break;
    case 28:
        addr = &G_CUR_SPEED_TEMP;
        break;
    case 29:
        addr = &G_SPEED_INCR;
        break;
    case 30:
        addr = &G_ACCEL_FLAG;
        break;
    default:
        break;
    }

    for (int i = 0; i < size; i++) {
        *(addr + i) = *(data + i);
    }
}

// TODO Merge WriteVar here, replace size with received variable
unsigned double
ReadVar (int id)
{
    unsigned char  *addr = 0;
    int             size = 0;
    unsigned double data = 0;

    switch (id) {
        // GUI variables
    case 1:
        addr = &G_FIRMWARE_VERSION;
        size = sizeof(G_FIRMWARE_VERSION);
        break;
    case 2:
        addr = &G_FULL_SCALE_CURRENT;
        size = sizeof(G_FULL_SCALE_CURRENT);
        break;
    case 3:
        addr = &G_TORQUE_OLD;
        size = sizeof(G_TORQUE_OLD);
        break;
    case 4:
        addr = &G_ISGAIN_OLD;
        size = sizeof(G_ISGAIN_OLD);
        break;
    case 5:
        addr = &G_BYPASS_INDEXER;
        size = sizeof(G_BYPASS_INDEXER);
        break;
    case 6:
        addr = &G_BYPASS_INDEXER_OLD;
        size = sizeof(G_BYPASS_INDEXER_OLD);
        break;
    case 7:
        addr = &G_WRITE_ALL_REG;
        size = sizeof(G_WRITE_ALL_REG);
        break;
    case 8:
        addr = &G_READ_ALL_REG;
        size = sizeof(G_READ_ALL_REG);
        break;
    case 9:
        addr = &G_RESET_FAULTS;
        size = sizeof(G_RESET_FAULTS);
        break;
    case 10:
        addr = &G_MANUAL_WRITE;
        size = sizeof(G_MANUAL_WRITE);
        break;
    case 11:
        addr = &G_WRITE_ADDR;
        size = sizeof(G_WRITE_ADDR);
        break;
    case 12:
        addr = &G_WRITE_DATA;
        size = sizeof(G_WRITE_DATA);
        break;
    case 13:
        addr = &G_MANUAL_READ;
        size = sizeof(G_MANUAL_READ);
        break;
    case 14:
        addr = &G_READ_ADDR;
        size = sizeof(G_READ_ADDR);
        break;
    case 15:
        addr = &G_READ_DATA;
        size = sizeof(G_READ_DATA);
        break;

        // Stepper motion profile
    case 16:
        addr = &G_START_STOP_SPEED;
        size = sizeof(G_START_STOP_SPEED);
        break;
    case 17:
        addr = &G_TARGET_SPEED;
        size = sizeof(G_TARGET_SPEED);
        break;
    case 18:
        addr = &G_ACCEL_RATE;
        size = sizeof(G_ACCEL_RATE);
        break;
    case 19:
        addr = &G_TOTAL_NUM_STEPS;
        size = sizeof(G_TOTAL_NUM_STEPS);
        break;
    case 20:
        addr = &G_STEPS_TO_ACCEL;
        size = sizeof(G_STEPS_TO_ACCEL);
        break;
    case 21:
        addr = &G_MOTOR_STATE;
        size = sizeof(G_MOTOR_STATE);
        break;
    case 22:
        addr = &G_SPEED_PROFILE;
        size = sizeof(G_SPEED_PROFILE);
        break;
    case 23:
        addr = &G_SPEED_PROFILE_LOCK;
        size = sizeof(G_SPEED_PROFILE_LOCK);
        break;
    case 24:
        addr = &G_STEP_PROFILE;
        size = sizeof(G_STEP_PROFILE);
        break;
    case 25:
        addr = &G_STEP_PROFILE_LOCK;
        size = sizeof(G_STEP_PROFILE_LOCK);
        break;

        // Motor status
    case 26:
        addr = &G_CUR_NUM_STEPS;
        size = sizeof(G_CUR_NUM_STEPS);
        break;
    case 27:
        addr = &G_CUR_SPEED;
        size = sizeof(G_CUR_SPEED);
        break;
    case 28:
        addr = &G_CUR_SPEED_TEMP;
        size = sizeof(G_CUR_SPEED_TEMP);
        break;
    case 29:
        addr = &G_SPEED_INCR;
        size = sizeof(G_SPEED_INCR);
        break;
    case 30:
        addr = &G_ACCEL_FLAG;
        size = sizeof(G_ACCEL_FLAG);
        break;
    default:
        break;
    }

    for (int i = 0; i < size; i++) {
        data |= *(addr + i) << (8 * (size - 1 - i));
    }

    return data;
}

// Override these depends on target
void
WriteByteToCOM (unsigned char c)
{
    uartTxByte (c & BYTE_MASK);
}

int
WriteToCmdBuffer (unsigned char  *buf,
                  unsigned short *bufIdx,
                  unsigned char   d)
{
    if ((*bufIdx) < CMD_BUFFER_SIZE) {
        buf[*bufIdx] = d & BYTE_MASK;
        (*bufIdx)++;
        return 0;
    }

    return 1;
}

int
WriteByteToInCmdBuffer (unsigned char d)
{
    return WriteToCmdBuffer (gInCmdBuffer, &gInCmdBufferIdx, d);
}

int
GetTransferSize () // Transfer size refer to the words to read/write of a
                   // given cmd, not the number of bytes for the whole cmd
                   // packet
{
    return (gInCmdBuffer[0] & TRANSFER_SIZE_MASK);
}

int
VerifyInputCmdHeaders ()
{
    return ((gInCmdBuffer[0] & RW_CMD) == RW_CMD) ? 0 : 1;
}

int
GetRWFlag () // Equivalent to endianness on the MAU in transmission
{
    return ((gInCmdBuffer[0] & RW_MASK) == RW_MASK) ? 1 : 0;
}

unsigned char
GetInCmdAddress ()
{
    return gInCmdBuffer[1];
}

unsigned char
GetWriteCmdDataMAU (int idx)
{
    unsigned char startIdx  = 2;

    return gInCmdBuffer[startIdx + idx];
}

void
ClearBufferRelatedParam ()
{
    gInCmdSkipCount = 0;
    gInCmdBufferIdx = 0;
}

void
MemAccessCmd (int RW)
{
    unsigned short  MAUsToRead  = 0;
    unsigned char   dataChar    = 0;
    unsigned char   addr        = GetInCmdAddress ();

    WriteByteToCOM (gInCmdBuffer[0]);

    MAUsToRead = GetTransferSize ();
    for (unsigned short i = 0; i < MAUsToRead; i++) {
        switch (RW) {
        case READ:          // TODO Modify here to assign variables
            dataChar = *(addr + i);
            WriteByteToCOM (dataChar);
            break;
        case WRITE:
        default:
            dataChar = GetWriteCmdDataMAU (i);
            *(addr + i) = dataChar;
            break;
        }
    }
}

int
ProcessCommand ()
{
    if (VerifyInputCmdHeaders ())
        return 1;
    else
        MemAccessCmd (GetRWFlag ());

    return 0;
}

void
receivedDataCommand (unsigned char d) // Only lower byte will be used even if
                                      // MAU is bigger than 1 byte
{
    WriteByteToInCmdBuffer (d);

    if (gInCmdSkipCount > 0) {
        gInCmdSkipCount--;
        return;
    }

    if (gInCmdBufferIdx > 0 && gInCmdSkipCount == 0) {
        // Wrong input header, clear cmd buffer
        if (VerifyInputCmdHeaders ()) {
            ClearBufferRelatedParam ();
            return;
        }

        if (gInCmdBufferIdx == 1) {
            if (GetRWFlag () == WRITE)
                gInCmdSkipCount = GetTransferSize ();
            else
                gInCmdSkipCount = 0;
        } else {
            ProcessCommand ();
            ClearBufferRelatedParam ();
        }
        return;
    }
}
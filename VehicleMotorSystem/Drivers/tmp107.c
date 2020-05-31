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
/*
 * tmp107.c
 *
 *  Created on: Dec 17, 2015
 *      Author: a0271474
 */

#include "tmp107.h"

char TMP107_GlobalAddressInit(){
	// initialize the chain and return the last device address
	char data[2];
	char rx[32];
	char last_response;
	data[0] = TMP107_Global_bit | 0x94; // AddressInit command code
	data[1] = 0x5 | TMP107_Encode5bitAddress(0x1); // give the first device logical address 1
	TMP107_Transmit(data,2);
    /* must wait 1250ms after receiving last device response
     * during address init operation. this is not baud rate
     * dependent. this is because address init writes to the
     * internal eeprom, which takes additional time.
     */
	last_response = TMP107_WaitForEcho(2,33,TMP107_AddrInitTimeout); //force timeout with unreachable count
	TMP107_RetrieveReadback(2, rx, last_response);
	/* the last address received is actually the address of
	 * the next device (which does not exist.) in order to get
	 * the address of the last device, we have to look at
	 * the address that was received 2nd to last.
	 */
	return rx[last_response - 1] & 0xF8;
}

char TMP107_LastDevicePoll(){
	// query the device chain to find the last device in the chain
	char tx[1];
	char rx[1];
	unsigned char retval;
	tx[0] = 0x57; // LastDevicePoll command code
	TMP107_Transmit(tx,1);
	TMP107_WaitForEcho(1,1,TMP107_Timeout); // normal timeout
	TMP107_RetrieveReadback(1,rx,1);
	retval = rx[0] & 0xF8; // mask the unused address bits that are always 0b11
	return retval;
}

void TMP107_GlobalSoftwareReset(){
	// reset all devices in chain
	char tx[1];
	tx[0] = 0x5D; // GlobalSoftwareReset command code
	TMP107_Transmit(tx,1);
	TMP107_WaitForEcho(1,0,TMP107_Timeout);
	// no need to RetrieveReadback
}
void TMP107_GlobalAlertClear1(){
	// clear all Alert1
	char tx[1];
	tx[0] = 0xB5; // GlobalAlertClear1 command code
	TMP107_Transmit(tx,1);
	TMP107_WaitForEcho(1,0,TMP107_Timeout);
	// no need to RetrieveReadback
}
void TMP107_GlobalAlertClear2(){
	// clear all Alert2
	char tx[1];
	tx[0] = 0x75; // GlobalAlertClear2 command code
	TMP107_Transmit(tx,1);
	TMP107_WaitForEcho(1,0,TMP107_Timeout);
	// no need to RetrieveReadback
}

float TMP107_DecodeTemperatureResult(int HByte, int LByte){
	// convert raw byte response to floating point temperature
	int Bytes;
	float temperature;
	Bytes = HByte << 8 | LByte;
	Bytes &= 0xFFFC; //Mask NVM bits not used in Temperature Result
	temperature = (float) Bytes / 256;
	return temperature;
}

unsigned char TMP107_Encode5bitAddress(unsigned char addr){
	// bit-reverse logical address to get TMP107-encoded address
	char i;
	unsigned char out = 0;
	for (i = 0; i < 5; i++){
		if (addr & (1 << i)){
			out |= 1 << (3+i);
		}
	}
	return out;
}

unsigned char TMP107_Decode5bitAddress(unsigned char addr){
	// bit-reverse TMP107-encoded address to get logical address
	char i;
	unsigned char out = 0;
	for (i = 3; i < 8; i++){
		if (addr & (1 << i)){
			out |= 1 << (i-3);
		}
	}
	return out;
}

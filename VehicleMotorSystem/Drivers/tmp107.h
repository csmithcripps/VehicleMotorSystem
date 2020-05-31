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
 * tmp107.h
 *
 *  Created on: Dec 17, 2015
 *      Author: a0271474
 */

#define TMP107_Success			0
#define TMP107_EchoError		1

#define TMP107_Timeout			40
#define TMP107_AddrInitTimeout	1250

/* Command Byte */
#define TMP107_Read_bit			0x2
#define TMP107_Global_bit		0x1

#define TMP107_AddressReset		0x2
#define TMP107_SoftwareReset	0xB
#define	TMP107_Alert1Clear		0x16
#define TMP107_Alert2Clear		0xE

 /* Registers */
#define TMP107_Temp_reg			0xA0
#define TMP107_Conf_reg			0xA1
#define TMP107_HiLimit1_reg		0xA2
#define TMP107_LoLimit1_reg		0xA3
#define TMP107_HiLimit2_reg		0xA4
#define TMP107_LoLimit2_reg		0xA5
#define TMP107_EEPROM1_reg		0xA6
#define TMP107_EEPROM2_reg		0xA7
#define TMP107_EEPROM3_reg		0xA8
#define TMP107_EEPROM4_reg		0xA9
#define TMP107_EEPROM5_reg		0xAA
#define TMP107_EEPROM6_reg		0xAB
#define TMP107_EEPROM7_reg		0xAC
#define TMP107_EEPROM8_reg		0xAD
#define TMP107_DieID_reg		0xAF //Read Only

float TMP107_DecodeTemperatureResult(int HByte, int LByte);
unsigned char TMP107_Encode5bitAddress(unsigned char addr);
unsigned char TMP107_Decode5bitAddress(unsigned char addr);

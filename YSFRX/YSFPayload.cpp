/*
*	Copyright (C) 2016 Jonathan Naylor, G4KLX
*	Copyright (C) 2016 Mathias Weyland, HB9FRV
*
*	This program is free software; you can redistribute it and/or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation; version 2 of the License.
*
*	This program is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*/

/*
Modified by Chris, K9EQ
*/

#include "YSFConvolution.h"
#include "YSFPayload.h"
#include "YSFDefines.h"
#include "Utils.h"
#include "CRC.h"
#include "Log.h"

#include <cstdio>
#include <cassert>
#include <cstring>
#include <cstdint>

const unsigned int INTERLEAVE_TABLE_9_20[] = {
        0U, 40U,  80U, 120U, 160U, 200U, 240U, 280U, 320U,
        2U, 42U,  82U, 122U, 162U, 202U, 242U, 282U, 322U,
        4U, 44U,  84U, 124U, 164U, 204U, 244U, 284U, 324U,
        6U, 46U,  86U, 126U, 166U, 206U, 246U, 286U, 326U,
        8U, 48U,  88U, 128U, 168U, 208U, 248U, 288U, 328U,
       10U, 50U,  90U, 130U, 170U, 210U, 250U, 290U, 330U,
       12U, 52U,  92U, 132U, 172U, 212U, 252U, 292U, 332U,
       14U, 54U,  94U, 134U, 174U, 214U, 254U, 294U, 334U,
       16U, 56U,  96U, 136U, 176U, 216U, 256U, 296U, 336U,
       18U, 58U,  98U, 138U, 178U, 218U, 258U, 298U, 338U,
       20U, 60U, 100U, 140U, 180U, 220U, 260U, 300U, 340U,
       22U, 62U, 102U, 142U, 182U, 222U, 262U, 302U, 342U,
       24U, 64U, 104U, 144U, 184U, 224U, 264U, 304U, 344U,
       26U, 66U, 106U, 146U, 186U, 226U, 266U, 306U, 346U,
       28U, 68U, 108U, 148U, 188U, 228U, 268U, 308U, 348U,
       30U, 70U, 110U, 150U, 190U, 230U, 270U, 310U, 350U,
       32U, 72U, 112U, 152U, 192U, 232U, 272U, 312U, 352U,
       34U, 74U, 114U, 154U, 194U, 234U, 274U, 314U, 354U,
       36U, 76U, 116U, 156U, 196U, 236U, 276U, 316U, 356U,
       38U, 78U, 118U, 158U, 198U, 238U, 278U, 318U, 358U};

const unsigned int INTERLEAVE_TABLE_5_20[] = {
	0U, 40U,  80U, 120U, 160U,
	2U, 42U,  82U, 122U, 162U,
	4U, 44U,  84U, 124U, 164U,
	6U, 46U,  86U, 126U, 166U,
	8U, 48U,  88U, 128U, 168U,
	10U, 50U,  90U, 130U, 170U,
	12U, 52U,  92U, 132U, 172U,
	14U, 54U,  94U, 134U, 174U,
	16U, 56U,  96U, 136U, 176U,
	18U, 58U,  98U, 138U, 178U,
	20U, 60U, 100U, 140U, 180U,
	22U, 62U, 102U, 142U, 182U,
	24U, 64U, 104U, 144U, 184U,
	26U, 66U, 106U, 146U, 186U,
	28U, 68U, 108U, 148U, 188U,
	30U, 70U, 110U, 150U, 190U,
	32U, 72U, 112U, 152U, 192U,
	34U, 74U, 114U, 154U, 194U,
	36U, 76U, 116U, 156U, 196U,
	38U, 78U, 118U, 158U, 198U};

// This one differs from the others in that it interleaves bits and not dibits
const unsigned int INTERLEAVE_TABLE_26_4[] = {
	0U, 4U,  8U, 12U, 16U, 20U, 24U, 28U, 32U, 36U, 40U, 44U, 48U, 52U, 56U, 60U, 64U, 68U, 72U, 76U, 80U, 84U, 88U, 92U, 96U, 100U,
	1U, 5U,  9U, 13U, 17U, 21U, 25U, 29U, 33U, 37U, 41U, 45U, 49U, 53U, 57U, 61U, 65U, 69U, 73U, 77U, 81U, 85U, 89U, 93U, 97U, 101U,
	2U, 6U, 10U, 14U, 18U, 22U, 26U, 30U, 34U, 38U, 42U, 46U, 50U, 54U, 58U, 62U, 66U, 70U, 74U, 78U, 82U, 86U, 90U, 94U, 98U, 102U,
	3U, 7U, 11U, 15U, 19U, 23U, 27U, 31U, 35U, 39U, 43U, 47U, 51U, 55U, 59U, 63U, 67U, 71U, 75U, 79U, 83U, 87U, 91U, 95U, 99U, 103U};

const unsigned char WHITENING_DATA[] = {0x93U, 0xD7U, 0x51U, 0x21U, 0x9CU, 0x2FU, 0x6CU, 0xD0U, 0xEFU, 0x0FU,
										0xF8U, 0x3DU, 0xF1U, 0x73U, 0x20U, 0x94U, 0xEDU, 0x1EU, 0x7CU, 0xD8U};

const unsigned char BIT_MASK_TABLE[] = {0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U};

#define WRITE_BIT1(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE[(i)&7])
#define READ_BIT1(p,i)    (p[(i)>>3] & BIT_MASK_TABLE[(i)&7])

CYSFPayload::CYSFPayload() :
m_fec()
{
}

CYSFPayload::~CYSFPayload()
{
}

bool CYSFPayload::processHeaderData(unsigned char* data)
{
	assert(data != NULL);

	data += YSF_SYNC_LENGTH_BYTES + YSF_FICH_LENGTH_BYTES;

	unsigned char dch[45U];

	unsigned char* p1 = data;
	unsigned char* p2 = dch;
	for (unsigned int i = 0U; i < 5U; i++) {
		::memcpy(p2, p1, 9U);
		p1 += 18U; p2 += 9U;
	}

	CYSFConvolution conv;
	conv.start();

	for (unsigned int i = 0U; i < 180U; i++) {
		unsigned int n = INTERLEAVE_TABLE_9_20[i];
		uint8_t s0 = READ_BIT1(dch, n) ? 1U : 0U;

		n++;
		uint8_t s1 = READ_BIT1(dch, n) ? 1U : 0U;

		conv.decode(s0, s1);
	}

	unsigned char output[23U];
	conv.chainback(output, 176U);

	bool valid1 = CCRC::checkCCITT162(output, 22U);
	if (valid1) {
		for (unsigned int i = 0U; i < 20U; i++)
			output[i] ^= WHITENING_DATA[i];

		CUtils::dump("Header, CSD1", output, 20U);
	}

	p1 = data + 9U;
	p2 = dch;
	for (unsigned int i = 0U; i < 5U; i++) {
		::memcpy(p2, p1, 9U);
		p1 += 18U; p2 += 9U;
	}

	conv.start();

	for (unsigned int i = 0U; i < 180U; i++) {
		unsigned int n = INTERLEAVE_TABLE_9_20[i];
		uint8_t s0 = READ_BIT1(dch, n) ? 1U : 0U;

		n++;
		uint8_t s1 = READ_BIT1(dch, n) ? 1U : 0U;

		conv.decode(s0, s1);
	}

	conv.chainback(output, 176U);

	bool valid2 = CCRC::checkCCITT162(output, 22U);
	if (valid2) {
		for (unsigned int i = 0U; i < 20U; i++)
			output[i] ^= WHITENING_DATA[i];

		CUtils::dump("Header, CSD2", output, 20U);
	}

	return valid1;
}

bool CYSFPayload::processTerminatorData(unsigned char* data)
{
	assert(data != NULL);

	data += YSF_SYNC_LENGTH_BYTES + YSF_FICH_LENGTH_BYTES;

	unsigned char dch[45U];

	unsigned char* p1 = data;
	unsigned char* p2 = dch;
	for (unsigned int i = 0U; i < 5U; i++) {
		::memcpy(p2, p1, 9U);
		p1 += 18U; p2 += 9U;
	}

	CYSFConvolution conv;
	conv.start();

	for (unsigned int i = 0U; i < 180U; i++) {
		unsigned int n = INTERLEAVE_TABLE_9_20[i];
		uint8_t s0 = READ_BIT1(dch, n) ? 1U : 0U;

		n++;
		uint8_t s1 = READ_BIT1(dch, n) ? 1U : 0U;

		conv.decode(s0, s1);
	}

	unsigned char output[23U];
	conv.chainback(output, 176U);

	bool valid1 = CCRC::checkCCITT162(output, 22U);
	if (valid1) {
		for (unsigned int i = 0U; i < 20U; i++)
			output[i] ^= WHITENING_DATA[i];

		CUtils::dump("Terminator, CSD1", output, 20U);
	}

	p1 = data + 9U;
	p2 = dch;
	for (unsigned int i = 0U; i < 5U; i++) {
		::memcpy(p2, p1, 9U);
		p1 += 18U; p2 += 9U;
	}

	conv.start();

	for (unsigned int i = 0U; i < 180U; i++) {
		unsigned int n = INTERLEAVE_TABLE_9_20[i];
		uint8_t s0 = READ_BIT1(dch, n) ? 1U : 0U;

		n++;
		uint8_t s1 = READ_BIT1(dch, n) ? 1U : 0U;

		conv.decode(s0, s1);
	}

	conv.chainback(output, 176U);

	bool valid2 = CCRC::checkCCITT162(output, 22U);
	if (valid2) {
		for (unsigned int i = 0U; i < 20U; i++)
			output[i] ^= WHITENING_DATA[i];

		CUtils::dump("Terminator, CSD2", output, 20U);
	}

	return valid1;
}

bool CYSFPayload::processVDMode1Data(unsigned char* data, unsigned char fn)
{
	assert(data != NULL);

	data += YSF_SYNC_LENGTH_BYTES + YSF_FICH_LENGTH_BYTES;

	unsigned char dch[45U];

	unsigned char* p1 = data;
	unsigned char* p2 = dch;
	for (unsigned int i = 0U; i < 5U; i++) {
		::memcpy(p2, p1, 9U);
		p1 += 18U; p2 += 9U;
	}

	CYSFConvolution conv;
	conv.start();

	for (unsigned int i = 0U; i < 180U; i++) {
		unsigned int n = INTERLEAVE_TABLE_9_20[i];
		uint8_t s0 = READ_BIT1(dch, n) ? 1U : 0U;

		n++;
		uint8_t s1 = READ_BIT1(dch, n) ? 1U : 0U;

		conv.decode(s0, s1);
	}

	unsigned char output[23U];
	conv.chainback(output, 176U);

	bool ret = CCRC::checkCCITT162(output, 22U);
	if (ret) {
		for (unsigned int i = 0U; i < 20U; i++)
			output[i] ^= WHITENING_DATA[i];

		switch (fn) {
		case 0U:
			CUtils::dump("V/D Mode 1, CSD1", output, 20U);
			break;

		case 1U:
			CUtils::dump("V/D Mode 1, CSD2", output, 20U);
			break;

		case 2U:
			CUtils::dump("V/D Mode 1, CSD3", output, 20U);
			break;

		case 3U:
			CUtils::dump("V/D Mode 1, DT1", output, 20U);
			break;

		case 4U:
			CUtils::dump("V/D Mode 1, DT2", output, 20U);
			break;

		case 5U:
			CUtils::dump("V/D Mode 1, DT3", output, 20U);
			break;

		case 6U:
			CUtils::dump("V/D Mode 1, DT4", output, 20U);
			break;

		case 7U:
			CUtils::dump("V/D Mode 1, DT5", output, 20U);
			break;

		default:
			break;
		}
	}

	return ret && (fn == 0U);
}

unsigned int CYSFPayload::processVDMode1Audio(unsigned char* data)
{
	assert(data != NULL);

	data += YSF_SYNC_LENGTH_BYTES + YSF_FICH_LENGTH_BYTES;

	// Regenerate the AMBE FEC
	unsigned int errors = 0U;
	errors += m_fec.regenerateDMR(data + 9U);
	errors += m_fec.regenerateDMR(data + 27U);
	errors += m_fec.regenerateDMR(data + 45U);
	errors += m_fec.regenerateDMR(data + 63U);
	errors += m_fec.regenerateDMR(data + 81U);

	return errors;
}

bool CYSFPayload::processVDMode2Data(unsigned char* data, unsigned char fn)
{
	assert(data != NULL);

	data += YSF_SYNC_LENGTH_BYTES + YSF_FICH_LENGTH_BYTES;

	unsigned char dch[25U];

	unsigned char* p1 = data;
	unsigned char* p2 = dch;
	for (unsigned int i = 0U; i < 5U; i++) {
		::memcpy(p2, p1, 5U);
		p1 += 18U; p2 += 5U;
	}

	CYSFConvolution conv;
	conv.start();

	for (unsigned int i = 0U; i < 100U; i++) {
		unsigned int n = INTERLEAVE_TABLE_5_20[i];
		uint8_t s0 = READ_BIT1(dch, n) ? 1U : 0U;

		n++;
		uint8_t s1 = READ_BIT1(dch, n) ? 1U : 0U;

		conv.decode(s0, s1);
	}

	unsigned char output[13U];
	conv.chainback(output, 96U);

	bool ret = CCRC::checkCCITT162(output, 12U);
	if (ret) {
		for (unsigned int i = 0U; i < YSF_CALLSIGN_LENGTH; i++)
			output[i] ^= WHITENING_DATA[i];

		switch (fn) {
		case 0U:
			CUtils::dump("V/D Mode 2, Dest", output, YSF_CALLSIGN_LENGTH);
			break;

		case 1U:
			CUtils::dump("V/D Mode 2, Src", output, YSF_CALLSIGN_LENGTH);
			break;

		case 2U:
			CUtils::dump("V/D Mode 2, Down", output, YSF_CALLSIGN_LENGTH);
			break;

		case 3U:
			CUtils::dump("V/D Mode 2, Up", output, YSF_CALLSIGN_LENGTH);
			break;

		case 4U:
			CUtils::dump("V/D Mode 2, Rem1+2", output, YSF_CALLSIGN_LENGTH);
			break;

		case 5U:
			CUtils::dump("V/D Mode 2, Rem3+4", output, YSF_CALLSIGN_LENGTH);
			break;

		case 6U:
			CUtils::dump("V/D Mode 2, DT1", output, YSF_CALLSIGN_LENGTH);
			break;

		case 7U:
			CUtils::dump("V/D Mode 2, DT2", output, YSF_CALLSIGN_LENGTH);
			break;

		default:
			break;
		}
	}

	return ret && (fn == 0U || fn == 1U);
}

unsigned int CYSFPayload::processVDMode2Audio(unsigned char* data)
{
	assert(data != NULL);

	data += YSF_SYNC_LENGTH_BYTES + YSF_FICH_LENGTH_BYTES;

	unsigned int errors = 0U;
	unsigned int offset = 40U; // DCH(0)

							   // We have a total of 5 VCH sections, iterate through each
	for (unsigned int j = 0U; j < 5U; j++, offset += 144U) {
		unsigned char vch[13U];

		// Deinterleave
		for (unsigned int i = 0U; i < 104U; i++) {
			unsigned int n = INTERLEAVE_TABLE_26_4[i];
			bool s = READ_BIT1(data, offset + n) != 0x00U;
			WRITE_BIT1(vch, i, s);
		}

		// "Un-whiten" (descramble)
		for (unsigned int i = 0U; i < 13U; i++)
			vch[i] ^= WHITENING_DATA[i];

		//		errors += READ_BIT1(vch, 103); // Padding bit must be zero but apparently it is not...

		for (unsigned int i = 0U; i < 81U; i += 3) {
			uint8_t vote = bool(READ_BIT1(vch, i)) + bool(READ_BIT1(vch, i + 1)) + bool(READ_BIT1(vch, i + 2));
			if (vote == 1 || vote == 2) {
				bool decision = vote / 2; // exploit integer division: 1/2 == 0, 2/2 == 1.
				WRITE_BIT1(vch, i, decision);
				WRITE_BIT1(vch, i + 1, decision);
				WRITE_BIT1(vch, i + 2, decision);
				errors++;
			}
		}

		// Reconstruct only if we have bit errors. Technically we could even
		// constrain it individually to the 5 VCH sections.
		if (errors > 0U) {
			// Scramble
			for (unsigned int i = 0U; i < 13U; i++)
				vch[i] ^= WHITENING_DATA[i];

			// Interleave
			for (unsigned int i = 0U; i < 104U; i++) {
				unsigned int n = INTERLEAVE_TABLE_26_4[i];
				bool s = READ_BIT1(vch, i);
				WRITE_BIT1(data, offset + n, s);
			}
		}
	}

	// "errors" is the number of triplets that were recognized to be corrupted
	// and that were corrected. There are 27 of those per VCH and 5 VCH per CC,
	// yielding a total of 27*5 = 135. I believe the expected value of this
	// error distribution to be Bin(1;3,BER)+Bin(2;3,BER) which entails 75% for
	// BER = 0.5.
	return errors;
}

bool CYSFPayload::processDataFRModeData(unsigned char* data, unsigned char fn)
{
	assert(data != NULL);

	data += YSF_SYNC_LENGTH_BYTES + YSF_FICH_LENGTH_BYTES;

	unsigned char dch[45U];

	unsigned char* p1 = data;
	unsigned char* p2 = dch;
	for (unsigned int i = 0U; i < 5U; i++) {
		::memcpy(p2, p1, 9U);
		p1 += 18U; p2 += 9U;
	}

	CYSFConvolution conv;
	conv.start();

	for (unsigned int i = 0U; i < 180U; i++) {
		unsigned int n = INTERLEAVE_TABLE_9_20[i];
		uint8_t s0 = READ_BIT1(dch, n) ? 1U : 0U;

		n++;
		uint8_t s1 = READ_BIT1(dch, n) ? 1U : 0U;

		conv.decode(s0, s1);
	}

	unsigned char output[23U];
	conv.chainback(output, 176U);

	bool ret1 = CCRC::checkCCITT162(output, 22U);
	if (ret1) {
		for (unsigned int i = 0U; i < 20U; i++)
			output[i] ^= WHITENING_DATA[i];

		switch (fn) {
		case 0U:
			CUtils::dump("Data FR Mode, CSD1", output, 20U);
			break;

		case 1U:
			CUtils::dump("Data FR Mode, CSD3", output, 20U);
			break;

		case 2U:
			CUtils::dump("Data FR Mode, DT2", output, 20U);
			break;

		case 3U:
			CUtils::dump("Data FR Mode, DT4", output, 20U);
			break;

		case 4U:
			CUtils::dump("Data FR Mode, DT6", output, 20U);
			break;

		case 5U:
			CUtils::dump("Data FR Mode, DT8", output, 20U);
			break;

		case 6U:
			CUtils::dump("Data FR Mode, DT10", output, 20U);
			break;

		case 7U:
			CUtils::dump("Data FR Mode, DT12", output, 20U);
			break;

		default:
			break;
		}
	} else {
		switch (fn) {
		case 0U:
			LogMessage("Data FR Mode, invalid CSD1");
			break;

		case 1U:
			LogMessage("Data FR Mode, invalid CSD3");
			break;

		case 2U:
			LogMessage("Data FR Mode, invalid DT2");
			break;

		case 3U:
			LogMessage("Data FR Mode, invalid DT4");
			break;

		case 4U:
			LogMessage("Data FR Mode, invalid DT6");
			break;

		case 5U:
			LogMessage("Data FR Mode, invalid DT8");
			break;

		case 6U:
			LogMessage("Data FR Mode, invalid DT10");
			break;

		case 7U:
			LogMessage("Data FR Mode, invalid DT12");
			break;

		default:
			LogMessage("Data FR Mode, invalid data");
			break;
		}

		CUtils::dump(2U, "DCH", dch, 45U);
		CUtils::dump(2U, "After FEC", output, 22U);
		for (unsigned int i = 0U; i < 20U; i++)
			output[i] ^= WHITENING_DATA[i];
		CUtils::dump(2U, "After Whitening", output, 20U);
	}

	p1 = data + 9U;
	p2 = dch;
	for (unsigned int i = 0U; i < 5U; i++) {
		::memcpy(p2, p1, 9U);
		p1 += 18U; p2 += 9U;
	}

	conv.start();

	for (unsigned int i = 0U; i < 180U; i++) {
		unsigned int n = INTERLEAVE_TABLE_9_20[i];
		uint8_t s0 = READ_BIT1(dch, n) ? 1U : 0U;

		n++;
		uint8_t s1 = READ_BIT1(dch, n) ? 1U : 0U;

		conv.decode(s0, s1);
	}

	conv.chainback(output, 176U);

	bool ret2 = CCRC::checkCCITT162(output, 22U);
	if (ret2) {
		for (unsigned int i = 0U; i < 20U; i++)
			output[i] ^= WHITENING_DATA[i];

		switch (fn) {
		case 0U:
			CUtils::dump("Data FR Mode, CSD2", output, 20U);
			break;

		case 1U:
			CUtils::dump("Data FR Mode, DT1", output, 20U);
			break;

		case 2U:
			CUtils::dump("Data FR Mode, DT3", output, 20U);
			break;

		case 3U:
			CUtils::dump("Data FR Mode, DT5", output, 20U);
			break;

		case 4U:
			CUtils::dump("Data FR Mode, DT7", output, 20U);
			break;

		case 5U:
			CUtils::dump("Data FR Mode, DT9", output, 20U);
			break;

		case 6U:
			CUtils::dump("Data FR Mode, DT11", output, 20U);
			break;

		case 7U:
			CUtils::dump("Data FR Mode, DT13", output, 20U);
			break;

		default:
			break;
		}
	} else {
		switch (fn) {
		case 0U:
			LogMessage("Data FR Mode, invalid CSD2");
			break;

		case 1U:
			LogMessage("Data FR Mode, invalid DT1");
			break;

		case 2U:
			LogMessage("Data FR Mode, invalid DT3");
			break;

		case 3U:
			LogMessage("Data FR Mode, invalid DT5");
			break;

		case 4U:
			LogMessage("Data FR Mode, invalid DT7");
			break;

		case 5U:
			LogMessage("Data FR Mode, invalid DT9");
			break;

		case 6U:
			LogMessage("Data FR Mode, invalid DT11");
			break;

		case 7U:
			LogMessage("Data FR Mode, invalid DT13");
			break;

		default:
			LogMessage("Data FR Mode, invalid data");
			break;
		}

		CUtils::dump(2U, "DCH", dch, 45U);
		CUtils::dump(2U, "After FEC", output, 22U);
		for (unsigned int i = 0U; i < 20U; i++)
			output[i] ^= WHITENING_DATA[i];
		CUtils::dump(2U, "After Whitening", output, 20U);
	}

	return ret1 && (fn == 0U);
}

bool CYSFPayload::processVoiceFRModeData(unsigned char* data)
{
	assert(data != NULL);

	data += YSF_SYNC_LENGTH_BYTES + YSF_FICH_LENGTH_BYTES;

	unsigned char dch[45U];
	::memcpy(dch, data, 45U);

	CYSFConvolution conv;
	conv.start();

	for (unsigned int i = 0U; i < 180U; i++) {
		unsigned int n = INTERLEAVE_TABLE_9_20[i];
		uint8_t s0 = READ_BIT1(dch, n) ? 1U : 0U;

		n++;
		uint8_t s1 = READ_BIT1(dch, n) ? 1U : 0U;

		conv.decode(s0, s1);
	}

	unsigned char output[23U];
	conv.chainback(output, 176U);

	bool ret = CCRC::checkCCITT162(output, 22U);
	if (ret) {
		for (unsigned int i = 0U; i < 20U; i++)
			output[i] ^= WHITENING_DATA[i];

		CUtils::dump("Voice FR Mode, CSD3", output, 20U);
	}

	return ret;
}

unsigned int CYSFPayload::processVoiceFRModeAudio(unsigned char* data)
{
	assert(data != NULL);

	data += YSF_SYNC_LENGTH_BYTES + YSF_FICH_LENGTH_BYTES;

	// Regenerate the AMBE FEC
	unsigned int errors = 0U;
	errors += m_fec.regenerateYSF3(data + 0U);
	errors += m_fec.regenerateYSF3(data + 18U);
	errors += m_fec.regenerateYSF3(data + 36U);
	errors += m_fec.regenerateYSF3(data + 54U);
	errors += m_fec.regenerateYSF3(data + 72U);

	return errors;
}

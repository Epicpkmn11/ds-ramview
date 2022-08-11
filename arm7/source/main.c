/*---------------------------------------------------------------------------------

	default ARM7 core

		Copyright (C) 2005 - 2010
		Michael Noland (joat)
		Jason Rogers (dovoto)
		Dave Murphy (WinterMute)

	This software is provided 'as-is', without any express or implied
	warranty.  In no event will the authors be held liable for any
	damages arising from the use of this software.

	Permission is granted to anyone to use this software for any
	purpose, including commercial applications, and to alter it and
	redistribute it freely, subject to the following restrictions:

	1.	The origin of this software must not be misrepresented; you
		must not claim that you wrote the original software. If you use
		this software in a product, an acknowledgment in the product
		documentation would be appreciated but is not required.

	2.	Altered source versions must be plainly marked as such, and
		must not be misrepresented as being the original software.

	3.	This notice may not be removed or altered from any source
		distribution.

---------------------------------------------------------------------------------*/
#include "tonccpy.h"

#include <nds.h>
// #include <dswifi7.h>
// #include <maxmod7.h>

#define SHARED_ADDR ((vu32 *)0x027FFA0C)
#define REG_EXTKEYINPUT (*(vuint16*)0x04000136)

//---------------------------------------------------------------------------------
void VblankHandler(void) {
//---------------------------------------------------------------------------------
	// Wifi_Update();
}


//---------------------------------------------------------------------------------
void VcountHandler() {
//---------------------------------------------------------------------------------
	inputGetAndSend();
}

volatile bool exitflag = false;

//---------------------------------------------------------------------------------
void powerButtonCB() {
//---------------------------------------------------------------------------------
	exitflag = true;
}

//---------------------------------------------------------------------------------
int main() {
//---------------------------------------------------------------------------------
	// clear sound registers
	dmaFillWords(0, (void*)0x04000400, 0x100);

	REG_SOUNDCNT |= SOUND_ENABLE;
	writePowerManagement(PM_CONTROL_REG, ( readPowerManagement(PM_CONTROL_REG) & ~PM_SOUND_MUTE ) | PM_SOUND_AMP );
	powerOn(POWER_SOUND);

	readUserSettings();
	ledBlink(0);

	irqInit();
	// Start the RTC tracking IRQ
	initClockIRQ();
	fifoInit();
	touchInit();

	// mmInstall(FIFO_MAXMOD);

	SetYtrigger(80);

	// installWifiFIFO();
	// installSoundFIFO();

	installSystemFIFO();

	irqSet(IRQ_VCOUNT, VcountHandler);
	irqSet(IRQ_VBLANK, VblankHandler);

	irqEnable( IRQ_VBLANK | IRQ_VCOUNT | IRQ_NETWORK);

	setPowerButtonCB(powerButtonCB);

	// Keep the ARM7 mostly idle
	while (!exitflag) {
		if ( 0 == (REG_KEYINPUT & (KEY_SELECT | KEY_START | KEY_L | KEY_R))) {
			exitflag = true;
		}
		swiWaitForVBlank();

		// Send key states
		SHARED_ADDR[5] = ~REG_KEYINPUT & 0x3FF;
		SHARED_ADDR[5] |= ((~REG_EXTKEYINPUT & 0x3) << 10) | ((~REG_EXTKEYINPUT & 0xC0) << 6);

		while (REG_VCOUNT != 191) swiDelay(100);
		while (REG_VCOUNT == 191) swiDelay(100);

		// Check if ARM9 wants us to read/write something
		switch (SHARED_ADDR[4]) {
			case 0x524D4152: // RAMR
				tonccpy((u32*)((u32)SHARED_ADDR[0]), (u32*)((u32)SHARED_ADDR[1]), 0xC0);
				break;
			case 0x574D4152: // RAMW
				tonccpy((u8*)((u32)SHARED_ADDR[1]) + SHARED_ADDR[2], (u8*)((u32)SHARED_ADDR[0]) + SHARED_ADDR[2], 1);
				break;
			default:
				break;
		}

		SHARED_ADDR[4] = 0x554E454D; // 'MENU'
	}
	return 0;
}

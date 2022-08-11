#include "ramViewer.h"
#include "tonccpy.h"

#include <nds/ndstypes.h> // has to be first

#include <nds/arm9/background.h>
#include <nds/bios.h>
#include <nds/input.h>
#include <nds/system.h>

extern vu32* volatile sharedAddr;

#define KEYS (sharedAddr[5])

static void print(int x, int y, const char *str, int palette) {
	u16 *dst = BG_MAP_RAM_SUB(15) + y * 0x20 + x;
	while(*str)
		*(dst++) = *(str++) | palette << 12;
}

static void printCenter(int x, int y, const char *str, int palette) {
	u16 *dst = BG_MAP_RAM_SUB(15) + y * 0x20 + x;
	const char *start = str;
	while(*str)
		str++;
	dst += (str - start) / 2;
	while(str != start)
		*(--dst) = *(--str) | palette << 12;
}

static void printChar(int x, int y, unsigned char c, int palette) {
	BG_MAP_RAM_SUB(15)[y * 0x20 + x] = c | palette << 12;
}

static void printHex(int x, int y, u32 val, u8 bytes, int palette) {
	u16 *dst = BG_MAP_RAM_SUB(15) + y * 0x20 + x;
	for(int i = bytes * 2 - 1; i >= 0; i--) {
		*(dst + i) = ((val & 0xF) >= 0xA ? 'A' + (val & 0xF) - 0xA : '0' + (val & 0xF)) | palette << 12;
		val >>= 4;
	}
}

static void waitKeys(u16 keys) {
	// Prevent key repeat for 10 frames
	for(int i = 0; i < 10 && (KEYS & keys); i++) {
		while (REG_VCOUNT != 191) swiDelay(100);
		while (REG_VCOUNT == 191) swiDelay(100);
	}

	do {
		while (REG_VCOUNT != 191) swiDelay(100);
		while (REG_VCOUNT == 191) swiDelay(100);
	} while(!(KEYS & keys));
}

static void clearScreen(void) {
	toncset16(BG_MAP_RAM_SUB(15), 0, 0x300);
}

static vu32 *jumpToAddress(vu32 *address) {
	vu32 *startAddress = address;

	clearScreen();

	u8 cursorPosition = 0;
	while(1) {
		toncset16(BG_MAP_RAM_SUB(15) + 0x20 * 9 + 5, '-', 20);
		printCenter(15, 10, "Jump to Address", 0);
		printHex(11, 12, (u32)address, 4, 3);
		BG_MAP_RAM_SUB(15)[0x20 * 12 + 11 + 6 - cursorPosition] = (BG_MAP_RAM_SUB(15)[0x20 * 12 + 11 + 6 - cursorPosition] & ~(0xF << 12)) | 4 << 12;
		toncset16(BG_MAP_RAM_SUB(15) + 0x20 * 13 + 5, '-', 20);

		waitKeys(KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT | KEY_A | KEY_B);

		if(KEYS & KEY_UP) {
			address = (vu32*)(((u32)address & ~(0xF0 << cursorPosition * 4)) | (((u32)address + (0x10 << (cursorPosition * 4))) & (0xF0 << cursorPosition * 4)));
		} else if(KEYS & KEY_DOWN) {
			address = (vu32*)(((u32)address & ~(0xF0 << cursorPosition * 4)) | (((u32)address - (0x10 << (cursorPosition * 4))) & (0xF0 << cursorPosition * 4)));
		} else if(KEYS & KEY_LEFT) {
			if(cursorPosition < 6)
				cursorPosition++;
		} else if(KEYS & KEY_RIGHT) {
			if(cursorPosition > 0)
				cursorPosition--;
		} else if(KEYS & KEY_A) {
			return address;
		} else if(KEYS & KEY_B) {
			return startAddress;
		}
	}
}

void ramViewer(void) {
	static vu32 *address = (vu32*)0x02000000;
	static bool arm7Ram = false;

	clearScreen();

	u8 *arm7RamBuffer = ((u8*)sharedAddr) - 0x74C;
	bool ramLoaded = false;
	u8 cursorPosition = 0, mode = 0;
	while(1) {
		u8 *ramPtr = arm7Ram ? arm7RamBuffer : (u8*)address;

		char armText[5] = {'A', 'R', 'M', arm7Ram ? '7' : '9', 0};
		printCenter(14, 0, "Ram Viewer", 0);
		print(27, 0, armText, 3);
		printHex(0, 0, (u32)address >> 0x10, 2, 3);

		if (arm7Ram && !ramLoaded) {
			sharedAddr[0] = (vu32)arm7RamBuffer;
			sharedAddr[1] = (vu32)address;
			sharedAddr[4] = 0x524D4152; // RAMR
			while (sharedAddr[4] == 0x524D4152) {
				while (REG_VCOUNT != 191) swiDelay(100);
				while (REG_VCOUNT == 191) swiDelay(100);
			}
		}
		ramLoaded = true;

		for(int i = 0; i < 23; i++) {
			printHex(0, i + 1, (u32)(address + (i * 2)) & 0xFFFF, 2, 3);
			for(int j = 0; j < 4; j++)
				printHex(5 + (j * 2), i + 1, ramPtr[(i * 8) + j], 1, 1 + j % 2);
			for(int j = 0; j < 4; j++)
				printHex(14 + (j * 2), i + 1, ramPtr[4 + (i * 8) + j], 1, 1 + j % 2);
			for(int j = 0; j < 8; j++)
				printChar(23 + j, i + 1, ramPtr[i * 8 + j], 0);
		}

		// Change color of selected byte
		if(mode > 0) {
			// Hex
			u16 loc = 0x20 * (1 + (cursorPosition / 8)) + 5 + ((cursorPosition % 8) * 2) + (cursorPosition % 8 >= 4);
			BG_MAP_RAM_SUB(15)[loc] = (BG_MAP_RAM_SUB(15)[loc] & ~(0xF << 12)) | (3 + mode) << 12;
			BG_MAP_RAM_SUB(15)[loc + 1] = (BG_MAP_RAM_SUB(15)[loc + 1] & ~(0xF << 12)) | (3 + mode) << 12;

			// Text
			loc = 0x20 * (1 + (cursorPosition / 8)) + 23 + (cursorPosition % 8);
			BG_MAP_RAM_SUB(15)[loc] = (BG_MAP_RAM_SUB(15)[loc] & ~(0xF << 12)) | (3 + mode) << 12;
		}

		waitKeys(KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT | KEY_A | KEY_B | KEY_Y | KEY_SELECT);

		if(mode == 0) {
			if(KEYS & KEY_R && KEYS & (KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT)) {
				if (KEYS & KEY_UP) {
					address -= 0x400;
					ramLoaded = false;
				} else if (KEYS & KEY_DOWN) {
					address += 0x400;
					ramLoaded = false;
				} else if (KEYS & KEY_LEFT) {
					address -= 0x4000;
					ramLoaded = false;
				} else if (KEYS & KEY_RIGHT) {
					address += 0x4000;
					ramLoaded = false;
				}
			} else {
				if (KEYS & KEY_UP) {
					address -= 2;
					ramLoaded = false;
				} else if (KEYS & KEY_DOWN) {
					address += 2;
					ramLoaded = false;
				} else if (KEYS & KEY_LEFT) {
					address -= 2 * 23;
					ramLoaded = false;
				} else if (KEYS & KEY_RIGHT) {
					address += 2 * 23;
					ramLoaded = false;
				} else if (KEYS & KEY_A) {
					mode = 1;
				} else if (KEYS & KEY_B) {
					return;
				} else if(KEYS & KEY_Y) {
					address = jumpToAddress(address);
					clearScreen();
					ramLoaded = false;
				}else if (KEYS & KEY_SELECT) {
					arm7Ram = !arm7Ram;
					ramLoaded = false;
				}
			}
		} else if(mode == 1) {
			if (KEYS & KEY_UP) {
				if(cursorPosition >= 8)
					cursorPosition -= 8;
				else
					address -= 2;
			} else if (KEYS & KEY_DOWN) {
				if(cursorPosition < 8 * 22)
					cursorPosition += 8;
				else
					address += 2;
			} else if (KEYS & KEY_LEFT) {
				if(cursorPosition > 0)
					cursorPosition--;
			} else if (KEYS & KEY_RIGHT) {
				if(cursorPosition < 8 * 23 - 1)
					cursorPosition++;
			} else if (KEYS & KEY_A) {
				mode = 2;
			} else if (KEYS & KEY_B) {
				mode = 0;
			} else if(KEYS & KEY_Y) {
				address = jumpToAddress(address);
				clearScreen();
			}
		} else if(mode == 2) {
			if (KEYS & KEY_UP) {
				ramPtr[cursorPosition]++;
			} else if (KEYS & KEY_DOWN) {
				ramPtr[cursorPosition]--;
			} else if (KEYS & KEY_LEFT) {
				ramPtr[cursorPosition] -= 0x10;
			} else if (KEYS & KEY_RIGHT) {
				ramPtr[cursorPosition] += 0x10;
			} else if (KEYS & (KEY_A | KEY_B)) {
				if(arm7Ram) {
					sharedAddr[0] = (vu32)arm7RamBuffer;
					sharedAddr[1] = (vu32)address;
					sharedAddr[2] = cursorPosition;
					sharedAddr[4] = 0x574D4152; // RAMW
					while (sharedAddr[4] == 0x574D4152) {
						while (REG_VCOUNT != 191) swiDelay(100);
						while (REG_VCOUNT == 191) swiDelay(100);
					}
					ramLoaded = false;
				}
				mode = 1;
			}
		}
	}
}

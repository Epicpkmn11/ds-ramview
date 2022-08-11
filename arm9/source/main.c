#include "ramViewer.h"
#include "tonccpy.h"

#include "ascii_bin.h"

#include <nds/arm9/background.h>
#include <nds/system.h>

#define SHARED_ADDR_DS (0x02FFFA0C)
#define SHARED_ADDR_DSI (0x0CFFFA0C)

vu32* volatile sharedAddr;

static u16 pal[] = {
	0xFFFF, // White
	0xDEF7, // Light gray
	0xCE73, // Darker gray
	0xF355, // Light blue
	0x801B, // Red
	0x8360, // Lime
};

int main(void) {
	REG_DISPCNT_SUB = MODE_0_2D | DISPLAY_BG3_ACTIVE;
	REG_BG0CNT_SUB = 0;
	REG_BG1CNT_SUB = 0;
	REG_BG2CNT_SUB = 0;
	REG_BG3CNT_SUB = (u16)(BG_MAP_BASE(15) | BG_TILE_BASE(0) | BgSize_T_256x256);

	VRAM_H_CR = VRAM_ENABLE | VRAM_H_SUB_BG;

	for(int i = 0; i < ascii_bin_size; i++) {	// Load font from 1bpp to 4bpp
		u8 val = ascii_bin[i];
		BG_GFX_SUB[i * 2]     = (val & 1) | ((val & 2) << 3) | ((val & 4) << 6) | ((val & 8) << 9);
		val >>= 4;
		BG_GFX_SUB[i * 2 + 1] = (val & 1) | ((val & 2) << 3) | ((val & 4) << 6) | ((val & 8) << 9);
	}

	for(int i = 0; i < sizeof(pal) / sizeof(pal[0]); i++) {
		BG_PALETTE_SUB[i * 0x10 + 1] = pal[i];
	}

	if(isDSiMode())
		sharedAddr = (vu32 *)SHARED_ADDR_DSI;
	else
		sharedAddr = (vu32 *)SHARED_ADDR_DS;

	ramViewer();

	return 0;
}

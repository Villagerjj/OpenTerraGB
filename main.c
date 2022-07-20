#include <gb/cgb.h>
#include <gb/drawing.h>
#include <gb/gb.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <rand.h>
#include <types.h>
#include "worldtiles.c"
#include "player.c"
#include "itemIDs.h"

#define SCREEN_WIDTH 22 
#define SCREEN_HEIGHT 20
#define TILE_SIZE 8 // Size of a tile in pixels.
#define MAP_WIDTH 512
#define MAP_HEIGHT 16

// The map is actually 12 times larger than this; it is split up across 12 SRAM
// banks into 16 tile tall rows.
extern uint8_t map[8192];

// Generic structure for entities, such as the player, NPCs, or enemies.
typedef struct entity {
	uint16_t x;
	uint8_t y;
} entity;

entity player;

struct {
	uint16_t x;
	uint8_t y;
} camera;

// Places a block into VRAM only.
void drawBlock(uint16_t x, uint8_t y, uint8_t blockID) {
	set_bkg_tile_xy(x, y, blockID);
}

// Places a block into SRAM only.
void setBlock(uint16_t x, uint8_t y, uint8_t blockID) {
	map[y * MAP_WIDTH + x] = blockID;
}

// Places a block in both SRAM and VRAM.
void placeBlock(uint16_t x, uint8_t y, uint8_t blockID) {
	map[y * MAP_WIDTH + x] = blockID;
	set_bkg_tile_xy(x, y, blockID);
}

uint8_t getBlock(uint16_t x, uint8_t y) {
	return map[y * MAP_WIDTH + x];
}

void drawWorld() {
	for (uint8_t y = camera.y; y < camera.y + SCREEN_HEIGHT; y++) {
		for (uint16_t x = camera.x; x < camera.x + SCREEN_WIDTH; x++) {
			// TODO: Checking this upon each redraw is extremely slow.
			// Design a block update system so that things such as grass
			// turning to dirt only take place when the nearby blocks have
			// changed.
			if (getBlock(camera.x, camera.y) == DIRT) {
				// If the above block is air, turn into grass.
				if (getBlock(camera.x, camera.y - 1) == AIR) {
					setBlock(camera.x, camera.y, GRASS);
				}
			}
			drawBlock(x, y);
		}
	}
}

uint8_t randomInRange(uint8_t lower, uint8_t upper) {
	return arand() % (upper - lower + 1) + lower;
}

bool randomPercent(uint8_t chance) {
	return arand() % (101) <= chance;
}

void display() {
	drawWorld();
	// TODO: Draw entities
}

void generateWorld() {
	initarand(sys_time);
	uint8_t noise[MAP_WIDTH] = {0};
	uint8_t level = randomInRange(0, 3);
	#define maxterhi 15
	#define minterhi 0

	for (uint16_t i = 0; i < MAP_WIDTH; i++) {
		if (randomPercent(25) && level != maxterhi) {
			level += 1;
			noise[i] = level;
		} else if (randomPercent(5) && level != maxterhi) {
			level += 2;
			noise[i] = level;
		} else if (randomPercent(35) && level != minterhi) {
			level -= 1;
			noise[i] = level;
		} else if (randomPercent(5) && level != minterhi) {
			level -= 2;
			noise[i] = level;
		} else {
			noise[i] = noise[i - 1];
		}
	}
	
	for (uint8_t y = 0; y < MAP_HEIGHT; y++) {
		for (uint16_t x = 0; x < MAP_WIDTH; x++) {
			if (y >= 25) {
				setBlock(x, y - noise[x] , STONE);
			} else if (y >= 20) {
				setBlock(x, y - noise[x], DIRT);
			}
		}
	}
}

void init() {
	memset(map, 0, sizeof(map));
	generateWorld();
	player = {
		.x = 10,
		.y = 8
	};
	blocklocation = player.y * MAP_WIDTH + player.x + 1;
}

void main(void) {
	ENABLE_RAM;
	scroll_bkg(8,8);
	move_sprite(0, 80, 72);
	init();
	set_bkg_data(0, 16, blocks);
	set_sprite_data(0, 7, player);
	set_sprite_tile(0, 6);
	//drawPlayer();
	SHOW_SPRITES;
	SHOW_BKG;
	drawWorld();
	display();
	// The player's selected block.
	uint8_t selectedBlock = 4;
	// The X position that the player is facing
	uint16_t facingX;

	while (1) {
		// joypad() takes a while to execute, so save its result and reuse it as needed.
		// This also ensures that the keys pressed will stay consistent across a game tick.
		uint8_t cur_keys = joypad();
		
		if (cur_keys & J_A) {
			// Attempt to fill the block to the right with the selected block.
			if (getBlock(facingX, player.y) == AIR) {
				setBlock(facingX, player.y, selectedBlock);
				// TODO: Design a more modular block update system.
				if (getBlock(facingX, player.y - 1) == GRASS) {
					setBlock(facingX, player.y - 1, DIRT);
				}
			}
		} 

		if (cur_keys & J_B) {
			if (getBlock(facingX, player.y) != 0) {
				setBlock(facingX, player.y, 0);
				if (getBlock(facingX, player.y - 1) == DIRT) {
					setBlock(facingX, player.y - 1, GRASS);
				}
			}
		}

		// Iterate through the available blocks.
		if (cur_keys & J_SELECT) {
			selectedBlock < 16 ? selectedBlock++ : selectedBlock = 1;
		}
		
		if (cur_keys & J_START) {
			init();
		}
		
		if (cur_keys & J_UP) {
			//scroll_bkg(0,-8);
			camera.y -= 1;
			player.y -= 1;
		} else if (cur_keys & J_DOWN) {
			//scroll_bkg(0,8);
			camera.y += 1;
			player.y += 1;
		}
		
		if (cur_keys & J_LEFT) {
			player.x -= 1;
			facingX = player.x - 1;
			set_sprite_prop(0, S_FLIPX);
			camera.x -= 1;
		} else if (cur_keys & J_RIGHT) {
			player.x += 1;
			facingX = player.x + 1;
			set_sprite_prop(0, 0);
			camera.x += 1;
		}
		
		// If any buttons are pressed, redraw the world.
		if (cur_keys) {
			display();
		}
		
		wait_vbl_done();
	}
}

#include <gb/cgb.h>
#include <gb/drawing.h>
#include <gb/gb.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <rand.h>
#include <types.h>
#include "worldtiles.h"
#include "player.h"
#include "itemIDs.h"
#include "bankLUT.h"

#define SCREEN_WIDTH 22
#define SCREEN_HEIGHT 20
#define CACHE_WIDTH 32 // how may tiles in the X direction to cache into VRAM
#define CACHE_HEIGHT 32
#define TILE_SIZE 8 // Size of a tile in pixels.
#define MAP_WIDTH 512
#define MAP_HEIGHT 192
#define MAP_CHUNK_HEIGHT 16
#define WORLD_BLOCK_COUNT 98304
#define CAM_JUMP_X 6   // the amount in the X value for the camera to jump too when loading stuff into VRAM
#define CAM_JUMP_Y 6   // the amount in the Y value for the camera to jump too when loading stuff into VRAM
#define CAM_JUMPBY_X 4 // the amount in the X value for the camera to jump too when loading stuff into VRAM
#define CAM_JUMPBY_Y 4 // the amount in the Y value for the camera to jump too when loading stuff into VRAM
#define MAX_DELAY 100  // the delay used when just scrolling
#define MIN_DELAY 10   // the delay used when writing to VRAM
#define VRAM_BORDER 12 // (CAM_JUMP_X + CAM_JUMPBY_X) the X/Y value Camera.Ox/Oy need to be at or above to trigger a refresh of VRAM

// The map is actually 12 times larger than this; it is split up across 12 SRAM
// banks into 16 tile tall rows.
extern uint8_t map[8192];
// Generic structure for entities, such as the player, NPCs, or enemies.
typedef struct entity
{
  uint16_t x;
  uint8_t y;
} entity;

entity player;

struct
{
  uint16_t x;
  uint8_t y;
  uint16_t Ox;
  uint8_t Oy;
} camera;

// Places a block into VRAM only.
void drawBlock(uint16_t x, uint8_t y, uint8_t blockID)
{
  set_bkg_tile_xy(x, y, blockID);
}

// Places a block into SRAM only.
void setBlock(uint16_t x, uint8_t y, uint8_t blockID)
{
  SWITCH_RAM(y/16);
  map[(y % 16) * MAP_WIDTH + x] = blockID;
}

// gets block from SRAM
uint8_t getBlockOLD(uint16_t x, uint8_t y)
{
  return map[y * MAP_WIDTH + x];
}

uint8_t getBlock(uint16_t x, uint8_t y)
{
  SWITCH_RAM(y/16);
  return map[(y % 16) * MAP_WIDTH + x];
}

// loads the block from SRAM and then places it into the world
void loadblock(uint16_t x, uint8_t y)
{
  set_bkg_tile_xy(x, y, map[y * MAP_WIDTH + x]);
}

void drawWorld()
{
  for (uint8_t y = 0; y < SCREEN_HEIGHT; y++)
  {
    for (uint16_t x = 0; x < SCREEN_WIDTH; x++)
    {
      set_bkg_tile_xy(x, y, getBlock(camera.x + x, camera.y + y));
    }
  }
}

uint8_t randomInRange(uint8_t lower, uint8_t upper)
{
  return arand() % (upper - lower + 1) + lower;
}

void placeBlockObject(uint16_t x, uint8_t y, uint8_t ObjectID)
{
       switch (ObjectID)
       {
       case 1: //tree
        setBlock(x, y, LOG);
        setBlock(x, y-1, LOG);
        setBlock(x, y-2, LOG);
        setBlock(x, y-3, LOG);
        setBlock(x, y-4, LEAVES);
        setBlock(x-1, y-5, LEAVES);
        setBlock(x, y-5, LEAVES);
        setBlock(x+1, y-5, LEAVES);
        setBlock(x-1, y-4, LEAVES);
        setBlock(x+1, y-4, LEAVES);
        break;
      
      case 2: //House
        setBlock(x-2, y, STONE);
        setBlock(x-2, y-1, STONE);
        setBlock(x-2, y-2, STONE);
        setBlock(x-2, y-2, STONE);
        setBlock(x-1, y-2, STONE);
        setBlock(x, y-2, STONE);
        setBlock(x+1, y-2, STONE);
        setBlock(x+2, y-2, STONE);
        setBlock(x+2, y-1, STONE);
        setBlock(x+2, y, STONE);
        break;
       
       default:
        break;
       }
}

void CaveGen(uint16_t x, uint8_t y, uint8_t size)
{
  for(uint8_t x1 = 0; x1 < size; x1++)
  {
    for (uint8_t y1 = 0; y1 < size; y1++)
    {
      setBlock(x+x1, y+y1, DIRT_WALL);
    }
  }
   
}


bool randomPercent(uint8_t chance)
{
  if ((arand() % (256)) <= chance)
  {
    return TRUE;
  }
  else
  {
    return FALSE;
  }
}

void display()
{
  drawWorld();
  // TODO: Draw entities
}

void generateWorld()
{
  initarand(sys_time);
  uint8_t noise[MAP_WIDTH];
  uint8_t level = 1;
#define maxterhi 12
#define minterhi 0

  for (uint16_t i = 0; i < MAP_WIDTH; i++)
  {

    if (randomPercent(50) == TRUE)
    {
      level += 1;
      goto SKIP0;
    }

    if (randomPercent(12) == TRUE)
    {
      level += 2;
      goto SKIP0;
    }

    if (randomPercent(102) == TRUE)
    {
      level -= 1;
      goto SKIP0;
    }

    if (randomPercent(12) == TRUE)
    {
      level -= 2;
      goto SKIP0;
    }

    noise[i] = noise[i - 1];
    goto SKIP1;

  SKIP0:

    noise[i] = level;

  SKIP1:
  }

  for (uint8_t y = 0; y < MAP_HEIGHT; y++)
  {
    for (uint16_t x = 0; x < MAP_WIDTH; x++)
    {
      // setBlock(x, y - noise[x], AIR);
      if (y <= 65)
      {
        goto SKIP2;
      }


      if (y >= 70)
      {
        setBlock(x, y - noise[x], STONE);
        
      }
      else
      {
        setBlock(x, y - noise[x], DIRT);
      }

      // setBlock(x, (y - noise[x]) + 1, STONE); 

      if (getBlock(x, (y - noise[x]) - 1) == AIR) // checks the above 2 blocks for air, and if there is air, turn into grass
      {
        if(randomPercent(10) == TRUE)
        {
          placeBlockObject(x, (y - noise[x]) - 1, TREE);
        }
        else
        {
          setBlock(x, y - noise[x], GRASS);
        }
        
      }

    SKIP2:
    }
  }
  
  for(uint8_t y = 0; y < MAP_HEIGHT; y++)
  {
    for(uint16_t x = 0; x < MAP_WIDTH; x++)
    {
      if(y <= 75)
      {
        goto SKIP4;
      }
      if(getBlock(x, y) == STONE && randomPercent(5) == TRUE)
        {
          CaveGen(x,y - noise[x], 6);
          
        }
      SKIP4:
    }
  }
  
}





void zeroworld()
{
  for (uint16_t y = 0; y < MAP_HEIGHT; y++)
  {
    for (uint16_t x = 0; x < MAP_WIDTH; x++)
    {
      setBlock(x, y, AIR);
    }
  }
}

void init()
{
  // memset(map, 0, sizeof(map));
  zeroworld();
  generateWorld();
  player.x = 11;
  player.y = 9;
  camera.x = 1;
  camera.y = 1;
//  camera.Ox = CAM_JUMP_X;
 // camera.Oy = CAM_JUMP_Y;
}

void main(void)
{
  ENABLE_RAM;
  init();
  set_bkg_data(0, 16, blocks);
  set_sprite_data(0, 7, playertiles);
  set_sprite_tile(0, 6);
  scroll_bkg(8, 8);
  move_sprite(0, 80, 72);
  uint16_t facingX = player.x + 1;
  SHOW_SPRITES;
  SHOW_BKG;
  // drawWorld();
  display();
  // The player's selected block.
  uint8_t selectedBlock = 4;

  while (1)
  {
    // joypad() takes a while to execute, so save its result and reuse it as needed.
    // This also ensures that the keys pressed will stay consistent across a game tick.
    uint8_t cur_keys = joypad();

    if (cur_keys & J_A)
    {
      // Attempt to fill the block to the right with the selected block.
      if (getBlock(facingX, player.y) == AIR)
      {
        setBlock(facingX, player.y, selectedBlock);
        // TODO: Design a more modular block update system.
        if (getBlock(facingX, player.y + 1) == GRASS)
        {
          setBlock(facingX, player.y + 1, DIRT);
        }
      }
    }

    if (cur_keys & J_B)
    {
      if (getBlock(facingX, player.y) != 0)
      {
        setBlock(facingX, player.y, 0);
        if (getBlock(facingX, player.y + 1) == DIRT)
        {
          setBlock(facingX, player.y + 1, GRASS);
        }
      }
    }

    // Iterate through the available blocks.
    if (cur_keys & J_SELECT)
    {
      if (selectedBlock < 16)
      {
        selectedBlock++;
      }
      else
      {
        selectedBlock = 1;
      }
    }

    if (cur_keys & J_START)
    {
      init();
    }

    if (cur_keys & J_UP)
    {

      camera.y -= 1;
      player.y -= 1;
    }
    else if (cur_keys & J_DOWN)
    {
      // scroll_bkg(0,8);
      camera.y += 1;
      player.y += 1;
    }

    if (cur_keys & J_LEFT)
    {
      player.x -= 1;
      facingX = player.x - 1;
      set_sprite_prop(0, S_FLIPX);
      camera.x -= 1;
    }
    else if (cur_keys & J_RIGHT)
    {
      player.x += 1;
      facingX = player.x + 1;
      set_sprite_prop(0, 0);
      camera.x += 1;
    }

    // If any buttons are pressed, redraw the world.
    if (cur_keys)
    {
      display();
    }

    //
    wait_vbl_done();
  }
}

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
#include "inventory.h"
#include "invTiles.h"
#include "numbers.h"
#include "Cursor.h"

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
#define INV_MAX 24
#define INV_TILE_OFFSET 22
// The map is actually 12 times larger than this; it is split up across 12 SRAM
// banks into 16 tile tall rows.
extern uint8_t map[8192];
extern uint8_t InvItems[INV_MAX];
extern uint8_t InvNumbers[INV_MAX];
uint8_t Gstate = 0; //0 for world, 1 for inventory, 2 for chests, 3 for title screen
uint16_t facingX;
uint8_t cursorx = 0;
uint8_t cursory = 0;
uint8_t itemCache;
uint8_t itemNumCache;
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

void menuDrawItem(uint16_t x, uint8_t y, uint8_t blockID)
{
  set_bkg_tile_xy(x, y, blockID+INV_TILE_OFFSET);
}


void addItem2Inv(uint8_t BlockID, uint8_t Amount)
{
  SWITCH_RAM(13);
  if(BlockID == 0)
  {
    goto Inv1Skip;
  }
  for(uint8_t i = 0; i < INV_MAX; i++)
  {
    if(InvItems[i] == BlockID)
    {
      InvNumbers[i] += Amount;
      break;
    }
    else if(InvItems[i] == 0)
    {
      InvItems[i] = BlockID;
      InvNumbers[i] = Amount;
      break;
    }
  
  }
  Inv1Skip:
}


void menuDrawNumbers(uint16_t x, uint8_t y, uint8_t val1)
{
  if(val1 == 0)
  {
    set_bkg_tile_xy(x, y, 12);
  set_bkg_tile_xy(x+1, y, 12);
  }
  else if(val1 < 10)
  {
  set_bkg_tile_xy(x, y, 12);
  set_bkg_tile_xy(x+1, y, (val1+12) );
  }
  else if (val1 > 9 && val1 < 20)
  {
  set_bkg_tile_xy(x, y, 1+12);
  set_bkg_tile_xy(x+1, y, (val1+12) - 10);
  }
  else if (val1 > 19 && val1 < 30)
  {
  set_bkg_tile_xy(x, y, 2+12);
  set_bkg_tile_xy(x+1, y, (val1+12) - 20);
  }
  else if (val1 > 29 && val1 < 40)
  {
  set_bkg_tile_xy(x, y, 3+12);
  set_bkg_tile_xy(x+1, y, (val1+12) - 30);
  }
  else if (val1 > 39 && val1 < 50)
  {
  set_bkg_tile_xy(x, y, 4+12);
  set_bkg_tile_xy(x+1, y, (val1+12) - 40);
  }
  else if (val1 > 49 && val1 < 60)
  {
  set_bkg_tile_xy(x, y, 5+12);
  set_bkg_tile_xy(x+1, y, (val1+12) - 50);
  }
  else if (val1 > 59 && val1 < 70)
  {
  set_bkg_tile_xy(x, y, 6+12);
  set_bkg_tile_xy(x+1, y, (val1+12) - 60);
  }
  else if (val1 > 69 && val1 < 80)
  {
  set_bkg_tile_xy(x, y, 7+12);
  set_bkg_tile_xy(x+1, y, (val1+12) - 70);
  }
  else if (val1 > 79 && val1 < 90)
  {
  set_bkg_tile_xy(x, y, 8+12);
  set_bkg_tile_xy(x+1, y, (val1+12) - 80);
  }
  else if (val1 > 89 && val1 < 100)
  {
  set_bkg_tile_xy(x, y, 9+12);
  set_bkg_tile_xy(x+1, y, (val1+12) - 90);
  }
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


void loadmenu()
{
  set_bkg_data(0,12,invtiles);
  set_bkg_tiles(1,1,20,18, inventorytilemap);
  set_sprite_data(0, 2, Cursor);
  set_sprite_tile(0, 0);
  move_sprite(0, 24, 48);
  cursorx = 0;
  cursory = 0;
  SWITCH_RAM(13);
  
  for(uint8_t i = 0; i < INV_MAX; i++)
  {
    
    set_bkg_data(InvItems[i]+INV_TILE_OFFSET, 1, blocks + 16 * InvItems[i]);
    
    
  }
  for(uint8_t i = 0; i < 10; i++)
  {
    set_bkg_data(i+12, 1, numbers + 16 * i);
  }
  for(uint16_t x = 0; x < 6; x += 1)
  {
    for(uint8_t y = 0; y < 4; y += 1)
    {
      menuDrawItem((x*3)+3,(y*3)+5,InvItems[(y * 6 + x)]);
      menuDrawNumbers((x*3)+3,(y*3)+6,InvNumbers[(y * 6 + x)]);
      
    }
  }
  
  
}

void Updatemenu()
{
  
  SWITCH_RAM(13);


  
  for(uint16_t x = 0; x < 6; x += 1)
  {
    for(uint8_t y = 0; y < 4; y += 1)
    {
      menuDrawItem((x*3)+3,(y*3)+5,InvItems[(y * 6 + x)]);
      menuDrawNumbers((x*3)+3,(y*3)+6,InvNumbers[(y * 6 + x)]);
      
    }
  }
  
  
}

void closemenu()
{
  set_bkg_data(0,8,blocks);
  set_sprite_data(0, 7, playertiles);
  set_sprite_tile(0, 6);
  move_sprite(0, 80, 72);
  SWITCH_RAM(0);
  display();
}

void controls(uint8_t cur)
{

   

uint8_t selectedBlock = 4;

switch (Gstate)
{
case 0: //world rendering
  if (cur & J_A)
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

    if (cur & J_B)
    {
      uint8_t tempblock = getBlock(facingX, player.y);
      
      if (tempblock != 0)
      {
        setBlock(facingX, player.y, 0);
        
        if (getBlock(facingX, player.y + 1) == DIRT)
        {
          setBlock(facingX, player.y + 1, GRASS);
        }
        addItem2Inv(tempblock, 1);
      }
    }

    // Iterate through the available blocks.
    if (cur & J_SELECT)
    {
      
      Gstate = 1;
      loadmenu();
      
    }

    if (cur & J_START)
    {
      //init();
    }

    if (cur & J_UP)
    {

      camera.y -= 1;
      player.y -= 1;
    }
    else if (cur & J_DOWN)
    {
      // scroll_bkg(0,8);
      camera.y += 1;
      player.y += 1;
    }

    if (cur & J_LEFT)
    {
      player.x -= 1;
      facingX = player.x - 1;
      set_sprite_prop(0, S_FLIPX);
      camera.x -= 1;
    }
    else if (cur & J_RIGHT)
    {
      player.x += 1;
      facingX = player.x + 1;
      set_sprite_prop(0, 0);
      camera.x += 1;
    
    }

    if(Gstate == 0 && cur)
    {
      display();
    }
    delay(100);
  break;

case 1: //inventory

  if (cur & J_A)
    {
      if(get_sprite_tile(0) == 0)
      {
        set_sprite_tile(0, 1);
        itemCache = InvItems[(cursory * 6 + cursorx)];
        itemNumCache = InvNumbers[(cursory * 6 + cursorx)];
        
      }
      else if(get_sprite_tile(0) == 1) {
       set_sprite_tile(0, 0);
       if(InvItems[(cursory * 6 + cursorx)] == itemCache)
       {
        InvNumbers[(cursory * 6 + cursorx)] += itemNumCache;
       }
       else
       {
        InvItems[(cursory * 6 + cursorx)] = itemCache;
        InvNumbers[(cursory * 6 + cursorx)] = itemNumCache;
       }
       
       Updatemenu();
      }

    
    delay(100);
      
    }

    if (cur & J_B)
    {
      
    }

    // Iterate through the available blocks.
    if (cur & J_SELECT)
    {
      
      Gstate = 0;
      closemenu();
      
    }

    if (cur & J_START)
    {
      
    }

    if (cur & J_UP)
    {
      if(cursory != 0)
      {
        cursory -= 1;
      }
      else
      {
        cursory = 0;
      }
        
    }
    else if (cur & J_DOWN)
    {
      if(cursory != 3)
      {
        cursory += 1;
      }
      else
      {
        cursory = 3;
      }
      
    }

    if (cur & J_LEFT)
    {
      if(cursorx != 0)
      {
        cursorx -= 1;
      }
      else
      {
        cursorx = 0;
      }
      
    }
    else if (cur & J_RIGHT)
    {
      if(cursorx != 5)
      {
        cursorx += 1;
      }
      else
      {
        cursorx = 5;
      }
    }

    if(Gstate == 1 && cur)
    {
      move_sprite(0, (cursorx+1)*24, (cursory+2)*24 );
    }
delay(100);
  break;

case 2: //chests
  /* code */
  break;

default:
  break;
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
  if(Gstate != 0)
  {

  }
  else
  {
    
  init();
  }
  SWITCH_RAM(13);
  for(uint8_t u = 0; u < INV_MAX; u++)
  {
    InvItems[u] = 0;
    InvNumbers[u] = 0;
  }
  SWITCH_RAM(0);
  set_bkg_data(0, 8, blocks);
  set_sprite_data(0, 7, playertiles);
  set_sprite_tile(0, 6);
  scroll_bkg(8, 8);
  move_sprite(0, 80, 72);
  
  SHOW_SPRITES;
  SHOW_BKG;
  // drawWorld();
  
  display();
  // The player's selected block.


  while (1)
  {
    // joypad() takes a while to execute, so save its result and reuse it as needed.
    // This also ensures that the keys pressed will stay consistent across a game tick.
    
    
    controls(joypad());

    // If any buttons are pressed, redraw the world.
    
    //
    wait_vbl_done();
  }
}

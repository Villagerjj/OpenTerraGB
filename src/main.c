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
#include "inventory.h"
#include "invTiles.h"
#include "numbers.h"
#include "Cursor.h"
#include "Crafting.h"
#include "Titlescreen.h"
#include "loadingscreen.h"
#include "Titletiles.h"
//#include "gbt_player.h"

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
#define INV_TILE_OFFSET 23
#define CRAFT_TILE_OFFSET 48
#define CRAFTS 3       // how many recipes there are in the game
#define BLOCK_TOTAL 20 // how many blocks there are in the game.
#define randomPercent(chance) ((arand()) % (256) <= (chance) ? 1 : 0)
#define INV_BANK 13
// The map is actually 12 times larger than this; it is split up across 12 SRAM
// banks into 16 tile tall rows.
extern uint_fast8_t map[8192];
extern uint8_t InvItems[INV_MAX];
extern uint8_t InvNumbers[INV_MAX];
extern const unsigned char *song_Data[];
uint8_t Gstate = 0; // 0 for world, 1 for inventory, 2 for chests, 3 for title screen
uint16_t facingX;
uint8_t facingY;
uint8_t cursorx = 0;
uint8_t cursory = 0;
uint8_t itemCache;
uint8_t itemNumCache;
uint8_t CraftItems[6];
uint8_t CraftNums[6];
uint8_t RecipesItems[6];
uint8_t RecipesNums[6];
uint8_t progress = 0;

bool NearWorkBench = FALSE;
// Generic structure for entities, such as the player, NPCs, or enemies.
typedef struct entity
{
  uint16_t x; // tile cord of thing
  uint8_t y;
  uint16_t Px; // pixel cord of thingy
  uint8_t Py;
} entity;

entity player;

struct
{
  uint16_t x;
  uint8_t y;
  uint16_t OLDx;
  uint8_t OLDy;
} camera;

// Places a block into VRAM only.
void drawBlock(uint16_t x, uint8_t y, uint8_t blockID)
{
  set_bkg_tile_xy(x, y, blockID);
}

void menuDrawItem(uint16_t x, uint8_t y, uint8_t blockID)
{
  set_bkg_tile_xy(x, y, blockID + INV_TILE_OFFSET);
}

void subItemFromInv(uint8_t BlockID, uint8_t Amount)
{
  SWITCH_RAM(INV_BANK);
  for (uint8_t i = 0; i < INV_MAX; i++)
  {
    if (InvItems[i] == BlockID)
    {
      if (InvNumbers[i] - Amount >= 0)
      {
        InvNumbers[i] -= Amount;
      }
      if (InvNumbers[i] == 0)
      {
        InvItems[i] = 0;
      }
      break;
    }
  }
}

void addItem2Inv(uint8_t BlockID, uint8_t Amount)
{
  SWITCH_RAM(INV_BANK);
  for (uint8_t i = 0; i < INV_MAX; i++)
  {
    if (InvItems[i] == BlockID && InvNumbers[i] < 99)
    {
      if (InvNumbers[i] + Amount <= 99)
      {
        InvNumbers[i] += Amount;
      }
      else
      {
        for (uint8_t i = 0; i < INV_MAX; i++)
        {
          if (InvItems[i] == BlockID)
          {
            InvNumbers[i] += Amount;
          }
          else if (InvItems[i] == 0)
          {
            InvItems[i] = BlockID;
            InvNumbers[i] = Amount;
            break;
          }
        }
      }
      break;
    }
    else if (InvItems[i] == 0)
    {
      InvItems[i] = BlockID;
      InvNumbers[i] = Amount;
      break;
    }
  }
}

void menuDrawNumbers(uint16_t x, uint8_t y, uint8_t val1)
{
  if (val1 == 0)
  {
    set_bkg_tile_xy(x, y, 12);
    set_bkg_tile_xy(x + 1, y, 12);
  }
  else if (val1 < 10)
  {
    set_bkg_tile_xy(x, y, 12);
    set_bkg_tile_xy(x + 1, y, (val1 + 12));
  }
  else if (val1 > 9 && val1 < 20)
  {
    set_bkg_tile_xy(x, y, 1 + 12);
    set_bkg_tile_xy(x + 1, y, (val1 + 12) - 10);
  }
  else if (val1 > 19 && val1 < 30)
  {
    set_bkg_tile_xy(x, y, 2 + 12);
    set_bkg_tile_xy(x + 1, y, (val1 + 12) - 20);
  }
  else if (val1 > 29 && val1 < 40)
  {
    set_bkg_tile_xy(x, y, 3 + 12);
    set_bkg_tile_xy(x + 1, y, (val1 + 12) - 30);
  }
  else if (val1 > 39 && val1 < 50)
  {
    set_bkg_tile_xy(x, y, 4 + 12);
    set_bkg_tile_xy(x + 1, y, (val1 + 12) - 40);
  }
  else if (val1 > 49 && val1 < 60)
  {
    set_bkg_tile_xy(x, y, 5 + 12);
    set_bkg_tile_xy(x + 1, y, (val1 + 12) - 50);
  }
  else if (val1 > 59 && val1 < 70)
  {
    set_bkg_tile_xy(x, y, 6 + 12);
    set_bkg_tile_xy(x + 1, y, (val1 + 12) - 60);
  }
  else if (val1 > 69 && val1 < 80)
  {
    set_bkg_tile_xy(x, y, 7 + 12);
    set_bkg_tile_xy(x + 1, y, (val1 + 12) - 70);
  }
  else if (val1 > 79 && val1 < 90)
  {
    set_bkg_tile_xy(x, y, 8 + 12);
    set_bkg_tile_xy(x + 1, y, (val1 + 12) - 80);
  }
  else if (val1 > 89 && val1 < 100)
  {
    set_bkg_tile_xy(x, y, 9 + 12);
    set_bkg_tile_xy(x + 1, y, (val1 + 12) - 90);
  }
}
// Places a block into SRAM only.
void setBlock(uint16_t x, uint8_t y, uint8_t blockID)
{
  SWITCH_RAM(y / 16);
  map[(y % 16) * MAP_WIDTH + x] = blockID;
  // drawBlock(x+camera.x, y+camera.y, blockID);
}

// gets block from SRAM
uint8_t getBlock(uint16_t x, uint8_t y)
{
  SWITCH_RAM(y / 16);
  return map[(y % 16) * MAP_WIDTH + x];
}

// loads the block from SRAM and then places it into the world
void loadblock(uint16_t x, uint8_t y)
{
  set_bkg_tile_xy(x, y, map[y * MAP_WIDTH + x]);
}

void submap(uint16_t x, uint8_t y, uint8_t w, uint8_t h)
{
  // uint8_t Ny = o + 17;
  // uint16_t Nx = p + 19;
  // uint8_t Nh = h + 18;
  // uint8_t Nw = w + 20;

  for (uint8_t o = y; o < h + y; o++)
  {
    for (uint16_t p = x; p < w + x; p++)
    {
      set_bkg_tile_xy(RenderLUT[p], RenderLUT[o], getBlock(p, o));
    }
  }
}
// draws the camera viewport
void drawWorld()
{
  SCX_REG = camera.x * 8;
  SCY_REG = camera.y * 8;
  if (camera.x > camera.OLDx)
  {
    submap(camera.x + 19, camera.y, 2, 18);
  }
  else if (camera.x < camera.OLDx)
  {
    submap(camera.x, camera.y, 2, 18);
  }

  if (camera.y > camera.OLDy)
  {
    submap(camera.x, camera.y + 17, 20, 2);
  }
  else if (camera.y < camera.OLDy)
  {
    submap(camera.x, camera.y, 20, 2);
  }
  camera.OLDx = camera.x;
  camera.OLDy = camera.y;
}

uint8_t randomInRange(uint8_t lower, uint8_t upper)
{
  return arand() % (upper - lower + 1) + lower;
}

void placeBlockObject(uint16_t x, uint8_t y, uint8_t ObjectID)
{
  switch (ObjectID)
  {
  case 1: // tree
    setBlock(x, y, LOG);
    setBlock(x, y - 1, LOG);
    setBlock(x, y - 2, LOG);
    setBlock(x, y - 3, LOG);
    setBlock(x, y - 4, LEAVES);
    setBlock(x - 1, y - 5, LEAVES);
    setBlock(x, y - 5, LEAVES);
    setBlock(x + 1, y - 5, LEAVES);
    setBlock(x - 1, y - 4, LEAVES);
    setBlock(x + 1, y - 4, LEAVES);
    break;

  case 2: // House
    setBlock(x - 2, y, STONE);
    setBlock(x - 2, y - 1, STONE);
    setBlock(x - 2, y - 2, STONE);
    setBlock(x - 2, y - 2, STONE);
    setBlock(x - 1, y - 2, STONE);
    setBlock(x, y - 2, STONE);
    setBlock(x + 1, y - 2, STONE);
    setBlock(x + 2, y - 2, STONE);
    setBlock(x + 2, y - 1, STONE);
    setBlock(x + 2, y, STONE);
    break;

  default:
    break;
  }
}

void CaveGen(uint16_t x, uint8_t y, uint8_t size)
{
  for (uint8_t x1 = 0; x1 < size; x1++)
  {
    for (uint8_t y1 = 0; y1 < size; y1++)
    {
      setBlock(x + x1, y + y1, DIRT_WALL);
    }
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
  uint8_t level = 0;
#define maxterhi 12
#define minterhi 0
#define heightFactor 2

  progress += 10;
  for (uint16_t i = 0; i < MAP_WIDTH; i += 4)
  {
    level = noise[i - 4];

    level += randomInRange(0, heightFactor);
    level -= randomInRange(0, heightFactor);

    noise[i] = level;
    noise[i + 1] = level;
    noise[i + 2] = level;
    noise[i + 3] = level;
  }
  progress += 10;
  for (uint16_t x = 0; x < MAP_WIDTH; x++)
  {
    for (uint8_t y = 47; y < 175; y++)
    {
      uint8_t temp = (y + noise[x]);

      if (y > 57)
      {
        setBlock(x, temp, STONE);
      }
      else
      {
        setBlock(x, temp, DIRT);
      }

      // setBlock(x, (y - noise[x]) + 1, STONE);

      if (getBlock(x, temp - 1) == AIR && getBlock(x - 1, temp - 2) == AIR) // checks the above 2 blocks for air, and if there is air, turn into grass
      {
        if (randomPercent(50) == TRUE)
        {
          placeBlockObject(x, temp - 1, TREE);
        }
        else
        {
          setBlock(x, temp, GRASS);
        }
      }
    }
  }
  progress += 10;
  for (uint16_t x = 0; x < MAP_WIDTH; x++)
  {
    for (uint8_t y = 57; y < 175; y++)
    {
      if (getBlock(x, y) == STONE && randomPercent(5) == TRUE)
      {
        CaveGen(x, y, 6);
      }
      setBlock(x, 192, STONE);
    }
  }
}

void findCrafts()
{

  for (uint8_t i = 0; i < 6; i++)
  {
    for (uint8_t t = 0; t < CRAFTS; t++)
    {

      switch (t)
      {
      case 0: // torch
        for (uint8_t r = 0; r < INV_MAX; r++)
        {
          if (InvItems[r] == WOOD)
          {
            if (InvNumbers[r] >= 1)
            {
              for (uint8_t r = 0; r < INV_MAX; r++)
              {
                if (InvItems[r] == GEL)
                {
                  if (InvNumbers[r] >= 1)
                  {
                    CraftItems[i] = TORCH;
                    CraftNums[i] = 3;
                  }
                }
                else
                {
                  break;
                }
              }
            }
          }
          else
          {
            break;
          }
        }
        break;

      case 1: // wooden platform
        for (uint8_t r = 0; r < INV_MAX; r++)
        {
          if (InvItems[r] == WOOD)
          {
            if (InvNumbers[r] >= 1)
            {
              CraftItems[i] = WOODEN_PLATFORM;
              CraftNums[i] = 2;
            }
          }
          else
          {
            break;
          }
        }
        break;

      case 2: // Work Bench
        for (uint8_t r = 0; r < INV_MAX; r++)
        {
          if (InvItems[r] == WOOD)
          {
            if (InvNumbers[r] >= 10)
            {
              CraftItems[i] = WORK_BENCH;
              CraftNums[i] = 1;
            }
          }
          else
          {
            break;
          }
        }
        break;

      case 3: // wooden wall - bench
        for (uint8_t r = 0; r < INV_MAX; r++)
        {
          if (NearWorkBench == TRUE && InvItems[r] == WOOD)
          {
            if (InvNumbers[r] >= 1)
            {
              CraftItems[i] = WOOD_WALL;
              CraftNums[i] = 2;
            }
          }
          else
          {
            break;
          }
        }
        break;
      }
    }
  }
}

void UpdateCraftMenu()
{
  findCrafts();
  for (uint8_t i = 0; i < 6; i++)
  {
    set_bkg_data(RecipesItems[i] + CRAFT_TILE_OFFSET, 1, blocks + 16 * RecipesItems[i]);
  }
  for (uint8_t x = 0; x < 6; x++)
  {

    menuDrawItem((x * 3) + 3, 3 + 8, RecipesItems[x]);
    menuDrawNumbers((x * 3) + 3, 3 + 9, RecipesNums[x]);
  }
}

void Craft(uint8_t slot)
{
  switch (CraftItems[slot])
  {
  case WOODEN_PLATFORM: // wooden platform
    subItemFromInv(WOOD, 1);
    addItem2Inv(WOODEN_PLATFORM, 2);
    UpdateCraftMenu();
    break;

  case WORK_BENCH: // work bench
    subItemFromInv(WOOD, 10);
    addItem2Inv(WORK_BENCH, 1);
    UpdateCraftMenu();
    break;

  case WOOD_WALL: // wooden platform - bench
    subItemFromInv(WOOD, 1);
    addItem2Inv(WOOD_WALL, 4);
    UpdateCraftMenu();
    break;

  default:
    break;
  }
}

void GetRecipes(uint8_t slot)
{
  switch (CraftItems[slot])
  {
  case WOODEN_PLATFORM: // wooden platform
    RecipesItems[0] = WOOD;
    RecipesNums[0] = 1;
    break;

  case WORK_BENCH: // work bench
    RecipesItems[0] = WOOD;
    RecipesNums[0] = 10;
    break;

  case WOOD_WALL: // wooden wall lol
    RecipesItems[0] = WOOD;
    RecipesNums[0] = 1;
    break;

  default:
    break;
  }
}

void LoadCraftMenu()
{
  set_bkg_data(0, 12, invtiles);
  set_bkg_tiles(1, 1, 20, 18, CraftingTileMap);
  set_sprite_data(0, 2, Cursor);
  set_sprite_tile(0, 0);
  SCX_REG = 8;
  SCY_REG = 8;
  move_sprite(0, 24, 72);
  findCrafts();
  GetRecipes(0);
  cursorx = 0;
  cursory = 0;
  SWITCH_RAM(INV_BANK);

  for (uint8_t i = 0; i < 6; i++)
  {

    set_bkg_data(CraftItems[i] + INV_TILE_OFFSET, 1, blocks + 16 * CraftItems[i]);
  }
  for (uint16_t x = 0; x < 6; x++)
  {

    menuDrawItem((x * 3) + 3, 8, CraftItems[x]);
    menuDrawNumbers((x * 3) + 3, 9, CraftNums[x]);
  }
  UpdateCraftMenu();
}

void loadInvmenu()
{
  set_bkg_data(0, 12, invtiles);
  set_bkg_tiles(1, 1, 20, 18, inventorytilemap);
  set_sprite_data(0, 2, Cursor);
  set_sprite_tile(0, 0);
  SCX_REG = 8;
  SCY_REG = 8;
  move_sprite(0, 24, 48);
  cursorx = 0;
  cursory = 0;
  SWITCH_RAM(INV_BANK);

  for (uint8_t i = 0; i < INV_MAX; i++)
  {

    set_bkg_data(InvItems[i] + INV_TILE_OFFSET, 1, blocks + 16 * InvItems[i]);
  }
  for (uint8_t i = 0; i < 10; i++)
  {
    set_bkg_data(i + 12, 1, numbers + 16 * i);
  }
  for (uint16_t x = 0; x < 6; x += 1)
  {
    for (uint8_t y = 0; y < 4; y += 1)
    {
      menuDrawItem((x * 3) + 3, (y * 3) + 5, InvItems[(y * 6 + x)]);
      menuDrawNumbers((x * 3) + 3, (y * 3) + 6, InvNumbers[(y * 6 + x)]);
    }
  }
}

void UpdateInvmenu()
{

  SWITCH_RAM(INV_BANK);
  for (uint8_t i = 0; i < INV_MAX; i++)
  {

    set_bkg_data(InvItems[i] + INV_TILE_OFFSET, 1, blocks + 16 * InvItems[i]);
  }

  for (uint16_t x = 0; x < 6; x += 1)
  {
    for (uint8_t y = 0; y < 4; y += 1)
    {
      menuDrawItem((x * 3) + 3, (y * 3) + 5, InvItems[(y * 6 + x)]);
      menuDrawNumbers((x * 3) + 3, (y * 3) + 6, InvNumbers[(y * 6 + x)]);
    }
  }
}

void closemenu()
{
  set_bkg_data(0, BLOCK_TOTAL, blocks);
  set_sprite_data(0, 7, playertiles);
  set_sprite_tile(0, 0);
  move_sprite(0, 80, 72);
  SWITCH_RAM(0);
  submap(camera.x, camera.y, 20, 18);
  display();
}

void LoadWorld()
{
  SWITCH_RAM(0);
  set_bkg_data(0, BLOCK_TOTAL, blocks);
  set_sprite_data(0, 7, playertiles);
  set_sprite_tile(0, 0);
  // scroll_bkg(8, 8);
  move_sprite(0, 80, 72);
  player.x = 10;
  player.y = 8;
  camera.x = 0;
  camera.y = 0;
  submap(camera.x, camera.y, 20, 18);
  Gstate = 0;
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
  progress++;
}

void init()
{
  SWITCH_RAM(INV_BANK);
  for (uint8_t u = 0; u < INV_MAX; u++)
  {
    InvItems[u] = 0;
    InvNumbers[u] = 0;
  }
  addItem2Inv(WOODEN_SWORD, 1);
  addItem2Inv(WOODEN_PICKAXE, 1);
  addItem2Inv(WOODEN_AXE, 1);
  // addItem2Inv(WOOD, 99);
  SWITCH_RAM(0);
  Gstate = 5;
  set_bkg_tiles(0, 0, 20, 18, loading);

  zeroworld();
  generateWorld();
  set_bkg_data(0, BLOCK_TOTAL, blocks);
  set_sprite_data(0, 7, playertiles);
  set_sprite_tile(0, 0);
  // scroll_bkg(8, 8);
  move_sprite(0, 80, 72);
  player.x = 10;
  player.y = 8;
  camera.x = 0;
  camera.y = 0;
  submap(camera.x, camera.y, 20, 18);
  Gstate = 0;
}

void controls(uint8_t cur)
{

  uint8_t selectedBlock = 4;

  switch (Gstate)
  {
  case 0: // world rendering
    if (cur & J_A)
    {
      SWITCH_RAM(INV_BANK);
      uint8_t tempblock = getBlock(facingX, facingY);
      SWITCH_RAM(INV_BANK);
      if (InvItems[0] == WOODEN_PICKAXE || InvItems[0] == WOODEN_AXE)
      {
        if (tempblock != 0)
        {
          setBlock(facingX, facingY, 0);

          if (getBlock(facingX, facingY + 1) == DIRT)
          {
            setBlock(facingX, facingY + 1, GRASS);
          }
          if (tempblock == GRASS)
          {
            addItem2Inv(DIRT, 1);
          }
          else if (tempblock == LOG)
          {
            addItem2Inv(WOOD, 1);
          }
          else
          {
            addItem2Inv(tempblock, 1);
          }
        }
      }
      else if (InvItems[0] != WOODEN_SWORD && InvItems[0] != WOODEN_PICKAXE && InvItems[0] != WOODEN_AXE)
      {
        if (getBlock(facingX, facingY) == AIR)
        {
          SWITCH_RAM(INV_BANK);
          if (InvNumbers[0] != 0)
          {
            InvNumbers[0] = InvNumbers[0] - 1;
            setBlock(facingX, facingY, InvItems[0]);
            SWITCH_RAM(INV_BANK);
            if (InvNumbers[0] == 0)
            {
              InvItems[0] = 0;
            }
          }

          // TODO: Design a more modular block update system.
          if (getBlock(facingX, facingY + 1) == GRASS)
          {
            setBlock(facingX, facingY + 1, DIRT);
          }
        }
      }
    }

    if (cur & J_B)
    {
      // TODO: add jump features
    }

    // Iterate through the available blocks.
    if (cur & J_SELECT)
    {

      Gstate = 1;
      loadInvmenu();
    }

    if (cur & J_START)
    {
      // init();
    }

    if (cur & J_UP)
    {
      facingX = player.x;
      facingY = player.y - 2;

      
      if (camera.y > 0)
      {
        camera.y -= 1;
        player.y -= 1;
        player.Py -= 8;
      }
      else
      {
        camera.y == 0;
      }
    }
    else if (cur & J_DOWN)
    {
      facingX = player.x;
      facingY = player.y + 2;
      // camera.y += 1;
      
      if (camera.y < 192)
      {
        camera.y += 1;
        player.y += 1;
        player.Py += 8;
      }
      else
      {
        camera.y == 192;
      }
    }

    if (cur & J_LEFT)
    {
      
      facingY = player.y;
      facingX = player.x - 1;
      set_sprite_prop(0, S_FLIPX);
      // camera.x -= 1;
      if (camera.x > 0)
      {
        camera.x -= 1;
        player.x -= 1;
        player.Px -= 8;
      }
      else
      {
        camera.x == 0;
      }
      if (get_sprite_tile(0) == 0)
      {
        set_sprite_tile(0, 1);
      }
      else
      {
        set_sprite_tile(0, 0);
      }
    }
    else if (cur & J_RIGHT)
    {
      
      facingY = player.y;
      facingX = player.x + 1;
      set_sprite_prop(0, 0);
      // camera.x += 1;
      if (camera.x < 512)
      {
        camera.x += 1;
        player.x += 1;
        player.Px += 8;
      }
      else
      {
        camera.x == 512;
      }
      if (get_sprite_tile(0) == 0)
      {
        set_sprite_tile(0, 1);
      }
      else
      {
        set_sprite_tile(0, 0);
      }
    }

    if (Gstate == 0 && cur)
    {
      display();
    }
    delay(100);
    break;

  case 1: // inventory

    if (cur & J_A)
    {
      uint8_t invIndex = (cursory * 6 + cursorx);

      if (get_sprite_tile(0) == 0)
      {

        set_sprite_tile(0, 1);

        itemCache = InvItems[invIndex];
        itemNumCache = InvNumbers[invIndex];
        InvItems[invIndex] = 255;
        InvNumbers[invIndex] = 0;
      }
      else if (get_sprite_tile(0) == 1)
      {
        set_sprite_tile(0, 0);
        if (InvItems[invIndex] == itemCache)
        {
          // InvNumbers[(cursory * 6 + cursorx)] += itemNumCache;
          if (InvNumbers[invIndex] < 99)
          {
            if ((InvNumbers[invIndex] + itemNumCache) <= 99)
            {
              InvNumbers[invIndex] += itemNumCache;
              for (uint8_t i = 0; i < INV_MAX; i++)
              {
                if (InvItems[i] == 255)
                {
                  InvItems[i] = 0;
                  InvNumbers[i] = 0;
                }
              }
            }
            else
            {
              InvNumbers[invIndex] += (99 - InvNumbers[invIndex]);
              itemNumCache -= 99;
              for (uint8_t i = 0; i < INV_MAX; i++)
              {
                if (InvItems[i] == itemCache)
                {
                  if (InvNumbers[invIndex] + itemNumCache <= 99)
                  {
                    InvNumbers[i] += itemNumCache;
                    for (uint8_t i = 0; i < INV_MAX; i++)
                    {
                      if (InvItems[i] == 255)
                      {
                        InvItems[i] = 0;
                        InvNumbers[i] = 0;
                      }
                    }
                  }

                  // InvNumbers[i] += itemNumCache;
                }
                else if (InvItems[i] == 0)
                {

                  for (uint8_t i = 0; i < INV_MAX; i++)
                  {
                    if (InvItems[i] == 255)
                    {
                      InvItems[i] = InvItems[invIndex];
                      InvNumbers[i] = InvNumbers[invIndex];
                    }
                  }
                  InvItems[i] = itemCache;
                  InvNumbers[i] = itemNumCache;
                  break;
                }
              }
            }
          }
          else if (InvItems[invIndex] == 0)
          {

            for (uint8_t i = 0; i < INV_MAX; i++)
            {
              if (InvItems[i] == 255)
              {
                InvItems[i] = InvItems[invIndex];
                InvNumbers[i] = InvNumbers[invIndex];
              }
            }
            InvItems[invIndex] = itemCache;
            InvNumbers[invIndex] = itemNumCache;
          }
        }
        else
        {

          for (uint8_t i = 0; i < INV_MAX; i++)
          {
            if (InvItems[i] == 255)
            {
              InvItems[i] = InvItems[invIndex];
              InvNumbers[i] = InvNumbers[invIndex];
            }
          }
          InvItems[invIndex] = itemCache;
          InvNumbers[invIndex] = itemNumCache;
        }

        UpdateInvmenu();
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
      Gstate = 2;
      cursorx = 0;
      cursory = 0;
      LoadCraftMenu();
    }

    if (cur & J_UP)
    {
      if (cursory != 0)
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
      if (cursory != 3)
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
      if (cursorx != 0)
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
      if (cursorx != 5)
      {
        cursorx += 1;
      }
      else
      {
        cursorx = 5;
      }
    }

    if (Gstate == 1 && cur)
    {
      move_sprite(0, (cursorx + 1) * 24, (cursory + 2) * 24);
    }
    delay(100);
    break;

  case 2: // Crafting
    if (cur & J_A)
    {
      Craft(cursorx);
    }

    if (cur & J_B)
    {
    }

    // Iterate through the available blocks.
    if (cur & J_SELECT)
    {
      cursorx = 0;
      cursory = 0;
      Gstate = 0;
      closemenu();
    }

    if (cur & J_START)
    {
      Gstate = 1;
      loadInvmenu();
    }

    if (cur & J_UP)
    {
    }
    else if (cur & J_DOWN)
    {
    }

    if (cur & J_LEFT)
    {
      if (cursorx != 0)
      {
        cursorx -= 1;
      }
      else
      {
        cursorx = 0;
      }
      GetRecipes(cursorx);
      UpdateCraftMenu();
    }
    else if (cur & J_RIGHT)
    {
      if (cursorx != 5)
      {
        cursorx += 1;
      }
      else
      {
        cursorx = 5;
      }
      GetRecipes(cursorx);
      UpdateCraftMenu();
    }

    if (Gstate == 2 && cur)
    {
      move_sprite(0, (cursorx + 1) * 24, (cursory + 3) * 24);
    }
    delay(100);
    break;

  case 4: // TitleScreen

    // Iterate through the available blocks.
    if (cur & J_SELECT)
    {
      ENABLE_RAM;
      LoadWorld();
    }

    if (cur & J_START)
    {
      ENABLE_RAM;
      init();
    }

    delay(100);
    break;

  case 5: // Loading/no controls
    menuDrawNumbers(19, 14, progress);

  default:
    break;
  }
}

void main(void)
{
  /* disable_interrupts();

    gbt_play(song_Data, 2, 7);
    gbt_loop(1);

    set_interrupts(VBL_IFLAG);
    enable_interrupts(); */

  Gstate = 4;
  SWITCH_RAM(INV_BANK);

  SHOW_SPRITES;
  SHOW_BKG;
  set_bkg_data(0, 49, titletiles);
  set_bkg_tiles(0, 0, 20, 18, Title);

  // display();
  //  The player's selected block.

  while (1)
  {
    wait_vbl_done();

    controls(joypad());
  }
}

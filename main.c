#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdint.h>
#include <gb/drawing.h>
#include <stdio.h>
#include <rand.h>
#include <types.h>
#include "worldtiles.c"
#include "player.c"
#include "itemIDs.h"
#define Vmapx 22 //the view port X value
#define Vmapy 20 //the max y value
#define mapS 8 // mapS is the size in pixels, of each box. NEVER CHANGES EVER!!!
#define mapX 512 //the max x value
#define mapY 16 //the max y value

extern uint8_t map[8192]; //(mapx * map y)

//64x64 4096
//256x16 = 4069
//128 x 64 8192
  //this is 2,720, i need 16,384 
//512x32 = 16,384
//512x16 = 8192
  //512x192 = 98304 blocks

uint16_t pmapx; // player x position in the map
uint8_t pmapy; // player y position in the map
uint32_t blocklocation;
uint32_t location;
uint8_t noise[mapX];
uint16_t dirx;
uint8_t blockcache;
uint16_t cx, camx;
uint8_t cy, camy;


void placeblock(uint8_t x, uint8_t y, uint8_t blockID) //puts the block into the world only
{
    //map[y * mapX + x] = blockID;
    set_bkg_tile_xy(x, y, blockID);
}

void setblock(uint8_t x, uint8_t y, uint8_t blockID) //puts the block into SRAM only
{
    map[y * mapX + x] = blockID;
}

void placeblockp(uint8_t x, uint8_t y, uint8_t blockID) //places a blcok in both SRAM and the world
{
    map[y * mapX + x] = blockID;
    set_bkg_tile_xy(x, y, blockID);
}

void loadblock(uint8_t x, uint8_t y) //loads the block from SRAM and then places it into the world
{
    set_bkg_tile_xy(x, y, map[y * mapX + x]);
}

uint16_t getblock(uint8_t x, uint8_t y, uint8_t mode) //mode 1 for tile ID, mode 2 for map array ID.  default is mode 1
{
    if(mode == 2)
    {
    return map[y * mapX + x];
    }
    else
    {
    return get_bkg_tile_xy(x,y);
    }
}

void drawworld() {
  for (uint8_t y = 0; y < Vmapy; y++) {
    camy = cy + y;
    for (uint16_t x = 0; x < Vmapx; x++) {
      location = ((cy+y) * mapX + (cx + x));
      camx = cx + x;
      if (getblock(camx,camy,ARRAYID) == DIRT) {
        if (getblock(camx, camy - 1, ARRAYID) == AIR) // checks above block to see if it is NOT air, and if there is air, turn into grass
        { 
          setblock(camx, camy, GRASS);
        } 

      }

    placeblock(x, y, map[location]); //loads the block at the selected cordinates in the array onto the screen.
      /*
      0 - air
      1 - grassblock
      4 - stone
      11 - treetrunk
      etc - ids are in itemIDs.h
      */
    }
  }
}
/*
void gametick() {
  for (y = 0; y < mapY; y++) {
    for (x = 0; x < mapX; x++) {
      uint32_t location = ((cy+y) * mapX + (cx + x));
      camx = cx + x;
      camy = cy + y;
      
    }
  }
}*/

void clearworld()
{
  for (uint8_t y = 0; y < 30; y++) {
    for (uint16_t x = 0; x < 30; x++) {
      placeblock(x,y,0);
    }
  }
}

void forcesave() //attempts to write to RAM, idk if it will worl
{
for (uint8_t y = 0; y < mapY; y++) {
    for (uint16_t x = 0; x < mapX; x++) {
      
      map[y * mapX + x]=getblock(x,y,2);

    }
  }
}

void zeroworld() {
  for (uint16_t i = 0; i < (mapX * mapY); i++) {
    map[i] = 0;
  }
}




uint16_t randomInRange(uint8_t lower, uint8_t upper)
{
    
    uint8_t num = (arand() % (upper - lower + 1)) + lower;
    return num;
}

uint8_t randomPercent(uint8_t chance) //a random chance a thingy will happen
{
    uint8_t num = (arand() % (101));
    if(num<=chance)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
    
}


void display() {
  drawworld();
  //draw entities
}

void dogravity() { //update for new camera system
  
}

void genworld() 
{
  initarand(sys_time);
  uint8_t noise[mapX] = {0};
  uint8_t level = randomInRange(0,3);
  #define maxterhi 15
  #define minterhi 0
//randomInRange(10,20);

  for(uint16_t i = 0; i < mapX; i++) {
    if(randomPercent(25)==TRUE && level != maxterhi) {
        level += 1;
        noise[i] = level;
    } else if (randomPercent(5)==TRUE && level != maxterhi) {
        level += 2;
        noise[i] = level;
    } else if(randomPercent(35)==TRUE && level != minterhi) {
        level -= 1;
        noise[i] = level;
    } else if (randomPercent(5)==TRUE && level != minterhi) {
        level -= 2;
        noise[i] = level;
    } else {
        noise[i]=noise[i-1];
    }
  }
  
  for (uint8_t y = 0; y < mapY; y++) {
    for (uint16_t x = 0; x < mapX; x++) {
      if (y >= 20) {
        setblock(x, y - noise[x], DIRT);
      }
      if (y >= 25) {
        setblock(x, y - noise[x] , STONE);
      } 


    }
  }
  /*
  for (y = 0; y < mapY; y++) {
      for (x = 0; x < mapX; x++) {
        if(getblock(x,y, ARRAYID)==STONE) 
        {
          if(getblock(x+1,y, ARRAYID)==AIR)
          {
              setblock(x+1,y,STONE);
          }
          if(getblock(x,y+1, ARRAYID)==AIR)
          {
              setblock(x,y+1,STONE);
          }
        }
      }
    } */
}

void init() {
  clearworld();
  zeroworld();
  genworld();
  pmapx = 10;
  pmapy = 8;
  blocklocation = pmapy * mapX + pmapx+1;
  dirx = pmapx + 1;
  

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
  drawworld();
  display();
  blockcache = 4;
    
  //uint16_t offset = ((mapX * 3) - mapY);

  while (1) {
    


    if (joypad() & J_A) {
      if (getblock(dirx, pmapy, ARRAYID) == 0) //checks the block to the right, and if it's air, it will fill the selected block in with what ever block is in the blockcashe.
          {
            setblock(dirx, pmapy, blockcache);
            if (getblock(dirx, pmapy-1, ARRAYID) == GRASS) {
                setblock(dirx, pmapy-1, DIRT);
            }
            display();
          }
    } 

    if (joypad() & J_B) {
      if (getblock(dirx, pmapy, ARRAYID) != 0) {
        setblock(dirx, pmapy, 0);
        if (getblock(dirx, pmapy-1, ARRAYID) == DIRT) {
                setblock(dirx, pmapy-1, GRASS);
            }
        display();
      }
    }

    if (joypad() & J_SELECT) {
        if(blockcache<16)
        {
        blockcache++;   
        }
        else
        {
        blockcache = 1;
        }
      
    }
    if (joypad() & J_START) {
        
        clearworld();
        zeroworld();
        genworld();
        display();
          
    }

    if (joypad() & J_UP) {

        
            //scroll_bkg(0,-8);
            cy -= 1;
            pmapy -= 1;
            display();
    
      

    }

    if (joypad() & J_DOWN) {

       
            //scroll_bkg(0,8);
            cy += 1;
            pmapy += 1;
            display();
        


    }
    if (joypad() & J_LEFT) {

       
            pmapx -= 1;
            blocklocation = pmapy * mapX + pmapx-1;
            dirx = pmapx -1;
            set_sprite_prop(0, S_FLIPX);
            cx -= 1;
            display();
        
      

    }

    if (joypad() & J_RIGHT) {

        
            pmapx += 1;
            blocklocation = pmapy * mapX + pmapx+1;
            dirx = pmapx + 1;
            set_sprite_prop(0, 0);
            cx += 1;
            display();
        
      

    }


    //delay(10);
    
    //dogravity();
    // update_world();
    // player y position was 36
    // Done processing, yield CPU and wait for start of next frame (VBlank)
    wait_vbl_done();
  }
}

/* serial link codes
add_SIO(), remove_SIO(), send_byte(), receive_byte()
*/

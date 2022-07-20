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
#define Blocks 8192 //the mapX * mapY very useful

extern uint8_t map[Blocks]; //(mapx * map y)
//544 x 16 = 8704
//64x64 4096
//256x16 = 4069
//128 x 64 8192
  //this is 2,720, i need 16,384 
//512x32 = 16,384
//512x16 = 8192
  //512x192 = 98304 blocks

uint16_t pmapx = 10; // player x position in the map
uint16_t pmapy = 8; // player y position in the map
uint16_t blocklocation;
uint16_t location;
uint8_t noise[mapX];
uint16_t dirx;
uint8_t blockcache = 4;
uint16_t cx, camx;
uint16_t cy, camy;


void placeblock(uint16_t x, uint8_t y, uint8_t blockID) //places a block in both SRAM and the world - not needed with current camera setup.
{
    map[y * mapX + x] = blockID;
    set_bkg_tile_xy(x, y, blockID);
}

void setblock(uint16_t x, uint8_t y, uint8_t blockID) //puts the block into SRAM only
{
    map[y * mapX + x] = blockID;
}

void loadblock(uint16_t x, uint8_t y) //loads the block from SRAM and then places it into the world
{
    set_bkg_tile_xy(x, y, map[y * mapX + x]);
}

uint16_t getblock(uint16_t x, uint8_t y) //gets the block at the specified cordinates from SRAM - Slower, less calculation needed to find the propper tile.
{
    
    return map[y * mapX + x];
    
}

uint16_t getblockVRAM(uint16_t x, uint8_t y) //gets the block at the specified cordinates from VRAM with camera offsets - Much faster, but could cause errors
{
    camy = cy + y;
    camx = cx + x;
    return get_bkg_tile_xy(camx,camy);
    
}

void drawworld() {
  for (uint8_t y = 0; y < Vmapy; y++) {
    camy = cy + y;
    for (uint16_t x = 0; x < Vmapx; x++) {
      location = ((cy+y) * mapX + (cx + x));
      camx = cx + x;
      

    set_bkg_tile_xy(x, y, map[location]); //loads the block at the selected cordinates in the array onto the screen.
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

void clearVRAM()
{
  for (uint8_t y = 0; y < 32; y++) {
    for (uint16_t x = 0; x < 32; x++) {
      set_bkg_tile_xy(x,y,0);
    }
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




uint8_t check4over(uint8_t a, uint8_t buff)
{
    if(buff > a)
    {
      return TRUE;
    }
    else
    {
      return FALSE;
    }
    
    
}

void genworld() 
{
  initarand(sys_time);
  uint8_t noise[mapX];
  uint8_t level = 1;
  #define maxterhi 8
  #define minterhi 0



  for(uint16_t i = 0; i < mapX; i++) 
  {

    

    if(randomPercent(30)==TRUE) 
    {
      level += 1;
      goto SKIP0;
    }
    
    if (randomPercent(5)==TRUE) 
    {
      level += 2;
      goto SKIP0;
    }
    
    if(randomPercent(30)==TRUE) 
    {
      level -= 1;
      goto SKIP0;
    }
    
    if (randomPercent(5)==TRUE) 
    {
      level -= 2;
      goto SKIP0;
    } 

    
    noise[i]=noise[i-1];
    goto SKIP01;
    
    
    SKIP0:
    

    noise[i] = level;
 

    SKIP01:
    if (level >= maxterhi) {
    
      level = maxterhi;
      noise[i] = level;
      goto SKIP1;
    }
    if (level <= minterhi) {
    
      level = minterhi;
      noise[i] = level;
      
    }
    
    SKIP1:
  }

  for (uint8_t y = 0; y < mapY; y++) 
  {
    for (uint16_t x = 0; x < mapX; x++) 
    {
      //setblock(x, y - noise[x], AIR);
      if(y <= 12)
      {
        goto SKIP2;
      }

      setblock(x, y - noise[x], DIRT);
      //setblock(x, (y - noise[x]) + 1, STONE);

      if (getblock(x, (y - noise[x]) - 1) == AIR && getblock(x, (y - noise[x]) - 2) == AIR) // checks the above 2 blocks for air, and if there is air, turn into grass
      { 
        setblock(x, y - noise[x], GRASS);
        
      }


      
      SKIP2:
      
    
    
    }
  
  }

  for (uint16_t y = 0; y < mapY; y++) 
  {
    for (uint16_t x = 0; x < mapX; x++) 
    {
      if(getblock(x,y)==STONE || getblock(x,y)==DIRT) 
      {
        if(y+1 != mapY && getblock(x,y+1)==AIR)
        {
          setblock(x,y+1,STONE);
        }
      }
    }
  } 

   
}

void init() {
  memset(map, 0, sizeof(map));
  clearVRAM();
  genworld();
  blocklocation = pmapy * mapX + pmapx+1;
  dirx = pmapx + 1;
  set_bkg_data(0, 16, blocks);
  set_sprite_data(0, 7, player);
  set_sprite_tile(0, 6);
  scroll_bkg(8,8);
  move_sprite(0, 80, 72);
}

    



void main(void) {
  ENABLE_RAM;
  SHOW_SPRITES;
  SHOW_BKG;
  init();
  drawworld();
  //display();
  
    
  //uint16_t offset = ((mapX * 3) - mapY);

  while (1) {
    


    if (joypad() & J_A) {
      if (getblock(dirx, pmapy) == 0) //checks the block to the right, and if it's air, it will fill the selected block in with what ever block is in the blockcashe.
          {
            setblock(dirx, pmapy, blockcache);
            if (getblock(dirx, pmapy+1) == GRASS) {
                setblock(dirx, pmapy+1, DIRT);
            }
            display();
          }
    } 

    if (joypad() & J_B) {
      if (getblock(dirx, pmapy) != 0) {
        setblock(dirx, pmapy, 0);
        if (getblock(dirx, pmapy+1) == DIRT) {
                setblock(dirx, pmapy+1, GRASS);
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
        blockcache = 0;
        }
      
    }
    if (joypad() & J_START) {
        
        memset(map, 0, sizeof(map)); //zeros world 
        clearVRAM();
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

/** @file itemIDs.h
    Item IDs in the order of the tileset.
*/

//MODES
#define TILEID 1
#define ARRAYID 2


/*litterally just air, kinda counts as light? */
#define AIR 0 

/*grass block, turns to dirt when not under the sun */
#define GRASS 1 

#define DIRT 2 //dirt block, just a grass block without the grass, will turn to grass if touching light AND grass

#define DIRT_WALL 3// dirt, but it the background, can be broken and walked through

#define STONE 4 //stone, kinda meh

#define IRON_ORE 5 //looks like stone, I will probably fix this

#define TL1 6 //the top left part of tree leaves

#define TL2 7 //the middle top part of tree leaves

#define TL3 8 //the top right part of tree leaves

#define TL4 9 //the bottom left part of tree leaves

#define TL5 10 //the middle bottom part of tree leaves

#define TL6 11 //the bottom right part of tree leaves

#define TREE_TRUNK 12 //the trunk of a tree, when obtained, it turns to normal wood

#define TREE_STUMP 13 //The stump of a tree, same as a log, just looks different

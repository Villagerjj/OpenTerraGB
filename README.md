
# OpenTerraGB

An open source clone of Terraria for the Nintendo Gameboy! built with GBDK!

<img src="https://user-images.githubusercontent.com/56765269/182481748-6df4429b-1045-4d03-bb33-c2d8e411bade.png" width="45%"></img> <img src="https://user-images.githubusercontent.com/56765269/180260422-41a0aff5-5e36-4386-aab2-77f024c29c47.png" width="45%"></img> 


## Console Support

OpenTerraGB currently only supports the Nintendo Gameboy, but in the future it might support other consoles


## World Size

OpenTerraGB has a max world size of 512x192 (98,304 blocks), this takes up 12/16 banks of MBC5 SRAM (128kb) and is the biggest world you can generate. 

| World Size (3DS version of Terraria) | Size of map | OpenTerraGB size |
| :----------------------------------- | :---------- | :---------------------- |
| `Normal`                             | `1750 × 900 (1,575,000 blocks)` | **6.24%** (98,304 blocks)  |
| `Expanded`                           | `4200 × 1200 (5,040,000 blocks)` | **1.95%** (98,304 blocks)  |

## Entity Limits

OpenTerraGB has an entity cap of 58. The 3DS version of Terraria has a cap of 200 entities, I have chosen 58 as this respects the GB's limitations, while also being proportinal to the 3DS's world sizes to entity cap ratio. 

<!--200/1750 =  0.1142857143 ( this is how many entities can be spawned per block).
512 *  0.1142857143 = 58.5142857216 ( this is then rounded down, because 58 sounds better than 59).-->

Note: Entities are calculated differently from the PC version, as things that are bigger than 8x8 take up more than 1 entity slot. EX: Bosses, and big slimes.		
		
## FAQ

#### What features are you planning?

You can view what features are getting worked on [here!](https://trello.com/b/CcQzzyzf/openterragb)

#### Can I contribute?

Yes! depending on the modifications you make, it will be merged into the main branch, or a seperate/new branch.

#### Where can I get a ROM?

A Pre release rom has been released, make sure to read the tips section!

## Credits

- [@Eievui](https://github.com/eievui5) - Code optimizations, general help.
- [@Pixel Shock](https://pixelshock.itch.io) - Creator of all assets, and a PICO 8 version of Terraria. (I made the new leaf textures tho)


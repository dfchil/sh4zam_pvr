# Sh4zam PVR

This is intended to be an example of how to apply the magic of [sh4zam](https://github.com/gyrovorbis/sh4zam) in your Dreamcast projects.

<img src="docs/img/gainz.png" alt="GAINZ!" width="240"/>

## Current Status
I've ported my previous spritecube example to use the c23 #embed feature for texture assets instead of the classical romdisk and sh4zam for 3d transformations.  p. 

## Requirements 
- A dreamcast toolchain with KallistoOS and a gcc version > 1.5.0 (for the #embed stuff). Instructions for setting that up [can be found here](https://dreamcast.wiki/Getting_Started_with_Dreamcast_development). Make sure to set the gcc version correctly in the [Configuring the dc-chain script](https://dreamcast.wiki/Getting_Started_with_Dreamcast_development#Configuring_the_dc-chain_script) step.
- sh4zam installed on top your KallistoOS toolchain, [as per these instructions](https://github.com/gyrovorbis/sh4zam?tab=readme-ov-file#make-kallistios).

## Tutorials
- [Part 1: Hello World, memcopy edition](./docs/Part1_HelloWorld.md)
- [Part 2: Perspective](./docs/Part2_Perspective.md) 
- [Part 3: Quaternion rotations](./docs/Part3_Quaternions.md)
- [Part 4: PVR Sprites](./docs/Part4_Sprites.md)
- [Part 5: Diffuse lighting](./docs/Part5_Diffuse.md)
- [Part 6: Specular Lighting](./docs/Part6_Specular.md)
- [Part 7: Per Vertex Specular Lighting](./docs/Part7_PerVertexSpecular.md) (and OCRAM)
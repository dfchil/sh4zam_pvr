#include <stdio.h>
#include <dc/pvr.h>
#include <sh4zam/shz_sh4zam.h>
#include <dc/minifont.h>



int main(void) {

  // setup up a 4bit pal texture
  pvr_ptr_t tex = pvr_mem_malloc(128 * 128 / 2); // 4bpp = 0.5 bytes per pixel
  if (tex == NULL) {
      printf("Failed to allocate texture memory\n");
  }


  /*
  effects: 
  drop shadow under font
  palette cycling animation
  horizontal wave distortion (many 1 pixel high horizontal sprites)
  */
  uint16_t buffer[16*8];

  for (uint32_t i = 33; i <= 126; i++) {
    minifont_draw(buffer, 8, i);
      // upload to texture memory
      // shz_sq_memcpy32
    }
    // pvr_txr_load(buffer, tex + ((i - 32) * 8), 8 * 2); // 8 pixels wide, 4bpp = 16 bytes per row


  pvr_mem_free(tex);
  return 0;
}

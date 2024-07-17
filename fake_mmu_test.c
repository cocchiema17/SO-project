#include "fake_mmu.h"

int main() {
  printf("Starting...\n");
  MMU* mmu = init_MMU(PAGES_NUM, "swapfile.txt");

  // for loop to fill all the frames in the ram
  int pagesUnswappable = sizeof(PageEntry) * PAGES_NUM / PAGE_SIZE;
  for (int p = pagesUnswappable; p < 300; p++) {  // PAGES_NUM
    for (int o = 0; o < 2; o++) {  // PAGE_SIZE
      LinearAddress linearAddress = {p, o};
      if (o % 2 == 1) {
        char* c = MMU_readByte(mmu, linearAddress);
        printf("The read value is %c\n", *c);
      }
      else
        MMU_writeByte(mmu, linearAddress, 'W');
    }
  }
  
  printf("Page fault: %d\n", mmu->pageFault);

  cleanup_MMU(mmu);
  printf("...finish!\n");
  return EXIT_SUCCESS;
}
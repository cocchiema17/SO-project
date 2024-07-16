#include "fake_mmu.h"

int main() {
  printf("Starting...\n");
  MMU* mmu = init_MMU(PAGES_NUM, "swapfile.txt");

  //printPagesTableInRam(mmu);

  printf("Page fault: %d\n", mmu->pageFault);

  cleanup_MMU(mmu);
  printf("...finish!\n");
  return EXIT_SUCCESS;
}
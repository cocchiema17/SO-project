#include "fake_mmu.h"

int main() {
  printf("Starting...\n");
  MMU* mmu = init_MMU(SEGMENTS_NUM, PAGES_NUM, "swapfile.txt");
  //printSegmentsTable(mmu);
  printPagesTable(mmu);
  //printRam(mmu);

  // ciclo per riempire la ram
  for (int i = PAGES_NUM * sizeof(PageEntry); i < MAX_MEMORY; i++) {
    MMU_writeByte(mmu, i, 'a');
  }

  LogicalAddress logical_address = {3, 0, 0};
  LinearAddress linear_address = getLinearAddress(mmu, logical_address);
  PhysicalAddress physical_address = getPhysicalAddress(mmu, linear_address);
  char* c = MMU_readByte(mmu, physical_address);
  printf("c = %c\n", *c);

  printPagesTable(mmu);
  printRam(mmu);

  logical_address.segment_id = 0;
  linear_address = getLinearAddress(mmu, logical_address);
  physical_address = getPhysicalAddress(mmu, linear_address);
  c = MMU_readByte(mmu, physical_address);
  printf("c = %c\n", *c);

  printPagesTable(mmu);
  printRam(mmu);

  printf("Page fault: %d\n", mmu->pageFault);

  cleanup_MMU(mmu);
  printf("...finish!\n");
  return EXIT_SUCCESS;
}
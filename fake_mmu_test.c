#include "fake_mmu.h"

int main() {
  printf("Starting...\n");
  MMU* mmu = init_MMU(SEGMENTS_NUM, PAGES_NUM, "swapfile.txt");
  //printSegmentsTable(mmu);
  printPagesTable(mmu);
  //printRam(mmu);

  /*TESTING SWAP IN

  // 300
  LogicalAddress logical_address = {3, 0, 0};
  LinearAddress linear_address = getLinearAddress(mmu, logical_address);
  PhysicalAddress physical_address = getPhysicalAddress(mmu, linear_address);
  char* c = MMU_readByte(mmu, physical_address);
  printf("c = %c\n", *c);

  printPagesTable(mmu);
  //printRam(mmu);

  // 000
  logical_address.segment_id = 0;
  linear_address = getLinearAddress(mmu, logical_address);
  physical_address = getPhysicalAddress(mmu, linear_address);
  c = MMU_readByte(mmu, physical_address);
  printf("c = %c\n", *c);

  printPagesTable(mmu);
  //printRam(mmu);

  // 221
  logical_address.segment_id = 2;
  logical_address.page_number = 2;
  logical_address.offset = 1;
  linear_address = getLinearAddress(mmu, logical_address);
  physical_address = getPhysicalAddress(mmu, linear_address);
  c = MMU_readByte(mmu, physical_address);
  printf("c = %c\n", *c);

  printPagesTable(mmu);
  //printRam(mmu);

  // 13c
  logical_address.segment_id = 1;
  logical_address.page_number = 3;
  logical_address.offset = 12;
  linear_address = getLinearAddress(mmu, logical_address);
  physical_address = getPhysicalAddress(mmu, linear_address);
  c = MMU_readByte(mmu, physical_address);
  printf("c = %c\n", *c);

  printPagesTable(mmu);
  //printRam(mmu);

  // 13f
  logical_address.segment_id = 1;
  logical_address.page_number = 3;
  logical_address.offset = 15;
  linear_address = getLinearAddress(mmu, logical_address);
  physical_address = getPhysicalAddress(mmu, linear_address);
  c = MMU_readByte(mmu, physical_address);
  printf("c = %c\n", *c);

  printPagesTable(mmu);
  printRam(mmu);

  */


  // TESTING SWAP OUT 
  // fill the page table
  for (int s = 0; s < mmu->num_segments; s++) {
    for (int p = 0; p < mmu->segments[s].limit; p++) {
      LogicalAddress logical_address = {
        .segment_id = s,
        .page_number = p,
        .offset = 0
      };
      LinearAddress linear_address = getLinearAddress(mmu, logical_address);
      PhysicalAddress physical_address = getPhysicalAddress(mmu, linear_address);
      char* c = MMU_readByte(mmu, physical_address);
      printf("c = %c\n", *c);
    }
  }

  printPagesTable(mmu);
  printRam(mmu);


  printf("Page fault: %d\n", mmu->pageFault);

  cleanup_MMU(mmu);
  printf("...finish!\n");
  return EXIT_SUCCESS;
}
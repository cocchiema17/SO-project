#include "fake_mmu.h"
#define NUM_LOGICAL_ADDRESS 3

int main() {
    printf("Starting...\n");
    MMU* mmu = init_MMU(SEGMENTS_NUM, PAGES_NUM, "swapfile.txt");
    printRam(mmu);

    generateLogicalAddress(mmu);

    // accesso ad una lista di indirizzi logici
    LogicalAddress listLogicalAddress[NUM_LOGICAL_ADDRESS];
    listLogicalAddress[0].segment_id = 0;
    listLogicalAddress[0].page_number = 0;
    listLogicalAddress[0].offset = 0;

    listLogicalAddress[1].segment_id = 3;
    listLogicalAddress[1].page_number = 15;
    listLogicalAddress[1].offset = 15;

    listLogicalAddress[2].segment_id = 0;
    listLogicalAddress[2].page_number = 0;
    listLogicalAddress[2].offset = 0;

    printf("Accesso agli indirizzi logici:\n");
    for (int i = 0; i < NUM_LOGICAL_ADDRESS; i++)
        printf("%x%x%x\n",
         listLogicalAddress[i].segment_id, listLogicalAddress[i].page_number, listLogicalAddress[i].offset);

    cleanup_MMU(mmu);
    printf("...finish!\n");
    return EXIT_SUCCESS;
}
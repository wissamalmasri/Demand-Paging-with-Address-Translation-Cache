#include <stdio.h>
#include "log.h"
#ifdef __cplusplus
using namespace std;
#endif

void hexnum(uint32_t number) {
    printf("%08X\n", number);
    fflush(stdout);
}

void log_bitmasks(int levels, uint32_t *masks) {
    printf("Bitmasks\n");
    for (int idx = 0; idx < levels; idx++) {
        printf("level %d mask %08X\n", idx, masks[idx]);
    }
    fflush(stdout);
}

void log_virtualAddr2physicalAddr(uint32_t src, uint32_t dest) {
    fprintf(stdout, "%08X -> %08X\n", src, dest);
    fflush(stdout);
}

void log_va2pa_ATC_PTwalk(uint32_t src, uint32_t dest, bool tlbhit, bool pthit) {
    fprintf(stdout, "%08X -> %08X, ", src, dest);
    if (tlbhit) {
        fprintf(stdout, "tlb hit\n");
    } else {
        fprintf(stdout, "tlb miss, pagetable %s\n", pthit ? "hit" : "miss");
    }
    fflush(stdout);
}

void log_pagemapping(int levels, uint32_t *pages, uint32_t frame) {
    for (int idx = 0; idx < levels; idx++) {
        printf("%X ", pages[idx]);
    }
    printf("-> %X\n", frame);
    fflush(stdout);
}

void log_summary(unsigned int page_size,
                 unsigned int cacheHits,
                 unsigned int pageTableHits,
                 unsigned int addresses, unsigned int frames_used,
                 unsigned long int pgtableEntries) {
    unsigned int misses = addresses - (cacheHits + pageTableHits);
    double hit_percent = (double)(cacheHits + pageTableHits) / addresses * 100.0;

    printf("Page size: %d bytes\n", page_size);
    printf("Addresses processed: %d\n", addresses);
    printf("Cache hits: %d, Page hits: %d, Total hits: %d, Misses: %d\n",
           cacheHits, pageTableHits, cacheHits + pageTableHits, misses);
    printf("Total hit percentage: %.2f%%, miss percentage: %.2f%%\n",
           hit_percent, 100.0 - hit_percent);
    printf("Frames allocated: %d\n", frames_used);
    printf("Number of page table entries: %lu\n", pgtableEntries);
    fflush(stdout);
}

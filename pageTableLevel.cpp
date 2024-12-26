#include "pageTableLevel.h"
#include "log.h"
#include <iostream>
#include <numeric>
#include <vector>
#include <algorithm>

// Initialize TLB Translation Lookaside Buffer with a specified capacity
TLB::TLB(unsigned int capacity) : capacity(capacity) {}

// Retrieve physical frame number PFN from TLB using virtual page number (VPN)
int TLB::get(unsigned int vpn) {
    auto it = cache.find(vpn);// Look for VPN in the cache
    if (it != cache.end()) { // If found in cache (TLB hit)
        entries.splice(entries.begin(), entries, it->second);// Move entry to front for LRU
        return it->second->second; // Return the PFN
    }
    return -1;// TLB miss, return -1
}

// Insert VPN-PFN pair into TLB, maintaining the capacity and removing LRU if needed
void TLB::put(unsigned int vpn, unsigned int pfn) {
    if (cache.find(vpn) != cache.end()) {// If VPN exists, erase it to avoid duplicates
        entries.erase(cache[vpn]);
        cache.erase(vpn);
    }
    entries.push_front({vpn, pfn});// Insert at the front for LRU update
    cache[vpn] = entries.begin();// Update cache with new location

    // Check if TLB exceeds capacity; remove least recently used entry if needed
    if (cache.size() > capacity) {
        cache.erase(entries.back().first);// Remove LRU entry from cache
        entries.pop_back();// Remove LRU entry from list
    }
}

// Initialize PageTable with multi-level structure based on levelBits and set TLB capacity
PageTable::PageTable(const std::vector<unsigned int>& levelBits, unsigned int tlbCapacity)
    : levelBits(levelBits),
      rootLevel(std::unique_ptr<Level>(new Level(0, 1U << levelBits[0], this))),
      tlb(tlbCapacity),
      pageSize(1U << (32 - std::accumulate(levelBits.begin(), levelBits.end(), 0))) {}

//Allocate a new frame number for a page that has no existing mapping
unsigned int PageTable::allocateFrame() {
    return frameCounter++;// Increment frame counter to get unique frame number
}

//Record access to a page based on address, and log details if outputStatus is true
unsigned int PageTable::recordPageAccess(unsigned int address, bool outputStatus) {
    unsigned int vpn = address >> 12;// Get VPN by shifting address bits
    int pfn = tlb.get(vpn);// Check if TLB has PFN for this VPN
    // True if TLB hit, otherwise false
    bool isTLBHit = (pfn != -1);
    bool isPageTableHit = false;// Track page table hit status

    if (isTLBHit) {
        cacheHits++;// Increment TLB hit counter
    } else {
        // TLB miss, check page table
        pfn = rootLevel->recordPageAccess(*this, address);
        isPageTableHit = (pfn != -1);// True if page table hit

        if (isPageTableHit) {
            pageTableHits++;// Increment page table hit counter
        } else {
            // If not in page table allocate a new frame for page
            pfn = allocateFrame();
            rootLevel->updateFrameNumber(*this, address, pfn);// Update page table with new mapping
        }

        // Update TLB with current or new frame
        tlb.put(vpn, pfn);
    }

    // Calculate physical address based on PFN and offset within the page
    unsigned int physicalAddress = (pfn << 12) | (address & 0xFFF);

    // Log virtual to physical translation if outputStatus is enabled
    if (outputStatus) {
        log_va2pa_ATC_PTwalk(
            address,
            physicalAddress,
            isTLBHit,
            isPageTableHit
        );
    }
    return pfn;
}

// Update the frame number at the given level in the page table hierarchy
void Level::updateFrameNumber(PageTable& pageTable, unsigned int address, unsigned int frameNumber) {
    const auto& levelBits = pageTable.getLevelBits();
    unsigned int shiftAmount = 32 - std::accumulate(levelBits.begin(), levelBits.begin() + depth + 1, 0);
    unsigned int index = (address >> shiftAmount) & ((1U << levelBits[depth]) - 1);

    if (depth == levelBits.size() - 1) {// If at leaf level, update map with frame number
        maps[index].valid = true;
        maps[index].frameNumber = frameNumber;
    } else {
        // If not at leaf level ensure next level exists and recursively update
        if (!nextLevels[index]) {
            nextLevels[index] = std::unique_ptr<Level>(new Level(depth + 1, 1U << levelBits[depth + 1], &pageTable));
        }
        nextLevels[index]->updateFrameNumber(pageTable, address, frameNumber);
    }
}

//Log summary of page accesses, including cache hits, page table hits, and frame usage
void PageTable::logSummary(unsigned int addressesProcessed) const {
    unsigned int misses = addressesProcessed - (cacheHits + pageTableHits);  // Calculate misses
    log_summary(pageSize, cacheHits, pageTableHits, addressesProcessed, frameCounter, getTotalPageTableEntries());
}

// Count total entries in the page table
unsigned long PageTable::getTotalPageTableEntries() const {
    return rootLevel->countEntries();
}

// Initialize a page table level, creating maps if it's a leaf level, or next levels otherwise
Level::Level(unsigned int depth, unsigned int entries, PageTable* pageTable)
    : depth(depth), pageTablePtr(pageTable) {
    if (depth == pageTablePtr->getLevelBits().size() - 1) {// Leaf level
        Map defaultMap;// Create default mapping entry
        defaultMap.valid = false;
        defaultMap.frameNumber = -1;
        maps.resize(entries, defaultMap);// Initialize map array with default
    } else {
        nextLevels.resize(entries);// Non-leaf level initializes next level pointers
    }
}

//Record page access at current level return frame number if valid, else -1 for miss
unsigned int Level::recordPageAccess(PageTable& pageTable, unsigned int address) {
    const auto& levelBits = pageTable.getLevelBits();
    unsigned int shiftAmount = 32 - std::accumulate(levelBits.begin(), levelBits.begin() + depth + 1, 0);
    unsigned int index = (address >> shiftAmount) & ((1U << levelBits[depth]) - 1);

    if (depth == levelBits.size() - 1) {
        return maps[index].valid ? maps[index].frameNumber : -1;// Return frame number if valid
    } else {
        return nextLevels[index] ? nextLevels[index]->recordPageAccess(pageTable, address) : -1;
    }
}

//Recursively count valid entries at this level and all sub-levels
unsigned long Level::countEntries() const {
    if (depth == pageTablePtr->getLevelBits().size() - 1) {
        return std::count_if(maps.begin(), maps.end(), [](const Map& m) { return m.valid; });
    } else {
        unsigned long total = 0;
        for (const auto& next : nextLevels) {
            if (next) total += next->countEntries();
        }
        return total;
    }
}

//Calculate bitmasks based on each level’s bit size to isolate page table indices
std::vector<uint32_t> PageTable::calculateBitmasks() const {
    std::vector<uint32_t> masks;
    unsigned int shift = 32;
    for (unsigned int bits : levelBits) {
        shift -= bits;
        masks.push_back(((1U << bits) - 1) << shift);
    }
    return masks;
}

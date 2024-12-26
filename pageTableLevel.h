#ifndef PAGETABLELEVEL_H
#define PAGETABLELEVEL_H

#include <unordered_map>
#include <vector>
#include <memory>
#include <list>

class TLB {
private:
    unsigned int capacity;
    std::list<std::pair<unsigned int, unsigned int>> entries;
    std::unordered_map<unsigned int, decltype(entries.begin())> cache;

public:
    TLB(unsigned int capacity);
    int get(unsigned int vpn);
    void put(unsigned int vpn, unsigned int pfn);
};

struct Map {
    bool valid = false;
    unsigned int frameNumber = 0;
};

class Level;

class PageTable {
private:
    std::vector<unsigned int> levelBits;
    std::unique_ptr<Level> rootLevel;
    TLB tlb;
    unsigned int cacheHits = 0;
    unsigned int pageTableHits = 0;
    unsigned int pageSize;

public:
    PageTable(const std::vector<unsigned int>& levelBits, unsigned int tlbCapacity);
    unsigned int recordPageAccess(unsigned int address, bool outputStatus = false);
    unsigned int allocateFrame();
    void logSummary(unsigned int addressesProcessed) const;
    const std::vector<unsigned int>& getLevelBits() const { return levelBits; }
    unsigned long getTotalPageTableEntries() const;
    std::vector<uint32_t> calculateBitmasks() const;
    unsigned int frameCounter = 0;
};

class Level {
private:
    unsigned int depth;
    PageTable* pageTablePtr;
    std::vector<std::unique_ptr<Level>> nextLevels;
    std::vector<Map> maps;

public:
    Level(unsigned int depth, unsigned int entries, PageTable* pageTable);
    unsigned int recordPageAccess(PageTable& pageTable, unsigned int address);
    unsigned long countEntries() const;
    void updateFrameNumber(PageTable& pageTable, unsigned int address, unsigned int frameNumber);
};

#endif // PAGETABLELEVEL_H

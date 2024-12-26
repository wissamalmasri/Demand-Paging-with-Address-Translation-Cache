#include <iostream>
#include <fstream>
#include <vector>
#include <numeric>
#include <getopt.h>
#include "pageTableLevel.h"
#include "log.h"
#include "tracereader.h"

using namespace std;

int main(int argc, char **argv) {
    int numAccesses = -1;// Set default number of accesses -1 means all
    int tlbCapacity = 0;// Default TLB capacity
    string outputMode = "summary"; // Default output mode is summary
    vector<unsigned int> levelBits; // Stores number of bits for each page table level

    // Parse command-line options for TLB capacity, number of accesses, and output mode
    int opt;
    while ((opt = getopt(argc, argv, "n:c:o:")) != -1) {
        switch (opt) {
            case 'n': numAccesses = atoi(optarg); 
                if (numAccesses < 0) {
                    cerr << "Number of memory accesses must be a number, greater than 0." << endl;
                    return 1;
                }
            break;// Set number of accesses
            case 'c': tlbCapacity = atoi(optarg); 
                if (tlbCapacity < 0) {
                    cerr << "Number of memory accesses must be a number, greater than 0." << endl;
                    return 1;
                }
            break;// Set TLB capacity
            case 'o': outputMode = optarg; break;// Set output mode
            default:
                cerr << "Usage: ./pagingwithatc [-n N] [-c N] [-o mode] <tracefile> <levelbits>" << endl;
                return 1;
        }
    }

    //Ensure a trace file argument is provided
    if (optind >= argc) {
        cerr << "Missing trace file argument" << endl;
        return 1;
    }

    //Read the trace file name from command-line arguments
    string traceFile = argv[optind++];

    //Read remaining arguments as level bits for the page table
    while (optind < argc) {
        int bits = atoi(argv[optind++]);
        if (bits <= 0) {
            cerr << "Number of memory accesses must be a number, greater than 0." << endl;
            return 1;
        }
        levelBits.push_back(bits);
    }

    //Validate levelBits total does not exceed maximum allowed 28 bits for this setup
    if (accumulate(levelBits.begin(), levelBits.end(), 0) > 28) {
        cerr << "Too many bits used in page tables" << endl;
        return 1;
    }

    //Initialize page table with specified level bits and TLB capacity
    PageTable pageTable(levelBits, tlbCapacity);

    //Open the trace file for reading in binary mode
    FILE* traceFilePtr = fopen(traceFile.c_str(), "rb");
    if (!traceFilePtr) {
        cerr << "Unable to open trace file " << traceFile << endl;
        return 1;
    }

    //If output mode is 'bitmasks', calculate and display bitmasks for each level
    if (outputMode == "bitmasks") {
        vector<uint32_t> masks = pageTable.calculateBitmasks();
        for (size_t i = 0; i < masks.size(); ++i) {
            printf("level %zu mask %08X\n", i, masks[i]);
        }
        fclose(traceFilePtr);
        return 0;
    }
    //Trace address structure
    p2AddrTr traceAddr;
    unsigned int addressCount = 0;//Count of processed addresses

    //Loop through addresses in trace file, process according to output mode
    while (NextAddress(traceFilePtr, &traceAddr) && (numAccesses == -1 || addressCount < numAccesses)) {
        unsigned int address = traceAddr.addr;//Get the next address

        // Debug Print the address being processed
        //std::cout << "Processing address " << std::hex << address << std::dec << " (count: " << addressCount + 1 << ")" << std::endl;

        //Different processing based on the output mode selected
        if (outputMode == "offset") {
            unsigned int offset = address & 0xFFF;// Extract page offset
            printf("%08X\n", offset);

        } else if (outputMode == "vpn2pfn") {
            unsigned int frame = pageTable.recordPageAccess(address);

            //Calculate VPN levels based on levelBits configuration
            std::vector<unsigned int> pages(levelBits.size());
            unsigned int shift = 32;
            for (size_t i = 0; i < levelBits.size(); i++) {
                shift -= levelBits[i];
                pages[i] = (address >> shift) & ((1U << levelBits[i]) - 1);
            }
            log_pagemapping(levelBits.size(), pages.data(), frame);

        } else if (outputMode == "va2pa") {
            unsigned int dest = pageTable.recordPageAccess(address);// Record and get physical frame
            unsigned int offset = 0;
            for (size_t i = 0; i < levelBits.size(); i++) {
                offset += levelBits[i];
            }
            offset = 32 - offset;

            dest = (dest << offset) | (address & 0xFFF);// Construct physical address
            log_virtualAddr2physicalAddr(address, dest);

        } else if (outputMode == "va2pa_atc_ptwalk") {
            // Trigger logging for 'va2pa_atc_ptwalk' mode by passing 'true'
            unsigned int dest = pageTable.recordPageAccess(address, true);
            dest = (dest << 12) | (address & 0xFFF);// Construct physical address
            // Debug statement for tracking address processing
            //std::cout << "Logging va2pa_atc_ptwalk for address: " << std::hex << address << std::dec << std::endl;

        } else {
            //Default mode just records the access without further output
            pageTable.recordPageAccess(address);
        }
        // Increment the count of processed addresses
        addressCount++;
    }

    // If summary mode, log final summary of page access statistics
    if (outputMode == "summary") {
        pageTable.logSummary(addressCount);
    }
    // Close the trace file after processing
    fclose(traceFilePtr);
    return 0;
}

#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <list>
#include <unordered_set>
#include <iomanip>
#include <stack>

//compile to test g++ -o test main.cpp
using namespace std;


struct Semaphore{   //binary Semaphore for wait/signal, used from last hw assigment
private:
    mutex mtx;
    condition_variable cv;
    size_t avail;
public:
    explicit Semaphore(int avail_ = 1) : avail(avail_){}

    void wait(){
        unique_lock<mutex> lock(mtx);
        cv.wait(lock,[this] {return avail > 0;});
        avail--;
    }
    void signal(){
        unique_lock<mutex> lock(mtx);
        avail++;
        cv.notify_one();
    }
    size_t available() const{
        return avail;
    }
};

//------------------------------------------------------------------------------------------------------------
//Global functions:

Semaphore accessSema(1);

//------------------------------------------------------------------------------------------------------------
//Structs and functions... this isn't standard to have these functions contain a class being a struct, but I am tired ok ;-;
struct Page{
    public:
        int pageNum;
        int lastUsedTime;
        int accessCount;

        Page(int p):pageNum(p), lastUsedTime(0), accessCount(0){}
};

struct VirtualMem{
    private:
        int totalPageFrames;                    //for LIFO, MRU, LRU-K, LFU, OPT. For WS it is delta
        int minPoolSize;
        int maxPoolSize;
        int pageFaults = 0;
        int lifoTotal = 0, lruTotal = 0, lfuTotal = 0, optTotal = 0, wsTotal = 0;
        int windowSize;
        int currentIndex = 0;                   //for OPT
        //int freePageFrames;
        unordered_map<int, int> pageTable;      //key is diskAddr and pair is memAddr
        list<Page*> lruList;                    //for LRU
        unordered_map<int, int> accessCounts;   //for LFU
        stack<int> lifoStack;                   //for LIFO
        vector<int> optRef;                     //for OPT
        unordered_map<int, unordered_set<int>> ws;  //for WS

    public:
        VirtualMem(int tp, int minPool, int maxPool) : totalPageFrames(tp), maxPoolSize(maxPool), minPoolSize(minPool) {}

        /*int getFree() const{
            return freePageFrames;
        }

        void decreaseFree(){
            freePageFrames--;
        }

        void increaseFree(){
            freePageFrames++;
        }
         */
        void setDelta(int val){
            windowSize = val;
        }


        void pageLIFO(int diskAddr, int memAddr){
            if(lifoStack.size() < maxPoolSize){
                if(pageTable.find(memAddr) == pageTable.end()){
                    //lifoTotal++;
                    //Page* newPage = new Page(memAddr);
                    pageTable[diskAddr] = memAddr;
                    lifoStack.push(memAddr);
                }
            }else{
                if(pageTable.find(memAddr) == pageTable.end()){
                    int lastPage = lifoStack.top();
                    lifoStack.pop();
                    pageTable.erase(lastPage);

                    lifoTotal++;
                    //Page* newPage = new Page(memAddr);
                    pageTable[diskAddr] = memAddr;
                    lifoStack.push(memAddr);
                }
            }

        }
        void printLifo() const{
            cout<< "Running LIFO:\n";
            cout<< "Page replacements: "<< lifoTotal<< endl;
        }

        void pageLRU(int diskAddr, int memAddr){
            if (lruList.size() < maxPoolSize) {
                if (pageTable.find(memAddr) == pageTable.end()) {
                    Page* newPage = new Page(memAddr);
                    pageTable[diskAddr] = memAddr;
                    lruList.push_front(newPage);
                }
                else {
                    // If the page is already in memory, move it to the front of the list
                    auto it = find_if(lruList.begin(), lruList.end(), [&](const Page* p) {
                        return p->pageNum == pageTable[memAddr];
                    });
                    if (it != lruList.end()) {
                        lruList.splice(lruList.begin(), lruList, it);
                    }
                }
            }
            else {
                if (pageTable.find(memAddr) == pageTable.end()) {
                    // If there's no free space, evict the least recently used page
                    Page* lruPage = lruList.back();
                    pageTable.erase(lruPage->pageNum); // Remove from page table
                    lruList.pop_back(); // Remove from the end of the list

                    // Add the new page to memory
                    Page* newPage = new Page(memAddr);
                    pageTable[diskAddr] = memAddr;
                    lruList.push_front(newPage);
                    lruTotal++;
                }
                else {
                    // If the page is already in memory, move it to the front of the list
                    auto it = find_if(lruList.begin(), lruList.end(), [&](const Page* p) {
                        return p->pageNum == pageTable[memAddr];
                    });
                    if (it != lruList.end()) {
                        lruList.splice(lruList.begin(), lruList, it);
                    }
                }
            }
        }
        void printLru() const{
            cout<< "Running LRU:\n";
            cout<< "Page replacements: "<< lruTotal<< endl;
        }

        void pageLFU(int diskAddr, int memAddr){
            if (pageTable.find(memAddr) == pageTable.end()) {
                // If the page is not in memory
                if (pageTable.size() < maxPoolSize) {
                    // If there's free space in memory
                    pageTable[memAddr] = diskAddr;
                    accessCounts[memAddr] = 1; // Initialize access count to 1
                }
                else {
                    // If there's no free space, find the page with the lowest access count
                    int minAccessCount = INT_MAX;
                    int pageToEvict = -1;
                    for (auto it = accessCounts.begin(); it != accessCounts.end(); ++it) {
                        if (it->second < minAccessCount) {
                            minAccessCount = it->second;
                            pageToEvict = it->first;
                        }
                    }
                    // Evict the page with the lowest access count
                    pageTable.erase(pageToEvict);
                    accessCounts.erase(pageToEvict);
                    // Add the new page to memory
                    pageTable[memAddr] = diskAddr;
                    accessCounts[memAddr] = 1; // Initialize access count to 1
                    lfuTotal++;
                }
            }
            else {
                // If the page is already in memory, update its access count
                accessCounts[memAddr]++;
            }
        }
        void printLfu() const{
            cout << "Running LFU:\n";
            cout << "Page replacements: " << lfuTotal << endl;
        }

        void pageOPT(int diskAddr, int memAddr){
            if (pageTable.find(memAddr) == pageTable.end()) {
                // If the page is not in memory
                if (pageTable.size() < maxPoolSize) {
                    // If there's free space in memory
                    pageTable[memAddr] = diskAddr;
                }
                else {
                    // If there's no free space, find the page with the furthest future reference
                    int furthestFutureIndex = -1;
                    int pageToEvict = -1;
                    for (const auto& entry : pageTable) {
                        int pageNum = entry.second;
                        int futureIndex = INT_MAX;
                        auto it = find(optRef.begin() + currentIndex, optRef.end(), pageNum);
                        if (it != optRef.end()) {
                            futureIndex = distance(optRef.begin(), it);
                        }
                        if (futureIndex > furthestFutureIndex) {
                            furthestFutureIndex = futureIndex;
                            pageToEvict = pageNum;
                        }
                    }
                    // Evict the page with the furthest future reference
                    pageTable.erase(pageToEvict);
                    // Add the new page to memory
                    pageTable[memAddr] = diskAddr;
                    optTotal++;
                }
            }
        }
        void printOpt() const{
            cout << "Running OPT:\n";
            cout << "Page replacements: " << optTotal << endl;
        }

        void pageWS(int diskAddr, int memAddr){
            ws[memAddr].insert(diskAddr);

            if (pageTable.find(memAddr) == pageTable.end()) {
                // If the page is not in memory
                if (pageTable.size() < maxPoolSize) {
                    // If there's free space in memory
                    pageTable[memAddr] = diskAddr;
                }
                else {
                    // If there's no free space, evict the page that is outside the window
                    for (auto it = pageTable.begin(); it != pageTable.end();) {
                        if (ws.find(it->first) == ws.end()) {
                            it = pageTable.erase(it);
                        }
                        else {
                            ++it;
                        }
                    }
                    // Add the new page to memory
                    pageTable[memAddr] = diskAddr;
                    wsTotal++;
                }
            }
        }
        void printWs() const{
            cout << "Running Working Set:\n";
            cout << "Page replacements: " << wsTotal << endl;
        }
};

struct DiskDriver{
    private:
        mutex queueMutex;
        condition_variable diskCon;
        bool finished;

    queue<string> diskQueue;

    public:
        DiskDriver() : finished(false){}

        void startDisk(const string& inputLine){
            {
                unique_lock<mutex> lock(queueMutex);
                diskQueue.push(inputLine);
            }
            diskCon.notify_one();
        }

        void diskRequest(VirtualMem& vm){             //this will call every page algorithm with proper scheduling
            while(!diskQueue.empty()){
                string inputLine;

                unique_lock<mutex> lock(queueMutex);
                diskCon.wait(lock, [this] {return !diskQueue.empty(); });

                inputLine = diskQueue.front();
                diskQueue.pop();

                lock.unlock();

                if(inputLine.empty() || inputLine == "\r"){
                    finished = true;
                    break;
                }

                diskRequest(inputLine, vm);
            }
        }

        void diskRequest(const string& inputLine, VirtualMem& vm){
            stringstream str(inputLine);
            int memoryAddr, diskAddr;
            str >> diskAddr>> hex>> memoryAddr;
            //cout<< "Disk location: "<< diskAddr<< "   Disk num: "<< memoryAddr<< endl;

            //page replacement algorithms
            vm.pageLIFO(diskAddr, memoryAddr);
            vm.pageLRU(diskAddr, memoryAddr);
            vm.pageLFU(diskAddr, memoryAddr);
            vm.pageOPT(diskAddr, memoryAddr);
            vm.pageWS(diskAddr, memoryAddr);

            diskCon.notify_one();
        }

};

int main(int argc, char** argv) {
//------------------------------------------------------------------------------------------------------------
//File reading:

    if(argc != 2){                      //error if input not provided
        cout<< "No input argument is found.\n";
        return 1;
    }

    ifstream file1(argv[1]);
    if(!file1.is_open()){               //If it can't find file
        cout<< "No file can be found.\n";
        return 1;
    }

    string inputLine;
    int numVal;
    int counter = 0;
    int tp, ps, r, x, min, max, k;
    for(int i = 0; i < 7; i++){         //to get the first designated values
        getline(file1, inputLine);
        stringstream str(inputLine);
        while (str >> numVal){
            if(counter == 0){            //total page in the main mem.
                tp = numVal;
                //cout<< "Total Page: "<< tp<< endl;
            }else if(counter == 1){      //page size
                ps = numVal;
                //cout<< "Page size: "<< ps<< endl;
            }else if(counter == 2){      //num of page frames per process (LIFO, MRU, LRU-K, LFU, and OPT)
                r = numVal;
                //cout<< "Num of Pages: "<< r<< endl;
            }else if(counter == 3){      //window size (OPT, X for LRU-X, 0 for others that don't use this)
                x = numVal;
                //cout<< "Window size: "<< x<< endl;
            }else if(counter == 4){      //Min Free pool size
                min = numVal;
                //cout<< "Min Free Pool: "<< min<< endl;
            }else if(counter == 5){      //Max free pool size
                max = numVal;
                //cout<< "Max Free Pool: "<< max<< endl;
            }else if(counter == 6){      //total num processes
                k = numVal;
                //cout<< "Num of Processes: "<< k<< endl;
            }
        }
        counter++;
    }

    VirtualMem virtualMem(tp, min, max);
    virtualMem.setDelta(x);
    DiskDriver diskDriver;

    while(getline(file1, inputLine)){   //reading the processes,
        diskDriver.startDisk(inputLine);
    }

    file1.close();

    thread diskThread([&]() {diskDriver.diskRequest(virtualMem);});     //lambda function to pass the functions
    diskThread.join();

    virtualMem.printLifo();
    virtualMem.printLru();
    virtualMem.printLfu();
    virtualMem.printOpt();
    virtualMem.printWs();

    return 0;
}

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
        int lifoTotal, lruTotal, lfuTotal, optTotal, wsTotal;
        //int freePageFrames;
        unordered_map<int, Page*> pageTable;
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

        /*chatgpt code
         // Page replacement algorithm: LIFO
    void pageLIFO(int pageNum) {
        if (freePageFrames > 0) {
            // If there are free page frames, allocate the page
            pageTable[pageNum] = new Page(pageNum);
            lruList.push_front(pageTable[pageNum]);
            decreaseFreePageFrames();
        } else {
            // If no free page frames, evict the last page (LIFO)
            int lastPage = lruList.back()->pageNum;
            lruList.pop_back();
            pageTable.erase(lastPage);
            pageTable[pageNum] = new Page(pageNum);
            lruList.push_front(pageTable[pageNum]);
        }
    }

    // Page replacement algorithm: LRU
    void pageLRU(int pageNum) {
        if (freePageFrames > 0) {
            // If there are free page frames, allocate the page
            pageTable[pageNum] = new Page(pageNum);
            lruList.push_front(pageTable[pageNum]);
            decreaseFreePageFrames();
        } else {
            // If no free page frames, evict the least recently used page (LRU)
            int lastPage = lruList.back()->pageNum;
            lruList.pop_back();
            pageTable.erase(lastPage);
            pageTable[pageNum] = new Page(pageNum);
            lruList.push_front(pageTable[pageNum]);
        }
    }

    // Page replacement algorithm: LFU
    void pageLFU(int pageNum) {
        if (freePageFrames > 0) {
            // If there are free page frames, allocate the page
            pageTable[pageNum] = new Page(pageNum);
            accessCounts[pageNum] = 1;
            decreaseFreePageFrames();
        } else {
            // If no free page frames, evict the least frequently used page (LFU)
            int minAccessCount = INT_MAX;
            int leastFrequentPage = -1;
            for (auto it = accessCounts.begin(); it != accessCounts.end(); ++it) {
                if (it->second < minAccessCount) {
                    minAccessCount = it->second;
                    leastFrequentPage = it->first;
                }
            }
            pageTable.erase(leastFrequentPage);
            accessCounts.erase(leastFrequentPage);
            pageTable[pageNum] = new Page(pageNum);
            accessCounts[pageNum] = 1;
        }
    }

    // Page replacement algorithm: OPT
    void pageOPT(int pageNum) {
        if (freePageFrames > 0) {
            // If there are free page frames, allocate the page
            pageTable[pageNum] = new Page(pageNum);
            optRef.push_back(pageNum);
            decreaseFreePageFrames();
        } else {
            // If no free page frames, evict the page with the farthest reference in the future (OPT)
            int farthestRefIndex = -1;
            int farthestRefPage = -1;
            for (int i = 0; i < optRef.size(); ++i) {
                bool found = false;
                for (int j = i + 1; j < optRef.size(); ++j) {
                    if (optRef[j] == optRef[i]) {
                        found = true;
                        if (j > farthestRefIndex) {
                            farthestRefIndex = j;
                            farthestRefPage = optRef[i];
                        }
                        break;
                    }
                }
                if (!found) {
                    farthestRefPage = optRef[i];
                    break;
                }
            }
            pageTable.erase(farthestRefPage);
            optRef.erase(optRef.begin());
            pageTable[pageNum] = new Page(pageNum);
            optRef.push_back(pageNum);
        }
    }
         */

        void pageLIFO(int diskAddr, int memAddr){
            int pageFaults = 0;

        }
        void pageLRU(){

        }
        void pageLFU(){

        }
        void pageOPT(){

        }
        void pageWS(){

        }
};

struct DiskDriver{
    private:
        mutex queueMutex;
        condition_variable diskCon;

    queue<string> diskQueue;

    public:
        void startDisk(const string& inputLine){
            {
                unique_lock<mutex> lock(queueMutex);
                diskQueue.push(inputLine);
            }
            diskCon.notify_one();
        }

        void diskRequest(VirtualMem& vm){             //this will call every page algorithm with proper scheduling
            while(true){
                string inputLine;

                unique_lock<mutex> lock(queueMutex);
                diskCon.wait(lock, [this] {return !diskQueue.empty(); });

                inputLine = diskQueue.front();
                diskQueue.pop();

                lock.unlock();

                if(inputLine.empty()){
                    break;
                }

                diskRequest(inputLine, vm);
            }
        }

        void diskRequest(const string& inputLine, VirtualMem& vm){
            stringstream str(inputLine);
            int memoryAddr, diskAddr;
            str >> diskAddr>> hex>> memoryAddr;
            cout<< "Disk location: "<< diskAddr<< "   Disk num: "<< memoryAddr<< endl;

            //page replacement algorithms
            vm.pageLIFO(diskAddr, memoryAddr);


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
    DiskDriver diskDriver;

    while(getline(file1, inputLine)){   //reading the processes,
        diskDriver.startDisk(inputLine);
    }
    file1.close();

    thread diskThread([&]() {diskDriver.diskRequest(virtualMem);});     //lambda function to pass the functions

    diskThread.join();

    return 0;
}

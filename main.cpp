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
#include <climits>
#include <map>

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
struct Node{            //for LRU
    int diskAddr;
    int memAddr;
    Node* prev;
    Node* next;

    Node(int dA, int mA): diskAddr(dA), memAddr(mA), prev(nullptr), next(nullptr){}
};

struct NodeLfu{        //for LFU
    int diskAddr; // Disk address (page number)
    int memAddr;  // Memory address (number inside the page)
    int freq;     // Frequency of page access
    int timestamp; // Timestamp of last access
    Node* prev;
    Node* next;

    NodeLfu(int dA, int mA, int ts) : diskAddr(dA), memAddr(mA), freq(1), timestamp(ts), prev(nullptr), next(nullptr) {}
};

struct Page{
    public:
        int pageNum;
        int diskAddr;

        Page():pageNum(0), diskAddr(0){}
        Page(int dA, int mD): pageNum(dA), diskAddr(mD){}
};

struct VirtualMem{
    private:
        int totalPageFrames;                    //for LIFO, MRU, LRU-K, LFU, OPT. For WS it is delta
        int minPoolSize;
        int maxPoolSize;
        int lifoTotal = 0, lruTotal = 0, lfuTotal = 0, optTotal = 0, wsTotal = 0;
        int windowSize;
        int currentIndex = 0;                   //for OPT
        unordered_map<int, Node*> pageTable;    //key is diskAddr and pair is memAddr (Using for LRU)
        Node* headLru;
        Node* tailLru;
        unordered_map<int, Node*> pageTableLfu;
        map<int, list<Node*>> freqMap;
        int timestamp = 0;
        unordered_map<int, int> accessCounts;   //for LFU
        stack<Page> lifo;                       //for LIFO
        unordered_set<int> lifoPageSet;         //for LIFO
        vector<int> optRef;                     //for OPT
        unordered_map<int, unordered_set<int>> ws;  //for WS

    public:
        VirtualMem(int tp, int minPool, int maxPool) : totalPageFrames(tp), maxPoolSize(maxPool), minPoolSize(minPool) {}

        void setDelta(int val){
            windowSize = val;
        }
        void clearPageTable() {
            pageTable.clear();
        }


        void pageLIFO(int diskAddr, int memAddr){
            Page newPage(diskAddr, memAddr);
            if(lifoPageSet.find(newPage.diskAddr) == lifoPageSet.end()){        //if not in memory
                if(lifo.size() == maxPoolSize){
                    Page oldestPage = lifo.top();
                    lifo.pop();
                    lifoPageSet.erase(oldestPage.diskAddr);
                }

                lifo.push(newPage);
                lifoPageSet.insert(newPage.diskAddr);
                lifoTotal++;
            }
        }
        void printLifo() const{
            cout<< "Running LIFO:\n";
            cout<< "Page replacements: "<< lifoTotal<< endl;
        }

        void pageLRU(int diskAddr, int memAddr){
            int key = (diskAddr << 16) | memAddr;

            if(pageTable.find(key) != pageTable.end()){
                Node* page = pageTable[key];
                if(page == headLru){
                    return;
                }
                if(page->prev){
                    page->prev->next = page->next;
                }
                if(page->next){
                    page->next->prev = page->prev;
                }

                if(page == tailLru){
                    tailLru = tailLru->prev;
                }

                page->prev = nullptr;
                page->next = headLru;
                headLru->prev = page;
                headLru = page;

                return;
            }

            if (pageTable.size() >= maxPoolSize) {
                if(!tailLru){
                    return;
                }
                Node* evictedPage = tailLru;
                if(headLru == tailLru){
                    headLru = nullptr;
                    tailLru = nullptr;
                }else{
                    tailLru = tailLru->prev;
                    tailLru->next = nullptr;
                }

                int key = (evictedPage->diskAddr << 16) | evictedPage->memAddr;
                pageTable.erase(key);

                delete evictedPage;
            }

            Node* newPage = new Node(diskAddr, memAddr);
            pageTable[key] = newPage;
            if(!headLru){
                headLru = newPage;
                tailLru = newPage;
            }else{
                headLru->prev = newPage;
                newPage->next = headLru;
                headLru = newPage;
            }
            lruTotal++;
        }
        void printLru() const{
            cout<< "Running LRU:\n";
            cout<< "Page replacements: "<< lruTotal<< endl;
        }

        void pageLFU(int diskAddr, int memAddr){
            timestamp++;
            int key = (diskAddr << 16) | memAddr;


        }
        void printLfu() const{
            cout << "Running LFU:\n";
            cout << "Page replacements: " << lfuTotal << endl;
        }

        /*void pageOPT(int diskAddr, int memAddr){
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
                    // If there's no free space, evict the page outside the window
                    for (auto it = pageTable.begin(); it != pageTable.end();) {
                        if (ws.find(it->first) == ws.end() && ws[it->first].empty()) {
                            // Evict pages outside the window that have no recent accesses
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
         */
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
            accessSema.signal();

            vm.pageLIFO(diskAddr, memoryAddr);
            vm.pageLRU(diskAddr, memoryAddr);
            //vm.pageLFU(diskAddr, memoryAddr);
            //vm.pageOPT(diskAddr, memoryAddr);
            //vm.pageWS(diskAddr, memoryAddr);
            accessSema.wait();

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
    //virtualMem.printLfu();
    //virtualMem.printOpt();
    //virtualMem.printWs();

    return 0;
}

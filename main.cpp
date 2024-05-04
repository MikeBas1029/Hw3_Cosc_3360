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

struct NodeX{
    int diskAddr;
    int memAddr;
    int refCount; // Number of references
    NodeX* prev;
    NodeX* next;

    NodeX(int dA, int mA): diskAddr(dA), memAddr(mA), refCount(0), prev(nullptr), next(nullptr){}
};

struct NodeLfu{        //for LFU
    int diskAddr; // Disk address (page number)
    int memAddr;  // Memory address (number inside the page)
    int freq;     // Frequency of page access
    int timestamp; // Timestamp of last access
    NodeLfu* prev;
    NodeLfu* next;

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
        int lifoTotal = 0, lruTotal = 0, lfuTotal = 0, optTotal = 0, wsTotal = 0, lruXTotal = 0;
        int windowSize;                         //For WS and LRU-X

        unordered_map<int, Node*> pageTable;    //key is diskAddr and pair is memAddr (Using for LRU)
        Node* headLru = nullptr;
        Node* tailLru = nullptr;

        unordered_map<int, NodeX*> pageTableX;
        NodeX* headLruX = nullptr;
        NodeX* tailLruX = nullptr;

        unordered_map<int, NodeLfu*> pageTableLfu;
        map<int, list<NodeLfu*>> freqMap;
        int timestamp = 0;

        stack<Page> lifo;                       //for LIFO
        unordered_set<int> lifoPageSet;         //for LIFO

        unordered_map<int, int> nextPageUse;    //for OPT

        unordered_map<int, unordered_set<int>> ws;  //for WS

    public:
        VirtualMem(int tp, int minPool, int maxPool) : totalPageFrames(tp), maxPoolSize(maxPool), minPoolSize(minPool) {}

        void setDelta(int val){
            windowSize = val;
        }


        void pageLIFO(int diskAddr, int memAddr){
            Page newPage(diskAddr, memAddr);
            if(lifoPageSet.find(newPage.diskAddr) == lifoPageSet.end()){        //if not in memory
                if(lifo.size() == maxPoolSize){
                    Page oldestPage = lifo.top();
                    lifo.pop();
                    lifoPageSet.erase(oldestPage.diskAddr);
                    lifoTotal++;
                }

                lifo.push(newPage);
                lifoPageSet.insert(newPage.diskAddr);

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
                if(page != headLru){
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
                    if (headLru)
                        headLru->prev = page;
                    headLru = page;
                }
                return;
            }

            Node* newPage = new Node(diskAddr, memAddr);
            pageTable[key] = newPage;

            if (pageTable.size() >= maxPoolSize) {
                lruTotal++;
                Node* evictedPage = tailLru;
                if(headLru == tailLru){
                    headLru = nullptr;
                    tailLru = nullptr;
                }else{
                    tailLru = tailLru->prev;
                    tailLru->next = nullptr;
                }

                int keyRemove = (evictedPage->diskAddr << 16) | evictedPage->memAddr;
                pageTable.erase(keyRemove);

                delete evictedPage;
            }

            if(headLru == nullptr){
                headLru = newPage;
                tailLru = newPage;
            }else{
                headLru->prev = newPage;
                newPage->next = headLru;
                headLru = newPage;
            }


        }
        void printLru() const{
            cout<< "Running LRU:\n";
            cout<< "Page replacements: "<< lruTotal<< endl;
        }

        void pageLRUX(int diskAddr, int memAddr){
            int key = (diskAddr << 16) | memAddr;

            if (pageTableX.find(key) != pageTableX.end()) {    // Check if the page is already in pageTabeX
                NodeX* page = pageTableX[key];
                if (page != headLruX) {
                    if (page->prev) {                         // Move to the front
                        page->prev->next = page->next;
                    }
                    if (page->next) {
                        page->next->prev = page->prev;
                    }

                    if (page == tailLruX) {
                        tailLruX = tailLruX->prev;
                    }

                    page->prev = nullptr;
                    page->next = headLruX;
                    headLruX->prev = page;
                    headLruX = page;
                }
                page->refCount++;                        //increase reference count
                return;
            }

            NodeX* newPage = new NodeX(diskAddr, memAddr);
            pageTableX[key] = newPage;

            if (pageTableX.size() >= totalPageFrames) {     // Check if filed, if so find the page with the lowest reference count in the pages exceeding the threshold
                lruXTotal++;
                NodeX* pageToRemove = nullptr;
                int minReferenceCount = INT_MAX;
                for (auto& entry : pageTableX) {
                    NodeX* page = entry.second;
                    if (page->refCount <= windowSize && page->refCount < minReferenceCount) {
                        minReferenceCount = page->refCount;
                        pageToRemove = page;
                    }
                }

                if (pageToRemove) {                        //removing the page
                    int removeKey = (pageToRemove->diskAddr << 16) | pageToRemove->memAddr;
                    pageTable.erase(removeKey);
                    if (pageToRemove == headLruX) {
                        headLruX = headLruX->next;
                    }
                    if (pageToRemove == tailLruX) {
                        tailLruX = tailLruX->prev;
                    }
                    if (pageToRemove->prev) {
                        pageToRemove->prev->next = pageToRemove->next;
                    }
                    if (pageToRemove->next) {
                        pageToRemove->next->prev = pageToRemove->prev;
                    }
                    delete pageToRemove;
                }
            }

            if (!headLruX) {                            //Move new page to front of list
                headLruX = newPage;
                tailLruX = newPage;
            } else {
                headLruX->prev = newPage;
                newPage->next = headLruX;
                headLruX = newPage;
            }
            
            newPage->refCount++;                       //increase referance count
        }
        void printLRUX() const{
            cout << "Running LRU-X:\n";
            cout << "Page replacements: " << lfuTotal << endl;
        }

        void pageLFU(int diskAddr, int memAddr){
            timestamp++;
            int key = (diskAddr << 16) | memAddr;

            if(pageTableLfu.find(key) != pageTableLfu.end()){
                NodeLfu* page = pageTableLfu[key];

                int freq = page->freq;
                freqMap[freq].remove(page);

                if(freqMap[freq].empty()){
                    freqMap.erase(freq);
                }

                page->freq++;
                page->timestamp = timestamp;
                freqMap[page->freq].push_front(page);
            }else{
                if(pageTableLfu.size() >= maxPoolSize){
                    lfuTotal++;
                    auto it = freqMap.begin();
                    NodeLfu* pageRemove = *(it->second.begin());
                    it->second.pop_front();
                    if(it->second.empty()){
                        freqMap.erase(it);
                    }
                    pageTableLfu.erase((pageRemove->diskAddr << 16) | pageRemove->memAddr);

                    delete pageRemove;
                }
                NodeLfu* newPage = new NodeLfu(diskAddr, memAddr, timestamp);
                pageTableLfu[key] = newPage;
                freqMap[1].push_front(newPage);
            }

        }
        void printLfu() const{
            cout << "Running LFU:\n";
            cout << "Page replacements: " << lfuTotal << endl;
        }

        void pageOPT(int diskAddr, int memAddr){
            Page page(diskAddr, memAddr);
            nextPageUse[page.diskAddr] = numeric_limits<int>::max();

            if(nextPageUse.find(diskAddr) == nextPageUse.end()){
                //optTotal++;
                if(nextPageUse.size() >= maxPoolSize){
                    int pageToRemove = -1;
                    int farNextUse = -1;

                    for(const auto& entry : nextPageUse){
                        if(entry.second > farNextUse){
                            pageToRemove = entry.first;
                            farNextUse = entry.second;
                        }
                    }
                    nextPageUse.erase(pageToRemove);
                    optTotal++;
                }
                nextPageUse[diskAddr] = numeric_limits<int>::max();
            }

        }
        void printOpt() const{
            cout << "Running OPT:\n";
            cout << "Page replacements: " << optTotal << endl;
        }

        void pageWS(int diskAddr, int memAddr){
            if(ws.find(memAddr) == ws.end()){
                if(ws.size() >= maxPoolSize){
                    for(auto it = ws.begin(); it != ws.end();){
                        if(it->second.empty()){
                            it = ws.erase(it);
                        }else{
                            it++;
                        }
                    }
                    wsTotal++;
                }
                ws[memAddr].insert(diskAddr);

            }
            for(auto& entry: ws){
                entry.second.insert(diskAddr);
                if(entry.second.size() > windowSize){
                    entry.second.erase(entry.second.begin());
                }
            }
        }
        void printWs() const{
            cout << "Running Working Set:\n";
            cout << "Page replacements: " << wsTotal << endl;
        }

        void printMinAndMax() const{
            cout<<"Min and Max Working set for all processes\n";

            cout<< "Min: "<< minPoolSize<< endl;
            cout<< "Max: "<< maxPoolSize<< endl;
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
            accessSema.signal();

            vm.pageLIFO(diskAddr, memoryAddr);
            vm.pageLRU(diskAddr, memoryAddr);
            vm.pageLRUX(diskAddr, memoryAddr);
            vm.pageLFU(diskAddr, memoryAddr);
            vm.pageOPT(diskAddr, memoryAddr);
            vm.pageWS(diskAddr, memoryAddr);
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
    virtualMem.printLRUX();
    virtualMem.printLfu();
    virtualMem.printOpt();
    virtualMem.printWs();
    cout<< "\n---------------------------\n\n";
    virtualMem.printMinAndMax();

    return 0;
}

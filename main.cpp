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
#include <cstdlib>
#include <ctime>
#include <queue>
#include <list>
#include <unordered_set>

//compile to test g++ -o test main.cpp
using namespace std;

//The struct for pages, this isn't standard to have this function contain a class, but I am tired ok ;-;
struct Page{
    public:
        int pageNum;
        int lastUsedTime;
        int accessCount;

        Page(int p):pageNum(p), lastUsedTime(0), accessCount(0){}
};
struct VirtualMem{
    private:
        int totalPageFrames;
        unordered_map<int, Page*> pageTable;
        list<Page*> lruList;                    //for LRU
        unordered_map<int, int> accessCounts;   //for LFU
        queue<int> fifoQueue;                   //for LIFO
        vector<int> optRef;                     //for OPT
        unordered_map<int, unordered_set<int>> ws;  //for WS

    public:
        VirtualMem(int tp) : totalPageFrames(tp) {}

        void pageLIFO(){

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
        condition_variable diskDriveSem;
        struct DiskRequest{
            int processId;
            bool readOrWrite;
            string operation;
            int frameIndex;
            int memAddr;
            int diskAddr;
        };
    queue<DiskRequest> diskQueue;

    public:
        void startDisk(int procID, bool readOrWrite, const string& operation, int memoryAddr, int diskAddr){
            diskQueue.push({procID, readOrWrite, operation, memoryAddr, diskAddr});
        }
        void pageFault(){

        }
        void diskRequest(){

        }
};

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

int HexToDec(string n) {
    return stoi(n, 0, 16);
}

int main(int argc, char** argv) {

    Semaphore accessSema(1);

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
    for(int i = 0; i < 7; i++){         //to get the first designated values
        getline(file1, inputLine);
        stringstream str(inputLine);
        while (str >> numVal){
            if(numVal == 0){            //total page in the main mem.

            }else if(numVal == 1){      //page size

            }else if(numVal == 2){      //num of page frames per process (LIFO, MRU, LRU-K, LFU, and OPT)

            }else if(numVal == 3){      //window size (OPT, X for LRU-X, 0 for others that don't use this)

            }else if(numVal == 4){      //Min Free pool size

            }else if(numVal == 5){      //Max free pool size

            }else if(numVal == 6){      //total num processes

            }
        }
    }

    int count = 0;
    while(getline(file1, inputLine)){   //reading the processes,
        stringstream str(inputLine);
        while(str >> numVal){
            if(count == 0) {                  //to determine the process id and page frames

            }else{

            }
            count++;
        }
        count = 0;
    }


    return 0;
}

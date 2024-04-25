#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <limits>
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

//compile to test g++ -o test main.cpp
using namespace std;

//The struct for pages
struct PageEntry{
    int frameNum;
    bool valid;
};
struct FrameEntry{
    int processId;
    int totalPageFrame;     //on disk
};
struct DiskDriver{
    private:
        struct DiskRequest{
            int processId;
            string operation;
            int memAddr;
            int diskAddr;
        };
    queue<DiskRequest> diskQueue;

    public:
        void startDisk(int procID, const string& operation, int memoryAddr, int diskAddr){
            diskQueue.push({procID, operation, memoryAddr, diskAddr});
        }
        void processDiskRequest(){

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

void Lifo(){

}
void Mru(){

}
void Lru1(){

};
void Lru2(){

};
void LruK(){

}
void Lfu(){

}
void Ws(){

};
void Opt(){

}

int main(int argc, char** argv) {

    vector<PageEntry> pageEntry;
    vector<FrameEntry> fameEntry;
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

    FrameEntry entries;
    int count = 0;
    while(getline(file1, inputLine)){   //reading the processes,
        stringstream str(inputLine);
        while(str >> numVal){
            if(count == 0) {                  //to determine the process id and page frames
                entries.processId = numVal;
            }else{
                entries.totalPageFrame = numVal;
            }
            count++;
        }
        count = 0;
    }


    return 0;
}

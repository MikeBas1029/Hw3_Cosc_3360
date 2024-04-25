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

//compile to test g++ -o test main.cpp
using namespace std;

//The struct for pages
struct PageEntry{
    int frameNum;
    bool valid;
};
struct FrameEntry{
    int processId;
    int pageNum;
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

void startDisk(){       //determines what is read/write also location where read/write for the algorithms

}

int main(int argc, char** argv) {

    vector<PageEntry> pageEntry;
    vector<FrameEntry> fameEntry;

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
    while(getline(file1, inputLine)){   //reading the processes,
                                              //it will use semaphores for proper scheduling through all algorithms
    }


    return 0;
}

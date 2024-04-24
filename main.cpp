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
    int addSpace;
    int pageNum;
    int forwardLink;
    int backwardLink;
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



//------------------------------------------------------------------------------------------------------------
//File reading:

    if(argc != 3){                      //error if input not provided
        cout<< "No input argument is found.\n";
        return 1;
    }

    ifstream file1(argv[1]);
    if(!file1.is_open()){               //If it can't find file
        cout<< "No file can be found.\n";
        return 1;
    }

    string inputLine;


    return 0;
}

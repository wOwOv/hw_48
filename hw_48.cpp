#include <iostream>
#include <process.h>
#include "LockFreeQueue.h"

struct TestDATA
{
    int data;
    unsigned long count = 0;
};


LFQueue<TestDATA*> lfQ;
unsigned __stdcall WorkerThread0(LPVOID arg);


int main()
{
    HANDLE hThread[4];
    for (int i = 0; i < 4; i++)
    {
        hThread[i] = (HANDLE)_beginthreadex(NULL, 0, WorkerThread0, NULL, 0, NULL);
        printf("thread%d start\n", i);
    }
    while (1)
    {
    
    }
}

unsigned __stdcall WorkerThread0(LPVOID arg)
{
    DWORD id = GetCurrentThreadId();
    int num = 0;
    while (1)
    {
        for (int i = 0; i < 5; i++)
        {
            TestDATA* data = new TestDATA;
            data->data = num++;
            data->count = 1;
            lfQ.Enqueue(data);
        }

        for (int i = 0; i < 5; i++)
        {
            TestDATA* data;
            bool res=lfQ.Dequeue(&data);
            if (res == false)
            {
                DebugBreak();
            }
            delete data;
        }
    }
    return 0;
}
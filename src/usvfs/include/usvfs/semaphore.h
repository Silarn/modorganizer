#pragma once
// based on code by Jeff Preshing

// this is a synchronization class that prefers
// undefined behaviour over deadlock. It's utterly broken
// and needs to be replaced in time.

class RecursiveBenaphore {

  public:
    RecursiveBenaphore();
    ~RecursiveBenaphore();

    // wait on the semaphore. after timeout this will check if the current owner
    // thread is still alive and steal the semaphore if it isn't. Otherwise this
    // will continue to wait.
    void wait(unsigned long timeout = 0xFFFFFFFF);
    void signal();

  private:
    long m_Counter;
    unsigned long m_OwnerId;
    int m_Recursion;
    void* m_Semaphore;
};

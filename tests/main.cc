#include<iostream>
static thread_local volatile int* k = nullptr;

int main()
{
    int l =10;
    k = &l;
    std::cout << "hello world" << std::endl << *k << std::endl;
    
    return 0;
}

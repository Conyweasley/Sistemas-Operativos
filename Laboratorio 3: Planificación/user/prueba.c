#include "../kernel/types.h"
#include "../kernel/stat.h"
#include "user.h"

int main(void){
    int pid = getpid();
    for (int i = 0; i < 100000000; i++){
        i++;
    }
    pstat(pid);
    return 0; 
}
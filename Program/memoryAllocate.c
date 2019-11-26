#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define GB 1
#define SLEEP 10 //sec

int main(void)
{
    pid_t pid, ppid;
    int i,j;

    //1 loop -> 16child*32MB -> 512MB -> 0.5GB
    int childNum=16;
    int memAlloc=32*GB*2;
    int memSize=1048576; //Byte(=1MB)

    //address array for allocated memory
    char *ptr[32];

    //number of pages (memory/4kb)
    int page=memSize/4096;

    ppid = getpid(); //parent
    pid = 1; //child

    //create child process
    for(i=0; i < childNum; i++)
    {
        if(pid!=0)
        {
            if((pid=fork())<0){
                printf("fork error\n");
                return 2;
            }
            else if(pid!=0){
                printf("%d'th child process sucessfully created!\n",i);
            }
        }
    }

    /****After Fork****/

    //Parent
    if(pid!=0){
        //Nothing
        wait();
    }

    //Child
    if(pid==0)
    {
        printf("starting allocation...\n");
        //memAlloc=32*(GB*2)   total memory Allocation
        //memSize=1MB
        for(j=0; j < memAlloc; j++)
        {
            ptr[j]=(char*)malloc(memSize);
        }
        printf("child(%d) : Memory Allocate %d MB\n\n",getpid(),memAlloc);


        printf("writing memories per page...\n");
        for(i=0; i < memAlloc; i++)
        {
            if(i != 0 && i%32 == 0)
            {//sleep every SLEEP time
                printf("using %d MB\n",i);
                sleep(SLEEP);
            }
            for(j=0; j<page; j++){
                *((char*)ptr[i]+(4096*j))='d';
            }
        }

        sleep(2);

        printf("release memory...\n");
        for(i=0; i<memAlloc; i++)
        {
            free(ptr[i]);
        }
        printf("child(%d) is FREE!\n",getpid());
    }

    return 0;
}
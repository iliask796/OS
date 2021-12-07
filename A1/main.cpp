#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>
#include "SharedData.h"

using namespace std;
#define SHMSIZE 124
#define SHM_PERMS 0666
#define SEMNAME1 "/my_named_semaphore_1"
#define SEMNAME2 "/my_named_semaphore_2"
#define SEM_PERMS 0644

int main(int argc,char* argv[]) {
//    sem_unlink(SEMNAME1);
//    sem_unlink(SEMNAME2);
//    exit(0);
    //Input checks
    if (argc!=4){
        cout << "Error: Invalid Amount of App Arguments" << endl;
        return -1;
    }
    int clients=atoi(argv[2]),requests=atoi(argv[3]);
    //TODO: int pid[clients] maybe needed
    int i,j,status,err,select,numLines=0;
    //Counting number of lines in text
    ifstream file;
    file.open(argv[1]);
    string line;
    while(getline(file,line))
    {
        numLines++;
    }
    file.close();
    srand(time(NULL));
    //Creating & Attaching Shared Memory
    int shmid = shmget((key_t)getpid(),SHMSIZE,SHM_PERMS|IPC_CREAT);
    if (shmid==-1){
        perror("Creating");
    }
    else{
        cout << "Allocated Shared Memory with ID: " << shmid << endl;
    }
    SharedData* mem;
    mem = (SharedData*) shmat(shmid,(void*)0,0);
    if (*(int*)mem == -1) perror("Attachment");
    //Creating Named Semaphores
    sem_t* sem1 = sem_open(SEMNAME1, O_CREAT | O_EXCL, SEM_PERMS, 0);
    if(sem1 != SEM_FAILED) cout << "Created new semaphore." << endl;
    else if(errno==EEXIST){
        cout << "Semaphore 1 already exists. Opening semaphore." << endl;
        sem1 = sem_open(SEMNAME1, 0);
    }
    sem_t* sem2 = sem_open(SEMNAME2, O_CREAT | O_EXCL, SEM_PERMS, 1);
    if(sem2 != SEM_FAILED) cout << "Created new semaphore." << endl;
    else if(errno==EEXIST){
        cout << "Semaphore 2 already exists. Opening semaphore." << endl;
        sem1 = sem_open(SEMNAME2, 0);
    }
    //Forking Child Processes
    pid_t childpid;
    for (i=0;i<clients;i++){
        childpid=fork();
        if (childpid == -1){
            perror("fork");
            exit(3);
        }
        //Child Process
        if (childpid == 0){
            cout << "Child #" << i+1 << ", process ID: " << getpid() << ", parent ID: " << getppid() << endl;
            mem = (SharedData*) shmat(shmid,(void*)0,0);
            if (*(int*)mem == -1) perror("Attachment");
            for (i=0;i<requests;i++){
                sem_wait(sem2);
                select = rand()%numLines+1;
                (*mem).setLine(select);
                cout << "C: " << (*mem).getLine() << " (" << getpid() << ")" << endl;
                sem_post(sem1);
            }
            err = shmdt((void *)mem);
            if(err==-1) perror("Detachment");
            exit(0);
        }
        //TODO: maybe not needed
        //pid[i]=childpid;
    }
    //Parent Process
    cout << "#Parent#" << " process ID: " << getpid() << ", parent ID: " << getppid() << endl;
    cout << "Text contains "<< numLines << " lines." << endl;
    for (i=0;i<requests;i++){
        sem_wait(sem1);
        cout << "P: " << (*mem).getLine() << " || ";
        file.open(argv[1]);
        j=0;
        while(getline(file,line))
        {
            j++;
            if (j==(*mem).getLine()){
                (*mem).setContent(line);
                break;
            }
        }
        file.close();
        cout << (*mem).getContent() << endl;
        sem_post(sem2);
    }
    //Get Exit Status from Child Processes
    for (j=0;j<clients;j++){
        i = wait(&status);
        cout << "Child finished with pid: " << i << endl;
    }
    //Remove Shared Memory and Semaphores
    sem_close(sem1);
    sem_close(sem2);
    sem_unlink(SEMNAME1);
    sem_unlink(SEMNAME2);
    cout << "Just Destroyed Both Semaphores." << endl;
    err = shmdt((void *)mem);
    if(err==-1) perror("Detachment");
    err = shmctl(shmid,IPC_RMID,0);
    if (err==-1){
        perror("Removal");
    }
    else{
        cout << "Just Removed Shared Segment." << endl;
    }
    return 0;
}
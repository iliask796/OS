#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <chrono>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>
#include "SharedData.h"

#define SHMSIZE 105
#define SHM_PERMS 0666
#define SEMNAME1 "/my_named_semaphore_1"
#define SEMNAME2 "/my_named_semaphore_2"
#define SEMNAME3 "/my_named_semaphore_3"
#define SEMNAME4 "/my_named_semaphore_4"
#define SEM_PERMS 0644
#define FILE_PERMS 0644
#define OUTPUTFILE "/output."
#define timeUnit microseconds

void catchINT(int);
int shmid,parent_pid;

int main(int argc,char* argv[]) {
    //Input check
    if (argc!=4){
        cout << "Error: Invalid Amount of App Arguments" << endl;
        return -1;
    }
    //Declarations+Initializations
    int clients=atoi(argv[2]),requests=atoi(argv[3]);
    int i,j,status,exit_status,err,select,filedes,numLines=0,count=0;
    char currentDirectory[400];
    auto start=chrono::steady_clock::now(),end=chrono::steady_clock::now();
    parent_pid=getpid();
    //Set Signal Handling
    static struct sigaction act1;
    act1.sa_handler=catchINT;
    sigfillset(&(act1.sa_mask));
    sigaction(SIGINT,&act1,NULL);
    sigaction(SIGQUIT,&act1,NULL);
    //Counting number of lines in text
    ifstream file;
    file.open(argv[1]);
    string line;
    while(getline(file,line))
    {
        numLines++;
    }
    file.close();
    //Creating & Attaching Shared Memory
    shmid = shmget((key_t)getpid(),SHMSIZE,SHM_PERMS|IPC_CREAT);
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
    if(sem1 != SEM_FAILED) cout << "Created new semaphore.(1)" << endl;
    else if(errno==EEXIST){
        cout << "Semaphore 1 already exists. Opening semaphore." << endl;
        sem1 = sem_open(SEMNAME1, 0);
    }
    sem_t* sem2 = sem_open(SEMNAME2, O_CREAT | O_EXCL, SEM_PERMS, 0);
    if(sem2 != SEM_FAILED) cout << "Created new semaphore.(2)" << endl;
    else if(errno==EEXIST){
        cout << "Semaphore 2 already exists. Opening semaphore." << endl;
        sem2 = sem_open(SEMNAME2, 0);
    }
    sem_t* sem3 = sem_open(SEMNAME3, O_CREAT | O_EXCL, SEM_PERMS, 0);
    if(sem3 != SEM_FAILED) cout << "Created new semaphore.(3)" << endl;
    else if(errno==EEXIST){
        cout << "Semaphore 3 already exists. Opening semaphore." << endl;
        sem3 = sem_open(SEMNAME3, 0);
    }
    sem_t* sem4 = sem_open(SEMNAME4, O_CREAT | O_EXCL, SEM_PERMS, 0);
    if(sem4 != SEM_FAILED) cout << "Created new semaphore.(4)" << endl;
    else if(errno==EEXIST){
        cout << "Semaphore 4 already exists. Opening semaphore." << endl;
        sem4 = sem_open(SEMNAME4, 0);
    }
    //Forking Child Processes
    pid_t childpid;
    cout << "#Parent#" << " process ID: " << getpid() << ", parent ID: " << getppid() << endl;
    for (i=0;i<clients;i++){
        childpid=fork();
        if (childpid == -1){
            perror("fork");
            exit(1);
        }
        //Child Process
        if (childpid == 0){
            //Use pid during seed to increase randomness
            srand(time(NULL)+100*getpid());
            cout << "Child #" << i+1 << ", process ID: " << getpid() << ", parent ID: " << getppid() << endl;
            mem = (SharedData*) shmat(shmid,(void*)0,0);
            if (*(int*)mem == -1) perror("Attachment");
            //Creating output file
            if (getcwd(currentDirectory, sizeof(currentDirectory)) != NULL){
                strcat(currentDirectory,OUTPUTFILE);
                strcat(currentDirectory,to_string(getpid()).c_str());
            }
            if ((filedes=creat(currentDirectory,FILE_PERMS))==-1){
                perror ("creating") ;
                exit(2);
            }
            write(filedes,"PID: ",5);
            write(filedes,to_string(getpid()).c_str(),to_string(getpid()).length());
            write(filedes,"\n",1);
            write(filedes,"Result from each request:\n",26);
            //Starting Requests
            for (i=0;i<requests;i++){
                sem_wait(sem3);
                select = rand()%numLines+1;
                (*mem).setLine(select);
//                cout << "C: " << (*mem).getLine() << " (" << getpid() << ")" << endl;
                start = chrono::steady_clock::now();
                sem_post(sem1);
                sem_wait(sem2);
                end = chrono::steady_clock::now();
                count += chrono::duration_cast<chrono::timeUnit>(end-start).count();
//                cout << "C: " << (*mem).getLine() << " || " << (*mem).getContent() << " (" << getpid() << ")" << endl;
//                cout << "Request Time Interval = " << chrono::duration_cast<chrono::microseconds>(end-start).count() << "μs" << endl;
                write(filedes,(*mem).getContent().c_str(),(*mem).getContent().length());
                write(filedes,"\n",1);
//                cout << "--------------------------------------------------------" << endl;
                sem_post(sem4);
            }
            err = shmdt((void *)mem);
            if(err==-1) perror("Detachment");
            write(filedes,"Average Request Time Interval in μs is ",40);
            write(filedes,to_string(count/requests).c_str(),to_string(count/requests).length());
            write(filedes,".\n",2);
            exit(0);
        }
    }
    //Parent Process
    //Creating output file
    if (getcwd(currentDirectory, sizeof(currentDirectory)) != NULL){
        strcat(currentDirectory,OUTPUTFILE);
        strcat(currentDirectory,to_string(getpid()).c_str());
    }
    if ((filedes=creat(currentDirectory,FILE_PERMS))==-1){
        perror ("creating") ;
        exit(2);
    }
    write(filedes,"PID: ",5);
    write(filedes,to_string(getpid()).c_str(),to_string(getpid()).length());
    write(filedes,"\n",1);
    write(filedes,"Text File contains ",19);
    write(filedes,to_string(numLines).c_str(),to_string(numLines).length());
    write(filedes," lines.\n",8);
    //Receiving Requests
    for (i=0;i<clients*requests;i++){
        sem_post(sem3);
        sem_wait(sem1);
//        cout << "P: " << (*mem).getLine() << " || ";
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
//        cout << (*mem).getContent() << endl;
        sem_post(sem2);
        sem_wait(sem4);
    }
    //Get Exit Status from Child Processes
    for (j=0;j<clients;j++){
        i = wait(&status);
        if (WIFEXITED(status)){
            exit_status = WEXITSTATUS(status);
            write(filedes,"Child Process #",15);
            write(filedes,to_string(i).c_str(),to_string(i).length());
            write(filedes," finished with exit status ",27);
            write(filedes,to_string(exit_status).c_str(),to_string(exit_status).length());
            write(filedes,".\n",2);
        }
    }
    //Remove Shared Memory and Semaphores
    sem_close(sem1);
    sem_close(sem2);
    sem_close(sem3);
    sem_close(sem4);
    sem_unlink(SEMNAME1);
    sem_unlink(SEMNAME2);
    sem_unlink(SEMNAME3);
    sem_unlink(SEMNAME4);
    cout << "Just Destroyed All Semaphores." << endl;
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

void catchINT(int signo){
    if (getpid()!=parent_pid){
        kill(parent_pid,SIGINT);
        exit(3);
    }
    sem_unlink(SEMNAME1);
    sem_unlink(SEMNAME2);
    sem_unlink(SEMNAME3);
    sem_unlink(SEMNAME4);
    shmctl(shmid,IPC_RMID,0);
    cout << "@INT/QUIT SIGNAL CAUGHT with: " << signo << endl;
    cout << "@Suspending Execution." << endl;
    cout << "@Destroyed All Semaphores." << endl;
    cout << "@Removed Shared Memory." << endl;
    exit(3);
}
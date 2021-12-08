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

#define SHMSIZE 124
#define SHM_PERMS 0666
#define SEMNAME1 "/my_named_semaphore_1"
#define SEMNAME2 "/my_named_semaphore_2"
#define SEM_PERMS 0644
#define FILE_PERMS 0644
#define OUTPUTFILE "/output."

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
    int i,j,status,exit_status,err,select,filedes,numLines=0,count=0;
    char currentDirectory[400];
    auto start=chrono::steady_clock::now(),end=chrono::steady_clock::now();
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
    if(sem1 != SEM_FAILED) cout << "Created new semaphore.(1)" << endl;
    else if(errno==EEXIST){
        cout << "Semaphore 1 already exists. Opening semaphore." << endl;
        sem1 = sem_open(SEMNAME1, 0);
    }
    sem_t* sem2 = sem_open(SEMNAME2, O_CREAT | O_EXCL, SEM_PERMS, 0);
    if(sem2 != SEM_FAILED) cout << "Created new semaphore.(2)" << endl;
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
            exit(1);
        }
        //Child Process
        if (childpid == 0){
            cout << "Child #" << i+1 << ", process ID: " << getpid() << ", parent ID: " << getppid() << endl;
            mem = (SharedData*) shmat(shmid,(void*)0,0);
            if (*(int*)mem == -1) perror("Attachment");
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
            for (i=0;i<requests;i++){
                select = rand()%numLines+1;
                (*mem).setLine(select);
                cout << "C: " << (*mem).getLine() << " (" << getpid() << ")" << endl;
                start = chrono::steady_clock::now();
                sem_post(sem1);
                sem_wait(sem2);
                end = chrono::steady_clock::now();
                count += chrono::duration_cast<chrono::microseconds>(end-start).count();
                cout << "C: " << (*mem).getLine() << " || " << (*mem).getContent() << " (" << getpid() << ")" << endl;
                cout << "Request Time Interval = " << chrono::duration_cast<chrono::microseconds>(end-start).count() << "μs" << endl;
                write(filedes,(*mem).getContent().c_str(),(*mem).getContent().length());
                write(filedes,"\n",1);
                cout << "--------------------------------------------------------" << endl;
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
    cout << "#Parent#" << " process ID: " << getpid() << ", parent ID: " << getppid() << endl;
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
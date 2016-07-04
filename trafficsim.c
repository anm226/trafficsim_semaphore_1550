/*Name: Andrew Masih
 *Email: anm226@pitt.edu
 *Project 2: Syscall
 */

#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <termios.h>
#include <sys/select.h>
#include <time.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/unistd.h>
/*
 *This struct contains all the elements that will
 *need to be shared among the different processes.
 *I also use the size of this struct to mmap the
 *space that I need.
 */
 struct Node{
 	struct task_struct *curTask;
 	struct Node *next;
 };
 struct cs1550_sem {
 	int value;
 	struct Node *head;
 	struct Node *tail;
 };
struct region{
  int headNorth;  //the heads and tails represented for each queue
  int headSouth;
  int tailNorth;
  int tailSouth;
  int north[10];  //2 queues, 1 for the south direction, and 1 for north direction
  int south[10];
  int n_1;        //These elements keep track of how many total cars are there
  int n_2;        //in line at one point.
  struct cs1550_sem northFull;
  struct cs1550_sem northEmpty;
  struct cs1550_sem mutex1;
  struct cs1550_sem southFull;
  struct cs1550_sem southEmpty;
  struct cs1550_sem mutex2;
  struct cs1550_sem bothFull;
};

void down(struct cs1550_sem *sem){
  syscall(__NR_cs1550_down, sem);
}
void up(struct cs1550_sem *sem){
	syscall(__NR_cs1550_up, sem);
}


//This is used to mmap the region.
struct region *mapRegion;
struct region *safe;

/*The two producers and one consumer function, northPro() produces cars from north direction
 *southPro() produces cars from south direction, flagCon() consumes cars from both direction.
 */
void northPro();
void southPro();
void flagCon();


int main(){
  //Maps the region that needs to be shared
  mapRegion = mmap(NULL, sizeof(struct region), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
  safe = mapRegion;
  //The following are the initializations of semaphore structs for
  //the necessary synchronization between the three processes.

  //In the beginning there are no cars,
  //so we start with empty being full = 10.
  (*safe).northEmpty.value = 10;
  (*safe).northEmpty.head= NULL;
  (*safe).northEmpty.tail= NULL;

  //In the beginnign there are no cars
  //so we start with full being empyt = 0.
  (*safe).northFull.value = 0;
  (*safe).northFull.head= NULL;
  (*safe).northFull.tail= NULL;

  //This mutex provides mutual exlclusion to
  //the process writing to the queue/buffer/road
  (*safe).mutex1.value = 1;
  (*safe).mutex1.head= NULL;
  (*safe).mutex1.tail= NULL;

  //Rest are replication of previous
  //semaphores but just for the south producer.
  (*safe).southEmpty.value = 10;
  (*safe).southEmpty.head= NULL;
  (*safe).southEmpty.tail= NULL;

  (*safe).southFull.value = 0;
  (*safe).southFull.head= NULL;
  (*safe).southFull.tail= NULL;

  (*safe).mutex2.value = 1;
  (*safe).mutex2.head= NULL;
  (*safe).mutex2.tail= NULL;

  //This semaphore is mainly for the consumer
  //it checks if there are any cars at all
  //if not it puts the consumer to sleep.
  (*safe).bothFull.value = 0;
  (*safe).bothFull.head = NULL;
  (*safe).bothFull.tail = NULL;

  printf("\nWelcome to trafficsim! Some things to take notice in the output you\n");
  printf("see are that the numebr followed by the word Car represents the total\n");
  printf("number of cars coming in or being let through per side.  The number \n");
  printf("followed by the word burst represents the number of cars by the burst \n");
  printf("where burst is the total number of cars that come from a single direction\n");
  printf("at a time till there are no more cars coming.\n");
  printf("\nPress enter to start!\n");
  getchar();
  sleep(2);
  //Starts the consumer child process
  if(fork()==0){
    flagCon();
  }
  //Starts the north producer child process
  if(fork()==0){
    northPro();
  }
  //Starts the south producere child process.
  if(fork()==0){
    southPro();
  }
//Waits till the child processes finish
wait(NULL);
}

//This function basically produces cars
//into the buffer of size 10 which is
//in the shared memory region.
void northPro(){

  //This counter counts the cars per burst.
  //It is incremented in the producer, and
  //decremented in the consumer.
  (*safe).n_1 = 1;
  //This counter counts the total number of
  //cars that are produced during the run of
  //the program, this number is never decremented.
  int n = 1;
  (*safe).tailNorth = 0;
  while(1){
    //This semaphore downs/decerments the northEmpty semaphore
    //that starts out with the value 10, which makes this northPro
    //process go to sleep if there are more cars being produced.
    down(&((*safe).northEmpty));
    //Mutex to protect the safe/shared region.
    down(&((*safe).mutex1));
    *((*safe).north + (*safe).tailNorth) = (*safe).tailNorth;
    (*safe).tailNorth = ((*safe).tailNorth+1)%10;
    printf("Car %d has arrived from north! This burst %d!\n", n++, (*safe).n_1);
    (*safe).n_1++;
    up(&((*safe).mutex1));
    up(&((*safe).northFull));
    up(&((*safe).bothFull));
    //This if statement checks for the 80% chance of another car Following
    //and continues to let the while loop of producer to produce more cars.
    if(((rand())%10)<8){}
    else{
      //The process goes to sleep for 20 seconds if there are no cars following.
      printf("\n\tNo more cars are coming from North!\n\n");
      struct timespec t, t2;
      t.tv_sec = 20;
      t.tv_nsec = 0;
      nanosleep(&t, &t2);
    }

  }
}
//The southPro is replica of northPro, but it does that
//for the southPro's buffer. That's the only difference.
void southPro(){

  (*safe).n_2 = 1;
  int n = 1;
  (*safe).tailSouth = 0;
  while(1){

    down(&((*safe).southEmpty));
    down(&((*safe).mutex2));
    *((*safe).south + (*safe).tailSouth) = (*safe).tailSouth;
    (*safe).tailSouth = ((*safe).tailSouth+1)%10;

    printf("Car %d has arrived from south! This burst %d!\n", n++, (*safe).n_2);
    (*safe).n_2++;
    up(&((*safe).mutex2));
    up(&((*safe).southFull));
    up(&((*safe).bothFull));
    if(((rand())%10)<8){}
    else{
      printf("\n\tNo more cars are coming from South!\n\n");
      struct timespec t, t2;
      t.tv_sec = 20;
      t.tv_nsec = 0;
      nanosleep(&t, &t2);
    }

  }
}

//This method consumes cars from either side depending
//on the rules given in the project description.
void flagCon(){

  int n1 = 1;
  int n2 = 1;
  int print = 0;
while(1){

  //If there are no cars, print out to UI that flagperson is going to sleep
  //and make this process go to sleep, and when if there are cars from either side
  //print out that there are cars on which ever side they appear and wake up.
  if(((*safe).n_1==1)&&((*safe).n_2==1)){
    printf("\nFlagperson going to sleep ZZZ ZZZ\n");
    down(&((*safe).bothFull));
    if((*safe).n_1>1){
      printf("\nCar from North has arrived! BEEP BEEP!\n");
    }
    if((*safe).n_2>1){
      printf("\nCar from South has arrived! BEEP BEEP!\n");
    }
    up(&((*safe).bothFull));
  }

  //While there are no cars from the North direction, continue to let the
  //cars that are there.  But this loop breaks if there are 10 cars from the South
  //direction.
  while((*safe).n_1>1){
    print = 0;
    down(&((*safe).northFull));
    down(&((*safe).mutex1));
    (*safe).headNorth = ((*safe).headNorth +1)%10;

    printf("Car %d was let through from North!\n", n1++);
    (*safe).n_1--;
    up(&((*safe).mutex1));
    up(&((*safe).northEmpty));
    down(&((*safe).bothFull));
    //sleeps every 2 seconds of letting a car go
    struct timespec t, t2;
    t.tv_sec = 2;
    t.tv_nsec = 0;
    nanosleep(&t, &t2);
    if((*safe).n_2==11){
      break;
    }
  }

  //Replica of the previous while loop, but for the south buffer.
  while((*safe).n_2>1){
    print = 0;
    down(&((*safe).southFull));
    down(&((*safe).mutex2));
    (*safe).headSouth = ((*safe).headSouth +1)%10;
    printf("Car %d was let through from South!\n", n2++);
    (*safe).n_2--;
    up(&((*safe).mutex2));
    up(&((*safe).southEmpty));
    down(&((*safe).bothFull));
    struct timespec t, t2;
    t.tv_sec = 2;
    t.tv_nsec = 0;
    nanosleep(&t, &t2);
    if((*safe).n_1==11){
      break;
    }
  }
}

}

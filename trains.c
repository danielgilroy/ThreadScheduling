/*---------------------------------------------------*/
/*               CSC 360 - p2 Assingment             */
/*                                                   */
/* Date: October 27, 2014                            */
/* Name: Daniel Gilroy                               */
/* Student ID: V00813027                             */       
/*                                                   */
/*---------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>

#define STRING_LENGTH 16
#define REQUIRED_ARGS 3
#define NUMBER_OF_QUEUES 4
#define DECISECOND 100000 /*Decisecond = 0.1 second*/
#define WEST "West"
#define EAST "East"
#define HIGH 1
#define LOW 0

typedef struct train{
	int id;
	char *direction;
	int priority;
	int load;
	int cross;
	char status; /*L for loading, R for ready, F for finished*/
	pthread_cond_t track_cond;
}train;

void *load_train(void *this_train);
void *dispatch_trains(void *all_trains);
void print_train_ready(int train_id, char *heading);
void print_train_crossing(int train_id, int crossing_time, char *heading);

int num_trains;
char *previous_train_direction = WEST;

pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t trains_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t dispatcher_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t dispatcher_cross_cond = PTHREAD_COND_INITIALIZER;

pthread_mutex_t track_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t temp_cond = PTHREAD_COND_INITIALIZER;


int main(int argc, char *argv[]){

	if(argc != REQUIRED_ARGS){
		printf("--Argument Error--\n");
		printf("Please enter arguments as shown: ./msi <filename> <number of trains>\n");
		exit(1);
	}

	FILE *file;
	char line[STRING_LENGTH];
	char *filename = argv[1];
	num_trains = strtol(argv[2], NULL, 10);
	int train_count;
	char temp_char;
	int return_value;
	train *all_trains[num_trains];
	pthread_t train_thread;
	pthread_t dispatcher_thread;

	if(num_trains <= 0){
		printf("--Argument Error--\n");
		printf("Please enter arguments as shown: ./mts <filename> <number of trains>\n");
		printf("Ensure the value for <number of trains> is a valid integer greater than 0\n");
		exit(1);
	}

	file = fopen(filename, "r");
	if(file == NULL){
        fprintf(stderr, "%s: File does not exist\n", filename);
        exit(1);
    }

    for(train_count = 0; train_count < num_trains; train_count++){

    	if(fgets(line, STRING_LENGTH, file) == NULL){
    		printf("--Argument Error--\n");
    		printf("<number of trains> entered is more than trains present in %s\n", filename);
    		exit(1);
    	}

      	train *this_train = (train *) malloc(sizeof(train));
    	if(this_train == NULL){
        	perror("memory error");
        	exit(1);
    	}

    	this_train->id = train_count; /*Set train id based on position in the input list*/
    	this_train->status = 'L'; /*Set initial status of train to loading*/
    	sscanf(line, "%c:%d,%d", &temp_char, &this_train->load, &this_train->cross);
    	
    	/*Set train direction and priority based on the train character in the input file*/
    	if(temp_char == 'W'){
    		this_train->direction = WEST;
    		this_train->priority = HIGH;
    	}else if(temp_char == 'w'){
    		this_train->direction = WEST;
    		this_train->priority = LOW;
    	}else if(temp_char == 'E'){
			this_train->direction = EAST;
    		this_train->priority = HIGH;
    	}else if(temp_char == 'e'){
			this_train->direction = EAST;
    		this_train->priority = LOW;
    	}
 
		this_train->track_cond = temp_cond;
		all_trains[train_count] = this_train;
    }

    fclose(file);

    /*Create dispatcher thread that processes trains in ready queues*/
    return_value = pthread_create (&dispatcher_thread, NULL, dispatch_trains, (void *) all_trains);
    if(return_value != 0){
    	perror("pthread create error");
    	exit(1);
   	}

 
    /*Create threads to simulate the loading of each train */
    for(train_count = 0; train_count < num_trains; train_count++){

    	return_value = pthread_create (&train_thread, NULL, load_train, (void *) all_trains[train_count]);
    	if(return_value != 0){
    		perror("pthread create error");
    		exit(1);
    	}

    	/*Make train threads detached so their resources automatically release on termination*/
		return_value = pthread_detach(train_thread);    	
		if(return_value != 0){
    		perror("pthread detach error");
    		exit(1);
    	}
    }

   	/*Wait for dispatcher to finish before exit*/
   	pthread_join(dispatcher_thread, NULL);

   	/*Free malloced trains*/
   	for(train_count = 0; train_count < num_trains; train_count++){
    	free(all_trains[train_count]);
    }

	return 0;
}

void *load_train(void *this_train){

	train *current_train = (train *) this_train;
	usleep(current_train->load * DECISECOND);

	/*Print train ready message*/
	print_train_ready(current_train->id, current_train->direction);

	pthread_mutex_lock(&trains_mutex);
	current_train->status = 'R'; /*Set status of current train to ready*/
	pthread_cond_signal(&dispatcher_cond);
	pthread_mutex_unlock(&trains_mutex);
	
	pthread_cond_wait(&current_train->track_cond, &track_mutex);
	print_train_crossing(current_train->id, current_train->cross, current_train->direction);
	pthread_mutex_unlock(&track_mutex);

	/*Get trains_mutex lock and signal dispatcher to process the next train*/
	pthread_mutex_lock(&trains_mutex);
	pthread_cond_signal(&dispatcher_cross_cond);
	pthread_mutex_unlock(&trains_mutex);

	return 0;
}

void *dispatch_trains(void *all_trains){

	train **queue = (train **) all_trains;
	int trains_processed = 0;
	int trains_waiting = 0;
	int index;

	train *next = (train *) malloc(sizeof(train));
    if(next == NULL) {
       	perror("memory error");
       	exit(1);
    }

    next->status = 'F';

	while(trains_processed < num_trains){

		/*Wait here for a train to signal when set to ready status*/
		pthread_cond_wait(&dispatcher_cond, &trains_mutex);
		trains_waiting = 1;

		while(trains_waiting){
				
			/*Assume trains_waiting is zero at this point and let dispatcher find a ready train*/
			trains_waiting = 0;

			for(index = 0; index < num_trains; index++){

				if(queue[index]->status == 'R' && next->status == 'F'){

					/*Get first train with ready status and set trains_waiting flag*/
					next = queue[index];
					trains_waiting = 1;

				}else if(queue[index]->status == 'R'){

					/*Get other trains with ready status and compare with current train in "next"*/
					if(queue[index]->priority > next->priority){

						next = queue[index];

					}else if(queue[index]->priority == next->priority){

						if(!strcmp(queue[index]->direction, next->direction)){
							if(queue[index]->load < next->load){
								next = queue[index];
							} /*else*/
								/*DO NOTHING: If load time is equal, next has the lower train ID already*/
						}else if(!strcmp(next->direction, previous_train_direction)){
							next = queue[index];
						}
					} 
				}
			}

			if(trains_waiting){
				/*Signal train to start crossing and set status to finish*/
				next->status = 'F';
				trains_processed++;

				/*Set "previous_train_direction" to the direction of the current train about to cross*/
				if(!strcmp(next->direction, WEST)){
					previous_train_direction = WEST;
				}else{
					previous_train_direction = EAST;
				}

				pthread_mutex_unlock(&trains_mutex);

				/*Get track_mutex lock and signal train to start crossing*/
				pthread_mutex_lock(&track_mutex);
				pthread_cond_signal(&next->track_cond);
				pthread_mutex_unlock(&track_mutex);

				/*Wait here for train to finish crossing the track*/
				pthread_cond_wait(&dispatcher_cross_cond, &trains_mutex);
			}
		}
	}

	return 0;
}

void print_train_ready(int train_id, char *heading){

	pthread_mutex_lock(&print_mutex);
	printf("Train %2d is ready to go %4s\n", train_id, heading);
	pthread_mutex_unlock(&print_mutex);
}

void print_train_crossing(int train_id, int crossing_time, char *heading){

	pthread_mutex_lock(&print_mutex);
	printf("Train %2d is ON the main track going %4s\n", train_id, heading);
	pthread_mutex_unlock(&print_mutex);

	usleep(crossing_time * DECISECOND);

	pthread_mutex_lock(&print_mutex);
	printf("Train %2d is OFF the main track after going %4s\n", train_id, heading);
	pthread_mutex_unlock(&print_mutex);
}
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <signal.h>
#include <errno.h>
#include <sys/stat.h>

#include "cs402.h"

#include "my402list.h"

pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue2_has_element = PTHREAD_COND_INITIALIZER;

long int servers_sleeping = 0;

int var = 0;
int terminate = 0;
long int tokens = 0;
long int token_num = 0;

double avg_packet_int_arrival = 0;
double avg_s_time = 0;

typedef struct MyPacket {


	double time_arrival;
	double time_enter_q1;
	double time_leave_q1;
	double time_enter_q2;
	double time_leave_q2;
	double time_start_s;
	double time_end_s;

	long int packet_id_no;
	long int token;
	double time_service;
	double inter_arrival;

} new_packet; 

new_packet* pack;

My402List q1, q2; 

long int packets = 0;
long int packet_id = 0;
long int max_packets = 0;
long int serviced_packets = 0;

int trace_mode = 0;
int line_number = 0;

double r = 1;
double lambda = 1;
double mu = 0;
long int p = 1;
long int b = 0;
long int packets_dropped = 0;
long int tokens_dropped = 0;

char* filename;
FILE* f = NULL;

double start_time = 0;
double end_time = 0;

double total_time_q1 = 0;
double total_time_q2 = 0;
double total_time_s1 = 0;
double total_time_s2 = 0;
double total_time_system = 0;
double square_sum = 0;

sigset_t mask;

void print_error(char* erro)
{
	fprintf(stderr, "Error: %s\n", erro);
	fprintf(stdout, "Usage information: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n");
	exit(1);
}

void check_if_num(char* arg, char* param)
{
	int i = 0;
	
	for(i=0; i < strlen(arg); i++)
	{
		//printf("%s\n",arg[i]);
		if(arg[i] != '.')
		{
			if(arg[i] < '0' || arg[i] > '9')
			{
				fprintf(stderr, "Error: Wrong format of parameter %s. It should have numbers only (Real number)\n", param);
				exit(1);
			}
		}
	}

}

void check_if_integer(char* arg, char* param)
{
	int i = 0;
	//printf("%s\n", arg);
	//printf("%c\n", arg[strlen(arg)-2]);
	for(i=0; i < strlen(arg)-1; i++)
	{
	//printf("%d\n",arg[i]);
		if(arg[i] < '0' || arg[i] > '9')
		{
			if(line_number>0)
			{
				fprintf(stderr, "Error: Line number: %d : Wrong format of parameter %s. It should be an integer only\n", line_number, param);
				exit(1);
			}

			else
			{		
				fprintf(stderr, "Error: Wrong format of parameter %s. It should be an integer only\n", param);
				exit(1);
			}
		}	
	}

}

void command_line(int argc, char* argv[])
{
	int i=1;
	int lam_found = 0;
	int mu_found = 0;
	int p_found = 0;
	int n_found = 0;
	int b_found = 0;
	int r_found = 0;

	
		
	if(argc % 2 == 0)
	print_error("Malformed Command! Wrong number of arguments in the command line!");
	
	for(i=1; i < argc; i = i+2)
	{
		if(strcmp(argv[i], "-t") == 0)
		{
			//printf("First check if the filename is proper! For now assume its proper!\n");
			filename = argv[i+1];
			//printf("%s\n", filename);
			trace_mode = 1; 
		}

		else if(strcmp(argv[i], "-lambda") == 0)
		{
			check_if_num(argv[i+1], "lambda (Rate of packet arrival)");
			lambda = (double)atof(argv[i+1]);
			lam_found = 1;
		} 

		else if(strcmp(argv[i], "-mu") == 0)
		{
			check_if_num(argv[i+1], "mu (Rate of service)");
			mu = (double)atof(argv[i+1]);
			mu_found = 1;
		} 

		else if(strcmp(argv[i], "-P") == 0)
		{
			check_if_integer(argv[i+1], "P (Numebr of tokens per packet)");
			p = (long int)atoi(argv[i+1]);
			p_found = 1;
		}

		else if(strcmp(argv[i], "-n") == 0)
		{
			check_if_integer(argv[i+1], "n (Numebr of packets)");
			max_packets = (long int)atoi(argv[i+1]);
			n_found = 1;
		}

		else if(strcmp(argv[i], "-B") == 0)
		{
			check_if_integer(argv[i+1], "B (Bucket limit)");
			b = (long int)atoi(argv[i+1]);
			b_found = 1;
		}	

		else if(strcmp(argv[i], "-r") == 0)
		{
			check_if_num(argv[i+1], "r (Rate of token arrival)");
			r = (double)atof(argv[i+1]);
			r_found = 1;
		}

		else
		{
			fprintf(stderr, "Error: Malformed Command : %s\n", argv[i]);
			fprintf(stdout, "Usage information: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n");
			exit(1);
		}
	}

	if(trace_mode == 1)
	{
		lambda = 0;
		mu = 0;
		p = 0;
		max_packets = 0;


	}	

	else 
	{
		if(!lam_found)
 		lambda = 1;

		if(!mu_found)
		mu = 0.35;

		if(!p_found)
		p =3;

		if(!n_found)
		max_packets = 20;

	}

	if(!b_found)
	b = 10;

	if(!r_found)
	r = 1.5;

	/*printf("Lambda: %lf\n", lambda);
	printf("mu: %lf\n", mu);
	printf("P: %ld\n", p);
	printf("r: %lf\n", r);
	printf("n: %ld\n", max_packets);
	printf("B: %ld\n", b);*/
}

void ProcessLine(char* line)
{
	char arrival[1024];
	char req_tok[1024];
	char s_time[1024];
	int i = 0;
	int j = 0;
	int k = 0;
	int space_tabs = 0;

	
	if(strlen(line) >= 1024)
	{
		fprintf(stderr, "Error: Line number: %d : Line length crossed limits\n", line_number);
		exit(1);
	}
	
	if(line[0] == '\t' || line[0] == ' ')
	{
		fprintf(stderr, "Error: Line number: %d : Line has a leading tab or space\n", line_number);
		exit(1);		
	}

	if(line[strlen(line)-2] == '\t' || line[strlen(line)-2] == ' ')
	{
		fprintf(stderr, "Error: Line number: %d : Line has a trailing tab or space\n", line_number);
		exit(1);
	}

	if(line_number == 1)
	{
		for(i = 0; i<strlen(line); i++)
		{
			if(line[i] == '\t' || line[i] == ' ')
			{
				fprintf(stderr, "Error: Line number: %d : This line has to have a maximum of 1 entry.\n", line_number);
				exit(1);			
			}
		}
		
		check_if_integer(line, "n (Numebr of packets)");
		
		max_packets = (long int)atoi(line);
	}

	else
	{	

		for(i = 0; i<strlen(line); i++)
		{
			if(line[i] == '\t' || line[i] == ' ')
			space_tabs++;	
			if(space_tabs >= 2)
			break;
	
		}

		if(space_tabs < 2)
		{
			fprintf(stderr, "Error: Line number: %d : This line has to have a minimum 3 entries.\n", line_number);
			exit(1);
		}

		i = 0;

		while(line[i] != '\t' && line[i] != ' ')
		{
			arrival[j] = line[i];
			i++;
			j++;
		}

		arrival[j] = '\0';
		
		check_if_integer(arrival, "inter arrival time");

		j=0;

		while(line[i] == '\t' || line[i] == ' ')
		i++;

		while(line[i] != '\t' && line[i] != ' ')
		{
			req_tok[j] = line[i];
			i++;
			j++;
		}
		
		req_tok[j] = '\0';
	
		check_if_integer(req_tok, "P (Numebr of tokens per packet)");
		j = 0;

		while(line[i] == '\t' || line[i] == ' ')
		i++;


		//while(line[i] != '\t' && line[i] != ' ')
		while(line[i] != '\n')
		{
			s_time[j] = line[i];
			i++;
			j++;
		}

		s_time[j] = '\0';	
		k = j;
		j = 0;

		while(j<=k)
		{
			if(s_time[j] == '\t' || s_time[j] == ' ')
			{
				fprintf(stderr, "Error: Line number: %d : This line can have a maximum of 3 entries.\n", line_number);
				exit(1);				
			}
			j++;
		}

		check_if_integer(s_time, "service time for the packet");


		pack->inter_arrival = (double)atof(arrival);
		pack->token = (long int)atoi(req_tok);
		pack->time_service = (double)atof(s_time);

	}



}

void Read_file()
{
	char line[1026];

	if(fgets(line, sizeof(line), f)!=NULL)
	{
		//printf("Crossed fgets\n");
		line_number++;
		ProcessLine(line);
		//printf("Came back from processing line!\n");
	}
}





void initialize()
{
	My402ListInit(&q1);
	My402ListInit(&q2);

	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	pthread_sigmask(SIG_BLOCK, &mask, NULL);

	pack = (new_packet *)malloc(sizeof(new_packet));
}



double get_time_in_usec(struct timeval time)
{
	double ret = ((double)time.tv_sec * pow(10,6)) + (double)time.tv_usec;
	return ret;
}

void add_data_to_packet(new_packet* packet)
{
	packet->packet_id_no = packet_id;

	if(trace_mode == 1)
	{
		packet->token = pack->token;	
		packet->time_service = pack->time_service;	
	}

	else{

		packet->token = p;	
		
		if((1/mu)>10)
		packet->time_service = 10 * 1000;

		else
		packet->time_service = round((1/mu)*1000);

	}
}


void *thread_packet(void* a)
{

// ----------------------------------------------      Initialize the variables      -------------------------------------------------//

	double interval;
	double diff;
	double sleep_time;
	double cur_pac_timeval;

	struct timeval v;
	int i = 0;

	double  prev_pac_timeval = 0; 
	

	//Read_file();
// ----------------------------------------------        While loop - Take the mutex -------------------------------------------------// 

	while(1)
	{
// ---------------------------------------------  See how much to sleep and Lock the mutex of waking up -----------------------------// 
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
		pthread_mutex_lock(&mut);

		

		if(terminate == 1)
		{
			pthread_cond_broadcast(&queue2_has_element);
			pthread_mutex_unlock(&mut); 
			break;
		}
	
		if(packet_id >= max_packets && My402ListEmpty(&q1) == 1)
		{
			pthread_cond_broadcast(&queue2_has_element);
			pthread_mutex_unlock(&mut); 
			break;
		}
	
		if(packet_id < max_packets)
		{
			if(trace_mode == 1)
			{
				Read_file();
				interval = pack->inter_arrival * 1000;
			}

			else
			{
				if((1/lambda) > 10)
				interval = 10 * (pow(10,6));

				else
				interval = round((1/lambda)*1000) * (pow(10,3));
			}

		gettimeofday(&v, NULL);
		cur_pac_timeval = get_time_in_usec(v);
		diff = cur_pac_timeval - start_time;
		diff = diff - prev_pac_timeval;
		sleep_time = interval - diff;

		pthread_mutex_unlock(&mut);
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

		if(sleep_time > 0)
		usleep(sleep_time);

// -----------------------------------------------  Lock the mutex and assign the details to the packet ------------------------//

		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
		pthread_mutex_lock(&mut); 

		if(terminate == 1)
		{
			pthread_cond_broadcast(&queue2_has_element);
			pthread_mutex_unlock(&mut);
			break;
		}
	

		new_packet* packet;

		packet = (new_packet *)malloc(sizeof(new_packet));

		packet_id++;
		



// -------------------------------------------------- Add data to the packet: ID, tokens, service time ---------------------------------// 
		add_data_to_packet(packet);

		gettimeofday(&v, NULL);
		cur_pac_timeval = get_time_in_usec(v);		
		diff = cur_pac_timeval - start_time;

// ------------------------------ DO not forget to use the round function here ----------------------------------------//		
		packet->time_arrival = diff/1000;

		packet->inter_arrival = (diff - prev_pac_timeval)/1000;

		prev_pac_timeval = packet->time_arrival * 1000;

		if(packet_id>=1)
		avg_packet_int_arrival = (avg_packet_int_arrival*(packet_id-1) + packet->inter_arrival)/packet_id ;
		

	

//--------------------------------------------Check if packet's tokens are more than B-----------------------------------// 

		if(packet->token > b)	
		{
			fprintf(stdout, "%012.3lfms: p%ld arrives, needs %ld tokens, inter-arrival time = %lfms, dropped\n", packet->time_arrival, packet->packet_id_no, packet->token, packet->inter_arrival);
			free(packet);
			packets_dropped++;
			pthread_mutex_unlock(&mut);
			continue;
		}

//-----------------------------------------------------Add to Q1---------------------------------------------------------------------------------------//	

		packets++;
		fprintf(stdout, "%012.3lfms: p%ld arrives, needs %ld tokens, inter-arrival time = %lfms\n", packet->time_arrival, packet->packet_id_no, packet->token, packet->inter_arrival);

		if(My402ListAppend(&q1, (void *)packet) == 0)
		{
			fprintf(stderr, "Prepending error: Could not add element to the Linked List\n");
			exit(1);
		}

		gettimeofday(&v, NULL);
		cur_pac_timeval = get_time_in_usec(v);
		diff = cur_pac_timeval - start_time;

		packet->time_enter_q1 = diff/1000;

		fprintf(stdout, "%012.3lfms: p%ld enters Q1\n", packet->time_enter_q1, packet->packet_id_no);
	}
//-------------------------------------------------------------------------------------------------------------------------------------------------------------//

	if(My402ListEmpty(&q1) == 0)
	{
		My402ListElem* first = My402ListFirst(&q1);
		if(first == NULL)
		{
			pthread_mutex_unlock(&mut);						
			continue;
		}

		new_packet* queue_pac = (new_packet *)first->obj;

		if(tokens >= queue_pac->token)
		{
			for(i=0; i<queue_pac->token; i++)
			tokens--;

			packets--;
			gettimeofday(&v, NULL);
			cur_pac_timeval = get_time_in_usec(v);	
			diff = cur_pac_timeval - start_time;	
	
			queue_pac->time_leave_q1 = diff/1000;	
	
			My402ListUnlink(&q1, first);

                   	fprintf(stdout, "%012.3lfms: p%ld leaves Q1, time in Q1 = %.3lfms, token bucket now has %ld tokens\n", queue_pac->time_leave_q1, queue_pac->packet_id_no, (queue_pac->time_leave_q1 - queue_pac->time_enter_q1), tokens);

// ------------------------------------- ckeck if the server is awake or not before adding an element--------------------------------------//	

			if(My402ListEmpty(&q2) == 1)
			{
				servers_sleeping = 1;
				//printf("The servers are sleeping\n");
			}	

// ------------------------------------------ Add it to Q2 -------------------------------------- //

			if(My402ListAppend(&q2, (void *)queue_pac) == 0)
			{
				fprintf(stderr, "Appending error: Could not add element to the Linked List\n");
				exit(1);
			}

			gettimeofday(&v, NULL);
			cur_pac_timeval = get_time_in_usec(v);
			diff = cur_pac_timeval - start_time;

			queue_pac->time_enter_q2 = diff/1000;
	
			fprintf(stdout, "%012.3lfms: p%ld enters Q2\n", queue_pac->time_enter_q2, queue_pac->packet_id_no);
	
			if(servers_sleeping == 1)
			{
				pthread_cond_broadcast(&queue2_has_element);
				servers_sleeping = 0;
				//printf("Woke up the servers\n");		
			}

		}
	}
	
		pthread_mutex_unlock(&mut);
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

	}
	
	return (void*)var;
}



void *thread_token(void* a)
{
	double interval;

	if((1/r) > 10)
	interval = 10 * (pow(10,6));

	else
	interval = round((1/r)*1000) * (pow(10,3));
		

	double diff = 0;
	double sleep_time = 0;
	double cur_token_timeval = 0;

	struct timeval k;
	
	int i = 0;
	double prev_token_timeval = 0;


	while(1)
	{
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
		pthread_mutex_lock(&mut);

		if(terminate == 1)
		{
			pthread_cond_broadcast(&queue2_has_element);
			pthread_mutex_unlock(&mut);
			break;
		}

		if(packet_id >= max_packets && My402ListEmpty(&q1) == 1)
		{
			pthread_cond_broadcast(&queue2_has_element);
			pthread_mutex_unlock(&mut);
			break;
		}

		gettimeofday(&k, NULL);
		cur_token_timeval = get_time_in_usec(k);
		diff = cur_token_timeval - start_time;
		diff = diff - prev_token_timeval;
		sleep_time = interval - diff;

		pthread_mutex_unlock(&mut);
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

		if(sleep_time > 0)
		usleep(sleep_time);

		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
		pthread_mutex_lock(&mut);

		if(terminate == 1)
		{
			pthread_cond_broadcast(&queue2_has_element);
			pthread_mutex_unlock(&mut);
			break;
		}

		if(packet_id >= max_packets && My402ListEmpty(&q1) == 1)
		{
			pthread_cond_broadcast(&queue2_has_element);
			pthread_mutex_unlock(&mut);
			break;
		}

		gettimeofday(&k, NULL);
		cur_token_timeval = get_time_in_usec(k);
		diff = cur_token_timeval - start_time;
		prev_token_timeval = diff;
		tokens++;
		token_num++;
		
		if(tokens > b)
		{
			fprintf(stdout, "%012.3lfms: token t%ld arrives, dropped\n", (diff/1000), token_num);
			tokens--;
			tokens_dropped++;
			pthread_mutex_unlock(&mut);
			continue;
		}

		fprintf(stdout, "%012.3lfms: token t%ld arrives, token bucket now has %ld tokens\n", (diff/1000), token_num, tokens);

	if(My402ListEmpty(&q1) == 0)
	{

		My402ListElem* first = My402ListFirst(&q1);

		if(first == NULL)
		{
			pthread_mutex_unlock(&mut);						
			continue;
		}

		new_packet* queue_pac = (new_packet *)first->obj;

		if(tokens >= queue_pac->token)
		{
			for(i=0; i<queue_pac->token; i++)
			tokens--;

			packets--;
			gettimeofday(&k, NULL);
			cur_token_timeval = get_time_in_usec(k);	
			diff = cur_token_timeval - start_time;	
	
			queue_pac->time_leave_q1 = diff/1000;	
	
			My402ListUnlink(&q1, first);

                   	fprintf(stdout, "%012.3lfms: p%ld leaves Q1, time in Q1 = %.3lfms, token bucket now has %ld tokens\n", queue_pac->time_leave_q1, queue_pac->packet_id_no, (queue_pac->time_leave_q1 - queue_pac->time_enter_q1), tokens);

		
			if(My402ListEmpty(&q2) == 1)
			{
				servers_sleeping = 1;
			}
// ------------------------------------------ Add it to Q2 -------------------------------------- //

			if(My402ListAppend(&q2, (void *)queue_pac) == 0)
			{
				fprintf(stderr, "Appending error: Could not add element to the Linked List\n");
				exit(1);
			}

			gettimeofday(&k, NULL);
			cur_token_timeval = get_time_in_usec(k);
			diff = cur_token_timeval - start_time;

			queue_pac->time_enter_q2 = diff/1000;

			fprintf(stdout, "%012.3lfms: p%ld enters Q2\n", queue_pac->time_enter_q2, queue_pac->packet_id_no);	

			if(servers_sleeping == 1)
			{
				pthread_cond_broadcast(&queue2_has_element);
				servers_sleeping = 0;		
			}
		
		}
	}
		
		
			

			pthread_mutex_unlock(&mut);
			pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

 
	}

	return (void*)var;
}

void *thread_server(void* a)
{ 
	
	double sleep_time = 0;
	double diff;
	double cur_timeval;
	struct timeval s1;
	
	while(1)
	{



		
		pthread_mutex_lock(&mut);

		if(terminate == 1)
		{
			pthread_mutex_unlock(&mut);
			break;
		}

		if(packet_id >= max_packets && My402ListEmpty(&q1) == 1 && My402ListEmpty(&q2)==1)
		{
			pthread_mutex_unlock(&mut);
			break;
		}


	
		if(My402ListEmpty(&q2) == 1)
		{
			servers_sleeping = 1;
		}

		if(servers_sleeping == 1)
		{		
			pthread_cond_wait(&queue2_has_element, &mut);
			servers_sleeping = 0;
		}
		
		My402ListElem* service = My402ListFirst(&q2);

		if(service == NULL)
		{
			pthread_mutex_unlock(&mut);						
			continue;
		}

		new_packet* q2_pac = (new_packet *)service->obj;

		gettimeofday(&s1, NULL);
		cur_timeval = get_time_in_usec(s1);
		diff = cur_timeval - start_time;

		q2_pac->time_leave_q2 = diff/1000;

		My402ListUnlink(&q2, service);
	
                fprintf(stdout, "%012.3lfms: p%ld leaves Q2, time in Q2 = %.3lfms\n", q2_pac->time_leave_q2, q2_pac->packet_id_no, (q2_pac->time_leave_q2 - q2_pac->time_enter_q2));	

		gettimeofday(&s1, NULL);
		cur_timeval = get_time_in_usec(s1);
		diff = cur_timeval - start_time;
		q2_pac->time_start_s = diff/1000;

		fprintf(stdout, "%012.3lfms: p%ld begins service at S%d, requesting %.3lfms of service\n", q2_pac->time_start_s, q2_pac->packet_id_no, (int)a, q2_pac->time_service);
		
		sleep_time = q2_pac->time_service * 1000;

		pthread_mutex_unlock(&mut);

		usleep(sleep_time);
		
		pthread_mutex_lock(&mut);

		gettimeofday(&s1, NULL);
		cur_timeval = get_time_in_usec(s1);
		diff = cur_timeval - start_time;
		q2_pac->time_end_s = diff/1000;

		fprintf(stdout, "%012.3lfms: p%ld departs from S%d, service time = %.3lfms, time in system = %.3lfms\n", q2_pac->time_end_s, q2_pac->packet_id_no, (int)a, (q2_pac->time_end_s - q2_pac->time_start_s), (q2_pac->time_end_s - q2_pac->time_arrival));

		serviced_packets++;
	
		if(serviced_packets>=1)
		{
			avg_s_time = (avg_s_time*(serviced_packets-1) + (q2_pac->time_end_s - q2_pac->time_start_s))/serviced_packets;

			total_time_q1 = total_time_q1 + (q2_pac->time_leave_q1 - q2_pac->time_enter_q1)/1000;
			total_time_q2 = total_time_q2 + (q2_pac->time_leave_q2 - q2_pac->time_enter_q2)/1000;
		
			if((int)a == 1)
			total_time_s1 = total_time_s1 + (q2_pac->time_end_s - q2_pac->time_start_s)/1000;
			else
			total_time_s2 = total_time_s2 + (q2_pac->time_end_s - q2_pac->time_start_s)/1000;

			total_time_system = total_time_system + (q2_pac->time_end_s - q2_pac->time_arrival)/1000;
	
			square_sum = square_sum + pow((q2_pac->time_end_s - q2_pac->time_arrival)/1000, 2);
		}


		
		pthread_mutex_unlock(&mut);
	
	}

	pthread_cond_broadcast(&queue2_has_element);
	
	return (void*)var;
}


	pthread_t packet;
	pthread_t server1;
	pthread_t server2;
	pthread_t token;
	pthread_t clt_c;

void *thread_clt_c(void* a)
{
	int ret_num;

	sigwait(&mask, &ret_num);
	pthread_mutex_lock(&mut);

	My402ListElem* temp;
	new_packet* pac;
	terminate = 1;
	pthread_cancel(token);
	pthread_cancel(packet);
		pthread_cond_broadcast(&queue2_has_element);
	
	fprintf(stdout, "\nSIGINT caught, no new packets or tokens will be allowed\n");

	//pthread_mutex_lock(&mut);

	while(My402ListEmpty(&q2) != 1)
	{
		temp = My402ListFirst(&q2);
		pac = (new_packet *)temp->obj;
		fprintf(stdout, "p%ld removed from Q2\n", pac->packet_id_no);
		My402ListUnlink(&q2, temp);		
	}
	

	while(My402ListEmpty(&q1) != 1)
	{
		temp = My402ListFirst(&q1);
		pac = (new_packet *)temp->obj;
		fprintf(stdout, "p%ld removed from Q1\n", pac->packet_id_no);
		My402ListUnlink(&q1, temp);		
	}

	

	pthread_mutex_unlock(&mut);

	return (void*)var;
}

void statistics()
{
	fprintf(stdout, "Statistics:\n\n\t");
	
	if(packet_id == 0)
	fprintf(stdout, "average packet inter-arrival time = N/A : No packet arrived.\n\t");
	else
	fprintf(stdout, "average packet inter-arrival time = %.6g\n\t", avg_packet_int_arrival/1000);

	if(serviced_packets == 0)
	fprintf(stdout, "average packet service time = N/A : No packet was serviced.\n\n\t");	
	else
	fprintf(stdout, "average packet service time = %.6g\n\n\t", avg_s_time/1000);


	fprintf(stdout, "average number of packets in Q1 = %.6g\n\t", total_time_q1/((end_time - start_time)/1000000));
	fprintf(stdout, "average number of packets in Q2 = %.6g\n\t", total_time_q2/((end_time - start_time)/1000000));
	fprintf(stdout, "average number of packets in S1 = %.6g\n\t", total_time_s1/((end_time - start_time)/1000000));
	fprintf(stdout, "average number of packets in S2 = %.6g\n\n\t", total_time_s2/((end_time - start_time)/1000000));

	if(serviced_packets>=1)
	{
		fprintf(stdout, "average time a packet spent in system = %.6g\n\t", total_time_system/(double)serviced_packets);
		fprintf(stdout, "standard deviation for time spent in system = %.6g\n\n\t", (double)sqrt( (square_sum/(double)serviced_packets) - pow((total_time_system/(double)serviced_packets), 2)));
	}

	else
	{
		fprintf(stdout, "average time a packet spent in system = N/A : no packet arrived at this facility! No packet was serviced!\n\t");
		fprintf(stdout, "standard deviation for time spent in system = N/A : no packet arrived at this facility! No packet was serviced!\n\n\t");
	}

	if(token_num == 0)
	fprintf(stdout, "token drop probability = N/A : No token arrived at all!\n\t");
	else
	fprintf(stdout, "token drop probability = %.6g\n\t", (double)((double)tokens_dropped/(double)token_num));	

	if(packet_id < 1)
	fprintf(stdout, "packet drop probability = N/A : No packet ever arrived!\n\n");
	else
	fprintf(stdout, "packet drop probability = %.6g\n\n", (double)((double)packets_dropped/(double)packet_id));
	

}




int main(int argc, char* argv[])
{


	//FILE* f = NULL;

	int ret_packet;
	int ret_server;
	int ret_token;
	//int ret_clt_c;

	struct stat check_dir;
		
	struct timeval t;

	command_line(argc, argv);



	if(trace_mode == 1)
	{
		f = fopen(filename, "r");
				
		if(f == NULL)
		{	
			fprintf(stderr, "File could not be opened: %s\n" ,strerror(errno));
			exit(1);
		}

		if(stat(filename, &check_dir)!=0)
		{
			fprintf(stderr, "Error: %s\n", "File Cannot be retrieved.");
			exit(1);		
		} 
       	
   		if( S_ISDIR(check_dir.st_mode) == 1)
		{
			fprintf(stderr, "Error: %s\n", "It is a directory");
			exit(1);		
		}

		Read_file();
	}

	initialize();


	gettimeofday(&t, NULL);
	start_time = get_time_in_usec(t);

	fprintf(stdout, "%012.3lfms: emulation begins\n", (start_time - start_time));

	//fprintf(stdout, "max_packets: %ld%ld%ld\n", max_packets, max_packets, max_packets);
	if(pthread_create(&token, 0, thread_token, 0) != 0)
	{
		printf("Token Thread could not be created!!\n");
		exit(1);
	}



	if(pthread_create(&packet, 0, thread_packet, 0) != 0)
	{
		printf("Packet Thread could not be created!!\n");
		exit(1);
	}

	if(pthread_create(&server1, 0, thread_server, (void*)1) != 0)
	{
		printf("Server Thread could not be created!!\n");
		exit(1);
	}

	if(pthread_create(&server2, 0, thread_server, (void*)2) != 0)
	{
		printf("Server Thread could not be created!!\n");
		exit(1);
	}

	if(pthread_create(&clt_c, 0, thread_clt_c, 0) != 0)
	{
		printf("Signal Thread could not be created!!\n");
		exit(1);
	}

	pthread_join(token, (void*)&ret_token);
	pthread_join(server1, (void*)&ret_server);
	pthread_join(server2, (void*)&ret_server);
	pthread_join(packet, (void*)&ret_packet);


	//pthread_join(clt_c, (void*)&ret_clt_c);
	if(trace_mode == 1)
	{
		fclose(f);
	}
	
	gettimeofday(&t, NULL);
	end_time = get_time_in_usec(t);
	
	fprintf(stdout, "%012.3lfms: emulation ends\n\n", (end_time - start_time)/1000);

	statistics();

	exit(0);
	
}	

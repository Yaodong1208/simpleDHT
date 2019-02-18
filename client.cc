#include "client.h"
#include <stdio.h>
#include <iostream>
#include <stdlib.h>    
#include <time.h>
#include <thread>
#include <sys/ipc.h> 
#include <sys/msg.h> 
#include <cstring>
#include <unistd.h>
#include <future>

using namespace std;


int interval;

int test_set_range;

int operation_number;

float put_percent;

key_t send_key;
		
int send_queue_id;

key_t receive_key;

int message_sent = 0;

int message_received = 0;

int receive_queue_id;

atomic_int put_success;

atomic_int get_success;

atomic_int put_fail;

atomic_int get_fail;

	int main(int argc, char* argv[]){
		const clock_t begin_time = clock();
		send_key = 12;

		send_queue_id = msgget(send_key, 0666 | IPC_CREAT);

		receive_key = 21;

		receive_queue_id = msgget(receive_key, 0666 | IPC_CREAT);

		srand(time(0));

		interval = atoi(argv[1]);

		test_set_range = atoi(argv[2]);

		operation_number = atoi(argv[3]);

		put_percent = atof(argv[4]);

		thread t1(iPCSend<int>);

		thread t2(iPCReceive<int>);

		t1.join();
		t2.join();

		float duration = float( clock () - begin_time ) /  CLOCKS_PER_SEC;
		//msgctl(receive_queue_id, IPC_RMID, NULL); 
		//msgctl(send_queue_id, IPC_RMID, NULL); 
		
		cout<<"\n****************metrics****************\n";
		cout<<"total_process number"<<operation_number<<"\n";
		cout<<"put_success number is "<<put_success<<"\n";
		cout<<"put_fail number is "<<put_fail<<"\n";
		cout<<"get_success number is "<<get_success<<"\n";
		cout<<"get_fail number is "<<get_fail<<"\n";
		cout<<"duration is "<<duration<<"\n";
		

	}

	template<typename T>
	void iPCSend(){
		// do some ipc config


		//continually send request
		while(message_sent < operation_number) {
			IPCMessageSTD request;

			//assemble the request by calling requestAssemble(request)
			if(requestAssemble<T>(&request)) {

				perror("assemble message failed");
				continue;

			}
			//send message by ipc_call

			msgsnd(send_queue_id,&request,BUFFER,0);

			//print readable_request

			IPCRequestInfo<T>* info = (IPCRequestInfo<T>*)(&request.message_text);

			//generate readable_request
			
			if(request.operation_type == PUT){

			cout << "put " << info->hash_key<< " " << info->hash_value << " at " <<

							std::asctime(std::localtime(&(info->time_stamp)))<<"\n";

			} else {

			cout << "get " << info->hash_key << " at " << 

							std::asctime(std::localtime(&(info->time_stamp)))<<"\n";
			}
			
			usleep(interval);

			message_sent ++;
		}
		
		cout<<"terminate ipc send\n";
	} 

	template<typename T>
	int requestAssemble(IPCMessageSTD* request) {

		int status = 0;

		IPCRequestInfo<T>* info = (IPCRequestInfo<T>*)request->message_text;

		// generate info by the hash_value_type and put_percent
			

		int hash_key = rand()%test_set_range;

		info->hash_key = hash_key;

		time_t now = time(NULL);

		info->time_stamp = now;
		
		//fulfill request->operation_type;
		if(rand()%100 < put_percent*100) {

			OperationType operation_type = PUT;

			request->operation_type = operation_type;

		}else {

			OperationType operation_type = GET;

			request->operation_type = operation_type;

		}

		if(request->operation_type == PUT){

			T value = generateValue<T>();

			info->hash_value =  value;
		}


		return status;

	}

	template<typename T>
	T generateValue() {
		T rand_val = (T)rand();
		return rand_val;
	}

	template<typename T>
	void iPCReceive(){
		
		//continually receive ipc response
		while(message_received < operation_number) {

			IPCMessageSTD response;

			//call ipc_call to receive response
			msgrcv(receive_queue_id, &response, BUFFER, 0, 0);

			message_received ++;

			//calling responseProcess to process the response

			future<int> result = async(responseProcess<T>, &response);
			int status = result.get();
			if(status)
			{
				perror("error in response process");
				continue;
			};

		}
		
		cout<<"terminate ipc receive\n";
	}

	template<typename T>
	int responseProcess(IPCMessageSTD* response){

		int status = 0;

		IPCResponseInfo<T> *info = (IPCResponseInfo<T>*) (response->message_text);

		//generate readable_resposne from info
		

		if(response->operation_type == PUT) {

			cout << "PUT " << info->hash_key;

			if(!info->status){
				cout <<" SUCCESS, ";
				put_success += 1;
			}



			else{

				cout << " FAIL, ";
				put_fail += 1;
			}

		} else {

			cout << "GET " << info->hash_key;

			if(!info->status){

				cout << " SUCCESS, ";
				get_success += 1;
				}

			else{

				cout << " FAIL, ";
				get_fail += 1;
			}

		}

		cout << "at time " << std::asctime(std::localtime(&(info->time_stamp)));

		if(!info->status) 

			cout << "value is " << info->hash_value << "\n";

		return status;

	}
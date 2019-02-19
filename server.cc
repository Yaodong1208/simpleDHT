#include "server.h"
#include <sys/ipc.h> 
#include <sys/msg.h>
#include <shared_mutex>
#define LOCK_NUM 100
#include <iostream>
#include <thread>
#include <future>
#include <string.h>
#include <map>
#include <mpi.h>

using namespace std;
	
	map<int,HashStore<int>> hash_table;

	shared_mutex latch[ LOCK_NUM ];

	int world_rank;

	int world_size;

	key_t receive_key;

	int receive_queue_id;

	key_t send_key;

	int send_queue_id;


	//do some config in main then start therads
	int main(){
		//
		MPI_Init(NULL,NULL);
		//set world_rank and world_size by mpi_call
		MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

		MPI_Comm_size(MPI_COMM_WORLD, &world_size);

		receive_key = 12;

		send_key = 21;

		receive_queue_id = msgget(receive_key, 0666 | IPC_CREAT);

		send_queue_id = msgget(send_key, 0666 | IPC_CREAT);

		cout<<"my world_rank is "<<world_rank<<"\n";

		thread t1(iPCReceive<int>);
		thread t2(mPIReceive<int>);
		t1.join();
		t2.join();
	}

	template<typename T>
	void iPCReceive() {

		//use ipc_call to continually receive reqeust;
		while(true) {

			IPCMessageSTD request;

			msgrcv(receive_queue_id, &request, BUFFER, 0, 0);
		
			//call iPCProcess to process IPC income stream

			future<int> result = async(iPCProcess<T>, &request);
			int status = result.get();
			if(status)
			{
				perror("error in ipcprocess");
				continue;
			};
		}
		
	}



	template<typename T>
	int iPCProcess(IPCMessageSTD* IPC_request) {
		int status = 0;

		IPCRequestInfo<T>* IPC_request_info = (IPCRequestInfo<T>*)(IPC_request->message_text);

		

		if(findNode(IPC_request_info->hash_key) == world_rank) {

			IPCMessageSTD IPC_response;

			if(IPC_request->operation_type == PUT){

				IPC_response.operation_type = PUT;

				//cout<<"local put "<<IPC_request_info->hash_key<<" "<<IPC_request_info->hash_value<<" at "<< std::asctime(std::localtime(&(IPC_request_info->time_stamp)))<<" on "<<world_rank<<"\n";

				if(localPut<T>(IPC_request_info, (IPCResponseInfo<T>*)(IPC_response.message_text))){
					
					perror("loacal put error");

					status = 1;
				}

			}else {

				IPC_response.operation_type = GET;

				//cout<<"local get "<<IPC_request_info->hash_key<<" at "<< std::asctime(std::localtime(&(IPC_request_info->time_stamp)))<<" on "<<world_rank<<"\n";

				if(localGet<T>(IPC_request_info, (IPCResponseInfo<T>*)(IPC_response.message_text))) {

					perror("local get error");

					status = 1;
				}
			}

			if(iPCSend<T>(&IPC_response)) {

				perror("IPC message send error");

				status = 1;
			}
		}

		else {

			MPIRequest<T> MPI_request;

			//call iPCRequestInfo2MPIRequest to transfer IPC_request_info to MPI_request

			if(iPCRequest2MPIRequest<T>(IPC_request, &MPI_request)){

				perror("IPC_reqeust_info to MPI_request error");

				status = 1;
			}

			//call mPISend to send mpi message

			if(mPISend<T>(&MPI_request)) {

				perror("MPI message send error");

				status = 1;
			}

		}
		
		return status;

	}

	int findNode(int key){

		return key % 3;

	}

	template <typename T>
	int iPCSend(IPCMessageSTD* IPC_response) {

		int status = 0;

		//do ipc_send call to send back message

		msgsnd(send_queue_id,IPC_response,BUFFER,0);

		return status;

	}

	template <typename T>
	int localGet(IPCRequestInfo<T>* IPC_request_info, IPCResponseInfo<T>* IPC_response_info){

		T value;

		int status = 0;

		int get_status;

		status = get<T>(IPC_request_info->hash_key, IPC_request_info->time_stamp, &value, &get_status);
		

		//generate IPC_response_info from status and IPC_request_info
		IPC_response_info->hash_key = IPC_request_info->hash_key;

		IPC_response_info->time_stamp = IPC_request_info->time_stamp;

		IPC_response_info->status = get_status;

		if(!get_status){

			memcpy(& IPC_response_info->hash_value, &value, sizeof(T));

		}

			

		return status;
	}

	template <typename T>
	int localPut(IPCRequestInfo<T>* IPC_request_info, IPCResponseInfo<T>* IPC_response_info){

		int status = 0;
		
		int put_status;

		status = put<T>(IPC_request_info->hash_key, &IPC_request_info->hash_value, IPC_request_info->time_stamp, &put_status);
		

		//generate IPC_response_info from status and IPC_request_info
		IPC_response_info->hash_key = IPC_request_info->hash_key;

		IPC_response_info->time_stamp = IPC_request_info->time_stamp;

		IPC_response_info->status = put_status;

		if(!put_status){

			memcpy(&IPC_response_info->hash_value, &IPC_request_info->hash_value, sizeof(T));

		}

		return status;
	}

	template <typename T>
	int iPCRequest2MPIRequest(IPCMessageSTD* IPC_request, MPIRequest<T>* MPI_request){

		int status = 0;

		//fulfill MPI_request with IPC_request;
		IPCRequestInfo<T>* IPC_request_info = (IPCRequestInfo<T>*)(IPC_request->message_text);

		MPI_request->operation_type = IPC_request->operation_type;

		MPI_request->hash_key = IPC_request_info->hash_key;

		MPI_request->time_stamp = IPC_request_info->time_stamp;

		MPI_request->source = world_rank;

		memcpy(&(MPI_request->hash_value), &(IPC_request_info->hash_value), sizeof(T));

		return status;
	}

	template<typename T>
	int mPIRequestProcess(MPIRequest<T>* MPI_request) {

		int status = 0;

		MPIResponse<T> MPI_response;
		//determine which function should be called depending on the operation_type

		

		switch (MPI_request->operation_type) {

			case GET:

				//cout<<"remote get "<<MPI_request->hash_key<<" at "<< std::asctime(std::localtime(&(MPI_request->time_stamp)))<<" on "<<world_rank<<"\n";
				
				T value;
				 status = get<T>(MPI_request->hash_key, MPI_request->time_stamp, &value, &MPI_response.status);

				 if(status) {

				 	perror("get<MPI> error");

				 }

				break;

			case PUT:
				//cout<<"remote put "<<MPI_request->hash_key<<" "<<MPI_request->hash_value<<" at "<< std::asctime(std::localtime(&(MPI_request->time_stamp)))<<" on "<<world_rank<<"\n";

				status = put<T>(MPI_request->hash_key, &MPI_request->hash_value, MPI_request->time_stamp, &MPI_response.status);

				 if(status) {

				 	perror("put<MPI> error");

				 }				
		} 
		
		//fulfill MPI_response by the status and value and MPI_request
		MPI_response.destination = MPI_request->source;

		MPI_response.operation_type = MPI_request->operation_type;

		MPI_response.hash_key = MPI_request->hash_key;

		MPI_response.time_stamp = MPI_request->time_stamp;

		memcpy(&MPI_response.hash_value, &MPI_request->hash_value, sizeof(T));

		mPISend<T>(&MPI_response);

		return status;

	}

	template<typename T>
	int mPISend(MPIRequest<T>* MPI_request){
		int status = 0;

		//do mpi_call to send MPI_request to server

      MPI_Send(
        /* data         = */ MPI_request, 
        /* count        = */ sizeof(MPIRequest<T>), 
        /* datatype     = */ MPI_BYTE, 
        /* destination  = */ findNode(MPI_request->hash_key), 
        /* tag          = */ REQUEST, 
        /* communicator = */ MPI_COMM_WORLD
        );

		return status;

	}
	template<typename T>
	void mPIReceive() {

		while(true) {

			MPI_Status MPI_status;

			MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &MPI_status);
			

			future<int> result;

			int status;

			switch(MPI_status.MPI_TAG) {

				case REQUEST:
					MPIRequest<T> MPI_request;
					MPI_Recv(
					/* data         = */ &MPI_request, 
					/* count        = */ sizeof(MPIRequest<T>), 
					/* datatype     = */ MPI_BYTE, 
					/* source       = */ MPI_status.MPI_SOURCE, 
					/* tag          = */ MPI_status.MPI_TAG, 
					/* communicator = */ MPI_COMM_WORLD, 
					/* status       = */ MPI_STATUS_IGNORE);

					//log the request
					
					if(MPI_request.operation_type == PUT){
						
						//cout<<"receive MPI Request from "<<MPI_request.source<<" PUT "<<MPI_request.hash_key<<" "

						//<<MPI_request.hash_value <<" at "<< std::asctime(std::localtime(&(MPI_request.time_stamp)))<<"\n";

					}else {

						//cout<<"receive MPI Request from "<<MPI_request.source<<" GET "<<MPI_request.hash_key<<

						//" at "<< std::asctime(std::localtime(&(MPI_request.time_stamp)))<<"\n";
					}

					//use mPIReqeustProcess to process MPI_request
					
					result = async(mPIRequestProcess<T>, &MPI_request);
					status = result.get();
					if(status) {

						perror("mpi request process error");

					}

					break;

				case RESPONSE:

					MPIResponse<T> MPI_response;

					//receive MPI_respponse
					MPI_Recv(
					/* data         = */ &MPI_response, 
					/* count        = */ sizeof(MPIResponse<T>), 
					/* datatype     = */ MPI_BYTE, 
					/* source       = */ MPI_status.MPI_SOURCE, 
					/* tag          = */ MPI_status.MPI_TAG, 
					/* communicator = */ MPI_COMM_WORLD, 
					/* status       = */ MPI_STATUS_IGNORE);
					
					//log the response

					if(MPI_response.operation_type == PUT){
						
						//cout<<"receive MPI Response " << "PUT "<< MPI_response.hash_key <<" "

						//<<MPI_response.hash_value <<" at "<< std::asctime(std::localtime(&(MPI_response.time_stamp)))<<"\n";

					}else {

						//cout<<"receive MPI Response" << " GET "<<MPI_response.hash_key <<" "

						//<<MPI_response.hash_value << " at "<<std::asctime(std::localtime(&(MPI_response.time_stamp)))<<"\n";
					}

					//use mPIReqeustProcess to process MPI_request
					result = async(mPIResponseProcess<T>, &MPI_response);
					status = result.get();
					if(status)
					{

						perror("mpi response process error");

					};	

			}

		}

	}

	template <typename T>
	int mPISend(MPIResponse<T>* MPI_response) {
		int status = 0;

		//do mpi_call to send MPI_response to server
		MPI_Send(
        /* data         = */ MPI_response, 
        /* count        = */ sizeof(MPIResponse<T>), 
        /* datatype     = */ MPI_BYTE, 
        /* destination  = */ MPI_response->destination, 
        /* tag          = */ RESPONSE, 
        /* communicator = */ MPI_COMM_WORLD);

		//log the mpi send message

		if(MPI_response->operation_type == PUT){
						
			//cout<<"send MPI Response " << "PUT "<< MPI_response->hash_key <<" "

				//<<MPI_response->hash_value <<" at "<< std::asctime(std::localtime(&(MPI_response->time_stamp)))<<"\n";

		}else {

			//cout<<"send MPI Response" << " GET "<<MPI_response->hash_key <<" "

			//<<MPI_response->hash_value << " at "<<std::asctime(std::localtime(&(MPI_response->time_stamp)))<<"\n";
		}


		return status;
	}

	template <typename T>
	int mPIResponseProcess(MPIResponse<T>* MPI_response) {

		int status = 0 ;

		IPCMessageSTD IPC_response;

		//call mPIResponse2IPCResponseInfo to transfer MPI_response to IPC_response_info

		mPIResponse2IPCResponse<T>(MPI_response, &IPC_response);

		//call iPCSend to send back message

		if(iPCSend<T>(&IPC_response)){

			perror("IPC send error");

			status = 1;

		};

		return status;

	}

	template <typename T>
	int mPIResponse2IPCResponse(MPIResponse<T>* MPI_response, IPCMessageSTD* IPC_response) {

		int status = 0;
		//transfer MPIResponse to IPCResponseInfo
		IPC_response->operation_type = MPI_response->operation_type;

		IPCResponseInfo<T> * IPC_response_info =  (IPCResponseInfo<T>*)IPC_response->message_text;

		IPC_response_info->hash_key = MPI_response->hash_key;

		IPC_response_info->time_stamp = MPI_response->time_stamp;

		IPC_response_info->status = MPI_response->status;

		memcpy(&IPC_response_info->hash_value , &MPI_response->hash_value, sizeof(T));

		return status;
	}
	

	template <typename T>
	int get(int hash_key, time_t time_stamp, T* value, int* get_status) {

		int status = 0;
		*get_status = 0;

		latch[hash_key%LOCK_NUM].lock_shared();

		auto it = hash_table.find(hash_key);
		if(it == hash_table.end()) {

			*get_status = 1;

		}
		else {

			if(time_stamp < hash_table[hash_key].time_stamp) {

				*get_status = 1;

			} else {

				memcpy(value, &(hash_table[hash_key].hash_value), sizeof(T));

			}
		}
		latch[hash_key%LOCK_NUM].unlock_shared();
		return status;
	}

	template <typename T>
	int put(int hash_key, T* value ,time_t time_stamp, int* put_status) {

		int status = 0;

		*put_status = 0;

		latch[hash_key%LOCK_NUM].lock();

		if(time_stamp < hash_table[hash_key].time_stamp) {

			*put_status = 1;

		}else {

			memcpy(&(hash_table[hash_key].hash_value), value, sizeof(T));

			hash_table[hash_key].time_stamp = time_stamp;

		}	

		latch[hash_key%LOCK_NUM].unlock();

		return status;
	}
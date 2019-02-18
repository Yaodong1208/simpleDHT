#include "common.h"

enum MPITag {
	REQUEST,
	RESPONSE,
};

template <typename T>
struct MPIRequest {
	int source;
	long operation_type;
	int hash_key;
	time_t time_stamp;
	T hash_value;

};

template <typename T>
struct MPIResponse {
	int destination;
	long operation_type;
	int hash_key;
	time_t time_stamp;
	int status;
	T hash_value;
};

template<typename T> 
struct HashStore{
	T hash_value;
	time_t time_stamp;
};

template <typename T>
void iPCReceive();

template <typename T>
int iPCProcess(IPCMessageSTD* IPC_request);

template <typename T>
int localGet(IPCRequestInfo<T>* IPC_request_info, IPCResponseInfo<T>* IPC_response_info);

template <typename T>
int localPut(IPCRequestInfo<T>* IPC_request_info, IPCResponseInfo<T>* IPC_response_info);

template <typename T>
int iPCRequest2MPIRequest(IPCMessageSTD* IPC_request, MPIRequest<T>* MPI_request);

template <typename T>
int mPISend(MPIRequest<T>* MPI_request);

template <typename T>
void mPIReceive();

template <typename T>
int mPIRequestProcess(MPIRequest<T>* MPI_request);

template <typename T>
int mPISend(MPIResponse<T>* MPI_response);

template <typename T>
int mPIResponseProcess(MPIResponse<T>* MPI_response);

template <typename T>
int mPIResponse2IPCResponse(MPIResponse<T>* MPI_response, IPCMessageSTD* IPC_response);

template <typename T>
int iPCSend(IPCMessageSTD* IPC_response);

int findNode(int key);

template <typename T>
int get(int hash_key, time_t time_stamp, T* value, int* status);

template <typename T>
int put(int hash_key, T* value ,time_t time_stamp, int* status);
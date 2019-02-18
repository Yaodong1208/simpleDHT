#include "common.h"
#include <string>
enum FrequenceLevel {
	LOW,
	MEDIAN,
	HIGH,

};

template<typename T>
void iPCSend();

template<typename T>
void iPCReceive();

template<typename T>
int requestAssemble(IPCMessageSTD* request);

template<typename T>
T generateValue();

template<typename T>
int responseProcess(IPCMessageSTD* response);

void timeout();

#include <ctime>
using namespace std;
#define BUFFER 400


enum OperationType {
	PUT = 1,
	GET,
};


struct IPCMessageSTD {
	long operation_type;
	char message_text[BUFFER];
};


template <typename T>
struct IPCRequestInfo {
	int hash_key;
	time_t time_stamp;
	T hash_value;
};

template <typename T>
struct IPCResponseInfo {
	int hash_key;
	time_t time_stamp;
	int status;
	T hash_value;
};








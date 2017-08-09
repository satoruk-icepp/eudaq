#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <iomanip>

#include <sys/stat.h>
#include <sys/types.h>
#include "SC.hh"
#include "class_Exchanger.h"

#ifdef _WIN32
#pragma comment(lib, "Ws2_32.lib")
#include <winsock.h>
#include <io.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#endif

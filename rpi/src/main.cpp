#include <iostream>
#include <stdio.h>
#include <string.h>

///RPI
#include "rpi/host.h"
#include "rpi/io.h"

/// for discovery service
#define PING_MS		500
#define HOSTS_MAX	3		// was 16
#define DROP_PINGS	3

// TO-DOs:
//
// Controller hot-plugging. Decide GUI framework. Figure out Settings store/load.
// Maybe, discovery service?
// 



int main()
{
	int ret;
	
	//ChiakiLog log;
	//chiaki_log_init(&log, 4, NULL, NULL);  // 1 is the log level, 4 also good, try 14
	//chiaki_log_init(&log, CHIAKI_LOG_ALL & ~CHIAKI_LOG_VERBOSE, NULL, NULL);

	Host host; //settings, controller state, session
	IO io;	   //input, rendering, audio
	ret = io.Init(&host);
	ret = host.Init(&io);
	ret = io.InitGamepads();
	ret = io.InitFFmpeg();
	if(ret>0) {
		printf("Error in InitFFmpeg\n");
		return 0;
	}
	
	if(ret==0) {  /// 0 is good
		printf("Will try starting the session\n");
		host.StartSession();
	}
	
	while(1) sleep(1);
	
	
	
	/// Finish


	printf("END rpidrm\n");
	return 0;
}

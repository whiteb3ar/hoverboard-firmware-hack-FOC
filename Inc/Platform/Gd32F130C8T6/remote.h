#ifndef REMOTE_H
#define REMOTE_H

	void RemoteCallback(void);		// must be implemented by all remotes
	void RemoteUpdate(void);			// must be implemented by all remotes


	#if defined(REMOTE_UART)
		#include "remoteUart.h"
	#elif defined(REMOTE_UARTBUS)
		#include "remoteUartBus.h"
	#elif defined(REMOTE_CRSF)
		#include "remoteCrsf.h"
	#elif defined(REMOTE_DUMMY)
		#include "remoteDummy.h"
	#elif defined(REMOTE_AUTODETECT)
		#include "remoteAutodetect.h"
	#endif

#endif
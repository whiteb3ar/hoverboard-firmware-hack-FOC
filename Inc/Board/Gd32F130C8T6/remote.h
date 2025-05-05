#pragma once

void RemoteCallback(void);	// must be implemented by all remotes
void RemoteUpdate(void);	// must be implemented by all remotes

#if defined(REMOTE_UART)
	#include "remoteUart.h"
#elif defined(REMOTE_UARTBUS)
	#include "remoteUartBus.h"
#endif

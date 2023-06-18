#ifndef __WEB_H__
#define __WEB_H__

#include <WebServer.h>
#include <Update.h>


extern const char *WEB_RESPONSE_OK;
extern const char *WEB_RESPONSE_FAIL;

extern const char *WEBPAGE_CONTROLS;
extern const char *WEBPAGE_LOGIN;
extern const char *WEBPAGE_UPDATE;


extern void sendJSONResponse(const char *content);

extern void sendJSONResponse(const char *content, int code);

extern void sendHTMLResponse(const char *content);

extern void setupServerDefaultActions();

extern WebServer server;


#endif
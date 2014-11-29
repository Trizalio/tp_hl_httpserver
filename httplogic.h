#ifndef HTTPLOGIC_H
#define HTTPLOGIC_H

#include <iostream>
#include <assert.h>

#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <cstring>
#include <string>
#include <sys/stat.h>
#include <fcntl.h>


#include <thread>


#define SERVER_NAME "myServer"
#define HEADER_CONNECTION "close"
#define DOCUMENT_ROOT "/home/trizalio/highload/www"


#define METHOD_GET "GET"
#define METHOD_POST "POST"
#define METHOD_HEAD "HEAD"

#define STATUS_OK "200 OK"
#define STATUS_FORBIDDEN "403 Forbidden"
#define STATUS_NOT_FOUND "404 Not Found"
#define STATUS_BAD_REQUEST "405 Method Not Allowed"

#define MASSAGE_OK "200 OK"
#define MASSAGE_FORBIDDEN "403 Forbidden"
#define MASSAGE_NOT_FOUND "404 Not Found"
#define MASSAGE_BAD_REQUEST "405 Method Not Allowed"

#define MASSAGE_LENGTH_OK 6
#define MASSAGE_LENGTH_FORBIDDEN 13
#define MASSAGE_LENGTH_NOT_FOUND 13
#define MASSAGE_LENGTH_BAD_REQUEST 22

#define TYPE_HTML "text/html"
#define TYPE_CSS "text/css"
#define TYPE_JS "application/javascript"
#define TYPE_JPG "image/jpeg"
#define TYPE_JPEG "image/jpeg"
#define TYPE_PNG "image/png"
#define TYPE_GIF "image/gif"
#define TYPE_SWF "application/x-shockwave-flash"
#define TYPE_UNKNOWN "unknown"

#define EXTENTION_HTML "html"
#define EXTENTION_CSS "css"
#define EXTENTION_JS "js"
#define EXTENTION_JPG "jpg"
#define EXTENTION_JPEG "jpeg"
#define EXTENTION_PNG "png"
#define EXTENTION_GIF "gif"
#define EXTENTION_SWF "swf"


#define CRLF "\r\n"

void createResponse(bufferevent* pBufferEvent);
void writeHeader(evbuffer *pOutputBuffer, const char* pcStatus, const char* pcType, int nLength);
//void urlDecode(char *dst, const char *src);

std::string urlDecode(std::string &sSource);
bool validatePath(const std::string sPath);
const char* getContentType(const std::string& sPath);

#endif // HTTPLOGIC_H

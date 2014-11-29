#include "httplogic.h"

void createResponse(bufferevent *pBufferEvent)
{
    //std::cout << "conn_readcb callback " << std::this_thread::get_id() <<"\n";
    struct evbuffer* pInputBuffer = bufferevent_get_input(pBufferEvent);
    struct evbuffer* pOutputBuffer = bufferevent_get_output(pBufferEvent);

    char *line;
    size_t size = 0;
    line = evbuffer_readln(pInputBuffer, &size, EVBUFFER_EOL_CRLF);

    std::string Request = std::string(line);
    //std::cout << Request << "\n";

    //read method and path
    size_t nMethodEnd = Request.find(" ");
    std::string sMethod = std::string(Request, 0, nMethodEnd);
    //std::cout << sMethod << "\n";

    size_t nPathEnd = Request.find(" ", nMethodEnd+1);
    std::string sRawPath = std::string(Request, nMethodEnd+1, nPathEnd-nMethodEnd-1);
    //std::cout << sRawPath << "\n";

    size_t nDelimPos = sRawPath.find('?');
    std::string sHalfRawPath = sRawPath.substr(0, nDelimPos);
    //std::cout << sHalfRawPath << "\n";

    std::string sPath = urlDecode(sHalfRawPath);
    //std::cout << sPath << "\n";

    if (Request.find(" ", nPathEnd+1) != std::string::npos) { // extra spaces in request
        //std::cout << "ERROR: extra spaces\n";
        writeHeader(pOutputBuffer, STATUS_BAD_REQUEST, TYPE_HTML, MASSAGE_LENGTH_BAD_REQUEST);
        evbuffer_add(pOutputBuffer, MASSAGE_BAD_REQUEST, MASSAGE_LENGTH_BAD_REQUEST);
        return;
    }

    //Validate path
    if(!validatePath(sPath))
    {
        //std::cout << "Warning: forbidden path\n";
        writeHeader(pOutputBuffer, STATUS_FORBIDDEN, TYPE_HTML, MASSAGE_LENGTH_FORBIDDEN);
        evbuffer_add(pOutputBuffer, MASSAGE_FORBIDDEN, MASSAGE_LENGTH_FORBIDDEN);
        return;
    }
    //std::cout << "Ok: path ok\n";



    //find target file
    std::string sRealPath = std::string(DOCUMENT_ROOT);
    sRealPath.append(std::string(sPath));

    bool bIndex = false;
    if (sRealPath[sRealPath.length()-1] == '/') {
        //std::cout << "Ok: index\n";
        sRealPath.append("index.html");
        bIndex = true;
    }

    int fFile = open(sRealPath.c_str(), O_NONBLOCK|O_RDONLY);
    if (fFile == -1) {
        //std::cout << "Warning: file not opened\n";
        if (bIndex) {
            writeHeader(pOutputBuffer, STATUS_FORBIDDEN, TYPE_HTML, MASSAGE_LENGTH_FORBIDDEN);
            evbuffer_add(pOutputBuffer, MASSAGE_FORBIDDEN, MASSAGE_LENGTH_FORBIDDEN);
        } else {
            writeHeader(pOutputBuffer, STATUS_NOT_FOUND, TYPE_HTML, MASSAGE_LENGTH_NOT_FOUND);
            evbuffer_add(pOutputBuffer, MASSAGE_NOT_FOUND, MASSAGE_LENGTH_NOT_FOUND);
        }
        return;
    }
    //std::cout << "Ok: file opened\n";

    struct stat FileStats;
    fstat(fFile, &FileStats);
    if(!strcmp(sMethod.c_str(), METHOD_GET)) {
        //std::cout << "Ok: method \"get\"\n";
        writeHeader(pOutputBuffer, STATUS_OK, getContentType(sRealPath), FileStats.st_size);
        evbuffer_add_file(pOutputBuffer, fFile, 0, FileStats.st_size);
    } else if(!strcmp(sMethod.c_str(), METHOD_HEAD)){
        //std::cout << "Ok: method \"head\"\n";
        // ctime gives only /n so we'll add proper CRLF
        writeHeader(pOutputBuffer, STATUS_OK, getContentType(sRealPath), FileStats.st_size);
        //evbuffer_add(pOutputBuffer, CRLF, 2);
    } else {
        writeHeader(pOutputBuffer, STATUS_BAD_REQUEST, TYPE_HTML, MASSAGE_LENGTH_BAD_REQUEST);
        evbuffer_add(pOutputBuffer, MASSAGE_BAD_REQUEST, MASSAGE_LENGTH_BAD_REQUEST);
    }
    return;
}

/*void writeBadRequest(evbuffer *pOutputBuffer)
{
    writeHeader(pOutputBuffer, STATUS_BAD_REQUEST, TYPE_HTML, 17);
    evbuffer_add(pOutputBuffer, MASSAGE_BAD_REQUEST, MASSAGE_LENGTH_BAD_REQUEST);
}*/

const char* getContentType(const std::string& sPath)
{
    int nDotPos = sPath.find_last_of('.');
    std::string sFileExtention = sPath.substr(nDotPos+1);
    if(!strcmp(sFileExtention.c_str(), EXTENTION_HTML)) {
        return TYPE_HTML;
    }else if(!strcmp(sFileExtention.c_str(), EXTENTION_CSS)) {
        return TYPE_CSS;
    }else if(!strcmp(sFileExtention.c_str(), EXTENTION_JS)) {
        return TYPE_JS;
    }else if(!strcmp(sFileExtention.c_str(), EXTENTION_JPG)) {
        return TYPE_JPG;
    }else if(!strcmp(sFileExtention.c_str(), EXTENTION_JPEG)) {
        return TYPE_JPEG;
    }else if(!strcmp(sFileExtention.c_str(), EXTENTION_PNG)) {
        return TYPE_PNG;
    }else if(!strcmp(sFileExtention.c_str(), EXTENTION_GIF)) {
        return TYPE_GIF;
    }else if(!strcmp(sFileExtention.c_str(), EXTENTION_SWF)) {
        return TYPE_SWF;
    }
    return TYPE_UNKNOWN;
}

inline void getTime(char* time_buf)
{
    time_t rawtime;
    struct tm * timeinfo;
    time (&rawtime);
    timeinfo = gmtime (&rawtime);

    strftime (time_buf, 64, "%a, %d %b %G %T %Z", timeinfo);
}
void writeHeader(evbuffer *pOutputBuffer, const char* sStatus, const char* sType, int nLength)
{
    char  Headers[]=    "HTTP/1.1 %s\r\n"
                        "Content-Type: %s\r\n"
                        "Content-Length: %d\r\n"
                        "Server: %s\r\n"
                        "Connection: %s\r\n"
                        "Date: %s\r\n\r\n";
    const time_t timer = time(NULL);
    //std::cout << sStatus << "\n";

    char time_buff[64];
    getTime(time_buff);
    /*evbuffer_add_printf(output, headers, statusMessgae(s), t,
                        len, SERVER, time_buff, CONNECTION);*/
    /*evbuffer_add_printf(pOutputBuffer, Headers, sStatus, sType,
                        nLength, SERVER_NAME, HEADER_CONNECTION, ctime(&timer));*/
    evbuffer_add_printf(pOutputBuffer, Headers, sStatus, sType,
                        nLength, SERVER_NAME, HEADER_CONNECTION, time_buff);
}

bool validatePath(const std::string sPath)
{
    int nLength = sPath.length();
    int nDepth = 0;
    for(int i = 0; i < nLength;)
    {
        if(sPath[i] == '/'){
            if(sPath[i+1] == '.' && sPath[i+2] == '.' && i < nLength - 2) {
                --nDepth;
                i += 3;
            } else {
                ++nDepth;
                ++i;
            }
        } else {
            ++i;
        }
        if (nDepth < 0) return false;
    }
    return true;
}


std::string urlDecode(std::string &sSource)
{
    std::string sResult;
    char cChar;
    int nTempChar;
    for (int i = 0; i < sSource.length(); ++i)
    {
        if (int(sSource[i]) == 37)
        {
            sscanf(sSource.substr(i + 1, 2).c_str(), "%x", &nTempChar);
            cChar = static_cast <char> (nTempChar);
            sResult += cChar;
            i = i + 2;
        }
        else
        {
            sResult += sSource[i];
        }
    }
    return sResult;
}
/*void urlDecode(char *dst, const char *src)
{
    for (i = 0; i < strlen(src); ++i)
    {
        if (int(SRC[i])==37) {

        } else {

        }
    strncpy()
    }
}*/

/*void urlDecode(char *dst, const char *src)
{
    sscanf();
    char a, b;
    while (*src) {
        if ((*src == '%') &&
            ((a = src[1]) && (b = src[2])) &&
            (isxdigit(a) && isxdigit(b))) {
                if (a >= 'a')
                        a -= 'a'-'A';
                if (a >= 'A')
                        a -= ('A' - 10);
                else
                        a -= '0';
                if (b >= 'a')
                        b -= 'a'-'A';
                if (b >= 'A')
                        b -= ('A' - 10);
                else
                        b -= '0';
                *dst++ = 16*a+b;
                src+=3;
        } else {
            *dst++ = *src++;
        }
    }
    *dst++ = '\0';
}*/

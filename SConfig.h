#ifndef SCONFIG_H
#define SCONFIG_H

#define DEFAULT_PORT 80

struct SConfig
{
    unsigned short nPort = DEFAULT_PORT;
    unsigned short nProcesses = 1;
};

#endif // SCONFIG_H

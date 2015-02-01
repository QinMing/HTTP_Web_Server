#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h> //for inet_ntop() 
#include "common.h"


void PrintSocketAddress(const struct sockaddr *address, FILE *stream) {
    // Test for address and stream
    if (address == NULL || stream == NULL)
        return;
    
    void *numericAddress; // Pointer to binary address
    // Buffer to contain result (IPv6 sufficient to hold IPv4)
    char addrBuffer[INET6_ADDRSTRLEN];
    in_port_t port; // Port to print
    // Set pointer to address based on address family
    switch (address->sa_family) {
        case AF_INET:
            numericAddress = &((struct sockaddr_in *) address)->sin_addr;
            port = ntohs(((struct sockaddr_in *) address)->sin_port);
            break;
        case AF_INET6:
            numericAddress = &((struct sockaddr_in6 *) address)->sin6_addr;
            port = ntohs(((struct sockaddr_in6 *) address)->sin6_port);
            break;
        default:
            fputs("[unknown type]", stream);    // Unhandled type
            return;
    }
    // Convert binary to printable address
    if (inet_ntop(address->sa_family, numericAddress, addrBuffer,
                  sizeof(addrBuffer)) == 0)
        fputs("[invalid address]", stream); // Unable to convert
    else {
        fprintf(stream, "%s", addrBuffer);
        if (port != 0)                // Zero not valid in any socket addr
            fprintf(stream, "-%u", port);
    }
}

//1=true, 0=false
int SockAddrsEqual(const struct sockaddr *addr1, const struct sockaddr *addr2) {
    if (addr1 == NULL || addr2 == NULL)
        return addr1 == addr2;
    else if (addr1->sa_family != addr2->sa_family)
        return 0;
    else if (addr1->sa_family == AF_INET) {
        struct sockaddr_in *ipv4Addr1 = (struct sockaddr_in *) addr1;
        struct sockaddr_in *ipv4Addr2 = (struct sockaddr_in *) addr2;
        return ipv4Addr1->sin_addr.s_addr == ipv4Addr2->sin_addr.s_addr
        && ipv4Addr1->sin_port == ipv4Addr2->sin_port;
    } else if (addr1->sa_family == AF_INET6) {
        struct sockaddr_in6 *ipv6Addr1 = (struct sockaddr_in6 *) addr1;
        struct sockaddr_in6 *ipv6Addr2 = (struct sockaddr_in6 *) addr2;
        return memcmp(&ipv6Addr1->sin6_addr, &ipv6Addr2->sin6_addr,
                      sizeof(struct in6_addr)) == 0 && ipv6Addr1->sin6_port
        == ipv6Addr2->sin6_port;
    } else
        return 0;
}

int cmpNumIP(unsigned long numIPAddr, unsigned long reqIP, unsigned int mask) {
    // return 1 means two ip are euqal otherwise different
    unsigned long slideMask; 
    unsigned int i;
    i = 0;
    slideMask = 1;
    slideMask = slideMask << (32 - mask);
    for (i = 0; i < mask; i++) {
        if ((reqIP & slideMask) != (numIPAddr & slideMask))
            break;
        slideMask = slideMask << 1;
    }   
    if (i == mask) {
        return 1;
    } 
    return 0;
}

int checkAuth(struct sockaddr_in clientIP, char* filename) {
    /*  
    input: open file directory, client ip address
    output: 0 for deny, 1 for allow
    check ./htaccess whether the final directory is allowed to access by client
    */
    FILE *fd;
    char* line = NULL;
    size_t len = 0;
    ssize_t read;
    char comm[6];
    char *p = comm;
    char *pline;
    unsigned long numIPAddr, reqIP;
    struct in_addr netIPAddr;
    char strIPAddr[20];
    unsigned int mask;
    char temp;
    struct addrinfo addrCriteria;
    struct addrinfo *addrList, *addr;
    struct sockaddr_in* ai_addr_in;
    char addrString[MAXDOMAINLEN];
    const char portString[] = "0";
    int rtnVal;
    memset(&addrCriteria, 0, sizeof(addrCriteria));
    addrCriteria.ai_family = AF_INET;
    addrCriteria.ai_socktype = SOCK_STREAM;
    addrCriteria.ai_protocol = IPPROTO_TCP;

    //reqIP = ntohl(clientIP.sin_addr.s_addr);
    reqIP = clientIP.sin_addr.s_addr;
    if (( fd = fopen(filename, "r") ) == NULL) {
        //error(".htaccess file open fail\n");
        //should have deny. But for experiment, allow
        return 1;
    }

    while ((read = getline(&line, &len, fd) != -1)) {
        bzero(comm, sizeof(comm));
        pline = line;
        p = comm;
        while (*pline != ' ')
            *p++ = *pline++;
        pline++;
        while (*pline != ' ')
            pline++;
        temp = *(++pline); // judge this is ip or domain name
        if ( (temp >= 65 && temp <= 90) || (temp >= 97 && temp <= 122) ) {
            bzero(addrString, sizeof(addrString));
            p = addrString;
            while (*pline != '\n')
                *p++ = *pline++;
            printf("domain name : %s\n", addrString);
            if ((rtnVal = getaddrinfo(addrString, portString, &addrCriteria, &addrList)) != 0)
                gai_strerror(rtnVal);
            if (addrList == NULL)
                printf("addrList is NULL\n");
            for (addr = addrList; addr != NULL; addr = addr->ai_next) {
                ai_addr_in = (struct sockaddr_in*)addr->ai_addr;
                numIPAddr = ai_addr_in->sin_addr.s_addr;      // network ip
                if (cmpNumIP(numIPAddr, reqIP, 32) == 1) { 
                    if (strcmp(comm, "deny") == 0) { 
                        return 0; 
                    } 
                    else if (strcmp(comm, "allow") == 0) {
                         return 1;
                    }
                }           
            }
            continue;
        }
        else if (temp >= 48 && temp <= 57) {
            while (1) {
                bzero(strIPAddr, sizeof(strIPAddr));
                p = strIPAddr;
                while (*pline != '/' && *pline != '\n')
                    *p++ = *pline++;
                if (*pline == '\n') {
                    mask = atoi(strIPAddr);
                    break;
                }
                printf("strIPAddr %s\n", strIPAddr);
                inet_aton(strIPAddr, &netIPAddr);
                numIPAddr = (unsigned long) netIPAddr.s_addr;
                pline++;
            }
        } 
        if (cmpNumIP(numIPAddr, reqIP, mask) == 1) {
            if (strcmp(comm, "deny") == 0) {
                return 0;
            }
            else if (strcmp(comm, "allow") == 0) {
                return 1;
            }
        }            
    }
    return 1;
}

int main2(int argc, char *argv[]) {
    
    if (argc != 3) return 0;// Test for correct number of arguments
        //DieWithUserMessage("Parameter(s)", "<Address/Name> <Port/Service>");
    
    char *addrString = argv[1];   // Server address/name
    char *portString = argv[2];   // Server port/service
    
    // Tell the system what kind(s) of address info we want
    struct addrinfo addrCriteria;                   // Criteria for address match
    memset(&addrCriteria, 0, sizeof(addrCriteria)); // Zero out structure
    addrCriteria.ai_family = AF_INET;//ipv4 only
    addrCriteria.ai_socktype = SOCK_STREAM;         // Only stream sockets
    addrCriteria.ai_protocol = IPPROTO_TCP;         // Only TCP protocol
    
    // Get address(es) associated with the specified name/service
    struct addrinfo *addrList; // Holder for list of addresses returned
    // Modify servAddr contents to reference linked list of addresses
    int rtnVal = getaddrinfo(addrString, portString, &addrCriteria, &addrList);
    if (rtnVal != 0)
        gai_strerror(rtnVal);
    
    // Display returned addresses
    for (struct addrinfo *addr = addrList; addr != NULL; addr = addr->ai_next) {
        PrintSocketAddress(addr->ai_addr, stdout);
        fputc('\n', stdout);
    }
    
    freeaddrinfo(addrList); // Free addrinfo allocated in getaddrinfo()
    
    exit(0);
}

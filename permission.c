#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h> //for inet_ntop()

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

int main(int argc, char *argv[]) {
    
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

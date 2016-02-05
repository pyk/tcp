/* tcp - A C module that provides an easy-to-use interface to work with TCP.
 *
 * Copyright (c) 2016, Bayu Aldi Yansyah <bayualdiyansyah@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *     1. Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *     2. Redistributions in binary form must reproduce the above copyright
 *        notice, this list of conditions and the following disclaimer in the
 *        documentation and/or other materials provided with the distribution.
 *     3. Neither the name of the copyright holder nor the names of its 
 *        contributors may be used to endorse or promote products derived
 *        from this software without specific prior written permission. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* This macro causes system header files to expose definitions corresponding 
 * to the POSIX.1-2008 base specification (excluding the XSI extension). */
#define _POSIX_C_SOURCE 200809L

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "tcp.h"

/* tcpdial connects to a TCP server. It supports IPV4 and IPV6.
 * 
 * The first parameter is the pointer of integer where the enpoint of 
 * connection is write into. The second and the third one is the host and port
 * of TCP server to connect to.
 *
 * If the function succeeds it returns 0 and the enpoint of connection can be
 * used by send(2) and recv(2).
 * If the function fails, it returns and set errno to one of the following 
 * non-zero values:
 *
 * ENOPROTOOPT
 * The TCP protocol entry is not available in the host protocols database. On
 * the linux host, the protocol database file is /etc/protocols.
 *
 * EACCES
 * The process does not have appropriate privileges.
 *
 * ENOMEM
 * Insufficient memory is available.
 *
 * ENETUNREACH
 * Currently the name server is unreachable. Try again later.
 *
 * ENETDOWN
 * The name server is down.
 *
 * EINVAL
 * The host or port is invalid.
 *
 * ENOTCONN
 * The socket is not connected.
 *
 * Other system error may returned, check errno for the details.
 *
 * Example
 *     int conn;
 *     int errdial = tcpdial(&conn, "localhost", "9090");
 *     if(errdial != 0) {
 *         fprintf(stderr, "E: tcpdial %s\n", strerror(errdial));
 *     }
 */
int tcpdial(int *conn, char host[], char port[])
{
    /* get the TCP protocol number from the host; 
     * conforming to POSIX.1-2001 */
    struct protoent *tcpproto = getprotobyname("tcp");
    if(tcpproto == NULL) {
        errno = ENOPROTOOPT;
        return errno;
    }

    /* create a socket; conforming to POSIX.1-2001 */
    *conn = socket(AF_INET, SOCK_STREAM, tcpproto->p_proto);
    if(*conn == -1) {
        return errno;
    }

    /* create a socket address; conforming to POSIX.1-2001 */
    struct addrinfo tcphints, *tcpsockaddr;
    memset(&tcphints, 0, sizeof tcphints);
    tcphints.ai_family = AF_UNSPEC;
    tcphints.ai_socktype = SOCK_STREAM;
    tcphints.ai_protocol = tcpproto->p_proto;
    errno = getaddrinfo(host, port, &tcphints, &tcpsockaddr);
    if(errno != 0) {
        switch(errno) {
            case EAI_AGAIN:
                errno = ENETUNREACH;
                break;
            case EAI_FAIL:
                errno = ENETDOWN;
                break;
            case EAI_MEMORY:
                errno = ENOMEM;
                break;
            case EAI_NONAME:
            case EAI_SERVICE:
                errno = EINVAL;
                break;
            default:
                break;
        }
        return errno;
    }

    /* iterate over returned address & test the connection for each address */
    struct addrinfo *addri;
    for(addri = tcpsockaddr; addri != NULL; addri = addri->ai_next) {
        /* try connect to a socket address; conforming to POSIX.1-2001 */
        int retcon = connect(*conn, addri->ai_addr, addri->ai_addrlen);
        if(retcon != 0) {
            /* continue until we can connect to the address */
            continue;
        }
        break;
    }
    if(addri == NULL) {
        errno = ENOTCONN;
        return errno;
    }

    return 0;
}

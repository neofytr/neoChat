#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>

#include "network.h"

#define CHAT_PORT "4040"
#define LISTEN_BUF_LEN (100)

#define THREAD_POOL_SIZE (16)

pthread_t thread_pool[THREAD_POOL_SIZE];

void *thread_function(void *arg);

void print_getaddrinfo(struct addrinfo *servinfo)
{
    struct addrinfo *p;
    char ip_addr[INET6_ADDRSTRLEN];

    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        void *addr;
        char *ip_version;

        if (p->ai_family == AF_INET) // IPv4
        {
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
            addr = &(ipv4->sin_addr);
            ip_version = "IPv4";
        }
        else if (p->ai_family == AF_INET6) // IPv6
        {
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
            addr = &(ipv6->sin6_addr);
            ip_version = "IPv6";
        }
        else
        {
            continue;
        }

        inet_ntop(p->ai_family, addr, ip_addr, sizeof(ip_addr));
        printf("%s Address: %s\n", ip_version, ip_addr);
    }
}

int main(int argc, char **argv)
{
    struct addrinfo hints, *server;

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    pthread_attr_t attr;
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    for (size_t counter = 0; counter < THREAD_POOL_SIZE; counter++)
    {
        if (pthread_create(&thread_pool[counter], &attr, thread_function, NULL))
        {
            pthread_attr_destroy(&attr);
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }

    pthread_attr_destroy(&attr);

    if (getaddrinfo(NULL, CHAT_PORT, &hints, &server))
    {
        perror("getaddrinfo");
        exit(EXIT_FAILURE);
    }

    int server_fd = socket(server->ai_family, server->ai_socktype, server->ai_protocol);
    if (server_fd == -1)
    {
        freeaddrinfo(server);
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int yes = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
    {
        perror("setsockopt");
        close(server_fd);
        freeaddrinfo(server);
        exit(EXIT_FAILURE);
    }

    if ((bind(server_fd, (struct sockaddr *)server->ai_addr, server->ai_addrlen)) == -1)
    {
        close(server_fd);
        freeaddrinfo(server);
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if ((listen(server_fd, LISTEN_BUF_LEN)) == -1)
    {
        close(server_fd);
        freeaddrinfo(server);
        perror("listen");
        exit(EXIT_FAILURE);
    }

    while (true)
    {
    }

    close(server_fd);
    return EXIT_SUCCESS;
}
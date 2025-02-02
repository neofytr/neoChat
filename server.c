#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <fcntl.h>
#include <stdio.h>

#include "network.h"
#include "hash_table.h"

#define CHAT_PORT "4040"
#define LISTEN_BUF_LEN (100)

#define MAX_DATA_LEN (8192)

#define THREAD_POOL_SIZE (16)
#define MAX_EVENTS (500)

pthread_t thread_pool[THREAD_POOL_SIZE];

void *thread_function(void *arg)
{
    return NULL;
}

void make_socket_nonblocking(int sockfd)
{
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}

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

    hash_table_t *userpass_table = create_hash_table();
    if (!userpass_table)
    {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    if (getaddrinfo(NULL, CHAT_PORT, &hints, &server))
    {
        destroy_hash_table(userpass_table);
        perror("getaddrinfo");
        exit(EXIT_FAILURE);
    }

    int server_fd = socket(server->ai_family, server->ai_socktype, server->ai_protocol);
    if (server_fd == -1)
    {
        destroy_hash_table(userpass_table);
        freeaddrinfo(server);
        perror("socket");
        exit(EXIT_FAILURE);
    }

    make_socket_nonblocking(server_fd);

    int yes = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
    {
        destroy_hash_table(userpass_table);
        perror("setsockopt");
        close(server_fd);
        freeaddrinfo(server);
        exit(EXIT_FAILURE);
    }

    if ((bind(server_fd, (struct sockaddr *)server->ai_addr, server->ai_addrlen)) == -1)
    {
        destroy_hash_table(userpass_table);
        close(server_fd);
        freeaddrinfo(server);
        perror("bind");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(server);

    if ((listen(server_fd, LISTEN_BUF_LEN)) == -1)
    {
        destroy_hash_table(userpass_table);
        close(server_fd);
        perror("listen");
        exit(EXIT_FAILURE);
    }

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1)
    {
        destroy_hash_table(userpass_table);
        perror("epoll_create1");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    struct epoll_event event, events[MAX_EVENTS];
    event.events = EPOLLIN;
    event.data.fd = server_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event);

    int client_fd;

    while (true)
    {
        int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        for (int counter = 0; counter < num_events; counter++)
        {
            if (events[counter].data.fd == server_fd)
            {
                // new incoming connection
                client_fd = accept(server_fd, NULL, NULL);
                if (client_fd >= 0)
                {
                    make_socket_nonblocking(client_fd);
                    event.events = EPOLLIN | EPOLLET;
                    event.data.fd = client_fd;
                    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event);
                    printf("new client added\n");
                }
                else
                {
                    destroy_hash_table(userpass_table);
                    close(server_fd);
                    close(epoll_fd);
                    perror("accept");
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                char *buffer = (char *)malloc(MAX_DATA_LEN * sizeof(char));
                if (!buffer)
                {
                    destroy_hash_table(userpass_table);
                    close(server_fd);
                    close(epoll_fd);
                    perror("malloc");
                    exit(EXIT_FAILURE);
                }
                int bytes_read = recv(events[counter].data.fd, buffer, MAX_DATA_LEN - 1, 0);
                buffer[bytes_read] = '\0';

                if (bytes_read == 0)
                {
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[counter].data.fd, NULL);
                    close(events[counter].data.fd);
                }
                else
                {
                }
            }
        }
    }

    close(server_fd);
    close(epoll_fd);
    destroy_hash_table(userpass_table);
    return EXIT_SUCCESS;
}
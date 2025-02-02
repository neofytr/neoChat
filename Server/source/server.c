#include "../include/server.h"

void send_data(int client_fd, char *buffer)
{
    size_t bytes_sent = 0;
    size_t len = strlen(buffer);
    while (bytes_sent < len)
    {
        ssize_t ret = send(client_fd, buffer + bytes_sent, len - bytes_sent, 0);
        if (ret == -1)
        {
            return;
        }
        bytes_sent += ret;
    }
}

void handle_service(int client_fd, char *service)
{
    char service_type[MAX_SERVICE_LEN];
    size_t len = strlen(service);
    size_t service_len = 0;

    for (; service_len < MAX_SERVICE_LEN - 1 && service_len < len; service_len++)
    {
        if (isspace(service[service_len]))
        {
            service_type[service_len] = '\0';
            break;
        }
        service_type[service_len] = service[service_len];
    }

    if (!strcmp(service_type, "SIGNUP"))
    {
#define MAX_USERNAME_LEN 256
#define MAX_PASS_LEN 256
        char username[MAX_USERNAME_LEN];
        char password[MAX_PASS_LEN];
#undef MAX_USERNAME_LEN
#undef MAX_PASS_LEN

        size_t username_len = 0;
        size_t current_pos = service_len + 1;

        while (current_pos < len && !isspace(service[current_pos]) && username_len < sizeof(username) - 1)
        {
            username[username_len++] = service[current_pos++];
        }
        username[username_len] = '\0';

        if (current_pos < len && isspace(service[current_pos]))
        {
            current_pos++;
        }

        size_t password_len = 0;
        while (current_pos < len && !isspace(service[current_pos]) && password_len < sizeof(password) - 1)
        {
            password[password_len++] = service[current_pos++];
        }
        password[password_len] = '\0';

        if (username_len == 0 || password_len == 0)
        {
            send_data(client_fd, "ERR 99\r\n");
            return;
        }

        hash_node_t *searched_node = (hash_node_t *)hash_table_search(registered_table, username);
        if (searched_node)
        {
            send_data(client_fd, "ERR 103\r\n");
        }
        else
        {
            pthread_mutex_lock(&registered_table_mutex);
            while (!hash_table_insert(registered_table, username, password, 0))
            {
            }
            send_data(client_fd, "OK_SIGNEDUP\r\n");
            pthread_mutex_unlock(&registered_table_mutex);
        }
    }
    else if (!strcmp(service_type, "LOGOUT"))
    {
#define MAX_USERNAME_LEN 256
        char username[MAX_USERNAME_LEN];
#undef MAX_USERNAME_LEN

        size_t username_len = 0;
        size_t current_pos = service_len + 1;

        while (current_pos < len && !isspace(service[current_pos]) && username_len < sizeof(username) - 1)
        {
            username[username_len++] = service[current_pos++];
        }
        username[username_len] = '\0';

        if (!username_len)
        {
            send_data(client_fd, "ERR 99\r\n");
            return;
        }

        pthread_mutex_lock(&curr_login_table_mutex);
        hash_node_t *logged_in_user = (hash_node_t *)hash_table_search(curr_login_table, username);
        if (!logged_in_user)
        {
            send_data(client_fd, "ERR 104\r\n");
            pthread_mutex_unlock(&curr_login_table_mutex);
            return;
        }

        while (!hash_table_delete(curr_login_table, username))
        {
        }

        send_data(client_fd, "OK_LOGGEDOUT\r\n");
        pthread_mutex_unlock(&curr_login_table_mutex);
    }
    else if (!strcmp(service_type, "LOGIN"))
    {
#define MAX_USERNAME_LEN 256
#define MAX_PASS_LEN 256
        char username[MAX_USERNAME_LEN];
        char password[MAX_PASS_LEN];
#undef MAX_USERNAME_LEN
#undef MAX_PASS_LEN

        size_t username_len = 0;
        size_t current_pos = service_len + 1;

        while (current_pos < len && !isspace(service[current_pos]) && username_len < sizeof(username) - 1)
        {
            username[username_len++] = service[current_pos++];
        }
        username[username_len] = '\0';

        if (current_pos < len && isspace(service[current_pos]))
        {
            current_pos++;
        }

        size_t password_len = 0;
        while (current_pos < len && !isspace(service[current_pos]) && password_len < sizeof(password) - 1)
        {
            password[password_len++] = service[current_pos++];
        }
        password[password_len] = '\0';

        if (!username_len || !password_len)
        {
            send_data(client_fd, "ERR 99\r\n");
            return;
        }

        pthread_mutex_lock(&registered_table_mutex);
        hash_node_t *registered_user = (hash_node_t *)hash_table_search(registered_table, username);
        if (!registered_user)
        {
            send_data(client_fd, "ERR 100\r\n");
            pthread_mutex_unlock(&registered_table_mutex);
            return;
        }
        pthread_mutex_unlock(&registered_table_mutex);

        pthread_mutex_lock(&curr_login_table_mutex);
        hash_node_t *logged_in_user = (hash_node_t *)hash_table_search(curr_login_table, username);
        if (logged_in_user)
        {
            send_data(client_fd, "ERR 101\r\n");
            pthread_mutex_unlock(&curr_login_table_mutex);
            return;
        }

        if (strcmp(registered_user->password, password) != 0)
        {
            send_data(client_fd, "ERR 102\r\n");
            pthread_mutex_unlock(&curr_login_table_mutex);
            return;
        }

        while (!hash_table_insert(curr_login_table, username, password, client_fd))
        {
        }

        send_data(client_fd, "OK_LOGGEDIN\r\n");
        pthread_mutex_unlock(&curr_login_table_mutex);
    }
    else
    {
        send_data(client_fd, "ERR 99\r\n");
    }
}

void *thread_function(void *arg)
{
    (int *)arg;
    while (true)
    {
        pthread_mutex_lock(&service_queue_mutex);
        while (is_empty(service_queue))
        {
            pthread_cond_wait(&service_queue_cond, &service_queue_mutex);
        }

        node_t *service_node = peek(service_queue);
        char *service = service_node->service;
        int client_fd = service_node->client_fd;
        dequeue(service_queue);
        pthread_mutex_unlock(&service_queue_mutex);

        mutex_node_t *user_mutex_node = mutex_table_search(mutex_table, client_fd);
        pthread_mutex_lock(&user_mutex_node->user_mutex);
        handle_service(client_fd, service);
        pthread_mutex_unlock(&user_mutex_node->user_mutex);
        free(service);
    }

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

int main()
{
    struct addrinfo hints, *server;

    memset(&hints, 0, sizeof(hints));
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

    curr_login_table = create_hash_table();
    if (!curr_login_table)
    {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    mutex_table = create_mutex_table();
    if (!mutex_table)
    {
        destroy_hash_table(curr_login_table);
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    registered_table = create_hash_table();
    if (!registered_table)
    {
        destroy_mutex_table(mutex_table);
        destroy_hash_table(curr_login_table);
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    if (getaddrinfo(NULL, CHAT_PORT, &hints, &server))
    {
        destroy_hash_table(curr_login_table);
        destroy_hash_table(registered_table);
        perror("getaddrinfo");
        exit(EXIT_FAILURE);
    }

    int server_fd = socket(server->ai_family, server->ai_socktype, server->ai_protocol);
    if (server_fd == -1)
    {
        destroy_hash_table(curr_login_table);
        destroy_hash_table(registered_table);
        freeaddrinfo(server);
        perror("socket");
        exit(EXIT_FAILURE);
    }

    make_socket_nonblocking(server_fd);

    int yes = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
    {
        destroy_hash_table(curr_login_table);
        destroy_hash_table(registered_table);
        perror("setsockopt");
        close(server_fd);
        freeaddrinfo(server);
        exit(EXIT_FAILURE);
    }

    if ((bind(server_fd, (struct sockaddr *)server->ai_addr, server->ai_addrlen)) == -1)
    {
        destroy_hash_table(curr_login_table);
        destroy_hash_table(registered_table);
        close(server_fd);
        freeaddrinfo(server);
        perror("bind");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(server);

    if ((listen(server_fd, LISTEN_BUF_LEN)) == -1)
    {
        destroy_hash_table(curr_login_table);
        destroy_hash_table(registered_table);
        close(server_fd);
        perror("listen");
        exit(EXIT_FAILURE);
    }

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1)
    {
        destroy_hash_table(curr_login_table);
        destroy_hash_table(registered_table);
        perror("epoll_create1");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    struct epoll_event event, events[MAX_EVENTS];
    event.events = EPOLLIN;
    event.data.fd = server_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event);

    int client_fd;

    service_queue = create_queue();
    if (!service_queue)
    {
        close(epoll_fd);
        perror("malloc");
        close(server_fd);
        destroy_hash_table(curr_login_table);
        destroy_hash_table(registered_table);
        exit(EXIT_FAILURE);
    }

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
                }
                else
                {
                    destroy_queue(service_queue);
                    destroy_hash_table(curr_login_table);
                    close(server_fd);
                    close(epoll_fd);
                    perror("accept");
                    destroy_hash_table(registered_table);
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                if (!mutex_table_search(mutex_table, events[counter].data.fd))
                {
                    mutex_table_insert(mutex_table, events[counter].data.fd);
                }
                char *buffer = (char *)malloc(MAX_DATA_LEN * sizeof(char)); // cleanup handled at the end of thread_handle function
                if (!buffer)
                {
                    destroy_queue(service_queue);
                    destroy_hash_table(curr_login_table);
                    close(server_fd);
                    close(epoll_fd);
                    perror("malloc");
                    destroy_hash_table(registered_table);
                    exit(EXIT_FAILURE);
                }

                int bytes_read = recv(events[counter].data.fd, buffer, MAX_DATA_LEN - 1, 0);
                buffer[bytes_read] = '\0';

                if (!bytes_read)
                {
                    // client closed connection; a recv with zero will occur in this case and only in this case
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[counter].data.fd, NULL);
                    close(events[counter].data.fd);
                    free(buffer);
                }
                else
                {
                    char *current_pos = buffer;
                    char *next_delim;

                    pthread_mutex_lock(&service_queue_mutex);

                    while ((next_delim = strstr(current_pos, "\r\n")) != NULL)
                    {
                        size_t service_len = next_delim - current_pos;
                        char *service = (char *)malloc((service_len + 1) * sizeof(char));

                        if (!service)
                        {
                            pthread_mutex_unlock(&service_queue_mutex);
                            destroy_queue(service_queue);
                            destroy_hash_table(curr_login_table);
                            free(buffer);
                            close(server_fd);
                            close(epoll_fd);
                            perror("malloc");
                            destroy_hash_table(registered_table);
                            exit(EXIT_FAILURE);
                        }

                        strncpy(service, current_pos, service_len);
                        service[service_len] = '\0';

                        if (enqueue(events[counter].data.fd, service, service_queue) == -1)
                        {
                            pthread_mutex_unlock(&service_queue_mutex);
                            destroy_queue(service_queue);
                            destroy_hash_table(curr_login_table);
                            free(service);
                            free(buffer);
                            close(server_fd);
                            close(epoll_fd);
                            perror("malloc");
                            destroy_hash_table(registered_table);
                            exit(EXIT_FAILURE);
                        }

                        pthread_cond_signal(&service_queue_cond);
                        current_pos = next_delim + 2; // Skip past \r\n
                    }

                    // Handle any remaining data that doesn't end with \r\n
                    if (*current_pos != '\0')
                    {
                        size_t service_len = strlen(current_pos);
                        char *service = (char *)malloc((service_len + 1) * sizeof(char));

                        if (!service)
                        {
                            pthread_mutex_unlock(&service_queue_mutex);
                            destroy_queue(service_queue);
                            destroy_hash_table(curr_login_table);
                            free(buffer);
                            close(server_fd);
                            close(epoll_fd);
                            perror("malloc");
                            destroy_hash_table(registered_table);
                            exit(EXIT_FAILURE);
                        }

                        strcpy(service, current_pos);

                        if (enqueue(events[counter].data.fd, service, service_queue) == -1)
                        {
                            pthread_mutex_unlock(&service_queue_mutex);
                            destroy_queue(service_queue);
                            destroy_hash_table(curr_login_table);
                            free(service);
                            free(buffer);
                            close(server_fd);
                            close(epoll_fd);
                            perror("malloc");
                            destroy_hash_table(registered_table);
                            exit(EXIT_FAILURE);
                        }

                        pthread_cond_signal(&service_queue_cond);
                    }

                    pthread_mutex_unlock(&service_queue_mutex);
                    free(buffer);
                }
            }
        }
    }

    close(server_fd);
    close(epoll_fd);
    destroy_queue(service_queue);
    destroy_hash_table(curr_login_table);
    destroy_hash_table(registered_table);
    return EXIT_SUCCESS;
}
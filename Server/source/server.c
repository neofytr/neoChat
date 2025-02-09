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
    if (!service)
    {
        send_data(client_fd, "ERR 99\r\n");
        return;
    }

    char service_type[MAX_SERVICE_LEN] = {0};
    size_t len = strlen(service);
    size_t service_len = 0;

    for (; service_len < MAX_SERVICE_LEN - 1 && service_len < len; service_len++)
    {
        if (isspace(service[service_len]))
        {
            break;
        }
        service_type[service_len] = service[service_len];
    }
    service_type[service_len] = '\0';

    if (!strcmp(service_type, "SIGNUP"))
    {
#define MAX_USERNAME_LEN 256
#define MAX_PASS_LEN 256
        char username[MAX_USERNAME_LEN] = {0};
        char password[MAX_PASS_LEN] = {0};

        size_t current_pos = service_len;
        while (current_pos < len && isspace(service[current_pos]))
        {
            current_pos++;
        }

        size_t username_len = 0;
        while (current_pos < len && !isspace(service[current_pos]) && username_len < MAX_USERNAME_LEN - 1)
        {
            username[username_len++] = service[current_pos++];
        }
        username[username_len] = '\0';

        while (current_pos < len && isspace(service[current_pos]))
        {
            current_pos++;
        }

        size_t password_len = 0;
        while (current_pos < len && !isspace(service[current_pos]) && password_len < MAX_PASS_LEN - 1)
        {
            password[password_len++] = service[current_pos++];
        }
        password[password_len] = '\0';

        if (username_len == 0 || password_len == 0)
        {
            send_data(client_fd, "ERR 99\r\n");
            return;
        }

        for (size_t i = 0; i < username_len; i++)
        {
            if (!isalnum(username[i]) && username[i] != '_')
            {
                send_data(client_fd, "ERR 99\r\n");
                return;
            }
        }

        pthread_mutex_lock(&registered_table_mutex);
        hash_node_t *searched_node = (hash_node_t *)hash_table_search(registered_table, username);
        if (searched_node)
        {
            pthread_mutex_unlock(&registered_table_mutex);
            send_data(client_fd, "ERR 103\r\n");
            return;
        }

        if (!hash_table_insert(registered_table, username, password, 0))
        {
            pthread_mutex_unlock(&registered_table_mutex);
            send_data(client_fd, "ERR 99\r\n");
            return;
        }

        send_data(client_fd, "OK_SIGNEDUP\r\n");
        pthread_mutex_unlock(&registered_table_mutex);
    }
    else if (!strcmp(service_type, "POLL"))
    {
        size_t total_len = 5; // USERS

        pthread_mutex_lock(&curr_login_table_mutex);
        hash_node_t **buckets = curr_login_table->buckets;

        for (size_t counter = 0; counter < NUM_BUCKETS; counter++)
        {
            hash_node_t *curr = buckets[counter];
            while (curr)
            {
                total_len += strlen(curr->username) + 1; // +1 for space
                curr = curr->next_node;
            }
        }

        total_len += 3; //\r\n\0

        char *buffer;
        while (!(buffer = (char *)malloc(sizeof(char) * total_len)))
        {
        }

        memcpy(buffer, "USERS", 5);
        size_t current_pos = 5;

        for (size_t counter = 0; counter < NUM_BUCKETS; counter++)
        {
            hash_node_t *curr = buckets[counter];
            while (curr)
            {
                buffer[current_pos++] = ' ';
                size_t username_len = strlen(curr->username);
                memcpy(buffer + current_pos, curr->username, username_len);
                current_pos += username_len;
                curr = curr->next_node;
            }
        }

        memcpy(buffer + current_pos, "\r\n", 2);
        buffer[current_pos + 2] = '\0';

        send_data(client_fd, buffer);

        free(buffer);
        pthread_mutex_unlock(&curr_login_table_mutex);
    }
    else if (!strcmp(service_type, "LOGIN"))
    {
#define MAX_USERNAME_LEN 256
#define MAX_PASS_LEN 256
        char username[MAX_USERNAME_LEN] = {0};
        char password[MAX_PASS_LEN] = {0};

        size_t current_pos = service_len;
        while (current_pos < len && isspace(service[current_pos]))
        {
            current_pos++;
        }

        size_t username_len = 0;
        while (current_pos < len && !isspace(service[current_pos]) && username_len < MAX_USERNAME_LEN - 1)
        {
            username[username_len++] = service[current_pos++];
        }
        username[username_len] = '\0';

        while (current_pos < len && isspace(service[current_pos]))
        {
            current_pos++;
        }

        size_t password_len = 0;
        while (current_pos < len && !isspace(service[current_pos]) && password_len < MAX_PASS_LEN - 1)
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
            pthread_mutex_unlock(&registered_table_mutex);
            send_data(client_fd, "ERR 100\r\n");
            return;
        }

        if (strcmp(registered_user->password, password) != 0)
        {
            pthread_mutex_unlock(&registered_table_mutex);
            send_data(client_fd, "ERR 102\r\n");
            return;
        }
        pthread_mutex_unlock(&registered_table_mutex);

        pthread_mutex_lock(&curr_login_table_mutex);
        hash_node_t *logged_in_user = (hash_node_t *)hash_table_search(curr_login_table, username);
        if (logged_in_user)
        {
            pthread_mutex_unlock(&curr_login_table_mutex);
            send_data(client_fd, "ERR 101\r\n");
            return;
        }

        if (!hash_table_insert(curr_login_table, username, password, client_fd))
        {
            pthread_mutex_unlock(&curr_login_table_mutex);
            send_data(client_fd, "ERR 99\r\n");
            return;
        }

        send_data(client_fd, "OK_LOGGEDIN\r\n");
        pthread_mutex_unlock(&curr_login_table_mutex);
    }
    else if (!strcmp(service_type, "LOGOUT"))
    {
#define MAX_USERNAME_LEN 256
        char username[MAX_USERNAME_LEN] = {0};

        size_t current_pos = service_len;
        while (current_pos < len && isspace(service[current_pos]))
        {
            current_pos++;
        }

        size_t username_len = 0;
        while (current_pos < len && !isspace(service[current_pos]) && username_len < MAX_USERNAME_LEN - 1)
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
            pthread_mutex_unlock(&curr_login_table_mutex);
            send_data(client_fd, "ERR 104\r\n");
            return;
        }

        if (!hash_table_delete(curr_login_table, username))
        {
            pthread_mutex_unlock(&curr_login_table_mutex);
            send_data(client_fd, "ERR 99\r\n");
            return;
        }

        send_data(client_fd, "OK_LOGGEDOUT\r\n");
        pthread_mutex_unlock(&curr_login_table_mutex);
    }
    else if (!strcmp(service_type, "REQ"))
    {
#define MAX_USERNAME_LEN 256
        char requested_username[MAX_USERNAME_LEN] = {0};
        char requesting_username[MAX_USERNAME_LEN] = {0};

        size_t current_pos = service_len;
        while (current_pos < len && isspace(service[current_pos]))
        {
            current_pos++;
        }

        size_t requested_username_len = 0;
        while (current_pos < len && !isspace(service[current_pos]) && requested_username_len < MAX_USERNAME_LEN - 1)
        {
            requested_username[requested_username_len++] = service[current_pos++];
        }
        requested_username[requested_username_len] = '\0';

        while (current_pos < len && isspace(service[current_pos]))
        {
            current_pos++;
        }

        size_t requesting_username_len = 0;
        while (current_pos < len && !isspace(service[current_pos]) && requesting_username_len < MAX_USERNAME_LEN - 1)
        {
            requesting_username[requesting_username_len++] = service[current_pos++];
        }
        requesting_username[requesting_username_len] = '\0';

        if (!requested_username_len || !requesting_username_len ||
            requested_username_len >= MAX_USERNAME_LEN ||
            requesting_username_len >= MAX_USERNAME_LEN)
        {
            send_data(client_fd, "ERR 99\r\n");
            return;
        }

        if (strcmp(requested_username, requesting_username) == 0)
        {
            send_data(client_fd, "ERR 99\r\n");
            return;
        }

        pthread_mutex_lock(&curr_login_table_mutex);

        hash_node_t *requesting_user = (hash_node_t *)hash_table_search(curr_login_table, requesting_username);
        if (!requesting_user || requesting_user->user_fd != client_fd)
        {
            pthread_mutex_unlock(&curr_login_table_mutex);
            send_data(client_fd, "ERR 104\r\n");
            return;
        }

        hash_node_t *requested_user = (hash_node_t *)hash_table_search(curr_login_table, requested_username);
        if (!requested_user)
        {
            pthread_mutex_unlock(&curr_login_table_mutex);
            send_data(client_fd, "ERR 106\r\n");
            return;
        }

        char request_msg[MAX_USERNAME_LEN + 32] = {0};
        snprintf(request_msg, sizeof(request_msg), "CHAT_REQ %s\r\n", requesting_username);
        send_data(requested_user->user_fd, request_msg);

        pthread_mutex_unlock(&curr_login_table_mutex);
    }
    else if (!strcmp(service_type, "CHAT_RESP"))
    {
#define MAX_RESPONSE_LEN 8
        char requesting_username[MAX_USERNAME_LEN] = {0};
        char response[MAX_RESPONSE_LEN] = {0};

        size_t current_pos = service_len;
        while (current_pos < len && isspace(service[current_pos]))
        {
            current_pos++;
        }

        size_t requesting_username_len = 0;
        while (current_pos < len && !isspace(service[current_pos]) && requesting_username_len < MAX_USERNAME_LEN - 1)
        {
            requesting_username[requesting_username_len++] = service[current_pos++];
        }
        requesting_username[requesting_username_len] = '\0';

        while (current_pos < len && isspace(service[current_pos]))
        {
            current_pos++;
        }

        size_t response_len = 0;
        while (current_pos < len && !isspace(service[current_pos]) && response_len < MAX_RESPONSE_LEN - 1)
        {
            response[response_len++] = service[current_pos++];
        }
        response[response_len] = '\0';

        if (!requesting_username_len || !response_len ||
            requesting_username_len >= MAX_USERNAME_LEN)
        {
            send_data(client_fd, "ERR 99\r\n");
            return;
        }

        pthread_mutex_lock(&curr_login_table_mutex);

        hash_node_t *requesting_user = (hash_node_t *)hash_table_search(curr_login_table, requesting_username);
        if (!requesting_user)
        {
            pthread_mutex_unlock(&curr_login_table_mutex);
            send_data(client_fd, "ERR 104\r\n");
            return;
        }

        char *accepting_user = (char *)hash_table_search_via_id(curr_login_table, client_fd);
        if (!accepting_user)
        {
            pthread_mutex_unlock(&curr_login_table_mutex);
            send_data(client_fd, "ERR 107\r\n"); // the accepting user not logged in
        }

        char response_msg[MAX_USERNAME_LEN + 32] = {0};
        if (strcmp(response, "ACCEPT") == 0)
        {
            snprintf(response_msg, sizeof(response_msg), "CHAT_ACCEPTED %s\r\n", accepting_user);
        }
        else if (strcmp(response, "REJECT") == 0)
        {
            snprintf(response_msg, sizeof(response_msg), "CHAT_REJECTED %s\r\n", accepting_user);
        }
        else
        {
            pthread_mutex_unlock(&curr_login_table_mutex);
            send_data(client_fd, "ERR 99\r\n");
            return;
        }

        send_data(requesting_user->user_fd, response_msg);

        pthread_mutex_unlock(&curr_login_table_mutex);
    }
    else if (!strcmp(service_type, "SEND"))
    {
#define MAX_USERNAME_LEN 256
#define MAX_MESSAGE_LEN 1024
        char sender[MAX_USERNAME_LEN] = {0};
        char receiver[MAX_USERNAME_LEN] = {0};

        size_t current_pos = service_len;
        while (current_pos < len && isspace(service[current_pos]))
        {
            current_pos++;
        }

        size_t sender_len = 0;
        while (current_pos < len && !isspace(service[current_pos]) && sender_len < MAX_USERNAME_LEN - 1)
        {
            sender[sender_len++] = service[current_pos++];
        }
        sender[sender_len] = '\0';

        while (current_pos < len && isspace(service[current_pos]))
        {
            current_pos++;
        }

        size_t receiver_len = 0;
        while (current_pos < len && !isspace(service[current_pos]) && receiver_len < MAX_USERNAME_LEN - 1)
        {
            receiver[receiver_len++] = service[current_pos++];
        }
        receiver[receiver_len] = '\0';

        if (!sender_len || !receiver_len)
        {
            send_data(client_fd, "ERR 99\r\n");
            return;
        }

        while (current_pos < len && isspace(service[current_pos]))
        {
            current_pos++;
        }

        pthread_mutex_lock(&curr_login_table_mutex);
        hash_node_t *sender_node = (hash_node_t *)hash_table_search(curr_login_table, sender);
        if (!sender_node)
        {
            pthread_mutex_unlock(&curr_login_table_mutex);
            send_data(client_fd, "ERR 104\r\n");
            return;
        }

        hash_node_t *receiver_node = (hash_node_t *)hash_table_search(curr_login_table, receiver);
        if (!receiver_node)
        {
            pthread_mutex_unlock(&curr_login_table_mutex);
            send_data(client_fd, "ERR 105\r\n");
            return;
        }

        size_t message_len = len - current_pos;
        if (message_len == 0 || message_len > MAX_MESSAGE_LEN - 50)
        {
            pthread_mutex_unlock(&curr_login_table_mutex);
            send_data(client_fd, "ERR 99\r\n");
            return;
        }

        char msg_buffer[MAX_MESSAGE_LEN];
        int written = snprintf(msg_buffer, sizeof(msg_buffer), "MESG %zu %s", message_len, service + current_pos);
        if (written < 0 || written >= (int)sizeof(msg_buffer))
        {
            pthread_mutex_unlock(&curr_login_table_mutex);
            send_data(client_fd, "ERR 99\r\n");
            return;
        }

        send_data(receiver_node->user_fd, msg_buffer);

        send_data(client_fd, "OK_SENT\r\n");
        pthread_mutex_unlock(&curr_login_table_mutex);
    }
    else
    {
        send_data(client_fd, "ERR 99\r\n");
    }
}

void *thread_function(void *arg)
{
    int index = (int)arg;

    while (true)
    {
        pthread_mutex_lock(&thread_data_arr[index].mutex);
        while (is_empty(thread_data_arr[index].queue))
        {
            pthread_cond_wait(&thread_data_arr[index].cond, &thread_data_arr[index].mutex);
        }

        node_t *service_node = peek(thread_data_arr[index].queue);
        char *service = service_node->service;
        int client_fd = service_node->client_fd;
        dequeue(thread_data_arr[index].queue);

        handle_service(client_fd, service);
        pthread_mutex_unlock(&thread_data_arr[index].mutex);
        free(service);
    }

    destroy_queue(thread_data_arr[index].queue);
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

void cleanup_client_connection(int epoll_fd, int client_fd)
{
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
    close(client_fd);

    pthread_mutex_lock(&curr_login_table_mutex);
    hash_table_delete_via_id(curr_login_table, client_fd);
    pthread_mutex_unlock(&curr_login_table_mutex);
}

int process_client_data(int client_fd, char *buffer, ssize_t bytes_read, size_t thread_index)
{
    buffer[bytes_read] = '\0';
    char *current_pos = buffer;
    char *next_delim;

    pthread_mutex_lock(&thread_data_arr[thread_index].mutex);

    while ((next_delim = strstr(current_pos, "\r\n")) != NULL)
    {
        size_t message_len = next_delim - current_pos;
        char *message = malloc(message_len + 1);

        if (!message)
        {
            pthread_mutex_unlock(&thread_data_arr[thread_index].mutex);
            return -1;
        }

        memcpy(message, current_pos, message_len);
        message[message_len] = '\0';

        if (enqueue(client_fd, message, thread_data_arr[thread_index].queue) == -1)
        {
            free(message);
            pthread_mutex_unlock(&thread_data_arr[thread_index].mutex);
            return -1;
        }

        current_pos = next_delim + 2; // Skip \r\n
    }

    if (*current_pos != '\0')
    {
        size_t remaining_len = strlen(current_pos);
        char *message = malloc(remaining_len + 1);

        if (!message)
        {
            pthread_mutex_unlock(&thread_data_arr[thread_index].mutex);
            return -1;
        }

        strcpy(message, current_pos);

        if (enqueue(client_fd, message, thread_data_arr[thread_index].queue) == -1)
        {
            free(message);
            pthread_mutex_unlock(&thread_data_arr[thread_index].mutex);
            return -1;
        }
    }

    pthread_cond_signal(&thread_data_arr[thread_index].cond);
    pthread_mutex_unlock(&thread_data_arr[thread_index].mutex);

    return 0;
}

int handle_client_data(int epoll_fd, struct epoll_event *event)
{
    char read_buffer[MAX_DATA_LEN];
    int client_fd = event->data.fd;
    size_t thread_index = client_fd & (THREAD_POOL_SIZE - 1); // lmao the load balancing im doings, aws who?

    while (true)
    {
        ssize_t bytes_read = recv(client_fd, read_buffer, sizeof(read_buffer) - 1, 0);

        if (bytes_read == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break;
            }
            perror("recv");
            cleanup_client_connection(epoll_fd, client_fd);
            return 0; // so that the program doesn't exit
        }

        if (bytes_read == 0)
        {
            cleanup_client_connection(epoll_fd, client_fd);
            return 0;
        }

        if (process_client_data(client_fd, read_buffer, bytes_read, thread_index) != 0)
        {
            cleanup_client_connection(epoll_fd, client_fd);
            return -1;
        }
    }

    return 0;
}

int handle_new_connection(int epoll_fd, int server_fd)
{
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    while (true)
    {
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break;
            }
            perror("accept");
            return -1;
        }

        make_socket_nonblocking(client_fd);

        struct epoll_event event = {
            .events = EPOLLIN | EPOLLET,
            .data.fd = client_fd,
        };

        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1)
        {
            perror("epoll_ctl");
            close(client_fd);
            return -1;
        }
    }

    return 0;
}

int server_event_loop(int epoll_fd, int server_fd)
{
    struct epoll_event events[MAX_EVENTS];

    while (true)
    {
        int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (num_events == -1)
        {
            if (errno == EINTR)
                continue;
            perror("epoll_wait");
            return EXIT_FAILURE;
        }

        for (int i = 0; i < num_events; i++)
        {
            if (events[i].data.fd == server_fd)
            {
                if (handle_new_connection(epoll_fd, server_fd) != 0)
                {
                    return EXIT_FAILURE;
                }
            }
            else
            {
                if (handle_client_data(epoll_fd, &events[i]) != 0)
                {
                    return EXIT_FAILURE;
                }
            }
        }
    }

    return EXIT_SUCCESS;
}

void cleanup_thread_data(size_t count)
{
    for (size_t i = 0; i < count; i++)
    {
        pthread_mutex_destroy(&thread_data_arr[i].mutex);
        pthread_cond_destroy(&thread_data_arr[i].cond);
        if (thread_data_arr[i].queue)
        {
            destroy_queue(thread_data_arr[i].queue);
        }
    }
}

int main()
{
    struct addrinfo hints, *server = NULL;
    int server_fd = -1, epoll_fd = -1;
    int ret = EXIT_FAILURE;

    pthread_attr_t attr;
    if (pthread_attr_init(&attr) != 0)
    {
        perror("pthread_attr_init");
        goto cleanup;
    }

    if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0)
    {
        perror("pthread_attr_setdetachstate");
        pthread_attr_destroy(&attr);
        goto cleanup;
    }

    for (size_t i = 0; i < THREAD_POOL_SIZE; i++)
    {
        if (pthread_mutex_init(&thread_data_arr[i].mutex, NULL) != 0)
        {
            perror("pthread_mutex_init");
            cleanup_thread_data(i);
            pthread_attr_destroy(&attr);
            goto cleanup;
        }
        if (pthread_cond_init(&thread_data_arr[i].cond, NULL) != 0)
        {
            perror("pthread_cond_init");
            cleanup_thread_data(i);
            pthread_attr_destroy(&attr);
            goto cleanup;
        }
        thread_data_arr[i].queue = create_queue();
        if (!thread_data_arr[i].queue)
        {
            perror("create_queue");
            cleanup_thread_data(i);
            pthread_attr_destroy(&attr);
            goto cleanup;
        }
    }

    for (size_t i = 0; i < THREAD_POOL_SIZE; i++)
    {
        if (pthread_create(&thread_pool[i], &attr, thread_function, (void *)i) != 0)
        {
            perror("pthread_create");
            cleanup_thread_data(THREAD_POOL_SIZE);
            pthread_attr_destroy(&attr);
            goto cleanup;
        }
    }

    pthread_attr_destroy(&attr);

    if (!(curr_login_table = create_hash_table()) ||
        !(registered_table = create_hash_table()))
    {
        perror("create_hash_table");
        goto cleanup;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, CHAT_PORT, &hints, &server) != 0)
    {
        perror("getaddrinfo");
        goto cleanup;
    }

    server_fd = socket(server->ai_family, server->ai_socktype, server->ai_protocol);
    if (server_fd == -1)
    {
        perror("socket");
        goto cleanup;
    }

    make_socket_nonblocking(server_fd);

    int yes = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
    {
        perror("setsockopt");
        goto cleanup;
    }

    if (bind(server_fd, server->ai_addr, server->ai_addrlen) == -1)
    {
        perror("bind");
        goto cleanup;
    }

    freeaddrinfo(server);
    server = NULL;

    if (listen(server_fd, LISTEN_BUF_LEN) == -1)
    {
        perror("listen");
        goto cleanup;
    }

    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1)
    {
        perror("epoll_create1");
        goto cleanup;
    }

    struct epoll_event event = {
        .events = EPOLLIN,
        .data.fd = server_fd};

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1)
    {
        perror("epoll_ctl");
        goto cleanup;
    }

    ret = server_event_loop(epoll_fd, server_fd);

cleanup:
    if (server)
        freeaddrinfo(server);
    if (epoll_fd != -1)
        close(epoll_fd);
    if (server_fd != -1)
        close(server_fd);
    if (curr_login_table)
        destroy_hash_table(curr_login_table);
    if (registered_table)
        destroy_hash_table(registered_table);

    return ret;
}

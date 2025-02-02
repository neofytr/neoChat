#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <fcntl.h>
#include <stdint.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>

#include "network.h"
#include "queue.h"
#include "hash_table.h"
#include "mutex_table.h"

#define CHAT_PORT "4040"
#define LISTEN_BUF_LEN (100)

#define MAX_DATA_LEN (8192)

#define THREAD_POOL_SIZE (16)
#define MAX_EVENTS (500)

#define MAX_SERVICE_LEN (256)

typedef struct
{
    queue_t *queue;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} thread_data_t;

static hash_table_t *curr_login_table;
static hash_table_t *registered_table;

static thread_data_t thread_data_arr[THREAD_POOL_SIZE];

static pthread_t thread_pool[THREAD_POOL_SIZE];

static pthread_mutex_t curr_login_table_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t registered_table_mutex = PTHREAD_MUTEX_INITIALIZER;

/*

Service format

ERR 99 is for bad request format
ERR 98 is for server error

1. login

LOGIN <username> <password>

check if there a user with username in the registered users table; otherwise deny the request in this
case with ERR 100

check if there is no user with username already in the curr_login_table; otherwise deny the request in this
case with ERR 101

check if the password is correct acc to the entry in the registered users table; otherwise deny the
request in this case with ERR 102

add an entry in the curr_login_table with username password user_fd; return OK_LOGGEDIN

2. signup

SIGNUP <username> <password>

check if there is no user with username in the registered users table; otherwise deny the request
with ERR 103

add an entry in the registered_users_table with username password; return OK_SIGNEDUP

3. logout

LOGOUT <username>

check if there is a user with username in curr login table; otherwise deny the request with
ERR 104

remove the entry username from curr login table; return OK_LOGGEDOUT

3. send

SEND <sender_username> <rcvr_username> msg

check if the sender is in curr login table; otherwise return ERR 104\r\n
check if the receiver is in curr login table; otherwise return ERR 105\r\n
send the msg to receiver (format: MESG <mesg_len> <msg>); return OK_SENT to the sender\r\n

*/
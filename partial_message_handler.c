#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

#include "partial_message_handler.h"

struct PartialMessage {
    struct Header *h;
    int sockfd;
    char data[INIT_BUFFER_LENGTH];
    int bytes_read;
    long last_modified;
    struct PartialMessage *next;
};

struct PartialMessageHandler {
    struct PartialMessage *head;
};

static struct PartialMessage * find_partial(struct PartialMessageHandler *m, 
                                            int sockfd);
static struct PartialMessage * new_partial(struct PartialMessageHandler *m, int sockfd);

struct PartialMessageHandler * init_partials()
{
    struct PartialMessageHandler *m = malloc(sizeof(*m));
    m->head = NULL;

    return m;
}

int getPartialHeader(struct PartialMessageHandler *p, int sockfd, 
                     char *headerBuf)
{
    struct PartialMessage *node = p->head;
    while (node !=NULL) {
        if (node->sockfd == sockfd) { 
            if (node->h == NULL) {
                memcpy(headerBuf, node->data, node->bytes_read);
                return node->bytes_read;
            }
            else {
                memcpy(headerBuf, node->h, HEADER_LENGTH);
                return HEADER_LENGTH;
            }
        }
        node = node->next;
    }
    return 0;
}

int add_partial(struct PartialMessageHandler *p, char *buffer, int sockfd,
                int length, char is_file_input)
{

    assert(p != NULL);
    assert(length <=INIT_BUFFER_LENGTH);
    int n;
    struct PartialMessage *temp;

    if ((temp = find_partial(p, sockfd)) == NULL) {
        assert(length <=HEADER_LENGTH);
        assert(is_file_input == 0);
        temp = new_partial(p, sockfd); /* allocate uninitialized PartialMessage */
        if (length == HEADER_LENGTH) {
            temp->h = malloc(sizeof(*(temp->h)));
            memcpy(temp->h, buffer, HEADER_LENGTH);
        } else {
            memcpy(temp->data, buffer, length); 
            temp->bytes_read = length;

        }
        return 0;

    } else if (temp->h == NULL) {
        temp->last_modified = time(NULL);
        assert(temp->bytes_read + length <= HEADER_LENGTH);
        assert(is_file_input == 0);
        memcpy(&(temp->data[temp->bytes_read]), buffer, length);

        temp->bytes_read += length;

            //if entire header read in
        if (temp->bytes_read == HEADER_LENGTH){
            temp->h = malloc(sizeof(struct Header));
            memcpy(temp->h, temp->data, HEADER_LENGTH);
            bzero(temp->data, HEADER_LENGTH);
            temp->bytes_read = 0;

            return 0;
        }
    }
            //CONTRACT: they're never gonna pass you the complete header if length == 0
    else{
        temp->last_modified = time(NULL);
        temp->bytes_read += length;
        if (is_file_input == 0){
            memcpy(&(temp->data[temp->bytes_read - length]), buffer, length);
            if (temp->bytes_read == temp->h->length){
                bzero(buffer, length);
                memcpy(buffer, temp->data, temp->bytes_read);
                //DELeTE_PARTIAL TODO
                return 1;
            }
            else
                return 0;
        }
        else
            return save_buffer(temp->h->filename, buffer, length, temp->h->length);
    }
}


static struct PartialMessage * new_partial(struct PartialMessageHandler *p, int sockfd)
{
    struct PartialMessage *temp, *last;
    
    if (p->head == NULL) {
        p->head = malloc(sizeof(*(p->head)));
        temp = p->head;
    } else {
        temp = p->head;
        while (temp != NULL) {
            last = temp;
            temp = temp->next;
        }
        last->next = malloc(sizeof(*(last->next)));
        temp = last->next;
    }
    temp->last_modified = time(NULL);
    temp->h = NULL;
    temp->bytes_read = 0;
    temp->next = NULL;
    temp->sockfd = sockfd;


    return temp;
}

void delete_partial(struct PartialMessageHandler *p, int sockfd){

    if (p == NULL){
        return;
    }
    struct PartialMessage *temp = p->head;
    while (temp != NULL){
        if (sockfd == temp->sockfd){
            if (temp->h != NULL){
                delete_temp_file(temp->h->filename);
                free(temp->h);
            }
            free(temp);
            return;
        }
        temp = temp->next;
    }

}

void timeout_sweep(struct PartialMessageHandler *p, fd_set *masterFDSet){
    if (p == NULL){
        return;
    }

    long current_time = time(NULL);
    struct PartialMessage *temp = p->head;
    while (temp != NULL){
        if (temp->last_modified - current_time >= MINUTE){
            if (temp->h != NULL){
                delete_temp_file(temp->h->filename);
                free(temp->h);
            }
            close(temp->sockfd);
            FD_CLR(temp->sockfd, masterFDSet);
            free(temp);
        }
        temp = temp->next;
    }

}


void free_partials(struct PartialMessageHandler *p)
{
    if (p == NULL) {
        return;
    } 

    struct PartialMessage *temp = p->head;
    struct PartialMessage *next = NULL;

    while (temp != NULL) {
        next = temp->next;
        if (temp->h != NULL) {
            delete_temp_file(temp->h->filename);
            free(temp->h);
        }
        free(temp);
        temp = next;
    }

    free(p);
}

static struct PartialMessage * find_partial(struct PartialMessageHandler *m, 
                                            int sockfd)
{
    assert(m != NULL);

    struct PartialMessage *temp;

    if (m->head == NULL) {
        return NULL;
    } else {
        temp = m->head;
        while (temp != NULL && temp->sockfd != sockfd) {
            temp = temp->next;
        }

        return temp;
    }
}


int get_bytes_read(struct PartialMessageHandler *p, int sockfd){
    struct PartialMessage *node = p->head;
    while(node != NULL){
        if (node->sockfd == sockfd){
            return node->bytes_read;
        }
    }
    return 0;
}
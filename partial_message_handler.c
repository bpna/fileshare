#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include "messages.h"
#include "partial_message_handler.h"

struct PartialMessage {
    struct Header *h;
    int sockfd;
    char data[INIT_BUFFER_LENGTH];
    int buflen;
    int bytes_read;
    struct PartialMessage *next;
};

struct PartialMessageHandler {
    struct PartialMessage *head;
};

static struct PartialMessage * find_partial(struct PartialMessageHandler *m, 
                                            int sockfd);
static struct PartialMessage * new_partial(struct PartialMessageHandler *m);

struct PartialMessageHandler * init_partials()
{
    struct PartialMessageHandler *m = malloc(sizeof(*m));
    m->head = NULL;

    return m;
}

int getPartialHeader(struct PartialMessageHandler *p, int sockfd, 
                     struct Header *headerBuf)
{
    struct PartialMessage *node = p->head;
    while (node !=NULL) {
        if (node->sockfd == sockfd) { 
            if (node->h == NULL) {
                memcpy(headerBuf, node->data, node->buflen);
                return node->buflen;
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
                int length)
{
    assert(p != NULL);
    assert(length <=10000);
    int n;
    struct PartialMessage *temp;

    if ((temp = find_partial(p, sockfd)) == NULL) {
        assert(length <=HEADER_LENGTH);
        temp = new_partial(p); /* allocate uninitialized PartialMessage */
        if (length == HEADER_LENGTH) {
            temp->h = malloc(sizeof(*(temp->h)));
            memcpy(temp->h, buffer, HEADER_LENGTH);
        } else {
            temp->h = NULL;
            memcpy(temp->data, buffer, length); 
            temp->buflen = length;

        }
        temp->bytes_read = 0;
        temp->sockfd = sockfd;
        temp->next = NULL;

        return 0;
    } else {
        if (temp->h == NULL) {
            assert(temp->buflen + length <= HEADER_LENGTH);
            memcpy(&(temp->data[temp->buflen]), buffer, length);

            temp->buflen += length;

            //if entire header read in
            if (temp->buflen == HEADER_LENGTH){
                temp->h = malloc(sizeof(struct Header));
                memcpy(temp->h, temp->data, HEADER_LENGTH);
                bzero(temp->data, HEADER_LENGTH);
                temp->buflen = 0;

                return 0;
            }
            //CONTRACT: they're never gonna pass you the complete header if length == 0
            else{
                memcpy(&(temp->data[n]), buffer, length);
                temp->bytes_read += length;
                temp->buflen += length;
                //if last thing to be read
                if (temp->bytes_read == temp->h->length) {
                    bzero(buffer, 20000);
                    memcpy(buffer, temp->data, temp->buflen);
                    return temp->buflen;
                }
                else if (temp->buflen >= 10000){
                    bzero(buffer, 20000);
                    memcpy(buffer, temp->data, 10000);
                    temp->buflen -= 10000;
                    memcpy(temp->data, &(temp->data[10000]), temp->buflen);
                    bzero(&(temp->data[temp->buflen]), 20000 - temp->buflen);
                    return 10000;
                }
                else{
                    return 0;
                }
            }
        }
    }
}


static struct PartialMessage * new_partial(struct PartialMessageHandler *p)
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

    return temp;
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

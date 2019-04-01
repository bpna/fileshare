#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include "messages.h"
#include "partial_message_handler.h"

struct PartialMessageHandler {
    struct PartialMessage *head;
};

struct PartialMessage {
    struct Header *h;
    int sockfd;
    char data[INIT_BUFFER_LENGTH];
    int buflen;
    int bytes_read;
    struct PartialMessage *next;
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

int add_partial(struct PartialMessageHandler *p, char *buffer, int sockfd,
                int length)
{
    assert(p != NULL);
    int n;
    struct PartialMessage *temp;

    if ((temp = find_partial(p, sockfd)) == NULL) {
        temp = new_partial(p); /* allocate uninitialized PartialMessage */
        if (length >= HEADER_LENGTH) {
            temp->h = malloc(sizeof(*(temp->h)));
            memcpy(temp->h, buffer, HEADER_LENGTH);
            if (temp->h->length == 0) { /* header read-in, no payload */
                free(temp->h);
                free(temp);
                return -1;
            } else if (temp->h->length == length - HEADER_LENGTH) {
                /* read-in all data, free list element and return payload */
                memcpy(buffer, &buffer[HEADER_LENGTH], length - HEADER_LENGTH);
                free(temp->h);
                free(temp);
                return length - HEADER_LENGTH;
            }

            memcpy(temp->data, &buffer[HEADER_LENGTH], length - HEADER_LENGTH);
            temp->buflen = length - HEADER_LENGTH;
            temp->bytes_read = length - HEADER_LENGTH;
        } else {
            temp->h = NULL;
            memcpy(temp->data, buffer, length); 
            temp->buflen = length;
            temp->bytes_read = 0;
        }
        
        temp->sockfd = sockfd;
        temp->next = NULL;

        return 0;
    } else {
        if (temp->h == NULL) {
            if (temp->buflen + length < HEADER_LENGTH) {
                memcpy(&(temp->data[temp->buflen]), buffer, length);
                temp->buflen += length;
                return 0;
            } else {
                n = temp->buflen;
                memcpy(&(temp->data[n]), buffer, HEADER_LENGTH - n);
                temp->h = malloc(sizeof(*(temp->h)));
                memcpy(temp->h, temp->data, HEADER_LENGTH);
                temp->buflen = 0;
                if (temp->h->length == 0) {
                    // free this shit, return -1?
                }
                if (length > HEADER_LENGTH - n) {
                    memcpy(temp->data, &buffer[HEADER_LENGTH - n], \
                           length - (HEADER_LENGTH - n));
                    temp->buflen = length - (HEADER_LENGTH - n);
                    temp->bytes_read += temp->buflen;
                }
            }
        } else {
            ; // Stuff happens
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

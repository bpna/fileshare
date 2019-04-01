
getPartialHeader(struct Node *head, int sockfd, struct header *headerBuf)
    struct Node *node = head;
    while (node !=NULL){
        if node->sockfd == sockfd
            if (node->header == NULL){
                memcpy(headerBuf, node->data, buflen);
                return buflen
            }
            else{
                memcpy(headerBuf, node->header, HEADER_LENGTH);
                return HEADER_LENGTH;
            }
    }


struct PartialNode{
    char headerBuf[70];
    char bodyBuf[10000];
    int sockfd;
    int bytesRead;
    int buflen
    long lastUpdated;

}





int add_partial(struct PartialMessageHandler *p, char *buffer, int sockfd,
                int length)
{
    assert(p != NULL);
    assert(length <=10000)
    int n;
    struct PartialMessage *temp;

    if ((temp = find_partial(p, sockfd)) == NULL) {
        assert(length <=HEADER_LENGTH)
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
            assert(temp->buflen + length <= HEADER_LENGTH)
            memcpy(&(temp->data[temp->buflen]), buffer, length);

            temp->buflen += length;

            //if entire header read in
            if (temp->buflen == HEADER_LENGTH){
                temp->h = malloc(sizeof(struct header));
                memcpy(temp->h, temp->data, HEADER_LENGTH);
                bzero(temp-data, HEADER_LENGTH);
                temp->buflen = 0
                
            return 0;
        }
            //CONTRACT: they're never gonna pass you the complete header if length == 0
        else{
            memcpy(&(temp->data[n]), buffer, length);
            temp->bytesRead += length;
            temp->buflen += length
            //if last thing to be read
            if (temp->bytesRead == temp->h->length){
                bzero(buffer, 20000);
                memcpy(buffer, temp->data, temp->buflen);
                return temp->buflen;
            }
            else if (temp->buflen >= 10000){
                bzero(buffer, 20000);
                memcpy(buffer, temp->data, 10000);
                temp->buflen -= 10000;
                memcpy(temp->data, &(temp->data[10000]), temp->buflen);
                bzero(&(temp-data[buflen]), 20000 - temp->buflen);
                return 10000;
            }
            else{
                return 0;
            }
        }
        //PROBLEM: they don't know when to close the TCP connection
            
        }
    }
}


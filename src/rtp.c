#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "queue.h"
#include "network.h"
#include "rtp.h"

typedef struct message {
    char *buffer;
    int length;
} message_t;

/* ================================================================ */
/*                  H E L P E R    F U N C T I O N S                */
/* ================================================================ */

/*
 *  Returns a number computed based on the data in the buffer.
 */
int checksum(char *buffer, int length) {


    /*  ----  FIXME  ----
     *
     *  The goal is to return a number that is determined by the contents
     *  of the buffer passed in as a parameter.  There a multitude of ways
     *  to implement this function.  For simplicity, the checksum value is
     *  the sum of each char value times its index (starting at 1). 
     *  (i.e. checksum = first char * 1 + second char * 2 
     *                   + third char * 3 ... + nth char * n )
     */
    int checkSum = 0; //checkSum var
    for (int indexChar = 0; indexChar < length; indexChar++) { //iterate over buffer
        checkSum = checkSum + ((indexChar+1) * buffer[indexChar]); //calculate checkSum
    }
    return checkSum;
}

/*
 *  Converts the given buffer into an array of PACKETs and returns
 *  the array.  The value of (*count) should be updated so that it 
 *  contains the length of the array created.
 */
packet_t *packetize(char *buffer, int length, int *count) {

    /*  ----  FIXME  ----
     *
     *  The goal is to turn the buffer into an array of packets.
     *  You should allocate the space for an array of packets and
     *  return a pointer to the first element in that array.  The
     *  integer pointed to by 'count' should be updated to indicate
     *  the number of packets in the array.
     */
    packet_t *packets;

    int packetArrSz = (length + MAX_PAYLOAD_LENGTH -1) / MAX_PAYLOAD_LENGTH; //packet Array size
    packets = calloc(packetArrSz, sizeof(packet_t)); //allocated array of packets

    for (int arrayInd = 0; arrayInd < length; arrayInd++) { //iterate over buffer
        int offset = arrayInd/MAX_PAYLOAD_LENGTH;
        int payInd = (arrayInd % MAX_PAYLOAD_LENGTH);
        packet_t* currInsert = packets + offset; //current packet
        currInsert->payload[payInd] = buffer[arrayInd];

        if (payInd == (MAX_PAYLOAD_LENGTH-1)) {
            currInsert->payload_length = MAX_PAYLOAD_LENGTH;
            currInsert->type = DATA; //set type for other packets
            currInsert->checksum = checksum(currInsert->payload, currInsert->payload_length);
        } else if (arrayInd == (length - 1)) { //last packet
            currInsert->payload_length = payInd+1; //payload length is based on left over
            currInsert->type = LAST_DATA; //set type for last packet
            currInsert->checksum = checksum(currInsert->payload, currInsert->payload_length);
        }
    }
    *count = packetArrSz; //set count equal to length of array
    return packets;
}

/* ================================================================ */
/*                      R T P       T H R E A D S                   */
/* ================================================================ */

static void *rtp_recv_thread(void *void_ptr) {

    rtp_connection_t *connection = (rtp_connection_t *) void_ptr;

    do {
        message_t *message;
        int buffer_length = 0;
        char *buffer = NULL;
        packet_t packet;

        /* Put messages in buffer until the last packet is received  */
        do {
            if (net_recv_packet(connection->net_connection_handle, &packet) <= 0 || packet.type == TERM) {
                /* remote side has disconnected */
                connection->alive = 0;
                pthread_cond_signal(&connection->recv_cond);
                pthread_cond_signal(&connection->send_cond);
                break;
            }

            /*  ----  FIXME  ----
            *
            * 1. check to make sure payload of packet is correct
            * 2. send an ACK or a NACK, whichever is appropriate
            * 3. if this is the last packet in a sequence of packets
            *    and the payload was corrupted, make sure the loop
            *    does not terminate
            * 4. if the payload matches, add the payload to the buffer
            */
            if (packet.type == LAST_DATA || packet.type == DATA) { //check packetType
                packet_t *ackBack = malloc(sizeof(packet_t)); //allocated space for the packet back

                int sumCheckPac = checksum(packet.payload, packet.payload_length);
                if (sumCheckPac == packet.checksum) { //packet isnt good
                    ackBack->type = ACK; //send ACK
                    char *bufferAlloc = realloc(buffer, (buffer_length + packet.payload_length) * sizeof(char));
                    buffer = bufferAlloc;

                    for (int payloadInd = 0; payloadInd < packet.payload_length; payloadInd++) { //iterate through the packet payload
                        buffer[buffer_length+payloadInd] = packet.payload[payloadInd];
                    }
                    buffer_length += packet.payload_length; //add so buffer knows where to start from
                } else { //packet is good
                    if (packet.type == LAST_DATA) {
                        packet.type = DATA;
                    }
                    ackBack->type = NACK; //send NACK
                }
                net_send_packet(connection->net_connection_handle, ackBack);
            }
            
            /*
            * 
            *  What if the packet received is not a data packet?
            *  If it is a NACK or an ACK, the sending thread should
            *  be notified so that it can finish sending the message.
            *   
            *  1. add the necessary fields to the CONNECTION data structure
            *     in rtp.h so that the sending thread has a way to determine
            *     whether a NACK or an ACK was received
            *  2. signal the sending thread that an ACK or a NACK has been
            *     received.
            */
            if (packet.type == NACK) {  //NACK
                pthread_mutex_lock(&connection->ack_mutex);
                connection->ack_sent = 1; //set acksent
                connection->ack = 0; //clear ack
                pthread_cond_signal(&connection->ack_cond);
                pthread_mutex_unlock(&connection->ack_mutex);
            } else if (packet.type == ACK) { //ACK
                pthread_mutex_lock(&connection->ack_mutex);
                connection->ack_sent = 1; //set acksent
                connection->ack = 1; //set ack
                pthread_cond_signal(&connection->ack_cond);
                pthread_mutex_unlock(&connection->ack_mutex);
            }
        } while (packet.type != LAST_DATA);

        if (connection->alive == 1) {
            /*  ----  FIXME  ----
            *
            * Now that an entire message has been received, we need to
            * add it to the queue to provide to the rtp client.
            *
            * 1. Add message to the received queue.
            * 2. Signal the client thread that a message has been received.
            */

            message = malloc(sizeof(message_t)); //allocated space of message
            message->length = buffer_length; //set len message
            message->buffer = calloc(buffer_length, sizeof(char));

            for (int bufferInd = 0; bufferInd < buffer_length; bufferInd++) { //deep copy buffer
                message->buffer[bufferInd] = buffer[bufferInd];
            }

            pthread_mutex_lock(&(connection->recv_mutex));
            queue_add(&(connection->recv_queue), message); //add message to receive queue
            pthread_cond_signal(&(connection->recv_cond)); //signal message received
            pthread_mutex_unlock(&(connection->recv_mutex));
        } else free(buffer);

    } while (connection->alive == 1);

    return NULL;

}

static void *rtp_send_thread(void *void_ptr) {

    rtp_connection_t *connection = (rtp_connection_t *) void_ptr;
    message_t *message;
    int array_length = 0;
    int i;
    packet_t *packet_array;

    do {
        /* Extract the next message from the send queue */
        pthread_mutex_lock(&connection->send_mutex);
        while (queue_size(&connection->send_queue) == 0 && connection->alive == 1) {
            pthread_cond_wait(&connection->send_cond, &connection->send_mutex);
        }

        if (connection->alive == 0) break;

        message = queue_extract(&connection->send_queue);

        pthread_mutex_unlock(&connection->send_mutex);

        /* Packetize the message and send it */
        packet_array = packetize(message->buffer, message->length, &array_length);

        for (i = 0; i < array_length; i++) {

            /* Start sending the packetized messages */
            if (net_send_packet(connection->net_connection_handle, &packet_array[i]) <= 0) {
                /* remote side has disconnected */
                connection->alive = 0;
                break;
            }

            /*  ----FIX ME ---- 
             * 
             *  1. wait for the recv thread to notify you of when a NACK or
             *     an ACK has been received
             *  2. check the data structure for this connection to determine
             *     if an ACK or NACK was received.  (You'll have to add the
             *     necessary fields yourself)
             *  3. If it was an ACK, continue sending the packets.
             *  4. If it was a NACK, resend the last packet
             */
            pthread_mutex_lock(&(connection->ack_mutex));

            while(connection->ack_sent == 0) { //wait to notify when a NACK/ACK has been received
                pthread_cond_wait(&(connection->ack_cond), &(connection->ack_mutex));
            }
            if (connection->ack == 0) { //resend the last pack bc NACK
                i = i - 1; //resend the last packet by decrementing the packetInd
            }
            connection->ack_sent = 0;
            pthread_mutex_unlock(&(connection->ack_mutex));
        }

        free(packet_array);
        free(message->buffer);
        free(message);
    } while (connection->alive == 1);
    return NULL;


}

static rtp_connection_t *rtp_init_connection(int net_connection_handle) {
    rtp_connection_t *rtp_connection = malloc(sizeof(rtp_connection_t));

    if (rtp_connection == NULL) {
        fprintf(stderr, "Out of memory!\n");
        exit(EXIT_FAILURE);
    }

    rtp_connection->net_connection_handle = net_connection_handle;

    queue_init(&rtp_connection->recv_queue);
    queue_init(&rtp_connection->send_queue);

    pthread_mutex_init(&rtp_connection->ack_mutex, NULL);
    pthread_mutex_init(&rtp_connection->recv_mutex, NULL);
    pthread_mutex_init(&rtp_connection->send_mutex, NULL);
    pthread_cond_init(&rtp_connection->ack_cond, NULL);
    pthread_cond_init(&rtp_connection->recv_cond, NULL);
    pthread_cond_init(&rtp_connection->send_cond, NULL);

    rtp_connection->alive = 1;

    pthread_create(&rtp_connection->recv_thread, NULL, rtp_recv_thread,
                   (void *) rtp_connection);
    pthread_create(&rtp_connection->send_thread, NULL, rtp_send_thread,
                   (void *) rtp_connection);

    return rtp_connection;
}

/* ================================================================ */
/*                           R T P    A P I                         */
/* ================================================================ */

rtp_connection_t *rtp_connect(char *host, int port) {

    int net_connection_handle;

    if ((net_connection_handle = net_connect(host, port)) < 1)
        return NULL;

    return (rtp_init_connection(net_connection_handle));
}

int rtp_disconnect(rtp_connection_t *connection) {

    message_t *message;
    packet_t term;

    term.type = TERM;
    term.payload_length = term.checksum = 0;
    net_send_packet(connection->net_connection_handle, &term);
    connection->alive = 0;

    net_disconnect(connection->net_connection_handle);
    pthread_cond_signal(&connection->send_cond);
    pthread_cond_signal(&connection->recv_cond);
    pthread_join(connection->send_thread, NULL);
    pthread_join(connection->recv_thread, NULL);
    net_release(connection->net_connection_handle);

    /* emtpy recv queue and free allocated memory */
    while ((message = queue_extract(&connection->recv_queue)) != NULL) {
        free(message->buffer);
        free(message);
    }
    queue_release(&connection->recv_queue);

    /* emtpy send queue and free allocated memory */
    while ((message = queue_extract(&connection->send_queue)) != NULL) {
        free(message);
    }
    queue_release(&connection->send_queue);

    free(connection);

    return 1;

}

int rtp_recv_message(rtp_connection_t *connection, char **buffer, int *length) {

    message_t *message;

    if (connection->alive == 0)
        return -1;
    /* lock */
    pthread_mutex_lock(&connection->recv_mutex);
    while (queue_size(&connection->recv_queue) == 0 && connection->alive == 1) {
        pthread_cond_wait(&connection->recv_cond, &connection->recv_mutex);
    }

    if (connection->alive == 0) {
        pthread_mutex_unlock(&connection->recv_mutex);
        return -1;
    }

    /* extract */
    message = queue_extract(&connection->recv_queue);
    *buffer = message->buffer;
    *length = message->length;
    free(message);

    /* unlock */
    pthread_mutex_unlock(&connection->recv_mutex);

    return *length;
}

int rtp_send_message(rtp_connection_t *connection, char *buffer, int length) {

    message_t *message;

    if (connection->alive == 0)
        return -1;

    message = malloc(sizeof(message_t));
    if (message == NULL) {
        return -1;
    }
    message->buffer = malloc((size_t) length);
    message->length = length;

    if (message->buffer == NULL) {
        free(message);
        return -1;
    }

    memcpy(message->buffer, buffer, (size_t) length);

    /* lock */
    pthread_mutex_lock(&connection->send_mutex);

    /* add */
    queue_add(&(connection->send_queue), message);

    /* unlock */
    pthread_mutex_unlock(&connection->send_mutex);
    pthread_cond_signal(&connection->send_cond);
    return 1;

}

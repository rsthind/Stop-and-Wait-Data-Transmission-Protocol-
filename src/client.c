#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "network.h"
#include "rtp.h"

static void printUsage()
{
    fprintf(stderr, "Usage:  rtp-client host port\n\n");
    fprintf(stderr, "   example ./rtp-client localhost 4000\n\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{

    char message_1[] = "Nby omy iz WIVIF wlcjjfym nby gchx; cnm"
                       "nyuwbcha mbiofx, nbylyzily, vy lyaulxyx um u wlcgchuf"
                       "izzyhmy. -- Yxmaul Xcdemnlu";

    char message_2[] = "W gueym cn yums ni mbiin siolmyfz ch nby "
                       "ziin; W++ gueym cn bulxyl, von qbyh sio xi, cn vfiqm uqus "
                       "siol qbify fya. -- Vdulhy Mnliomnloj";

    char message_3[] = "Ch nby zonoly, wigjonylm gus qycab hi gily nbuh 1.5 nihhym."
                       " -- Jijoful gywbuhcwm, 1949";

    char message_4[] = "Alipy acpynb uhx Aunym nueynb uqus. "
                       " -- Viv Gynwufzy (chpyhnil iz Ynbylhyn) ih nby nlyhx iz "
                       "bulxquly mjyyxojm hin vycha uvfy ni eyyj oj qcnb miznquly xyguhxm";

    char message_5[] = "Wixy ayhyluncih, fcey xlchecha ufwibif, cm aiix ch gixyluncih."
                       " -- Ufyr Fiqy ";

    char *rcv_buffer;
    int length, ret;
    rtp_connection_t *connection;

    if (argc < 3)
    {
        printUsage();
        return EXIT_FAILURE;
    }

    if ((connection = rtp_connect(argv[1], atoi(argv[2]))) == NULL)
    {
        printUsage();
        return EXIT_FAILURE;
    }

    printf("Sending quotes to a remote host to have them "
           "decoded using Ceaser Cypher.\n\n");

    rtp_send_message(connection, message_1, strlen(message_1));
    ret = rtp_recv_message(connection, &rcv_buffer, &length);
    if (rcv_buffer == NULL || ret == -1)
    {
        printf("Connection reset by peer.\n");
        return EXIT_FAILURE;
    }
    printf("%.*s\n\n", length, rcv_buffer);
    free(rcv_buffer);

    rtp_send_message(connection, message_2, strlen(message_2));
    ret = rtp_recv_message(connection, &rcv_buffer, &length);
    if (rcv_buffer == NULL || ret == -1)
    {
        printf("Connection reset by peer.\n");
        return EXIT_FAILURE;
    }
    printf("%.*s\n\n", length, rcv_buffer);
    free(rcv_buffer);

    rtp_send_message(connection, message_3, strlen(message_3));
    ret = rtp_recv_message(connection, &rcv_buffer, &length);
    if (rcv_buffer == NULL || ret == -1)
    {
        printf("Connection reset by peer.\n");
        return EXIT_FAILURE;
    }
    printf("%.*s\n\n", length, rcv_buffer);
    free(rcv_buffer);

    rtp_send_message(connection, message_4, strlen(message_4));
    ret = rtp_recv_message(connection, &rcv_buffer, &length);
    if (rcv_buffer == NULL || ret == -1)
    {
        printf("Connection reset by peer.\n");
        return EXIT_FAILURE;
    }
    printf("%.*s\n\n", length, rcv_buffer);
    free(rcv_buffer);

    rtp_send_message(connection, message_5, strlen(message_5));
    ret = rtp_recv_message(connection, &rcv_buffer, &length);
    if (rcv_buffer == NULL || ret == -1)
    {
        printf("Connection reset by peer.\n");
        return EXIT_FAILURE;
    }
    printf("%.*s\n\n", length, rcv_buffer);
    free(rcv_buffer);
    rtp_disconnect(connection);
    return EXIT_SUCCESS;
}

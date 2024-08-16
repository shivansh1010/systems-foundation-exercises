#include "http_server.hh"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sstream>
#include <string>
#include <iostream>
#include <queue>
#include <netinet/in.h>
#include <pthread.h>

#define buffer_size 4096
#define max_queue_size 65536
#define num_of_threads 100

using namespace std;

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t p = PTHREAD_COND_INITIALIZER;
pthread_cond_t c = PTHREAD_COND_INITIALIZER;
pthread_t thread_id[num_of_threads];
bool threads_die = false;
queue<int> q;


void error(char *msg) {
    perror(msg);
    exit(1);
}

void *start_function(void *arg) {
    int newsockfd;
    while (1) {
        pthread_mutex_lock(&m);
        while (q.empty()) {
            pthread_cond_wait(&c, &m);
            if (threads_die){
                pthread_mutex_unlock(&m);
                pthread_exit(NULL);
            }
        }
        newsockfd = q.front();
        q.pop();
        pthread_cond_signal(&p);
        pthread_mutex_unlock(&m);

        int n;
        char buffer[buffer_size];

        /* read message from client */
        bzero(buffer, buffer_size);
        n = read(newsockfd, buffer, buffer_size - 1);
        if (n < 0)
            error("ERROR reading from socket");
        if (buffer[0] == '\0') continue;

        /* generate the reply */
        HTTP_Response *resp = handle_request(string(buffer));
        string resp_str = resp->get_string();
        delete resp;

        /* send reply to client */
        n = write(newsockfd, resp_str.c_str(), resp_str.size());
        if (n < 0)
            error("ERROR writing to socket");
        close(newsockfd);
    }
}

static void sigint_handler (int signo) {
    printf ("signal SIGINT!\n");
    threads_die = true;
    for (int i=0 ; i<num_of_threads ; i++) {
        pthread_cond_signal(&c);
    }
    for (int i=0 ; i<num_of_threads ; i++) {
        pthread_join(thread_id[i], NULL);
    }
    exit (EXIT_SUCCESS);
}


/*
 * ###############################################################################
 */

int main(int argc, char *argv[]) {

    signal (SIGINT, sigint_handler);
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    char buffer[buffer_size];
    struct sockaddr_in serv_addr, cli_addr;
    if (argc < 2) {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    bzero((char *)&serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");
    listen(sockfd, 256);
    clilen = sizeof(cli_addr);


    /* create threads */
    int thread_arg = 0;
    for (int i = 0 ; i < num_of_threads ; i++) {
        pthread_create(&thread_id[i], NULL, start_function, &thread_arg);
    }


    /* accept a new request, create a newsockfd */
    while (1) {
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0)
            error("ERROR on accept");

        pthread_mutex_lock(&m);
        while (q.size() == max_queue_size)
            pthread_cond_wait(&p, &m);
        q.push(newsockfd);
        pthread_cond_signal(&c);
        pthread_mutex_unlock(&m);
    }
    return 0;
}

/* run using ./server <port> */
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
#include <unordered_map>

#include <netinet/in.h>

#include <pthread.h>

#define buffer_size 4096
#define red   "\x1B[31m"
#define grn   "\x1B[32m"
#define yel   "\x1B[33m"
#define blu   "\x1B[34m"
#define wht   "\x1B[37m"
#define rst "\x1B[0m"

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t l = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t c = PTHREAD_COND_INITIALIZER;
queue<int> q;
unordered_map<int, int> x;

void error(char *msg) {
    perror(msg);
    exit(1);
}

void *start_function(void *arg) {
    int newsockfd;
    int tid = x[pthread_self()];
    string tidstr = " [" + to_string(tid) + "] ";
    while (1) {
        pthread_mutex_lock(&m);
        cout << tidstr << red << "Putting thread to sleep" << rst << "\n";
        pthread_cond_wait(&c, &m);
        pthread_mutex_unlock(&m);

        pthread_mutex_lock(&l);
        newsockfd = q.front();
        q.pop();
        pthread_mutex_unlock(&l);

        cout << tidstr << grn << "Thread woken up, " << newsockfd << " is fd" << rst << endl;

        // return 0;


        int n;
        char buffer[buffer_size];
        // int newsockfd = *((int *) arg);

        /* read message from client */
        bzero(buffer, buffer_size);
        n = read(newsockfd, buffer, buffer_size - 1);
        if (n < 0)
            error("ERROR reading from socket");
        // printf(" [>] Request received\n");

        if (buffer[0] == '\0') continue;
        HTTP_Response *resp = handle_request(string(buffer));
        string resp_str = resp->get_string();

        /* send reply to client */
        cout << tidstr << "Response generated, status_code: " << resp->status_code << "\n";
        // cout << " [!] Response is:\n" << resp_str << endl;
        n = write(newsockfd, resp_str.c_str(), resp_str.size());
        if (n < 0)
            error("ERROR writing to socket");
        close(newsockfd);
    }
}

int main(int argc, char *argv[]) {

    int num_of_threads = 10;
    int thread_arg = 0;
    pthread_t thread_id[num_of_threads];
    for (int i = 0 ; i < num_of_threads ; i++) {
        pthread_create(&thread_id[i], NULL, start_function, &thread_arg);
        x[thread_id[i]] = i;
    }


    printf("MY PID IS: %d\n", getpid());

    int sockfd, newsockfd, portno;
    socklen_t clilen;
    char buffer[buffer_size];
    struct sockaddr_in serv_addr, cli_addr;

    if (argc < 2) {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }

    /* create socket */

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    /* fill in port number to listen on. IP address can be anything (INADDR_ANY)
     */

    bzero((char *)&serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    /* bind socket to this port number on this machine */

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    /* listen for incoming connection requests */

    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    /* accept a new request, create a newsockfd */
    int reqs = 0;
    while (1) {
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        reqs++;
        cout << "----------------------------------- #" << reqs << "\n";
        if (newsockfd < 0)
            error("ERROR on accept");

        pthread_mutex_lock(&l);
        q.push(newsockfd);
        pthread_mutex_unlock(&l);

        pthread_mutex_lock(&m);
        pthread_cond_signal(&c);
        pthread_mutex_unlock(&m);
    }
    return 0;
}

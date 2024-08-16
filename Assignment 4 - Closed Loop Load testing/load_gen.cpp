/* run using: ./load_gen localhost <server port> <number of concurrent users>
   <think time (in s)> <test duration (in s)> */

#include <stdio.h>

#include <stdlib.h>

#include <string.h>

#include <unistd.h>

#include <netdb.h>

#include <pthread.h>
#include <cstdlib>

#include <sys/time.h>
#include <iostream>
using namespace std;

int time_up;
FILE * log_file;

vector<string> urls = {
    "/",
    "/apart",
    "/apart1",
    "/apart1.html",
    "/apart1/flat11",
    "/apart2",
    "/apart2/flat21",
    "/apart2/flat32/gfcgv",
    "/apart3/flat32/index.html",
    "/index",
    "/index.html"
}

// user info struct
struct user_info {
    // user id
    int id;

    // socket info
    int portno;
    char * hostname;
    float think_time;

    // user metrics
    int total_count;
    float total_rtt;
};

// error handling function
void error(char * msg) {
    perror(msg);
    exit(0);
}

// time diff in seconds
float time_diff(struct timeval * t2, struct timeval * t1) {
    return (t2 -> tv_sec - t1 -> tv_sec) + (t2 -> tv_usec - t1 -> tv_usec) / 1e6;
}

// user thread function
void * user_function(void * arg) {
    /* get user info */
    struct user_info * info = (struct user_info * ) arg;
    // cout << "|" << info->id << "|" << info->portno << "|" << info->hostname << "|" << endl;

    int sockfd, n;
    char buffer[4096];
    struct timeval start, end;

    struct sockaddr_in serv_addr;
    struct hostent * server;

    while (1) {
        /* start timer */
        gettimeofday( & start, NULL);

        /* TODO: create socket */
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
            error("ERROR opening socket");

        /* TODO: set server attrs */
        server = gethostbyname(info->hostname);
        if (server == NULL) {
            fprintf(stderr, "ERROR, no such host\n");
            exit(0);
        }
        bzero((char * ) & serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        bcopy((char * ) server -> h_addr, (char * ) & serv_addr.sin_addr.s_addr,
            server -> h_length);
        serv_addr.sin_port = htons(info->portno);

        /* connect to server */
        if (connect(sockfd, (struct sockaddr * ) & serv_addr, sizeof(serv_addr)) < 0)
            error("ERROR connecting");

        /* TODO: send message to server */
        bzero(buffer, 4096);
        string url = "GET " + urls[rand()%11] + " HTTP/1.1";
        strcpy(buffer, url.c_str());
        n = write(sockfd, buffer, strlen(buffer));
        if (n < 0)
            error("ERROR writing to socket");
        bzero(buffer, 4096);

        /* TODO: read reply from server */
        n = read(sockfd, buffer, 4095);
        if (n < 0)
            error("ERROR reading from socket");
        
        // cout << buffer << endl;
        /* TODO: close socket */
        close(sockfd);

        /* end timer */
        gettimeofday( & end, NULL);

        /* if time up, break */
        if (time_up)
            break;

        /* TODO: update user metrics */
        info->total_count++;
        info->total_rtt += time_diff(&end, &start);

        /* sleep for think time */
        usleep(1000000 * info->think_time);
    }

    /* exit thread */
    fprintf(log_file, "User #%d finished\n", info -> id);
    fflush(log_file);
    pthread_exit(NULL);
}

int main(int argc, char * argv[]) {
    int user_count, portno, test_duration;
    float think_time;
    char * hostname;

    if (argc != 6) {
        fprintf(stderr,
            "Usage: %s <hostname> <server port> <number of concurrent users> "
            "<think time (in s)> <test duration (in s)>\n",
            argv[0]);
        exit(0);
    }

    hostname = argv[1];
    portno = atoi(argv[2]);
    user_count = atoi(argv[3]);
    think_time = atof(argv[4]);
    test_duration = atoi(argv[5]);

    // printf("Hostname: %s\n", hostname);
    // printf("Port: %d\n", portno);
    // printf("User Count: %d\n", user_count);
    // printf("Think Time: %f s\n", think_time);
    // printf("Test Duration: %d s\n", test_duration);

    /* open log file */
    log_file = fopen("load_gen.log", "w");

    pthread_t threads[user_count];
    struct user_info info[user_count];
    struct timeval start, end;

    /* start timer */
    gettimeofday( & start, NULL);
    time_up = 0;
    for (int i = 0; i < user_count; ++i) {
        /* TODO: initialize user info */
        info[i] = {i, portno, hostname, think_time, 0, 0};

        /* TODO: create user thread */
        pthread_create(&threads[i], NULL, user_function, &info[i]);

        fprintf(log_file, "Created thread %d\n", i);
    }

    /* TODO: wait for test duration */
    sleep(test_duration);

    fprintf(log_file, "Woke up\n");

    /* end timer */
    time_up = 1;
    gettimeofday( & end, NULL);

    /* TODO: wait for all threads to finish */
    for (int i=0 ; i<user_count ; i++) {
        pthread_join(threads[i], NULL);
    }

    /* TODO: print results */
    int total_count = 0;
    float total_rtt = 0;
    for (int i=0 ; i<user_count ; i++) {
        // cout << info[i].total_count << " | " << info[i].total_rtt << endl;
        total_count += info[i].total_count;
        total_rtt += info[i].total_rtt;
    }
    cout << total_count << " " << total_rtt;

    /* close log file */
    fclose(log_file);

    return 0;
}
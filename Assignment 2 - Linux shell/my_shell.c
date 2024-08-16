#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64

int background_pid[64];
int foreground_pid;

void change_directory(char ** path) {
    if (path[1] == NULL || path[2] != NULL)
        printf("Shell: Incorrect command\n");
    else if (chdir(path[1]) == -1)
        printf("Shell: Not a valid directory\n");
}

void exit_command() {
    for (int i = 0 ; i < 64 ; i++)
        if (background_pid[i] != -1)
            kill(background_pid[i], SIGKILL);
}

void insert_into_array(int pid) {
    for (int i = 0 ; i < 64 ; i++) {
        if (background_pid[i] == -1) {
            background_pid[i] = pid;
            break;
        }
    }
}

void sigchld_handler() {
    for (int i = 0 ; i < 64 ; i++) {
        if (background_pid[i] != -1 && waitpid(background_pid[i], NULL, WNOHANG) > 0) {
            waitpid(background_pid[i], NULL, 0);
            background_pid[i] = -1;
            printf("Shell: Background process finished\n");
        }
    }
}

void sigint_handler() {
    if (foreground_pid != -1) {
        kill(foreground_pid, SIGINT);
        foreground_pid = -1;
    }
}

char ** tokenize(char* line) {
    char ** tokens = (char ** ) malloc(MAX_NUM_TOKENS * sizeof(char * ));
    char * token = (char * ) malloc(MAX_TOKEN_SIZE * sizeof(char));
    int i, tokenIndex = 0, tokenNo = 0;

    for (i = 0; i < strlen(line); i++) {

        char readChar = line[i];

        if (readChar == ' ' || readChar == '\n' || readChar == '\t') {
            token[tokenIndex] = '\0';
            if (tokenIndex != 0) {
                tokens[tokenNo] = (char * ) malloc(MAX_TOKEN_SIZE * sizeof(char));
                strcpy(tokens[tokenNo++], token);
                tokenIndex = 0;
            }
        } else {
            token[tokenIndex++] = readChar;
        }
    }

    free(token);
    tokens[tokenNo] = NULL;
    return tokens;
}

int main(int argc, char * argv[]) {

    signal(SIGINT , sigint_handler);
    signal(SIGCHLD, sigchld_handler);
    char line[MAX_INPUT_SIZE];
    char ** tokens;

    for (int i = 0 ; i < 64 ; i++) background_pid[i] = -1;
    while (1) {

        bzero(line, sizeof(line));
        printf(" $ ");
        scanf("%[^\n]", line);
        getchar();
        line[strlen(line)] = '\n';
        tokens = tokenize(line);

        bool bg_execution = false;
        for (int i = 0; tokens[i] != NULL; i++) {
            if (strcmp(tokens[i], "&") == 0) {
                bg_execution = true;
                tokens[i] = NULL;
                break;
            }
        }
        if (tokens[0] == NULL) continue;
        else if (strcmp("cd",   tokens[0]) == 0) change_directory(tokens);
        else if (strcmp("exit", tokens[0]) == 0) {exit_command(); break;}
        else {
            int pid = fork();
            if (pid == 0) {
                if (bg_execution) setpgid(pid, pid);
                if (execvp(tokens[0], tokens) == -1) printf("Shell: Command not found: %s\n", tokens[0]);
                exit(0);
            } else if (pid > 0) {
                if (bg_execution) insert_into_array(pid);
                else {
                    foreground_pid = pid;
                    waitpid(pid, NULL, 0);
                    foreground_pid = -1;
                }
            } else {
                printf("Shell: Error encountered while creating child process\n");
            }
        }
        for (int i = 0; tokens[i] != NULL; i++) free(tokens[i]);
        free(tokens);
    }
    return 0;
}

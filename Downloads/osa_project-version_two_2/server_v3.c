#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/msg.h>
#include <errno.h>
#include <string.h>

#define NUM_SIGNALS 3
#define MAX_CHUNK_SIZE 100

// struct for messages in message queue
struct message {
    long messageType;
    size_t chunkSize;
    char chunk[MAX_CHUNK_SIZE];
};

void handleSignal(int signal);
void connectPipe(pid_t pid);
int get_message_queue_access(int queueType);
int read_cooking_recipe(int msgQueueID);

char *categories[NUM_SIGNALS] = {"Student", "Azerbaijani", "French"};

int main() {

    pid_t pid = getpid();  // get the current process ID (to create connection with client)
    struct sigaction action;
    
    // write current process ID to the pipe
    connectPipe(pid);

    // initializes an empty signal set for sa_mask 
    // (set of signals to be blocked during the execution of the signal handler)
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    action.sa_handler = handleSignal;
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGQUIT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);

    // pausing after each signal received
    while (1) {
        sleep(1);
    }
}

void connectPipe(pid_t pid){
    
    int fd;

    // Create a named pipe
    mkfifo("server_pipe", 0666);
    // Open the pipe for writing
    fd = open("server_pipe", O_WRONLY);
    // Write the server process ID to the pipe (to be read by client)
    write(fd, &pid, sizeof(pid));
    // Close the pipe
    close(fd);
}

void handleSignal(int signal) {

    int categoryIndex;
    // receive signal and set the type of recipe according to the signal received
    switch(signal) {
        case SIGINT:
            categoryIndex = 0; // Student
            break;
        case SIGQUIT:
            categoryIndex = 1; // Azeri
            break;
        case SIGTERM:
            categoryIndex = 2; // French
            break;
        default:
            return;
    }

    int msg_id = get_message_queue_access(categoryIndex + 1);
    printf("Recipe for category %s:\n", categories[categoryIndex]);
    read_cooking_recipe(msg_id);
}

int get_message_queue_access(int queueType) {
// function to get access to the message queue of the given type
    key_t k;
    int id;
    // choosing key according to the type of queue
    switch (queueType){
    case 1:
        k = ftok("file1", 'A');
        break;
    case 2:
        k = ftok("file2", 'B');
        break;
    case 3:
        k = ftok("file3", 'C');
        break;
    }

    if (k == -1) { perror("Error in ftok"); }
    // accessing the queue
    id = msgget(k, 0);
    if (id == -1) { perror("Error getting the message"); }
    return id;
}

int read_cooking_recipe(int msgQueueID) {
    // function to read data from the given message queue
    int r;
    struct message msg;

    // getting chunks of recipes
    while (1) {
        r = msgrcv(msgQueueID, &msg, sizeof(msg), 1, 0);
        if (r == -1) {
            perror("Error receiving the message");
            break;
        }
        // writing to the terminal
        fwrite(msg.chunk, 1, msg.chunkSize, stdout);
        // if it is the last chunk
        if (msg.chunkSize < MAX_CHUNK_SIZE) {
            printf("\n---\n");
            break;
        }
    }
    return 0;
}
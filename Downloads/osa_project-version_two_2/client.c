#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


pid_t getServerPID();
void sendSignals(pid_t serverPID);


int main() {

    pid_t serverPID = getServerPID();
    sendSignals(serverPID);
}


void sendSignals(pid_t serverPID){

    for (int i=0;i<100;i++) {
        int signal = rand()%3+1;
        switch (signal) {
            case 1:
                kill(serverPID, SIGINT);
                printf("\nSent SIGINT\n");
                break;
            case 2:
                kill(serverPID, SIGQUIT);
                printf("\nSent SIGQUIT\n");
                break;
            case 3:
                kill(serverPID, SIGTERM);
                printf("\nSent SIGTERM\n");
                break;
            default:
                printf("Unknown signal chosen\n");
        }
        sleep(1);
    }
}


pid_t getServerPID(){

    int fd;
    pid_t server_pid;
    srand(time(NULL));

    // open the named pipe for reading the process id of server
    fd = open("server_pipe", O_RDONLY);
    // read the server process ID from the pipe
    read(fd, &server_pid, sizeof(server_pid));
    // Close the pipe
    close(fd);

    return server_pid;

}
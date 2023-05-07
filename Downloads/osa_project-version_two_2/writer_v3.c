#include <stdio.h>
#include <stdlib.h> 
#include <sys/msg.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/msg.h>
#include <sys/ipc.h>

#define NUM_OF_RECIPES 2
#define NUM_OF_CATEGORIES 3
#define MAX_CHUNK_SIZE 100


struct recipe {
    int categoryIndex;
    char * categoryName;
    char * recipeContent;
};

// struct for messages in message queue
struct message {
    long messageType;
    size_t chunkSize;
    char chunk[MAX_CHUNK_SIZE];
};


char * categories[NUM_OF_CATEGORIES] = {"Student", "Azeri", "French"};
// array with all recipes of all categories
struct recipe recipes[NUM_OF_RECIPES*NUM_OF_CATEGORIES];

void write_to_array();
void Cooking_recipe_writer(int msgQueueID,char *recipeContent);
int create_message_queue(int queueType);
int msgqueue_empty(int msgQueueID);

void main() {
    // read all recipes from files and store them in the array
    write_to_array();

    // create a message queue for each category and store 2 recipes there
    for (int i = 0; i < 3; i++) {
        int msg_id = create_message_queue(i + 1);
        // Refill the message queue if necessary
        if (msgqueue_empty(msg_id)) {
            Cooking_recipe_writer(msg_id, recipes[i * 2].recipeContent);
            Cooking_recipe_writer(msg_id, recipes[i * 2 + 1].recipeContent);
            printf("\nFinished writing to message queue for category %i!!!\n", i + 1);
        }
    }
}

int msgqueue_empty(int msgQueueID) {
    struct msqid_ds buf;

    // Get message queue status
    if (msgctl(msgQueueID, IPC_STAT, &buf) == -1) {
        perror("msgctl");
        exit(1);
    }

    // Check if the message queue is empty (msg_qnum is the number of messages in the queue)
    if (buf.msg_qnum == 0) {
        return 1;
    } else {
        return 0;
    }
}

int create_message_queue(int queueType){
// function to create message queue of a given type (category)
    key_t k ;
    int id ;
    // choosing different keys for each category
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
    
    if (k == -1){ perror("Error in ftok");}
    // creating the message queue
    id = msgget (k, IPC_CREAT | 0666) ;
    if (id == -1){ perror("Error creating the queue");}
    return id ;
}


void Cooking_recipe_writer(int msgQueueID, char *recipeContent){
// function to send the given recipe to the given message queue
    int r;
    struct message msg;
    size_t content_length = strlen(recipeContent);

    msg.messageType = 1;
    
    // as recipes are long and determining their size is difficult
    // it was decided to split messages into parts and send each part as a separate message
    for (size_t offset = 0; offset < content_length; offset += MAX_CHUNK_SIZE) {
        // size of chunk is either the fixed maximum
        // or the leftover length if the chunk is last
        msg.chunkSize = (content_length - offset > MAX_CHUNK_SIZE) ? MAX_CHUNK_SIZE : content_length - offset;
        // copying chunkSize length of the recipe starting from the offset to the message
        memcpy(msg.chunk, recipeContent + offset, msg.chunkSize);
        // sending the message via message queue
        r = msgsnd(msgQueueID, &msg, sizeof(msg), 0);
        if (r == -1) {
            perror("Error sending the message");
            break;
        }
    }
}



void write_to_array(){
// function to read all recipes from files and store them in the array

    // counter of recipes array entries
    int insertionCounter = 0;

    // iterating through each directory (created for each category)
    for(int i=0; i<NUM_OF_CATEGORIES; i++){
        DIR *directory;
        struct stat currrent_file;
        struct dirent *dir_read;

        directory = opendir(categories[i]);
        if(directory==NULL){ perror("No such directory");}    

        // going through each recipe in the given directory
        while((dir_read=readdir(directory))!=NULL){

            if(dir_read->d_name[0] != '.'){

                char filename[300];
                // creating the correct filepath to be able to open it
                snprintf(filename, sizeof(filename), "%s/%s", categories[i], dir_read->d_name);
                char *buffer;
                ssize_t bytesRead;
                int fd;

                // defining recipes properties
                recipes[insertionCounter].categoryIndex = i+1;
                recipes[insertionCounter].categoryName = (char *)malloc(sizeof(dir_read->d_name));

                // getting the name of the recipe 
                for(int k=0; dir_read->d_name[k]!='.';k++){
                    // reading until ".txt"
                    recipes[insertionCounter].categoryName[k] =  dir_read->d_name[k];
                }

                // opening the file for reading only
                fd = open(filename, O_RDONLY);
                if(fd == -1) perror("Error opening file");

                // determining the size of the file
                off_t fileSize = lseek(fd, 0, SEEK_END);

                // allocating a buffer to store the contents of the file
                recipes[insertionCounter].recipeContent = (char*)malloc(fileSize);

                // resetting the file pointer to the beginning of the file
                lseek(fd, 0, SEEK_SET);

                // reading the contents of the file into the buffer
                bytesRead = read(fd, recipes[insertionCounter].recipeContent, fileSize);

                insertionCounter++;
                close(fd);
            }
        }
        closedir(directory);
    }
}
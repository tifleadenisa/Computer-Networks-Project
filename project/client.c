#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdbool.h>

extern int errno;

bool isIntervalOk(char msg[])
{
    char digits[] = "0123456789";
    if(strlen(msg) != 12){
        return false;
    }

    //check to be digits
    if(strchr(digits, msg[0]) == NULL || strchr(digits, msg[1]) == NULL || strchr(digits, msg[3]) == NULL || strchr(digits, msg[4]) == NULL ||
            strchr(digits, msg[6]) == NULL || strchr(digits, msg[7]) == NULL || strchr(digits, msg[9]) == NULL || strchr(digits, msg[10]) == NULL){
        return false;
    }

    //check for special characters
    if(msg[2] != ':' || msg[5] != '-' || msg[8] != ':'){
        return false;
    }

    //check if hours and minutes are valid
    int hour1 = (msg[0] - '0') * 10 + (msg[1] - '0');
    int minutes1 = (msg[3] - '0') * 10 + (msg[4] - '0');
    int hour2 = (msg[6] - '0') * 10 + (msg[7] - '0');
    int minutes2 = (msg[9] - '0') * 10 + (msg[10] - '0');

    if(hour1 < 0 || hour1 > 23 || minutes1 < 0 || minutes1 > 59 ||
            hour2 < 0 || hour2 > 23 || minutes2 < 0 || minutes2 > 59){
        return false;
    }

    if(hour1 == hour2 && minutes1 == minutes2){
        return false;
    }

    if(hour1 > hour2){
        return false;
    }else{
        if(hour1 == hour2 && minutes1 > minutes2)
            return false;
    }
    return true;
}

int main (int argc, char *argv[])
{
    int sd;
    struct sockaddr_in server;
    char message[200];

    if (argc != 3){
        printf ("Valid syntax:: %s <server_address> <port>\n", argv[0]);
        return -1;
    }

    int port = atoi(argv[2]);

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror ("socket() error.\n");
        return errno;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(argv[1]);
    server.sin_port = htons(port);

    if (connect(sd, (struct sockaddr *) &server,sizeof (struct sockaddr)) == -1){
        perror("connect() error.\n");
        return errno;
    }

    providing_interval:

    bzero(message, 200);
    printf("Exit or Provide an interval, format: hh:mm-hh:mm.\n");
    fflush(stdout);
    read(0, message, 200);
    if(strcmp(message, "exit\n") == 0) {
        close(sd);
    }else{
        while(!isIntervalOk(message)){
            printf("Wrong interval!\n");
            fflush(stdout);
            goto providing_interval;
        }

        if (write(sd, message, 200) <= 0){
            perror("write() error.\n");
            return errno;
        }

        bzero(message, 200);
        if (read(sd, message, 200) < 0){
            perror("read() error.\n");
            return errno;
        }

        printf("%s\n", message);

        if(strcmp(message, "There aren't logs in this interval") == 0){
            goto providing_interval;
        }else{
            choose_option:

            printf("Choose 1 of these options or exit:\n1) memory load on the provided interval\n"
                   "2) number of connections for every user in the provided interval\n"
                   "3) number of connections for every service for the provided interval\n"
                   "4) information about current processes\n"
                   "5) information about current connections\n");
            bzero(message, 200);
            fflush(stdout);
            read(0, message, 200);
            while(message[0] != '1' && message[0] != '2' && message[0] != '3' && message[0] != '4' && message[0] != '5' && strstr(message, "exit") == NULL){
                printf("Provide an option: 1, 2, 3, 4, 5 or exit!\n");
                bzero(message, 200);
                fflush(stdout);
                read(0, message, 200);
            }

            if (write(sd, message, 200) <= 0){
                perror("write() error.\n");
                return errno;
            }

            if(strstr(message, "exit") != NULL){
                close(sd);
            }else{
                char statistic[5000] = "";
                bzero(statistic, 5000);
                if (read(sd, statistic, 5000) <= 0){
                    perror("read() error.\n");
                    return errno;
                }
                printf ("%s\n", statistic);
                if(strstr(message, "2") != NULL || strstr(message, "3") != NULL){
                    printf(":: means the number of connections\n");
                }
                goto choose_option;
            }
        }
        close (sd);
    }
}
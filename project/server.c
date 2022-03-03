#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <stdbool.h>
#include <dirent.h>
#include<utmp.h>

extern int errno;

struct file_content{
    double system_load;
    char users[1000];
    char running_services[5000];
};

struct statistic{
    double avg_load;
    char users_connections[5000];
    char services_connections[5000];
};

bool isTimeInInterval(char time[], int hour1, int minutes1, int hour2, int minutes2)
{
    int hour;
    int minutes;
    if(time[1] == ':'){
        hour = time[0] - '0';
        if(strchr("0123456789", time[3]) == NULL){
            minutes = time[2] - '0';
        }else{
            minutes = (time[2] - '0') * 10 + (time[3] - '0');
        }
    }else{
        hour = (time[0] - '0') * 10 + (time[1] - '0');
        if(strchr("0123456789", time[4]) == NULL){
            minutes = time[3] - '0';
        }else{
            minutes = (time[3] - '0') * 10 + (time[4] - '0');
        }
    }
    if(hour1 > hour || hour2 < hour)
        return false;
    if(hour1 == hour && minutes1 > minutes)
        return false;
    if(hour2 == hour && minutes2 < minutes)
        return false;
    return true;
}

bool existsLogs(int hour1, int minutes1, int hour2, int minutes2)
{
    DIR *d;
    struct dirent *dir;
    d = opendir("./logs");
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if(strlen(dir->d_name) > 4 && isTimeInInterval(dir->d_name,hour1, minutes1, hour2, minutes2))
                return true;
        }
        closedir(d);
    }
    return false;
}

//check if is a log file saved less than 4 minutes ago
bool isLogFrom5Min(int hour, int minutes){
    int hour1, minute1;
    if(minutes >= 4){
        hour1 = hour;
        minute1 = minutes - 4;
    }else{
        if(hour == 0){ hour1 = 23; }
        else{ hour1 = hour - 1; }
        minute1 = minutes + 56;
    }
    return existsLogs(hour1, minute1, hour, minutes);
}

double getSystemLoad(){
    double loadavg[3];
    int nelem = 3;
    getloadavg(loadavg, nelem);
    return loadavg[2];
}

char * getUsers(){
    struct utmp *n;
    setutent();
    n=getutent();
    char * users= malloc(100);
    bzero(users, 100);
    while(n){
        if(n->ut_type==USER_PROCESS) {
            strcat(users, n->ut_user);
            strcat(users, ", ");
        }
        n=getutent();
    }
    users[strlen(users) - 2] = '\0';
    return users;
}

char * getRunningServices(){
    FILE *fp;
    char path[1035];

    fp = popen("systemctl --type=service --state=running", "r");
    if (fp == NULL){
        printf("Failed to run command for listing services\n" );
        exit(1);
    }

    char * services = malloc(5000);
    bzero(services, 5000);
    while (fgets(path, sizeof(path), fp) != NULL) {
        char aux[100] = "";
        strcpy(aux, strtok(path, " ."));
        if(aux[0] >= 'a' && aux[0] <= 'z'){
            strcat(services, aux);
            strcat(services, ", ");
        }
    }
    services[strlen(services) - 2] = '\0';
    pclose(fp);
    return services;
}

char * getConnectionTypes(){
    FILE *fp;
    char path[1035];

    fp = popen("netstat -tulnp", "r");
    if (fp == NULL) {
        printf("Failed to run command for listing connections\n" );
        exit(1);
    }

    char * connections = malloc(5000);
    bzero(connections, 5000);
    while (fgets(path, sizeof(path), fp) != NULL) {
        strcat(connections, path);
    }
    strcat(connections, "\n\n");
    connections[strlen(connections) - 2] = '\0';
    pclose(fp);
    return connections;
}

char * getProcesses(){
    FILE *fp;
    char path[1035];

    fp = popen("ps", "r");
    if (fp == NULL) {
        printf("Failed to run command for listing processes\n" );
        exit(1);
    }

    char * processes = malloc(5000);
    bzero(processes, 5000);
    while (fgets(path, sizeof(path), fp) != NULL) {
        strcat(processes, path);
    }
    processes[strlen(processes) - 2] = '\0';
    pclose(fp);
    return processes;
}

struct file_content read_file(char * filename){
    FILE *fp;
    char file_path[100] = "./logs/";

    strcat(file_path, filename);
    if ((fp = fopen(file_path,"r")) == NULL){
        printf("Error! opening file");
        exit(1);
    }

    char content[10000] ="";
    char cont[10000];
    if (fp) {
        while (fgets(cont, 10000, fp)) {
            strcat(content, cont);
            bzero(cont, 10000);
        }
        fclose(fp);
    }

    struct file_content fileContent;
    fileContent.system_load = 0.0;
    char * token;
    token = strtok(content, "\n");
    fileContent.system_load = strtod(token, NULL);
    token = strtok(NULL, "\r\n\t");
    strcpy(fileContent.users, token);
    token = strtok(NULL, "\r\n\t");
    strcpy(fileContent.running_services, token);
    return fileContent;
}

int substringCount(char* string, char* substring) {
    int i, l1, l2;
    int count = 0;

    l1 = strlen(string);
    l2 = strlen(substring);

    for(i = 0; i < l1 - l2 + 1; i++) {
        if(strstr(string + i, substring) == string + i) {
            count++;
            i = i + l2 -1;
        }
    }
    return count;
}

char* noOfOccurrences(char * string){
    char * occurrences = malloc(5000);
    strcpy(occurrences, "");
    char string_copy[5000];
    strcpy(string_copy, string);
    char * token;
    token = strtok(string_copy, ", ");
    while(token){
        if(substringCount(occurrences, token) == 0){
            strcat(occurrences, token);
            strcat(occurrences, "::");
            char aux[2];
            sprintf(aux, "%d", substringCount(string, token));
            strcat(occurrences, aux);
            strcat(occurrences, "\n");
        }
        token = strtok(NULL, ", ");
    }
    return occurrences;
}

struct statistic computeStatistics(int hour1, int minutes1, int hour2, int minutes2){
    struct statistic stats;
    stats.avg_load = 0.0;
    char users[5000] = "";
    char services[5000] = "";
    int no_of_logs = 0;
    DIR *d;
    struct dirent *dir;
    d = opendir("./logs");
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if(strlen(dir->d_name) > 4 && isTimeInInterval(dir->d_name,hour1, minutes1, hour2, minutes2)){
                struct file_content fileContent = read_file(dir->d_name);
                no_of_logs++;
                stats.avg_load += fileContent.system_load;
                strcat(users, fileContent.users);
                strcat(users, ", ");
                strcat(services, fileContent.running_services);
                strcat(services, ", ");
            }
        }
        closedir(d);
    }
    stats.avg_load /= no_of_logs;
    strcpy(stats.users_connections, noOfOccurrences(users));
    strcpy(stats.services_connections, noOfOccurrences(services));
    return stats;
}

int main ()
{
    struct sockaddr_in server;
    struct sockaddr_in from;
    char message[200];
    int sd;

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("socket() error.\n");
        return errno;
    }

    bzero(&server, sizeof (server));
    bzero(&from, sizeof (from));

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(2024);

    if (bind(sd, (struct sockaddr *) &server, sizeof (struct sockaddr)) == -1){
        perror("bind() error.\n");
        return errno;
    }

    if (listen (sd, 1) == -1){
        perror ("listen() error.\n");
        return errno;
    }

    while (1){
        int client;
        unsigned int length = sizeof (from);

        printf("Waiting on port 2024");
        fflush(stdout);

        client = accept(sd, (struct sockaddr *) &from, &length);

        if (client < 0){
            perror ("accept() error.\n");
            continue;
        }

        int pid;
        if ((pid = fork()) == -1) {
            close(client);
            continue;
        } else if (pid > 0) {
            //parent
            close(client);
            int pid_logs;
            if ((pid_logs = fork()) == -1){
                perror("fork() error");
            }
            else if (pid_logs > 0){
                //parent
                while(waitpid(-1,NULL,WNOHANG));
                continue;
            }else if(pid_logs == 0) {
                //child
                while(1){
                    time_t seconds;
                    struct tm *timeStruct;
                    seconds = time(NULL);
                    timeStruct = localtime(&seconds);
                    if(!isLogFrom5Min(timeStruct->tm_hour,timeStruct->tm_min)){
                        char filename[15] = "logs/";
                        char hour[3] = "";
                        char minute[3] = "";
                        sprintf(hour, "%d", timeStruct->tm_hour);
                        sprintf(minute, "%d", timeStruct->tm_min);
                        strcat(filename, hour);
                        strcat(filename, ":");
                        strcat(filename, minute);
                        strcat(filename, ".log");

                        FILE *fp;
                        fp = fopen(filename, "w");
                        if (fp == NULL) {
                            printf("error when opening a log file");
                        }

                        fprintf(fp, "%f\n\n", getSystemLoad());
                        fprintf(fp,"%s\n\n",getUsers());
                        fprintf(fp,"%s\n\n", getRunningServices());
                        fflush(fp);
                        fclose(fp);
                    }
                    sleep(300);
                }
            }

        } else if (pid == 0) {
            //child
            reading_interval:

            bzero(message, 200);
            fflush(stdout);

            if (read(client, message, 200) <= 0){
                perror("read() error.\n");
                close(client);
                continue;
            }

            printf("I received: %s\n", message);
            if(strcmp(message, "exit\n") == 0){
                close(client);
                exit(0);
            }
            else
            {
                int hour1 = (message[0] - '0') * 10 + (message[1] - '0');
                int minutes1 = (message[3] - '0') * 10 + (message[4] - '0');
                int hour2 = (message[6] - '0') * 10 + (message[7] - '0');
                int minutes2 = (message[9] - '0') * 10 + (message[10] - '0');

                bzero(message,200);
                if(existsLogs(hour1, minutes1, hour2, minutes2)){
                    strcat(message,"There are logs in this interval");
                }else{
                    strcat(message,"There aren't logs in this interval");
                }

                printf("Sent:%s\n",message);

                if (write(client, message, 200) <= 0){
                    perror("write() error.\n");
                    continue;
                }

                if(strcmp(message, "There aren't logs in this interval") == 0){
                    goto reading_interval;
                }else{
                    struct statistic stat = computeStatistics(hour1, minutes1, hour2, minutes2);

                    if_logs:

                    bzero (message, 200);
                    if (read(client, message, 200) <= 0){
                        perror("read() error.\n");
                        close(client);
                        continue;
                    }
                    if(strstr(message, "exit") != NULL){
                        close(client);
                        exit(0);
                    }
                    else{
                        printf("I received: %s\n", message);

                        int choice = message[0] - '0';
                        char statistic_to_sent[5000];

                        if(choice == 1){
                            sprintf(statistic_to_sent, "%f", stat.avg_load);
                        }else{
                            if(choice == 2){
                                strcpy(statistic_to_sent, stat.users_connections);
                            }else{
                                if(choice == 3){
                                    strcpy(statistic_to_sent, stat.services_connections);
                                }else{
                                    if(choice == 4){
                                        strcpy(statistic_to_sent, getProcesses());
                                    }else{
                                        strcpy(statistic_to_sent, getConnectionTypes());
                                    }
                                }
                            }
                        }
                        if (write(client, statistic_to_sent, 5000) <= 0){
                            perror("write() error.\n");
                            continue;
                        }
                        goto if_logs;
                    }
                }
            }
        }
    }
}
//For VSCode to recognize addrinfo struct
#define _POSIX_C_SOURCE 200112L
#define  _GNU_SOURCE

//#Includes
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <ctype.h>


//Defines command constants
#define RETRIEVE "retrieve"
#define PARSE "parse"
#define MIME "mime"
#define LIST "list"


#define IMAP_PORT "143" //Port 
#define BUFFER 1024 //Max buffer
#define MAX_MESSAGES 10000 // Maximum messages
#define MIN_ARGUMENTS 7 // Mimimum CI Arguments 

//Task 2.3 header sizes
#define HEADER_FROM_SIZE 5
#define HEADER_TO_SIZE 3
#define HEADER_SUBJ_SIZE 8
#define HEADER_DATE_SIZE 5

//Task 2.4 states
#define HEADER_NOT_FOUND 0
#define MIME_VERSION_FOUND 1
#define CTYPE_HEADER_FOUND 2
#define DELIMITER_FOUND 3
#define BODYPART_FOUND 4
#define VALID_BODYPART_FOUND 5

//Functions for task 2.3
void retrieve(int sockfd, int messageNum);
int get_msg_bytes(char *line);

//Functions for task 2.4
void parse(int sockfd, int messageNum);
char* trim_header_value(const char* line);

//Functions for task 2.5
void mime(int sockfd, int messageNum);
char* remove_quotes(char *str);

//Functions for task 2.6
void list(int sockfd);





int main(int argc, char *argv[]) {
    int DEBUG = 0;

    char *username = NULL;
    char *password = NULL;
    char *folder= "INBOX";
    int messageNum = -1;
    char *command = NULL;
    char *server_Name = NULL;
    int tsl_flag = 0;
    int message_flag = 0;

    struct addrinfo hints, *servinfo, *rp;
    int sockfd, n;
    char buffer[1024];

    if (argc < MIN_ARGUMENTS) {
        fprintf(stderr, "arguments:%d less than minimum\n", argc);
        return EXIT_FAILURE;
    }

    //Reads command line arguments into variables
    for(int i = 1; i < argc; i++){
        if(strcmp(argv[i], "-t") == 0) tsl_flag = 1;
        else if(argv[i][0] == '-'){
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: Option %s requires an argument.\n", argv[i]);
                return EXIT_FAILURE;
            }
            //Flags with 1 argument after it
            if(strcmp(argv[i], "-u") == 0){username = argv[i+1];}
            else if(strcmp(argv[i], "-p") == 0){password = argv[i+1];}
            else if(strcmp(argv[i], "-f") == 0){folder = argv[i+1];}
            else if(strcmp(argv[i], "-p") == 0){password = argv[i+1];}
            else if(strcmp(argv[i], "-n") == 0){
                if ((strspn(argv[i+1], "0123456789")!=strlen(argv[i+1]))) {
                    fprintf(stderr, "Only positive integers allowed\n");
                    return EXIT_FAILURE;
                }
                messageNum = atoi(argv[i+1]);
                message_flag=1;    
            }
            i++;
        }
        else{
            if(command == NULL) command = argv[i];
            else if(server_Name==NULL) server_Name = argv[i];
        }
    }

    if(DEBUG && 1){
        fprintf(stderr, "Main: Read u: %s, p: %s, command: %s, serverName: %s", username, password, command, server_Name);
        if(folder != NULL) fprintf(stderr, ", folder: %s", folder);
        if(messageNum != 0) fprintf(stderr, ", messageNum: %d", messageNum);
        if(tsl_flag == 1) fprintf(stderr, ", tsl flag is true");
        fprintf(stderr, "\n");
    }
    if(strcmp(command,LIST)==0){
        messageNum=1;
    }
    //Checking parameters
    if (!username || !password || !command || !server_Name ) {
        fprintf(stderr, "Critical Argument Missing, Expected:Executable -u <username> -p <password> <command> <server_name>\n");
        exit(EXIT_FAILURE);
    }

    //Too many messages, 0 or overflow because of very large input
    if (message_flag == 1 && (messageNum > MAX_MESSAGES || messageNum==0 || messageNum<0) ) {
        fprintf(stderr, "Error: Invalid message number. Must be between 0 and 10000.\n");
        exit(EXIT_FAILURE);
    }

    //Login uses Practical 8 COMP30023 2024 code and also ed512 email's suggestions from this project 
    //which can be found on ret-ed512.out rest is RFC3501

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // Both IPv4 and IPv6
    hints.ai_socktype = SOCK_STREAM; //TCP

    if (getaddrinfo(server_Name, IMAP_PORT, &hints, &servinfo) != 0) {
        perror("getaddrinfo failed");
        exit(EXIT_FAILURE);
    }

    for (rp = servinfo; rp != NULL; rp = rp->ai_next) {
        sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sockfd == -1) continue;

        if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) != -1) break; // success

        close(sockfd);
    }
    if (rp == NULL) {
        fprintf(stderr, "Could not connect\n");
        exit(EXIT_FAILURE);
    }
    freeaddrinfo(servinfo);

    // Receive the server greeting
    if ((n = read(sockfd, buffer, sizeof(buffer) - 1)) < 0) {
        perror("read");
        exit(EXIT_FAILURE);
    }
    buffer[n] = '\0';
    if (DEBUG) {
        printf("Server greeting: %s\n", buffer);
    }

    // Send LOGIN command
    snprintf(buffer, sizeof(buffer), "A01 LOGIN \"%s\" \"%s\"\r\n", username, password);
    if (write(sockfd, buffer, strlen(buffer)) < 0) {
        perror("write");
        exit(EXIT_FAILURE);
    }

    // Receive login response
    if ((n = read(sockfd, buffer, sizeof(buffer) - 1)) < 0) {
        perror("read");
        exit(EXIT_FAILURE);
    }
    buffer[n] = '\0';
    if (strstr(buffer, "A01 OK") == NULL) {
        printf("Login failure\n");
        exit(3);
    }

    // Send SELECT command
    snprintf(buffer, sizeof(buffer), "A02 SELECT \"%s\"\r\n", folder);
    if (write(sockfd, buffer, strlen(buffer)) < 0) {
        perror("write");
        exit(EXIT_FAILURE);
    }

    // Receive SELECT response
    if ((n = read(sockfd, buffer, sizeof(buffer) - 1)) < 0) {
        perror("read");
        exit(EXIT_FAILURE);
    }
    buffer[n] = '\0';
    if (strstr(buffer, "A02 OK") == NULL) {
        printf("Folder not found\n");
        exit(3);
    }

    if (DEBUG) {
        fprintf(stderr, "Folder '%s' selected successfully.\n", folder);
    }

    //Login complete, attempt to run retrieve command
    if(strcmp(command, RETRIEVE) == 0){
        retrieve(sockfd, messageNum);
    }

    //login complete attempt to run fetch command
    if(strcmp(command, PARSE) == 0){
        parse(sockfd,messageNum);
    }

    //Login complete, attempt to run mime command
    if(strcmp(command, MIME) == 0){
        mime(sockfd, messageNum);
    }

    if(strcmp(command,LIST)==0){
        list(sockfd);
    }


    close(sockfd);
    return 0;
}

void retrieve(int sockfd, int messageNum){
    int DEBUG = 0;
    char buffer[1024];

    //Gets most recent message if messageNum not specified
    //RFC 3501 specifes * as largest number in use = most recent
    if(messageNum == -1){
        snprintf(buffer, sizeof(buffer), "A04 FETCH * BODY.PEEK[]\r\n");
    }
    else{
        snprintf(buffer, sizeof(buffer), "A04 FETCH %d BODY.PEEK[]\r\n", messageNum);
    }
    if (write(sockfd, buffer, strlen(buffer)) < 0) {
        perror("write");
        exit(EXIT_FAILURE);
    }

    //Recieves response and converts it to file
    FILE *socketFile = fdopen(sockfd, "r");
    assert(socketFile != NULL);

    char *line = NULL;
    size_t line_len = 0;
    int line_num = 0, line_size = 0;
    int read_bytes = 0, msg_bytes = 0;
    if(DEBUG && 1) fprintf(stderr, "Recieve: Reading response\n");
    //Iterates line by line, reading file into buffer
    while((line_size = getline(&line, &line_len, socketFile)) != -1){
        if(DEBUG && 1) fprintf(stderr, "Recieve: Getting line %d\n", line_num);
        if(line_num == 0){
            if(DEBUG && 1) fprintf(stderr, "Recieve: Line 0 is %s\n", line);
            //First line, either message data with body length, or response fatal
            if(strstr(line, "BAD") != NULL || strstr(line, "NO") != NULL){
                
                //Error has occured, exit with code 3
                printf("Message not found\n");
                exit(3);
            }
            else{
                msg_bytes = get_msg_bytes(line);
                if(DEBUG && 1) fprintf(stderr, "Recieve: msg_bytes is %d\n", msg_bytes);
            }
        }
        else{
            printf("%s", line);
            read_bytes += line_size *sizeof(char);
        }
        if(read_bytes >= msg_bytes && line_num > 0){
            //Reached the end of email contents, read the last line and break
            getline(&line, &line_len, socketFile);
            //printf("%s", line);
            break;
        }
        line_num++;
    }
    free(line);
    exit(0);
}

int get_msg_bytes(char *line){
    int DEBUG = 0;
    if(DEBUG && 1) fprintf(stderr, "get_msg_bytes: called with line %s", line);
    //Returns the integer representation of the message length
    char ch = line[0];
    int i = 0;
    int numstart, numend;
    char *num_string = NULL;

    //Gets the location of the message length in the string
    while(ch != '\0'){
        if(DEBUG && 0) fprintf(stderr, "get_msg_bytes: read %c\n", ch);
        if(ch == '{') numstart = i + 1;
        if(ch == '}') numend = i;
        i++;
        ch = line[i];
    }
    if(DEBUG && 1) fprintf(stderr, "get_msg_bytes: done reading %s,%d,%d\n", line+numstart, numend, numstart);
    
    //Extracts the numerical part of the string and converts it to int
    num_string = malloc((1+sizeof(char))*(numend-numstart));
    memcpy(num_string, line + numstart, numend-numstart);
    num_string[numend-numstart] = '\0';
    if(DEBUG && 1) fprintf(stderr, "get_msg_bytes: num_string is %s with length %d\n", num_string, numend-numstart);
    int msg_bytes = atoi(num_string);
    free(num_string);
    
    if(DEBUG && 1) fprintf(stderr, "get_msg_bytes: returning %d\n", msg_bytes);
    return(msg_bytes);

}

void parse(int sockfd, int messageNum){

    int DEBUG = 0;
    char buffer[BUFFER];
    int subject_flag=1,to_flag=1;

    //Fetch email headers, * to fetch latest if nothing specified in terminal
    if(messageNum == -1){
        snprintf(buffer, sizeof(buffer), "A04 FETCH * BODY.PEEK[HEADER.FIELDS (FROM TO DATE SUBJECT)]\r\n");
    }
    else{
        snprintf(buffer, sizeof(buffer), "A04 FETCH %d BODY.PEEK[HEADER.FIELDS (FROM TO DATE SUBJECT)]\r\n", messageNum);
    }

    if (write(sockfd, buffer, strlen(buffer)) < 0) {
        perror("write");
        exit(EXIT_FAILURE);
    }

    FILE *socketFile = fdopen(sockfd, "r");
    assert(socketFile != NULL);

    char *headers = NULL;
    size_t headers_size = 0;
    char *line = NULL;
    size_t line_len = 0;
    ssize_t read_len;
    char *from = NULL, *to = NULL, *date = NULL, *subject = NULL;

    /*
    Using strncasecmp(), strstr() and strchr() functions from online document
    Found via google searching useful c functions
    https://linux.die.net/man/
    */

    while ((read_len = getline(&line, &line_len, socketFile)) != -1) {
        if (DEBUG) {
            fprintf(stderr, "Debug:%s", line); // Debug statement for raw buffer
            if (strstr(line, "OK Fetch completed") != NULL) {
                break;
            }

        }

        if(strncasecmp(line,"From:",HEADER_FROM_SIZE) == 0 ||strncasecmp(line,"Date:",HEADER_DATE_SIZE) == 0 ||
            strncasecmp(line,"To:",HEADER_TO_SIZE)==0 || strncasecmp(line,"Subject:",HEADER_SUBJ_SIZE)==0){
            headers_size += strlen(line);
            headers = realloc(headers, headers_size + 1);
            assert(headers);

            if (headers_size == strlen(line)) {
                strcpy(headers, line);
            } 
            else {
                strcat(headers, line);
            }
        }

        
        if(line[0] == '\t' || line[0] == ' '){
            headers[headers_size - 2] = '\0';
            headers_size -= 2;
            headers_size += strlen(line);
            headers = realloc(headers, headers_size + 1);
            assert(headers);

            if (headers_size == strlen(line)) {
                strcpy(headers, line);
            } 
            else {
                strcat(headers, line);
            }
        }
        if (strstr(line, "OK Fetch completed") != NULL) {
                break;
        }
    }

    //print each line of headers for debugging
    char *header_line = strtok(headers, "\r\n");  
    while (header_line != NULL) {
        
        if(strncasecmp(header_line,"From:",HEADER_FROM_SIZE) == 0){
            from = trim_header_value(header_line);
        }
        if(strncasecmp(header_line,"To:",HEADER_TO_SIZE) == 0){
            to = trim_header_value(header_line);
        }
        if(strncasecmp(header_line,"Date:",HEADER_DATE_SIZE) == 0){
            date = trim_header_value(header_line);
        }
        if(strncasecmp(header_line,"Subject:",HEADER_SUBJ_SIZE) == 0){
            subject = trim_header_value(header_line);
        }
        header_line = strtok(NULL, "\r\n");  
    }

    //handling no subject or to headers
    if(subject==NULL){
        subject_flag=0;
        subject = " <No subject>";
    }
    if(to==NULL){
        to_flag=0;
        to="";
    }

    if(date == NULL || from == NULL){      
        //Error has occured, exit with code 3
        printf("Message not found\n");
        exit(3);
    }


    printf("From:%s\nTo:%s\nDate:%s\nSubject:%s\n",from,to,date,subject);
    free(line);
    free(headers);
    free(from);
    free(date);

    if(subject_flag){
        free(subject);
    }
    if(to_flag){
        free(to);
    }
    
}

char* trim_header_value(const char* line) {
    // find colon and skip it
    const char* start = strchr(line, ':');
    start++;   
    
    // Find the end of the line (stop at CRLF
    const char* end = start;
    while (*end && *end != '\r' && *end != '\n') {
        end++;
    }

    // Allocate memory for the new string
    char* value = malloc(end - start + 1);
    if (value) {
        strncpy(value, start, end - start);
        value[end - start] = '\0'; // not by lf as easier to debug - we manually add \n in stdout
    }
    return value;
}

void mime(int sockfd, int messageNum){
    int DEBUG = 0;
    char buffer[1024];
    //./fetchmail -u test@comp30023 -p pass -n 1 mime unimelb-comp30023-2024.cloud.edu.au

    //Gets most recent message if messageNum not specified
    //RFC 3501 specifes * as largest number in use = most recent
    if(messageNum == -1){
        snprintf(buffer, sizeof(buffer), "A04 FETCH * BODY.PEEK[]\r\n");
    }
    else{
        snprintf(buffer, sizeof(buffer), "A04 FETCH %d BODY.PEEK[]\r\n", messageNum);
    }
    if (write(sockfd, buffer, strlen(buffer)) < 0) {
        perror("write");
        exit(EXIT_FAILURE);
    }

    //Recieves response and converts it to file
    FILE *socketFile = fdopen(sockfd, "r");
    assert(socketFile != NULL);

    char *line = NULL, lastline[1024], delimiter[1024];
    size_t line_len = 0;
    int line_num = 0, line_size = 0, read_bytes = 0, msg_bytes = 0;
    
    char *boundary = NULL;
    int loc_flag = 0, ctype_flag = 0, ctrans_flag = 0, line_flag = 0;
    if(DEBUG && 0) fprintf(stderr, "Mime: Reading response\n");


    //Iterates line by line, reading file into buffer
    while((line_size = getline(&line, &line_len, socketFile)) != -1){
        if(DEBUG && 0) fprintf(stderr, "Mime: Getting line %d\n", line_num);
        if(line_num == 0){
            if(DEBUG && 0) fprintf(stderr, "Mime: Line 0 is %s\n", line);
            //First line, either message data with body length, or response fatal
            if(strstr(line, "BAD") != NULL || strstr(line, "NO") != NULL){
                
                //Error has occured, exit with code 3
                printf("Message not found\n");
                exit(3);
            }
            else{
                msg_bytes = get_msg_bytes(line);
                if(DEBUG && 1) fprintf(stderr, "Mime: msg_bytes is %d\n", msg_bytes);
            }
        }
        else{
            if(DEBUG && 0)fprintf(stderr, "Mime: Checking for folding\n");
            if(lastline != NULL && (line[0] == ' ' || line[0] == '\t')){
                //Folded Line, combine with lastline
                if(DEBUG && 0)fprintf(stderr, "Mime: Line folds\n");
                strcat(lastline, line);
            }
            else{
                //Normal line, copy into lastline instead
                if(DEBUG && 0)fprintf(stderr, "Mime: Line does not fold\n");
                strcpy(lastline, line);
            }
            
            //Rest of the lines are here, we only want to print the parts of the message in MIME
            read_bytes += line_size *sizeof(char);
            if(DEBUG && 0) fprintf(stderr, "%s", lastline);
            
            //Match MIME
            if(loc_flag == HEADER_NOT_FOUND && strcasestr(lastline, "mime-version:") != NULL){
                if(strcasestr(lastline, "1.0") != NULL) loc_flag = MIME_VERSION_FOUND;
                if(DEBUG && 1) fprintf(stderr, "Mime: read MIME-Version at line %d\n", line_num);
            }

            //Matches first Content Type Header
            if(loc_flag == MIME_VERSION_FOUND && strcasestr(lastline, "content-type:") != NULL){  
                if(strcasestr(lastline, "multipart/alternative") != NULL) loc_flag = CTYPE_HEADER_FOUND;
                if(DEBUG && 1) fprintf(stderr, "Mime: read Content-Type at line %d\n", line_num);
            }

            //Matches boundary header
            if(loc_flag == CTYPE_HEADER_FOUND && (boundary = strcasestr(lastline, "boundary=")) != NULL){
                loc_flag = DELIMITER_FOUND;
                if(DEBUG && 1) fprintf(stderr, "Mime: initial boundary is %s", boundary);
                strcpy(delimiter, remove_quotes(boundary));
                if(DEBUG && 1) fprintf(stderr, "Mime: Formatted boundary parameter is %s\n", delimiter);
            }

            //Matches boundary delimiter
            char* delimit_loc = NULL;
            if((loc_flag == DELIMITER_FOUND || loc_flag == VALID_BODYPART_FOUND) && (delimit_loc = strcasestr(lastline, delimiter)) != NULL){
                if(DEBUG && 0) fprintf(stderr, "Mime: Delimter found\n%s\n", lastline);
                
                //We have finished reading the message body
                if(loc_flag == VALID_BODYPART_FOUND){
                    if(DEBUG && 1) fprintf(stderr, "All Done!\n");
                    break;
                }

                //End of the message, terminate and return with error
                if(delimit_loc[strlen(delimiter)] == '-'){
                    printf("Matching failed, try again");
                    exit(4);
                }
                
                //The beginning of a body part is found, check next lines
                loc_flag = BODYPART_FOUND;
                ctrans_flag = 0;
                ctype_flag = 0;
            }
            if(DEBUG && 1 && loc_flag == BODYPART_FOUND) fprintf(stderr, "Mine: Checking %s", lastline);

            //Matches content type
            if(loc_flag == BODYPART_FOUND && (strcasestr(lastline, "content-type")) != NULL){
                //Check for one of the accepted content types
                if(strcasestr(lastline, "text/plain") != NULL && strcasestr(lastline, "charset=utf-8") != NULL){
                    if(DEBUG && 1) fprintf(stderr, "Mime: Valid Ctype found\n%s\n", lastline);
                    ctype_flag = 1;
                }
                else ctype_flag = 0;
            }

            //Matches content transfer encoding header
            if(loc_flag == BODYPART_FOUND && (strcasestr(lastline, "content-transfer-encoding")) != NULL){
                //Check for one of the accepted content encoding
                if(strcasestr(lastline, "quoted-printable") != NULL || strcasestr(lastline, "7bit") != NULL || strcasestr(lastline, "8bit") != NULL){
                    if(DEBUG && 1) fprintf(stderr, "Mime: Valid Ctrans found\n%s\n", lastline);
                    ctrans_flag = 1;
                }
                else ctrans_flag = 0;
            }

            //Empty line found, start of body content
            if(strcmp(lastline, "\r\n") == 0){
                //Body part with valid ctype and ctrans found, start printing
                if(loc_flag == BODYPART_FOUND && ctype_flag && ctrans_flag){
                    if(DEBUG && 1) fprintf(stderr, "Mime: End of header, printing from here\n");
                    loc_flag = VALID_BODYPART_FOUND;
                    continue;
                }
                //Invalid body, go back to loc_flag 3 (looking for delimiter)
                else if(loc_flag != VALID_BODYPART_FOUND){
                    loc_flag = BODYPART_FOUND;
                }
            }

            if(loc_flag == VALID_BODYPART_FOUND){
                if(line_flag == 1){
                    printf("\r\n"); 
                    line_flag = 0; 
                }
                if(strcmp(lastline, "\r\n") == 0){
                    line_flag = 1;
                }
                else{
                    printf("%s", line);
                }
            }
        }
        if(read_bytes >= msg_bytes && line_num > 0){
            //Reached the end of email contents, read the last line and break
            getline(&line, &line_len, socketFile);
            break;
        }
        line_num++;
    }
    free(line);
    exit(0);
}


char* remove_quotes(char *str){
    int DEBUG = 0;

    //Removes quotations
    char cleaned_str[strlen(str)];
    char ch;
    int i = 9, j = 0;
    ch = str[i];
    while(ch != '\0' && ch != ';'){
        ch = str[i];
        if(ch != '"' && ch != ';'){
            cleaned_str[j] = ch;
            j++;
        }
        i++;
    }
    cleaned_str[j] = '\0';

    strcpy(str, cleaned_str);
    if(DEBUG && 1) fprintf(stderr, "remove_quotes: output is %s", str);
    return str;
}


void list(int sockfd){
    int DEBUG = 0; 
    char buffer[BUFFER];
    FILE *socketFile = fdopen(sockfd, "r");
    if (!socketFile) {
        perror("Failed to open socket file descriptor");
        exit(0);
    }

    // Command to fetch all email subjects
    snprintf(buffer, sizeof(buffer), "A04 FETCH 1:* (BODY[HEADER.FIELDS (SUBJECT)])\r\n");
    
    if (write(sockfd, buffer, strlen(buffer)) < 0) {
        perror("write");
        exit(EXIT_FAILURE);
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    char *headers = NULL;
    size_t headers_size = 0;
    int count =0;
    int email_count=0;
    int subject_count=0;

    // Read and process server responses
    while ((read = getline(&line, &len, socketFile)) != -1) {
        if (DEBUG) fprintf(stderr, "Debug:%s", line);

        
        //subject found, update subject count and add line to headers
        if(strncasecmp(line,"Subject:",HEADER_SUBJ_SIZE)==0){
            subject_count++;
            headers_size += strlen(line);
            headers = realloc(headers, headers_size + 1);
            assert(headers);

            if (headers_size == strlen(line)) {
                strcpy(headers, line);
            } 
            else {
                strcat(headers, line);
            }
        }

        //folding logic implemented
        if(line[0] == '\t' || line[0] == ' '){
            headers[headers_size - 2] = '\0';
            headers_size -= 2;
            headers_size += strlen(line);
            headers = realloc(headers, headers_size + 1);
            assert(headers);

            if (headers_size == strlen(line)) {
                strcpy(headers, line);
            } 
            else {
                strcat(headers, line);
            }
        }

        //end of email reached: update mail_count
        if(strncasecmp(line,")",1)==0){
            email_count++;
            //check empty email
            if(email_count!=subject_count){
                headers_size += strlen("Subject: <No subject>");
                headers = realloc(headers, headers_size + 1);
                assert(headers);
                strcat(headers, "Subject: <No subject>");
            }
        }

        if (strstr(line, "OK Fetch completed") != NULL) {
                break;
        }
    }

    if(email_count==0){
        exit(0);
    }

    //print each line of headers for debugging
    char *header_line = strtok(headers, "\r\n");  
    while (header_line != NULL) {
        if(strncasecmp(header_line,"Subject:",HEADER_SUBJ_SIZE) == 0){
            count++;
            char *trimmed_value = trim_header_value(header_line);
            if(strstr(header_line,"Subject: <No subject>")!=NULL){
                printf("%d: <No subject>\n",(count));
            }
            else{
                printf("%d:%s\n",count,trimmed_value);
            }
            free(trimmed_value);
        }
        header_line = strtok(NULL, "\r\n");  
    }
    free(headers);
    free(line);
    fclose(socketFile);
}
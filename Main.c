#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <poll.h>
#include <ctype.h>
#define BUF_SIZE 1024
#define MAX_EVENTS 10

// Decode a URL-encoded string (modifies the original string)
void url_decode(char *str)
{
    char *src = str, *dst = str;
    while (*src) {
        if (*src == '%' && isxdigit(src[1]) && isxdigit(src[2])) {
            if (src[1] == '2' && src[2] == '0') {
                *dst++ = ' ';
            } else {
                *dst++ = (char) strtol(src + 1, NULL, 16);
            }
            src += 3;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}
// Print an error message and exit
 void error(char *msg) {
    perror(msg);
    exit(1);
}
// Send file contents to socket
 int send_file(int sockfd, char *filepath) {
    // Open the file
    int fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        return -1;
    }
     // Determine the size of the file
    struct stat filestat;
    if (fstat(fd, &filestat) < 0) {
        close(fd);
        return -1;
    }
    off_t filesize = filestat.st_size;
     // Send HTTP response header
    char response[BUF_SIZE];
    snprintf(response, BUF_SIZE, "HTTP/1.1 200 OK\r\n"
             "Content-Type: application/octet-stream\r\n"
             "Content-Length: %ld\r\n"
             "Content-Disposition: attachment; filename=\"%s\"\r\n"
             "Connection: close\r\n\r\n", filesize, filepath);
    if (send(sockfd, response, strlen(response), 0) < 0) {
        close(fd);
        return -1;
    }
     // Send file contents
    char buf[BUF_SIZE];
    ssize_t nread = 0;
    while ((nread = read(fd, buf, BUF_SIZE)) > 0) {
        if (send(sockfd, buf, nread, 0) < 0) {
            close(fd);
            return -1;
        }
    }
    close(fd);
     return 0;
}


 int main(int argc, char *argv[]) {
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    char buffer[BUF_SIZE];
    struct sockaddr_in serv_addr, cli_addr;
    

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        error("ERROR opening socket");
    }
     // Bind socket to port
    memset(&serv_addr, 0, sizeof(serv_addr));
    portno =atoi(argv[1]);
    char *dirpath = argv[2];
    char *temp=(char*) malloc(strlen(dirpath) + 1);
    strcpy(temp,dirpath);
    strcpy(temp,dirpath);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        error("ERROR on binding");
    }

    // Listen for connections
    listen(sockfd, 5);
    printf("Listening on port %d...\n", portno);
    printf("Serving directory: %s\n", dirpath);
    printf("http://localhost:%d\n", portno);

    // Set up pollfd array
    struct pollfd fds[MAX_EVENTS];
    nfds_t nfds = 1;
    fds[0].fd = sockfd;
    fds[0].events = POLLIN;

    // Main loop
    while (1) {
        // Wait for events on sockets
        int nready = poll(fds, nfds, -1);
        if (nready < 0) {
            error("ERROR on poll");
        }
        // Check for new connection
        if (fds[0].revents & POLLIN) {
            clilen = sizeof(cli_addr);
            newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
            if (newsockfd < 0) {
                error("ERROR on accept");
            }
             // Add new socket to pollfd array
            fds[nfds].fd = newsockfd;
            fds[nfds].events = POLLIN;
            nfds++;
        }
        // Check for events on existing sockets
        for (int i = 1; i < nfds; i++) {
            if (fds[i].revents & POLLIN) {
                // Read HTTP request
                memset(buffer, 0, BUF_SIZE);
                int n = recv(fds[i].fd, buffer, BUF_SIZE - 1, 0);
                if (n < 0) {
                    error("ERROR reading from socket");
                }
                if (n == 0) {
                    // Connection closed by client
                    close(fds[i].fd);
                    nfds--;
                    fds[i].fd = -1;
                    continue;
                }
                 // Parse HTTP request
                char method[BUF_SIZE], path[BUF_SIZE], protocol[BUF_SIZE];
                sscanf(buffer, "%s %s %s", method, path, protocol);              
                url_decode(path);  // Decode URL-encoded filename

                 // Handle GET requests
                if (strcmp(method, "GET") == 0) {
                    //imprime path
                    printf("path: %s\n", path);

                    if (strcmp(path, "/") == 0 || path[strlen(path) - 1] == '/') {
                     // Send HTTP response header
                     char response[BUF_SIZE];
                     snprintf(response, BUF_SIZE, "HTTP/1.1 200 OK\r\n"
                             "Content-Type: text/html\r\n"
                             "Connection: close\r\n\r\n"
                             "<html><body><ul>");
                     if (send(fds[i].fd, response, strlen(response), 0) < 0) {
                        error("ERROR sending HTTP response header");
                     }

                     // Send directory listing
                     if(path[strlen(path) - 1] == '/'){
                        
                        //imprime temp
                        printf("temp: %s\n", temp);

                           
                        //concatena los strings dirpath con path detras de dirpath
                        dirpath = strcat(dirpath, path);                        
                     }

                     DIR *dir;
                     struct dirent *entry;
                     struct stat filestat;
                     if ((dir = opendir(dirpath)) != NULL) {
                         while ((entry = readdir(dir)) != NULL) {
                           char *filename = entry->d_name;
                         url_decode(filename);  // Decode URL-encoded filename
                         
                          if (strcmp(filename, ".") != 0 && strcmp(filename, "..") != 0) {
                             char filepath[BUF_SIZE];
                             snprintf(filepath, BUF_SIZE, "%s/%s", dirpath, filename);
            
                             if (stat(filepath, &filestat) == 0 && S_ISDIR(filestat.st_mode)) {
                                 snprintf(response, BUF_SIZE, "<li><a href=\"%s/\">%s/</a></li>", filename, filename);
                                } else {
                                 snprintf(response, BUF_SIZE, "<li><a href=\"%s\">%s</a></li>", filename, filename);
                                }
                             if (send(fds[i].fd, response, strlen(response), 0) < 0) {
                                 error("ERROR sending directory listing");
                                }
                            }
                        }
                        closedir(dir);
                        //imprime dirpath
                        printf("dirpath: %s\n", dirpath);
                        //imprime temp
                        printf("temp: %s\n", temp);
                        strcpy(dirpath,temp);
                        //imprime dirpath
                        printf("dirpath2: %s\n", dirpath);
                        } 
                        else {
                         error("ERROR opening directory");
                         }
               
                    
                     // Send HTML footer
                     snprintf(response, BUF_SIZE, "</ul></body></html>");
                     if (send(fds[i].fd, response, strlen(response), 0) < 0) {error("ERROR sending HTML footer");}    
                    } 
                    else if (strcmp(path, "/favicon.ico") == 0) {
                      // Ignore request for favicon.ico
                      close(fds[i].fd);
                      nfds--;
                      fds[i].fd = -1;
                      continue;
                    }
                
                    else {
                    //  char* lastDirectory;
                    //  lastDirectory = strrchr(path, '/');
                    //  if(lastDirectory != NULL) {
                    //  lastDirectory++;
                    //  } else {
                    //    lastDirectory = path;
                    //  }
                    //  //imprime lastDirectory
                    //     printf("lastDirectory: %s\n", lastDirectory);

                      
                        


                     // If a file is requested, send the file
                     char filepath[BUF_SIZE];
                     snprintf(filepath, BUF_SIZE, "%s%s", dirpath, path); 
                     //imprime filepath
                        printf("filepath: %s\n", filepath);
                        //imprime dirpath
                        printf("dirpath: %s\n", dirpath);
                        //imprime path
                        printf("path: %s\n", path);


                     if (send_file(fds[i].fd, filepath) < 0) {
                         error("ERROR sending file");
                        }
                    }
                }
                // Close socket and remove from pollfd array
                close(fds[i].fd);
                nfds--;
                for (int j = i; j < nfds; j++) {
                    fds[j] = fds[j + 1];
                }
                i--; // Decrement i so that next iteration will process the same index
            }
        }
    }
    close(sockfd);
    return 0;
}

//cambio radical otra
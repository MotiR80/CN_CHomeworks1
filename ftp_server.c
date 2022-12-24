#include <iostream>
#include <stdio.h>
#include <string>
#include <stdio.h>
#include <string.h>   //strlen
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>   //close
#include <arpa/inet.h>    //close
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <json/json.h>
#include <json/value.h>
#include <fstream>

#define F_SIZE 1048576  //1MB
#define True 1
#define False 0
#define PATH "../Files/"

using namespace std;

struct Client {
    int isAdmin = 0,
            isUser = 0,
            isLogin = 0,
            index = -1,
            free_space = 0,
            cmd_socket = 0;
    void reset(){
        isAdmin = 0;
        isUser = 0;
        isLogin = 0;
        index = -1;
        free_space = 0;
        cmd_socket = 0;
    }
};
struct CMD {
    string order;
    string arg;

    void arg_value(string s){
        CMD cmd;
        for (int i = 0; i < s.length(); ++i) {
            if (s[i] == ' '){
                arg = s.substr(0, i);
                order = s.substr(i+1, s.length() - i);
            }
            else cmd.order = s;
        }
    }
};

int username(string, Client &, Json::Value&);
int password(string, Client &, Json::Value&);
int upload(string, Client &, Json::Value&);
int download(int, struct sockaddr_in, string, Client &, Json::Value &);
string message(int);
int quit();
int help();


//void commands(CMD cmd, Json::Value &config){
//    int m;
//    if (cmd.order == "user"){
//        m = username(cmd.arg);
//        cout << message(m);
//    }
//    else if (cmd.order == "pass"){
//        m = password(cmd.arg);
//        cout << message(m);
//    }
//    else if (cmd.order== "retr"){
//        download(cmd.arg);
//        cout << message(m);
//    }
//    else if (cmd.order == "Upload"){
//        upload(cmd.arg);
//        cout << message(m);
//    }
//    else if (cmd.order == "help"){
//        help();
//        cout << message(m);
//    }
//    else if (cmd.order == "quit"){
//        quit();
//        cout << message(m);
//    }
//    else {
//        message(m);
//    }
//}

int main() {
    // creates a Files directory if it was not created before
    if (mkdir(PATH, S_IRWXG | S_IRWXU | S_IRWXO) == -1)
        cout << "Files directory already existing.\n";
    else
        cout << "Files directory created successfully.\n";

    // read json file
    ifstream configFile("../config.json");
    Json::Value config;
    Json::Reader reader;

    // using reader for parsing json file and store data in config
    reader.parse(configFile, config);

    // client commands will store in cmd
    CMD cmd;

    // get server ip and cmd_port and data_port from json file.
    const char* ip = config["server"]["ip"].asCString();
    int cmd_port = config["server"]["cmd_port"].asInt();
    int data_port = config["server"]["data_port"].asInt();

    int opt = True;
    int cmd_listener, data_listener , cmdLen , new_socket,
            dataLen, max_clients = 10 , activity, i , valread , sd;

    // server can handle maximum 10 clients
    Client client[10];
    int max_sd;

    struct sockaddr_in cmd_channel;
    struct sockaddr_in data_channel;

    // data buffer with size 1k
    char buffer[1024];

    // set socket descriptors
    fd_set readsds;

    // command channel socket type
    cmd_channel.sin_family = AF_INET;
    cmd_channel.sin_addr.s_addr = inet_addr((const char*)ip);
    cmd_channel.sin_port = htons(cmd_port);
    //----------------------------------------------------------------------------------------------

    // data channel socket type
    cmd_channel.sin_family = AF_INET;
    cmd_channel.sin_addr.s_addr = inet_addr((const char*)ip);
    cmd_channel.sin_port = htons(data_port);
    //----------------------------------------------------------------------------------------------

    //create master socket or listen socket descriptor
    if ((cmd_listener = socket(AF_INET, SOCK_STREAM, 0)) == 0){
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    if ((data_listener = socket(AF_INET, SOCK_STREAM, 0)) == 0){
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    //----------------------------------------------------------------------------------------------

    //make master socket address reusable
    if (setsockopt(cmd_listener, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,
                   sizeof(opt)) < 0){
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(data_listener, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,
                   sizeof(opt)) < 0){
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    //----------------------------------------------------------------------------------------------

    //bind master socket to ip and cmd_port from config.json
    if ((bind(cmd_listener, (struct sockaddr*)&cmd_channel, sizeof(cmd_channel))) < 0){
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if ((bind(data_listener, (struct sockaddr*)&data_channel, sizeof(data_channel))) < 0){
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    //-----------------------------------------------------------------------------------------------

    cout << "Listen on cmd_port: " << cmd_port<<'\n';
    cout << "Listen on data_port: " << data_port<<'\n';
    // listen on cmd_port and data_port with maximum 5 pending connection
    if ((listen(cmd_listener, 5)) < 0){
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if ((listen(data_listener, 5)) < 0){
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    //------------------------------------------------------------------------------------------------

    // accept the incoming connection
    cmdLen = sizeof(cmd_channel);
    puts("Waiting for connections ...");

    while (true){
        // first clear the socket discriptor set
        FD_ZERO(&readsds);

        // add master socket to socket discriptor set (readsds)
        FD_SET(cmd_listener, &readsds);
        max_sd = cmd_listener;

        // add valid client socket to the set
        for (int j = 0; j < max_clients; ++j) {
            // socket discriptor
            sd = client[j].cmd_socket;

            //if valid socket decriptor then add to read list
            if (sd > 0)
                FD_SET (sd, &readsds);

            //highest descriptor number needs for select function
            if (sd > max_sd)
                max_sd = sd;
        }
        //--------------------------------------------------------------------------------------------

        // wait for an activity on a socket
        activity = select(max_sd+1, &readsds, NULL, NULL, NULL);

        if ((activity < 0) && (errno!=EINTR))
        {
            printf("select error");
        }

        // if we see an activity on master socket
        if (FD_ISSET(cmd_listener, &readsds)){
            if ((new_socket = accept(cmd_listener, (struct sockaddr*)& cmd_channel, (socklen_t *)&cmdLen)) < 0){
                perror("accept: ");
                exit(EXIT_FAILURE);
            }
            cout << "New connection socket fd is " << new_socket << ", ip is " << inet_ntoa(cmd_channel.sin_addr) <<
                 ", cmd_port is " << ntohs(cmd_channel.sin_port);

            // add new_socket to array of client sockets
            for (int j = 0; j < max_clients; ++j) {
                if (client[j].cmd_socket == 0) {
                    client[j].cmd_socket = new_socket;
                    break;
                }
            }
        }
        //--------------------------------------------------------------------------------------------

        //else its some IO operation on some other socket
        for (i = 0; i < max_clients; i++)
        {
            sd = client[i].cmd_socket;

            if (FD_ISSET( sd , &readsds))
            {
                //Check if client closed the program , and also read the
                //incoming command
                if ((valread = read( sd , buffer, 1024)) == 0)
                {
                    //Somebody disconnected , get his details and print
                    getpeername(sd , (struct sockaddr*)&cmd_channel , (socklen_t *)(&cmdLen));
                    printf("Host disconnected , ip %s , cmd_port %d \n" ,
                           inet_ntoa(cmd_channel.sin_addr) , ntohs(cmd_channel.sin_port));


                    //Close the socket and mark as 0 in list for reuse
                    close( sd );
                    client[i].reset();
                }

                    //Echo back the message that came in
                else
                {
                    //set the string terminating NULL byte on the end
                    //of the data read
                    buffer[valread-2] = '\0';
                    cmd.arg_value(buffer);
//                    commands(cmd);
                }
            }
        }

    }
}


int username(string uname, Client &client, Json::Value &config){
    for (unsigned int i = 0; auto allUsers: config["users"]) {
        if (uname == allUsers["user"].asString()){
            client.isUser = 1;
            client.index = i;
            if (allUsers["isAdmin"].asInt() == 1)
                client.isAdmin = 1;
            return 331;
        }
        i++;
    }
    return 430;
}

int password(string pwd, Client &client, Json::Value &config){
    if (pwd == config["users"][client.index]["password"].asString()){
        client.isLogin = 1;
        client.free_space = config["users"][client.index]["space"].asInt();
        return 230;
    }
    else
        return 430;
}

int download(int fd, struct sockaddr_in data_chanedl, string url, Client &client, Json::Value &config) {
    // accept calling
    int new_socket = accept(fd, (struct sockaddr *) &data_chanedl, (socklen_t *) sizeof(data_chanedl));

    // char array with size 1MB
    char data[F_SIZE];

    // check file size
    struct stat st;
    stat((const char *) &url, &st);
    int size = st.st_size;

    if (size > F_SIZE) {
        close(new_socket);
        return (425);
    }
    else if (size > client.free_space){
        close(new_socket);
        return (425);
    }

    FILE *fp;
    // store file
    fp = fopen((const char*)&url, "r");
    if (fgets(data, F_SIZE, fp) != NULL)
        send(new_socket, data, strlen(data), 0);
    close(new_socket);
    return 226;
}

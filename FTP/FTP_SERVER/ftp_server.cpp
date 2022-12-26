#include <iostream>
#include <cstdio>
#include <string>
#include <cstring>   //strlen
#include <cstdlib>
#include <cerrno>
#include <unistd.h>   //close
#include <arpa/inet.h>    //close
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <json/json.h>
#include <json/value.h>
#include <dirent.h>
#include <fstream>

#define F_SIZE 1048576  //1MB
#define True 1
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

    void arg_value(string s){;
        for (int i = 0; i < s.length(); ++i) {
            if (s[i] == ' '){
                order = s.substr(0, i);
                arg = s.substr(i+1, s.length() - i);
                return;
            }
        }
        order = s;
    }

    void reset(){
        arg = "";
        order = "";
    };
};

int username(string, Client &, Json::Value&);
int password(string, Client &, Json::Value&);
int upload(int, struct sockaddr_in, string, Client &, Json::Value &);
int download(int, struct sockaddr_in, string, Client &, Json::Value &);
string message(int);
string List_Of_Files(string, Client &, Json::Value &);
int help();
int remove(string, Client &, Json::Value &);
string commands(CMD, int, struct sockaddr_in, Client&, Json::Value&);
string currentDateTime();
void writeIntoLogFile(string);
int Make_Dir(string, Client &, Json::Value &);



int main() {
    // create or open log file
    writeIntoLogFile(currentDateTime()+ "Server runs.");

    // creates a Files directory if it was not created before
    if (mkdir(PATH, S_IRWXG | S_IRWXU | S_IRWXO) == -1)
        cout << "Files directory already existing.\n";
    else {
        cout << "Files directory created successfully.\n";
        writeIntoLogFile(currentDateTime()+ "Files directory created.");
    }



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
        max_clients = 10 , activity, i , valread , sd;

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
    data_channel.sin_family = AF_INET;
    data_channel.sin_addr.s_addr = inet_addr((const char*)ip);
    data_channel.sin_port = htons(data_port);
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
        perror("bind1 failed");
        exit(EXIT_FAILURE);
    }
    if ((bind(data_listener, (struct sockaddr*)&data_channel, sizeof(data_channel))) < 0){
        perror("bind2 failed");
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
                 ", cmd_port is " << ntohs(cmd_channel.sin_port)<<'\n';

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

                    // update log file
                    writeIntoLogFile(currentDateTime() + config["users"][client[i].index]["user"].asString()
                                     +" logged out and quit the server.");

                    client[i].reset();
                }

                    //Echo back the message that came in
                else
                {
                    string msg;

                    //set the string terminating NULL byte on the end
                    //of the data read
                    buffer[valread-2] = '\0';
                    cmd.arg_value(buffer);
                    bzero(buffer, 1024);

                    if (cmd.order == "quit"){
                        msg = message(221);
                        strcpy(buffer, msg.c_str());
                        send(sd, buffer, sizeof(msg), 0);
                        close(sd);

                        // update log file
                        writeIntoLogFile(currentDateTime() + config["users"][client[i].index]["user"].asString()
                                         +" logged out and quit the server.");

                        client[i].reset();
                    }
                    else {
                        msg = commands(cmd, data_listener, data_channel, client[i], config);
                        strcpy(buffer, msg.c_str());
                        send(sd, buffer, strlen(buffer), 0);
                    }
                }
            }
            cmd.reset();
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
    if (client.isUser != 1)
        return 503;
    else if (pwd == config["users"][client.index]["password"].asString()){
        client.isLogin = 1;
        client.free_space = config["users"][client.index]["space"].asInt()*1024;
        writeIntoLogFile(currentDateTime() + config["users"][client.index]["user"].asString()+" logged in.");
        return 230;
    }
    else
        return 430;
}

int download(int fd, struct sockaddr_in data_channel, string url, Client &client, Json::Value &config) {

    // accept calling
    int new_socket;
    if((new_socket = accept(fd, (struct sockaddr *) &data_channel, (socklen_t *) sizeof(data_channel))) < 0){
        return 500;
    }

    // check the client is log in
    if (client.isLogin != 1){
        close(new_socket);
        return 332;
    }

    if (client.isAdmin != 1){
        for (auto f: config["admin files"]) {
            if (url == f.asString()) {
                close(new_socket);
                return 550;
            }
        }
    }

    string addressFile = PATH + url;

    // check if file is valid then get file size
    struct stat st;
    if (stat((const char *) &addressFile, &st) == -1) {
        close(new_socket);
        return (425);
    }
    int size = st.st_size;

    // check file size less than restriction
    if (size > F_SIZE) {
        close(new_socket);
        return (425);
    }
    else if (size > client.free_space){
        close(new_socket);
        return (425);
    }

    // send file size
    char buff[10];
    sprintf(buff,"%ld", size);
    send(new_socket, buff, 10, 0);

    // read file and send
    char data[size];
    FILE *fp;

    fp = fopen((const char*)&addressFile, "r");
    fread(data, 1, size, fp);
    send(new_socket, data, strlen(data), 0);
    client.free_space -= size;
    fclose(fp);
    close(new_socket);

    // update client free space in config file
    ofstream m_file("../config.json");
    config["users"][client.index]["remaining_space"] = client.free_space/1024;

    // update log file
    writeIntoLogFile(currentDateTime() + config["users"][client.index]["user"].asString()+" downloaded "+ url+'.');

    return 226;
}

int upload(int fd, struct sockaddr_in data_channel, string url, Client &client, Json::Value &config){

    // accept the call
    int new_socket;
    if((new_socket = accept(fd, (struct sockaddr *) &data_channel, (socklen_t *) sizeof(data_channel))) < 0){
        return 500;
    }

    // check the client is log in
    if (client.isLogin != 1){
        close(new_socket);
        return 332;
    }

    // check if the client is admin
    if (client.isAdmin == 0){
        close(new_socket);
        return 550;
    }

    char buff[10];
    unsigned int fsize;
    string file_addr = PATH + url;

    // get file size first
    if ((read(new_socket, buff, 10)) <= 0){
        perror("read size: ");
        close(new_socket);
        return 500;
    };

    // check if file size less than acceptable file size
    fsize = atoi(buff);
    if ( fsize > F_SIZE){
        close(new_socket);
        return 425;
    }

    char data[fsize];

    // read the file
    if(read(new_socket, data, fsize) <= 0){
        perror("read file: ");
        close(new_socket);
        return 500;
    }

    // store the file in Files directory
    FILE *fp;
    fp = fopen((const char*)&file_addr, "w");
    fwrite(data, 1, fsize, fp);
    fclose(fp);
    close(new_socket);

    // update log file
    writeIntoLogFile(currentDateTime() + config["users"][client.index]["user"].asString()+" uploaded "+ url+'.');

    return 226;
}

int remove(string url, Client &client, Json::Value &config){
    if (client.isAdmin) {
        string faddr = PATH + url;
        char buffer[30];
        strcpy(buffer, faddr.c_str());
        if (remove(buffer) != 0)
            return 500;
        else {

            writeIntoLogFile(currentDateTime() + url+" removed by admin("+config["users"][client.index]["user"].asString()+").");
            return 225;
        }
    }
    return 550;
}

string List_Of_Files(string url, Client &client, Json::Value &config){
    string fnames;
    // check the client is log in
    if (client.isLogin != 1){
        fnames = message(332);
        return fnames;
    }
    char faddr[30];
    if (url.length() != 0)
        strcpy(faddr, (PATH+url).c_str());
    else
        strcpy(faddr, PATH);

    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir (faddr)) != NULL) {
        /* print all the files and directories within directory */
        while ((ent = readdir (dir)) != NULL) {
            string temp(ent->d_name);
            if (temp == "." || temp == "..")
                continue;
            fnames += temp + '\n';
        }
        closedir (dir);

        if (fnames.length() == 0)
            return "There is no files in this directory.\n";
        return fnames;
    }
    else {
        // No files or could not open the directory
        return "The directory does not exist\n";
    }
}

int help(){
    return 214;
}

string message(int codeNo){
    switch (codeNo) {
        case 331: return "331: Username okay, need password.\n";
        case 503: return "503: Bad sequence of commands.\n";
        case 230: return "230: User logged in, proceed. Logged out if appropriate.\n";
        case 430: return "430: Invalid username or password.\n";
        case 550: return "550: File unavailable.\n";
        case 226: return "226: Successful Download.\n";
        case 332: return "332: Need account for login.\n";
        case 221: return "221: Successful quit.\n";
        case 501: return "501: Syntax error in parameters or arguments.\n";
        case 500: return "500: Error\n";
        case 425: return "425: Can't open data connection.\n";
        case 404: return "404: Files directory already existing.\n";
        case 224: return "224: Files directory created successfully.\n";
        case 225: return "225: File successfully deleted\n";
        case 214: return "214:\n\nuser [name]\t\t\tIt's an argument is used to specify the users string. "
                         "It is used for user authentication.\n"
                         "pass [password]\t\t\tIt's an argument is used to specify the passwords string. "
                         "It is used for user authentication.\n"
                         "retr [file name/url]\t\tIt's an argument is used to specify the file url. "
                         "It is used for download authorized files.\n"
                         "upload [file name/url]\t\tIt's an argument is used to specify the file url. "
                         "Admin can upload files on server with this command.\n"
                         "quit\t\t\t\tThe user will be disconnected from the server and the socket connection will be closed.\n"
                         "help\t\t\t\tShow commands.\n";
        default:
             return "500: Error\n";
    }
}

string commands(CMD cmd, int fd, struct sockaddr_in data_channel, Client &client, Json::Value &config){
    int codeNo;
    if (cmd.order == "user"){
        codeNo = username(cmd.arg, client, config);
    }
    else if (cmd.order == "pass"){
        codeNo = password(cmd.arg, client, config);
    }
    else if (cmd.order== "retr"){
        codeNo = download(fd, data_channel, cmd.arg, client, config);
    }
    else if (cmd.order == "upload"){
        codeNo = upload(fd, data_channel, cmd.arg, client, config);
    }
    else if (cmd.order == "help"){
        codeNo = help();
    }
    else if (cmd.order == "remove"){
        codeNo = remove(cmd.arg, client, config);
    }
    else if (cmd.order == "ls") {
        return List_Of_Files(cmd.arg, client, config);
    }
    else if (cmd.order == "mkdir") {
        codeNo =  Make_Dir(cmd.arg, client, config);
    }
    else{
        codeNo = 501;
    }

    return message(codeNo);
}

string currentDateTime() {
    time_t     now = time(&now);
    struct tm  *tstruct;
    char       buf[80];
    tstruct = gmtime(&now);
    tstruct->tm_hour = (tstruct->tm_hour+3);
    tstruct->tm_min = (tstruct->tm_min +30);

    if ((tstruct->tm_min)/60 >= 1){
        tstruct->tm_min = (tstruct->tm_min)%60;
        tstruct->tm_hour += 1;
    }
    if ((tstruct->tm_hour)/24 >= 1){
        tstruct->tm_hour = (tstruct->tm_hour)%24;
        tstruct->tm_mday += 1;
    }
    if ((tstruct->tm_mday)/30 >= 1){
        tstruct->tm_mday = (tstruct->tm_mday)%30;
        tstruct->tm_mon += 1;
    }
    if ((tstruct->tm_mon)/12 >= 1){
        tstruct->tm_mon = (tstruct->tm_mon)%12;
        tstruct->tm_year += 1;
    }

    strftime(buf, sizeof(buf), "%Y-%m-%d %X --> ", tstruct);

    return buf;
}

int Make_Dir(string url, Client &client, Json::Value &config){
    // check the client is log in
    if (client.isLogin != 1){
        return 332;
    }
    string faddr = PATH+url;
    char buffer[30];
    strcpy(buffer, faddr.c_str());

    if (mkdir(buffer, S_IRWXG | S_IRWXU | S_IRWXO) == -1)
        return 404;
    else {
        writeIntoLogFile(currentDateTime()+ url + " directory created by " +config["users"][client.index]["user"].asString()+'.');
        return 224;
    }
}

void writeIntoLogFile(string pm){
    ofstream log;
    log.open("../log.log", ios::app);
    if (log.is_open()){
        log << pm <<'\n';
    }
    log.close();
}

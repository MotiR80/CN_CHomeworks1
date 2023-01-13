# FTP

### A simple FTP server and client program.

## Server Code
A struct to save clients' information.
```cpp
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
```

`isLogin` is used to check whether the username is correct or not. `isLogin` is used to check client is logged in or not. `isAdmin` check whether the client is admin or not. `index` is the index of the client in the config.json file. `free_space` is remaining space of client. `cmd_socket` is the id of client socket.

A struct to parsing clients' commands.
```cpp
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
```
###
```cpp
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
```
The function `void writeIntoLogFile(string);` make a log file if not exist and write a message to it.
After writing into the log file we make a Files directory if not exist.

###

```cpp
// read json file
    ifstream configFile("../config.json");
    Json::Value config;
    Json::Reader reader;

    // using reader for parsing json file and store data in config
    reader.parse(configFile, config);
```
Read the config.json file and parse it into `config`.

###

```cpp
// client commands will store in cmd
    CMD cmd;
```

###

```cpp
// get server ip and cmd_port and data_port from json file.
    const char* ip = config["server"]["ip"].asCString();
    int cmd_port = config["server"]["cmd_port"].asInt();
    int data_port = config["server"]["data_port"].asInt();
```
Extract IP, command port, and data port from `config`.

###

```cpp
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

    // bind master socket to ip and cmd_port
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

    // listen on cmd_port and data_port with maximum 5 pending connection
    if ((listen(cmd_listener, 5)) < 0){
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    cout << "Listen on data_port: " << data_port<<'\n';
    if ((listen(data_listener, 2)) < 0){
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    //------------------------------------------------------------------------------------------------

    // accept the incoming connection
    cmdLen = sizeof(cmd_channel);
    puts("Waiting for connections ...");

    memset(&cmd_channel, 0, sizeof(cmd_channel));

```
Create two sockets, one for command (`cmd_listener`) and another for data (`data_listener`). Then bind and listen.

We create a file descriptor (`fd_set readsds;`) to handle clients with `select()`.

###

```cpp
    while (true){
        // first clear the socket descriptor set
        FD_ZERO(&readsds);

        // add master socket to socket descriptor set (readsds)
        FD_SET(cmd_listener, &readsds);
        max_sd = cmd_listener;

        // add valid client socket to the set
        for (int j = 0; j < max_clients; ++j) {
            // socket descriptor
            sd = client[j].cmd_socket;

            //if valid socket descriptor then add to read list
            if (sd > 0)
                FD_SET (sd, &readsds);

            //highest descriptor number needs for select function
            if (sd > max_sd)
                max_sd = sd;
        }
        //--------------------------------------------------------------------------------------------
```
In an infinity loop first, we clear our file descriptor(fd) set `FD_ZERO(&readsds);`,  then add `cmd_listener` to the fd set to accept new calls, then we check if a client was already connected to the server then add its socket to the fd set to read its commands.

###

```cpp
// wait for an activity on a socket
        activity = select(max_sd+1, &readsds, NULL, NULL, NULL);

        if ((activity < 0) && (errno!=EINTR))
        {
            printf("select error");
        }
```
Wait for an activity. For example a new call or command from a client.

###

```cpp
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
```
If we see new call on `cmd_listener` then we accept it on `new_socket` and add it to the client list `client[j].cmd_socket = new_socket;`

###

```cpp
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
                    if (client[i].index != -1)
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
                    buffer[valread] = '\0';
                    cmd.arg_value(buffer);
                    bzero(buffer, 1024);

                    if (cmd.order == "quit"){
                        msg = message(221);
                        strcpy(buffer, msg.c_str());
                        send(sd, buffer, sizeof(msg), 0);
                        //Somebody disconnected , get his details and print
                        getpeername(sd , (struct sockaddr*)&cmd_channel , (socklen_t *)(&cmdLen));
                        printf("Host disconnected , ip %s , cmd_port %d \n" ,
                               inet_ntoa(cmd_channel.sin_addr) , ntohs(cmd_channel.sin_port));
                        close(sd);

                        if (client[i].isLogin)
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
```
Else if we see a new command then read it. `if ((valread = read( sd , buffer, 1024)) == 0)`.

If `valread == 0` this means the client closed the program so we close its socket and update our log file and reset client information to initial values.
Else we read its command. If `cmd.order == "quit"` then we send a successful quit message `(msg = message(221);)` then we close its socket and update the log file. Else we call the `commands()` function to decide what should we do. Then send the result: Error or success message.

###

```cpp
string commands(CMD cmd, int fd, struct sockaddr_in& data_channel, Client &client, Json::Value &config){
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
```
The `commands()` function decides what should do base on the client command: `upload()`, `download()`, `username()`, `password()`, ... or syntax error. Then return the result as a string.
`codeNo` is the error/success code number that is sent as the argument of the `message(codeNo)` function.

###

```cpp
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
```
`username()` function checks whether the username exists or not. If it was correct then check if it is admin or not. Then it returns the error/success code number.

###

```cpp
int password(string pwd, Client &client, Json::Value &config){
    if (client.isUser != 1)
        return 503;
    else if (pwd == config["users"][client.index]["password"].asString()){
        client.isLogin = 1;
        client.free_space = config["users"][client.index]["remaining_space"].asInt()*1024;
        writeIntoLogFile(currentDateTime() + config["users"][client.index]["user"].asString()+" logged in.");
        return 230;
    }
    else
        return 430;
}
```
Check password is correct or not then returns the error/success code number.

###

```cpp
string List_Of_Files(string url, Client &client);
```
This function returns the list of all files in a directory like ls in Linux.

###

```cpp
int help(){
    return 214;
}
```
Return all commands and how to use them.

###

```cpp
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
        case 505: return "505: File doesn't exist or ...\n";
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
```
The `string message(int codeNo)` function gets the error/success code number and then returns the corresponding message.

###

```cpp
string currentDateTime();
```
This function returns the current date and time which use for the log file.

###

```cpp
int Make_Dir(string url, Client &client, Json::Value &config);
```
This function makes a directory like mkdir in Linux.

###

```cpp
void writeIntoLogFile(string pm);
```
Write into the log file.

###

```cpp
string fileName(string str){
    const size_t last_slash_idx = str.find_last_of("\\/");
    if (std::string::npos != last_slash_idx)
    {
        str.erase(0, last_slash_idx + 1);
    }
    return str;
}
```
Extract a file name with Extention from a path.

###

```cpp
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
```
This function removes the file like rm in Linux.
Only the admin can do this.

###

```cpp
int download(int fd, struct sockaddr_in& data_channel, string url, Client &client, Json::Value &config) {

    // accept calling
    int new_socket;
    int dataLen = sizeof(data_channel);
    if((new_socket = accept(fd, (struct sockaddr *) &data_channel, (socklen_t *)& dataLen)) < 0){
        perror("accept ");
        return 500;
    }

    getpeername(new_socket , (struct sockaddr*)&data_channel , (socklen_t *)(&dataLen));
    cout << "New data connection socket sd is " << new_socket << ", ip is " << inet_ntoa(data_channel.sin_addr) <<
         ", cmd_port is " << ntohs(data_channel.sin_port)<<'\n';

    // check the client is log in
    if (client.isLogin != 1){
        close(new_socket);
        return 332;
    }

    if (client.isAdmin != 1){
        for (auto f: config["admin files"]) {
            if (url == f["file url"].asString()) {
                close(new_socket);
                return 550;
            }
        }
    }

    // extract file name from path
    string addressFile = PATH + url;
    char fPath[30];
    strcpy(fPath, addressFile.c_str());

    // check if file is valid then get file size
    struct stat st;
    if (stat(fPath, &st) == -1) {
        perror("not exist ");
        close(new_socket);
        return (505);
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

    // read file and send
    char data[size];

    FILE *fp;
    fp = fopen(fPath, "r");
    int nb;

    while (!feof(fp)){
        nb = fread(data, 1, BUFFSIZE, fp);
        write(new_socket, data, nb);
    }

    // update config file
    client.free_space -= size;
    fclose(fp);
    close(new_socket);

    // update client free space in config file
    ofstream m_file("../config.json");
    config["users"][client.index]["remaining_space"] = client.free_space/1024;
    m_file << config;
    m_file.close();

    // update log file
    writeIntoLogFile(currentDateTime() + config["users"][client.index]["user"].asString()+" downloaded "+ url+'.');

    return 226;
}
```
The `download()` function sends the file to the client from the data socket.
First we create a data connection `new_socket = accept(fd, (struct sockaddr *) &data_channel, (socklen_t *)& dataLen)) < 0)`.
Then check if the client is logged in `if (client.isLogin != 1)`.
After that we check client is admin or not, we check this because the client cannot access some files like `config.json`. Then we get the file size `int size = st.st_size;` and check if it is less than `client.free_space`. After this, if there is no problem we open the file and read the data and send it. Finally, we subtract the file size from the `client.free_ space` and update the `config.json` and the log file.

###

```cpp
int upload(int fd, struct sockaddr_in& data_channel, string url, Client &client, Json::Value &config){
    memset(&data_channel, 0, sizeof(data_channel));
    // accept the call
    int new_socket;
    int dataLen = sizeof(data_channel);
    if((new_socket = accept(fd, (struct sockaddr *) &data_channel, (socklen_t *)& dataLen)) < 0){
        perror("accept ");
        return 500;
    }

    getpeername(new_socket , (struct sockaddr*)&data_channel , (socklen_t *)(&dataLen));
    cout << "New data connection socket sd is " << new_socket << ", ip is " << inet_ntoa(data_channel.sin_addr) <<
         ", cmd_port is " << ntohs(data_channel.sin_port)<<'\n';

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
    int fsize;
    string file_addr = PATH + url;

    int valread;

    // get file size first
    if ((valread = recv(new_socket, buff, 10, 0)) <= 0){
        perror("read size ");
        close(new_socket);
        return 505;
    }

    // check if file size less than acceptable file size
    fsize = stoi(buff);
    if ( fsize > F_SIZE){
        close(new_socket);
        return 425;
    }

    // extract file name from path
    string fName = fileName(url);
    char fPath[30];
    strcpy(fPath, (PATH+fName).c_str());

    char data[BUFFSIZE];
    FILE *fp;
    fp = fopen(fPath, "w");

    // read the data and store in Files directory
    int nb = read(new_socket, data, BUFFSIZE);
    while (nb > 0){
        fwrite(data, 1, nb, fp);
        nb = read(new_socket, data, BUFFSIZE);
    }

    fclose(fp);
    close(new_socket);

    // update log file
    writeIntoLogFile(currentDateTime() + config["users"][client.index]["user"].asString()+" uploaded "+ url+'.');

    return 226;
}
```
The structure of `upload()` function is like `download()` function but the opposite of download, reads the data from client and stores it in the File directory. Only the admin can upload a file.
In this function first we receive file size from the client `(valread = recv(new_socket, buff, 10, 0)) <= 0)` then check the file size `if ( fsize > F_SIZE)` if it is less than the restriction we start to read data from the client.

## Client Code
```cpp
int main() {

      // creates a downloads directory if it was not created before
    if (mkdir(PATH, S_IRWXG | S_IRWXU | S_IRWXO) == -1)
        cout << "downloads directory already existing.\n";
    else {
        cout << "downloads directory created successfully.\n";
    }
```
Most of the client code is like server code with a little difference. First, we make a downloads directory if doesn't exist.

###

```cpp
if(connect(cmd_socket, (struct sockaddr*)&cmd_addr, sizeof(cmd_addr)) < 0)
```
We make two sockets like server codes then we use `connect()` to connect the server.

###

```cpp
 do {

        // get input and store in cmd
        getline(cin, command);
        cmd.get(command);
```
In a do-while loop, we get input and parse it.
Then send it to the server.

###

```cpp
string fileName(string);
void download(struct sockaddr_in&, string);
void upload(struct sockaddr_in&, string);
```
We have three functions in client code that were in server code.

## Client commands syntax
`user [username]` for entering username.

`pass [password]` for entering username.

`retr [path]` for downloading a file.

`upload [path]` for uploading a file.

`ls [path]` to get the list of files.

`mkdir [path/foldername]` to make a folder.

`help` to get the list of commands.

`rm [path]` to remove a file or folder.

## Example


#include <iostream>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sstream>
#include <csignal>
#include <sys/wait.h>
#include <ctime>
#include <signal.h>

using namespace std;

bool running = true;
int timeout = -1;

// Function to split a string by spaces into a vector of strings
vector<string> split(const string &str)
{
    vector<string> result;
    istringstream iss(str);
    for (string s; iss >> s;)
    {
        result.push_back(s);
    }
    return result;
}

// Function to handle a client connection for input redirection
void handle_client_input(int client_sock)
{
    dup2(client_sock, STDIN_FILENO);
}

// Function to handle a client connection for output redirection
void handle_client_output(int client_sock)
{
    dup2(client_sock, STDOUT_FILENO);
    dup2(client_sock, STDERR_FILENO); // Also redirect stderr to the client socket
}

// Function to start a TCP server
int start_tcp_server(const string &port)
{
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0)
    {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("Error setting socket options");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr = {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(stoi(port));
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Error binding socket");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    if (listen(server_sock, 1) < 0)
    {
        perror("Error listening on socket");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    return server_sock;
}

// Function to start a TCP client
int start_tcp_client(const string &hostname, const string &port)
{
    int client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sock < 0)
    {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    struct hostent *server = gethostbyname(hostname.c_str());
    if (server == nullptr)
    {
        cerr << "Error: No such host" << endl;
        close(client_sock);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr = {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(stoi(port));
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    cout << "Connecting to " << hostname << " on port " << port << endl;
    if (connect(client_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Error connecting to server");
        close(client_sock);
        exit(EXIT_FAILURE);
    }
    cout << "Connected to server" << endl;
    return client_sock;
}

// Function to start a UDP server
int start_udp_server(const string &port)
{
    int server_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_sock < 0)
    {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr = {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(stoi(port));
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Error binding socket");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    return server_sock;
}

// Function to start a UDP client
int start_udp_client(const string &hostname, const string &port, struct sockaddr_in &server_addr)
{
    int client_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_sock < 0)
    {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    struct hostent *server = gethostbyname(hostname.c_str());
    if (server == nullptr)
    {
        cerr << "Error: No such host" << endl;
        close(client_sock);
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(stoi(port));
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);

    return client_sock;
}

void signal_handler(int signal)
{
    if (signal == SIGINT)
    {
        running = false;
    }
}

void alarm_handler(int signal)
{
    if (signal == SIGALRM)
    {
        running = false;
    }
}

int main(int argc, char *argv[])
{
    signal(SIGINT, signal_handler); // Handle Ctrl+C to terminate the program gracefully
    signal(SIGALRM, alarm_handler); // Handle alarm signal for timeout

    int opt;
    char *program = nullptr;
    string input_redirect, output_redirect;

    // Using getopt to parse the command-line arguments
    while ((opt = getopt(argc, argv, "e:i:o:b:t:")) != -1)
    {
        switch (opt)
        {
            case 'e':
                program = optarg;
                break;
            case 'i':
                input_redirect = optarg;
                break;
            case 'o':
                output_redirect = optarg;
                if (!program)
                {
                    input_redirect = optarg;
                }
                break;
            case 'b':
                input_redirect = optarg;
                output_redirect = optarg;
                break;
            case 't':
                timeout = stoi(optarg);
                break;
            default:
                cerr << "Usage: " << argv[0]
                     << " -e <program> [args] [-i <input_redirect>] [-o <output_redirect>] [-b <bi_redirect>] [-t <timeout>]" << endl;
                return EXIT_FAILURE;
        }
    }

    if (timeout > 0)
    {
        alarm(timeout);
    }

    if (program)
    { // If the program name is provided, execute it with redirections if specified
        cout << "The input was: " << input_redirect << endl;
        cout << "The output was: " << output_redirect << endl;

        // Add "./" to the beginning of the program name
        string program_str = "./" + string(program);

        vector<string> split_program = split(program_str); // Split the program name and arguments
        vector<char *> args;                               // Vector of char* to store the arguments for execvp
        for (const auto &arg : split_program)
        { // Convert the arguments to char* and store in the vector
            args.push_back(const_cast<char *>(arg.c_str()));
        }
        args.push_back(NULL);

        pid_t pid = fork();
        if (pid < 0)
        {
            perror("Error forking process"); // If fork fails, print error and exit
            return EXIT_FAILURE;
        }

        if (pid == 0)
        { // Child process
            if (!input_redirect.empty())
            {
                if (input_redirect.substr(0, 4) == "TCPS")
                {
                    int port = stoi(input_redirect.substr(4));
                    int server_sock = start_tcp_server(to_string(port));
                    int client_sock = accept(server_sock, nullptr, nullptr);
                    if (client_sock < 0)
                    {
                        perror("Error accepting connection");
                        close(server_sock);
                        return EXIT_FAILURE;
                    }
                    handle_client_input(client_sock);
                    if (input_redirect == output_redirect)
                    {
                        handle_client_output(client_sock);
                    }
                    close(server_sock);
                }
                else if (input_redirect.substr(0, 4) == "UDPS")
                {
                    int port = stoi(input_redirect.substr(4));
                    int server_sock = start_udp_server(to_string(port));
                    struct sockaddr_in client_addr = {};
                    socklen_t client_len = sizeof(client_addr);
                    char buffer[1024];
                    ssize_t n = recvfrom(server_sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &client_len);
                    if (n < 0)
                    {
                        perror("Error receiving UDP message");
                        close(server_sock);
                        return EXIT_FAILURE;
                    }
                    handle_client_input(server_sock);
                }
            }

            if (!output_redirect.empty() && output_redirect != input_redirect)
            {
                if (output_redirect.substr(0, 4) == "TCPS")
                {
                    int port = stoi(output_redirect.substr(4));
                    int server_sock = start_tcp_server(to_string(port));
                    int client_sock = accept(server_sock, nullptr, nullptr);
                    if (client_sock < 0)
                    {
                        perror("Error accepting connection");
                        close(server_sock);
                        return EXIT_FAILURE;
                    }
                    handle_client_output(client_sock);
                    close(server_sock);
                }
                else if (output_redirect.substr(0, 4) == "TCPC")
                {
                    string host_port = output_redirect.substr(4);
                    size_t comma_pos = host_port.find(',');
                    if (comma_pos != string::npos)
                    {
                        string hostname = host_port.substr(0, comma_pos);
                        string port = host_port.substr(comma_pos + 1);
                        int client_sock = start_tcp_client(hostname, port);
                        handle_client_output(client_sock);
                    }
                    else
                    {
                        cerr << "Invalid TCPC format. Expected TCPC<hostname,port>" << endl;
                        return EXIT_FAILURE;
                    }
                }
                else if (output_redirect.substr(0, 4) == "UDPC")
                {
                    string host_port = output_redirect.substr(4);
                    size_t comma_pos = host_port.find(',');
                    if (comma_pos != string::npos)
                    {
                        string hostname = host_port.substr(0, comma_pos);
                        string port = host_port.substr(comma_pos + 1);
                        struct sockaddr_in server_addr;
                        int client_sock = start_udp_client(hostname, port, server_addr);
                        handle_client_output(client_sock);
                        char buffer[1024];
                        ssize_t n;
                        while ((n = read(STDIN_FILENO, buffer, sizeof(buffer) - 1)) > 0)
                        {
                            buffer[n] = '\0';
                            sendto(client_sock, buffer, n, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
                        }
                        close(client_sock);
                    }
                    else
                    {
                        cerr << "Invalid UDPC format. Expected UDPC<hostname,port>" << endl;
                        return EXIT_FAILURE;
                    }
                }
            }

            // Set the buffer to be line-buffered
            setvbuf(stdout, nullptr, _IOLBF, BUFSIZ);

            // Executing the specified program with the given arguments
            execvp(args[0], args.data());
            // execvp only returns if an error occurs
            perror("Error executing program");
            return EXIT_FAILURE;
        }
        else
        { // Parent process
            int status;
            while (running && waitpid(pid, &status, WNOHANG) == 0)
            {
                sleep(1);
            }
            if (!running)
            {
                kill(pid, SIGTERM);
            }
        }
    }
    else
    { // If no program is provided, set up a live chat between two terminals
        if (!input_redirect.empty())
        {
            if (input_redirect.substr(0, 4) == "TCPS")
            {
                int port = stoi(input_redirect.substr(4));
                int server_sock = start_tcp_server(to_string(port));
                int client_sock = accept(server_sock, nullptr, nullptr);
                if (client_sock < 0)
                {
                    perror("Error accepting connection");
                    close(server_sock);
                    return EXIT_FAILURE;
                }
                cout << "The chosen port is: " << port << " and option " << argv[2] << endl;
                cout << "Connected to client" << endl;
                cout << "Enter message to send to client" << endl;
                fd_set read_fds;
                char buffer[1024];
                ssize_t n;

                while (running)
                {
                    FD_ZERO(&read_fds);
                    FD_SET(client_sock, &read_fds);
                    FD_SET(STDIN_FILENO, &read_fds);

                    int max_fd = max(client_sock, STDIN_FILENO) + 1;
                    int activity = select(max_fd, &read_fds, nullptr, nullptr, nullptr);

                    if (activity < 0 && errno != EINTR)
                    {
                        perror("Error in select");
                        break;
                    }

                    if (FD_ISSET(client_sock, &read_fds))
                    {
                        n = read(client_sock, buffer, sizeof(buffer) - 1);
                        if (n <= 0)
                        {
                            break;
                        }
                        buffer[n] = '\0';
                        cout << buffer << flush;
                    }

                    if (FD_ISSET(STDIN_FILENO, &read_fds))
                    {
                        n = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
                        if (n <= 0)
                        {
                            break;
                        }
                        buffer[n] = '\0';
                        write(client_sock, buffer, n);
                    }
                }

                close(client_sock);
                close(server_sock);
            }
            else if (input_redirect.substr(0, 4) == "TCPC")
            {
                string host_port = input_redirect.substr(4);
                size_t comma_pos = host_port.find(',');
                if (comma_pos != string::npos)
                {
                    string hostname = host_port.substr(0, comma_pos);
                    string port = host_port.substr(comma_pos + 1);
                    int client_sock = start_tcp_client(hostname, port);

                    fd_set read_fds;
                    char buffer[1024];
                    ssize_t n;

                    while (running)
                    {
                        FD_ZERO(&read_fds);
                        FD_SET(client_sock, &read_fds);
                        FD_SET(STDIN_FILENO, &read_fds);

                        int max_fd = max(client_sock, STDIN_FILENO) + 1;
                        int activity = select(max_fd, &read_fds, nullptr, nullptr, nullptr);

                        if (activity < 0 && errno != EINTR)
                        {
                            perror("Error in select");
                            break;
                        }

                        if (FD_ISSET(client_sock, &read_fds))
                        {
                            n = read(client_sock, buffer, sizeof(buffer) - 1);
                            if (n <= 0)
                            {
                                break;
                            }
                            buffer[n] = '\0';
                            cout << buffer << flush;
                        }

                        if (FD_ISSET(STDIN_FILENO, &read_fds))
                        {
                            n = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
                            if (n <= 0)
                            {
                                break;
                            }
                            buffer[n] = '\0';
                            write(client_sock, buffer, n);
                        }
                    }

                    close(client_sock);
                }
                else
                {
                    cerr << "Invalid TCPC format. Expected TCPC<hostname,port>" << endl;
                    return EXIT_FAILURE;
                }
            }
        }
        else
        {
            cerr << "Usage: " << argv[0]
                 << " -e <program> [args] [-i <input_redirect>] [-o <output_redirect>] [-b <bi_redirect>] [-t <timeout>]" << endl;
            return EXIT_FAILURE;
        }
    }

    return 0;
}

//start :
// ./mync -e "ttt 123456789" -i TCPS4090 -o TCPClocalhost,4455
//  nc -l 4455
//  nc localhost 4090


///
////./mync -e "ttt 123456789" -o TCPClocalhost,4455
////for run this first run in a new terminal:
//// nc -l -p 4455
////after run:
////./mync -e "ttt 123456789" -o TCPClocalhost,4455
//
//
//// ./mync -e "ttt 123456789" -b TCPS4055
//// RUN:
//// ./mync -e "ttt 123456789" -b TCPS4095
//// telnet localhost 4095
//
//
//// ./mync -e "ttt 123456789" -i TCPS4092
////RUN:
//// ./mync -e "ttt 123456789" -i TCPS4092
//// telnet localhost 4092
//
//
////./mync -i TCPS4092
////./mync -i TCPClocalhost,4092
//
////./mync -b TCPS4095
////telnet localhost 4095
//
////nc -l -p 4455
////./mync -o TCPClocalhost,4455
//
////nc -l -p 4455
////./mync -i TCPS4090 -o TCPClocalhost,4455
////
////telnet localhost 4090




// ./mync -e "ttt 123456789" -i UDPS5555
// nc -u localhost 5555
//
// nc -l 5556
// ./mync -e "ttt 123456789" -i UDPS5555 -o TCPClocalhost,5556
// nc -u localhost 4050
//
//
// nc -l 5556
// ./mync -e "ttt 123456789" -o TCPClocalhost,5556
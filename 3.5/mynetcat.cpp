#include <iostream>
#include <sstream>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <cstdlib>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <csignal>

using namespace std;

bool running = true;

void print_error_and_exit(const string& msg) {
    cerr << msg << endl;
    exit(EXIT_FAILURE);
}

int start_tcp_server(const string& port, bool handle_output = false) {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    cout << "Starting TCP server on port " << port << endl;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(stoi(port));

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    cout << "Accepted connection on port " << port << endl;

    if (handle_output) {
        if (dup2(new_socket, STDOUT_FILENO) < 0) {
            perror("dup2 output failed");
            exit(EXIT_FAILURE);
        }
        if (dup2(new_socket, STDERR_FILENO) < 0) {
            perror("dup2 error output failed");
            exit(EXIT_FAILURE);
        }
        cout << "Output redirected to socket" << endl;
    }

    return new_socket;
}

int start_tcp_client(const string& hostname, const string& port) {
    struct sockaddr_in serv_addr;
    struct hostent *server;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("ERROR opening socket");
        exit(EXIT_FAILURE);
    }
    server = gethostbyname(hostname.c_str());
    if (server == nullptr) {
        fprintf(stderr, "ERROR, no such host\n");
        exit(EXIT_FAILURE);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(stoi(port));

    cout << "Connecting to " << hostname << " on port " << port << endl;

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR connecting");
        exit(EXIT_FAILURE);
    }

    cout << "Connected to " << hostname << " on port " << port << endl;

    return sock;
}

void handle_client_input(int client_sock) {
    dup2(client_sock, STDIN_FILENO);
}

void handle_client_output(int client_sock) {
    dup2(client_sock, STDOUT_FILENO);
    dup2(client_sock, STDERR_FILENO);
}

void redirect_io(int input_fd, int output_fd) {
    cout << "Redirecting input and output" << endl;

    if (input_fd != STDIN_FILENO) {
        if (dup2(input_fd, STDIN_FILENO) < 0) {
            perror("dup2 input failed");
            exit(EXIT_FAILURE);
        }
    }

    if (output_fd != STDOUT_FILENO) {
        if (dup2(output_fd, STDOUT_FILENO) < 0) {
            perror("dup2 output failed");
            exit(EXIT_FAILURE);
        }
        if (dup2(output_fd, STDERR_FILENO) < 0) {
            perror("dup2 error output failed");
            exit(EXIT_FAILURE);
        }
    }

    cout << "Input and output redirected" << endl;
}

void handle_chat(int input_fd, int output_fd) {
    fd_set read_fds;
    char buffer[1024];
    ssize_t bytes_read;

    while (running) {
        FD_ZERO(&read_fds);
        FD_SET(input_fd, &read_fds);
        FD_SET(STDIN_FILENO, &read_fds);

        int max_fd = max(input_fd, STDIN_FILENO) + 1;
        int activity = select(max_fd, &read_fds, nullptr, nullptr, nullptr);

        if (activity < 0 && errno != EINTR) {
            perror("select");
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(input_fd, &read_fds)) {
            memset(buffer, 0, sizeof(buffer));
            bytes_read = read(input_fd, buffer, sizeof(buffer));
            if (bytes_read > 0) {
                cout << "Received: " << buffer << flush;
            } else if (bytes_read == 0) {
                cout << "Connection closed by peer" << endl;
                break;
            } else {
                perror("read");
                exit(EXIT_FAILURE);
            }
        }

        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            memset(buffer, 0, sizeof(buffer));
            bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer));
            if (bytes_read > 0) {
                write(output_fd, buffer, bytes_read);
            } else if (bytes_read == 0) {
                break;
            } else {
                perror("read");
                exit(EXIT_FAILURE);
            }
        }
    }
}

void signal_handler(int signal) {
    if (signal == SIGINT) {
        running = false;
    }
}

vector<string> split(const string& str) {
    vector<string> result;
    istringstream iss(str);
    for (string s; iss >> s;) {
        result.push_back(s);
    }
    return result;
}

int main(int argc, char* argv[]) {
    signal(SIGINT, signal_handler);

    int opt;
    string command;
    string input_source, output_dest;

    while ((opt = getopt(argc, argv, "e:i:o:b:")) != -1) {
        switch (opt) {
            case 'e':
                command = optarg;
                break;
            case 'i':
                input_source = optarg;
                break;
            case 'o':
                output_dest = optarg;
                if (command.empty()) {
                    input_source = optarg;
                }
                break;
            case 'b':
                input_source = optarg;
                output_dest = optarg;
                break;
            default:
                print_error_and_exit("Usage: mync [-e \"command\"] [-i TCPS<PORT> | -i TCPC<IP,PORT> | -o TCPC<IP,PORT> | -b TCPS<PORT>]");
        }
    }

    int input_fd = STDIN_FILENO;
    int output_fd = STDOUT_FILENO;

    if (!input_source.empty()) {
        if (input_source.substr(0, 4) == "TCPS") {
            string port = input_source.substr(4);
            bool handle_output = (input_source == output_dest);
            input_fd = start_tcp_server(port, handle_output);
            if (handle_output) {
                output_fd = input_fd;
            }
        } else if (input_source.substr(0, 4) == "TCPC") {
            size_t comma_pos = input_source.find(',');
            string hostname = input_source.substr(4, comma_pos - 4);
            string port = input_source.substr(comma_pos + 1);
            input_fd = start_tcp_client(hostname, port);
        }
    }

    if (!output_dest.empty()) {
        if (output_dest.substr(0, 4) == "TCPS") {
            string port = output_dest.substr(4);
            output_fd = start_tcp_server(port, true);
        } else if (output_dest.substr(0, 4) == "TCPC") {
            size_t comma_pos = output_dest.find(',');
            string hostname = output_dest.substr(4, comma_pos - 4);
            string port = output_dest.substr(comma_pos + 1);
            output_fd = start_tcp_client(hostname, port);
        }
    }

    if (command.empty()) {
        handle_chat(input_fd, output_fd);
        return 0;
    }

    cout << "Command: " << command << endl;
    cout << "Input source: " << input_source << endl;
    cout << "Output destination: " << output_dest << endl;

    vector<string> split_command = split(command);
    vector<char*> exec_args;
    for (const auto& arg : split_command) {
        exec_args.push_back(const_cast<char*>(arg.c_str()));
    }
    exec_args.push_back(nullptr);

    cout << "Executing program: " << split_command[0] << endl;
    cout << "Arguments: ";
    for (size_t i = 1; i < split_command.size(); i++) {
        cout << split_command[i] << " ";
    }
    cout << endl;

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return EXIT_FAILURE;
    } else if (pid == 0) {
        cout << "In child process, executing command" << endl;
        redirect_io(input_fd, output_fd);
        execvp(exec_args[0], exec_args.data());
        perror("execvp");
        exit(EXIT_FAILURE);
    } else {
        int status;
        cout << "In parent process, waiting for child" << endl;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            cout << "Child process exited with status " << WEXITSTATUS(status) << endl;
        } else {
            cout << "Child process did not exit successfully" << endl;
        }
    }

    return 0;
}
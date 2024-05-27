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

    using namespace std;

    void print_error_and_exit(const string& msg) {
        cerr << msg << endl;
        exit(EXIT_FAILURE);
    }

    int start_tcp_server(int port, bool handle_output = false) {
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
        address.sin_port = htons(port);

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

    int start_tcp_client(const string& hostname, int port) {
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
        serv_addr.sin_port = htons(port);

        cout << "Connecting to " << hostname << " on port " << port << endl;

        if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
            perror("ERROR connecting");
            exit(EXIT_FAILURE);
        }

        cout << "Connected to " << hostname << " on port " << port << endl;

        return sock;
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

    int main(int argc, char* argv[]) {
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
                    break;
                case 'b':
                    input_source = optarg;
                    output_dest = optarg;
                    break;
                default:
                    print_error_and_exit("Usage: mync -e \"command\" [-i TCPS<PORT> | -o TCPC<IP,PORT> | -b TCPS<PORT>]");
            }
        }

        if (command.empty()) {
            print_error_and_exit("Usage: mync -e \"command\" [-i TCPS<PORT> | -o TCPC<IP,PORT> | -b TCPS<PORT>]");
        }

        cout << "Command: " << command << endl;
        cout << "Input source: " << input_source << endl;
        cout << "Output destination: " << output_dest << endl;

        stringstream ss(command);
        string program;
        vector<string> args;
        ss >> program;
        string arg;
        while (ss >> arg) {
            args.push_back(arg);
        }

        vector<char*> exec_args;
        exec_args.push_back(strdup(program.c_str()));
        for (const auto& a : args) {
            exec_args.push_back(strdup(a.c_str()));
        }
        exec_args.push_back(nullptr);

        // Print the command and arguments for debugging
        cout << "Executing program: " << program << endl;
        cout << "Arguments: ";
        for (const auto& a : args) {
            cout << a << " ";
        }
        cout << endl;

        int input_fd = STDIN_FILENO;
        int output_fd = STDOUT_FILENO;

        if (!input_source.empty()) {
            if (input_source.substr(0, 4) == "TCPS") {
                int port = stoi(input_source.substr(4));
                bool handle_output = (input_source == output_dest);
                input_fd = start_tcp_server(port, handle_output);
                if (handle_output) {
                    output_fd = input_fd;
                }
            }
        }

        if (!output_dest.empty()) {
            if (output_dest.substr(0, 4) == "TCPC") {
                size_t comma_pos = output_dest.find(',');
                string hostname = output_dest.substr(4, comma_pos - 4);
                int port = stoi(output_dest.substr(comma_pos + 1));
                output_fd = start_tcp_client(hostname, port);
            } else if (output_dest.substr(0, 4) == "TCPS" && output_dest != input_source) {
                int port = stoi(output_dest.substr(4));
                output_fd = start_tcp_server(port, true);
            }
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            return EXIT_FAILURE;
        } else if (pid == 0) {
            cout << "In child process, executing command" << endl;
            redirect_io(input_fd, output_fd);
            execv(exec_args[0], exec_args.data());
            perror("execv");
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

        for (auto arg : exec_args) {
            free(arg);
        }

        return 0;
    }

    // ./mync -e “ttt 123456789” -i TCPS4090 -o TCPClocalhost,4455
    //for run this first run in a new terminal:
    // nc -l -p  4455
    //after run
    // ./mync -e “ttt 123456789” -i TCPS4090 -o TCPClocalhost,4455
    //and then run in another terminal
    // telnet localhost 4090


    //
    //./mync -e "ttt 123456789" -o TCPClocalhost,4455
    //for run this first run in a new terminal:
    // nc -l -p 4455
    //after run:
    //./mync -e "ttt 123456789" -o TCPClocalhost,4455


    // ./mync -e "ttt 123456789" -b TCPS4055
    // RUN:
    // ./mync -e "ttt 123456789" -b TCPS4095
    // telnet localhost 4095


    // ./mync -e "ttt 123456789" -i TCPS4092
    //RUN:
    // ./mync -e "ttt 123456789" -i TCPS4092
    // telnet localhost 4092
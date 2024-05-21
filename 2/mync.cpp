#include <iostream>
#include <sstream>
#include <vector>
#include <unistd.h>  // for fork and execv
#include <cstring>   // for c_str() and strdup
#include <cstdlib>   // for exit()
#include <sys/wait.h> // for waitpid

using namespace std;

void print_error_and_exit() {
    cerr << "Usage: mync -e \"command\"" << endl;
    exit(EXIT_FAILURE);
}

int main(int argc, char* argv[]) {
    // Check the number of arguments and the first argument value

    if (argc != 3 || string(argv[1]) != "-e") {
        print_error_and_exit();
    }

    // Get the command from the second argument
    string command = argv[2];

    // Create a stringstream from the command string
    stringstream ss(command);
    string program;
//    cout<< "Command: " << command << endl;
//    cout<<"ss: "<<ss.str()<<endl;
    vector<string> args;

    // Extract the first word as the program name
    ss >> program;
    cout<<"Program: "<<program<<endl;
    // Extract the rest of the words as arguments
    string arg;
    while (ss >> arg) {
        args.push_back(arg);
//         cout<<"Arg: "<<arg<<endl;
    }

    // Convert vector<.tring> to char* array for execv
    vector<char*> exec_args;
    exec_args.push_back(strdup(program.c_str())); // Program name
    cout<<"Program: "<<program.c_str()<<endl;
    for (const auto& a : args) {
        exec_args.push_back(strdup(a.c_str())); // Arguments
    }

    exec_args.push_back(nullptr); // Null-terminate the array

    // Fork the process
    pid_t pid = fork();

    if (pid < 0) {
        // Fork failed
        perror("fork");
        return EXIT_FAILURE;
    } else if (pid == 0) {
        // Child process: execute the command
//        cout << "exec_args[0]: " << exec_args[0] << endl;
//        cout<<"exec_args.data: "<<exec_args.data()<<endl;
        execv(exec_args[0], exec_args.data());

        // If execv returns, it must have failed
        perror("execv");
        exit(EXIT_FAILURE);
    } else {
        // Parent process: wait for the child to complete
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            cout << "Child process exited with status " << WEXITSTATUS(status) << endl;
        } else {
            cout << "Child process did not exit successfully" << endl;
        }
    }

    // Free the duplicated strings
    for (auto arg : exec_args) {
        free(arg);
    }

    return 0;
}


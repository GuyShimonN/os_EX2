//
// Created by guy on 5/21/24.
//
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <vector>
#include <sstream>

using namespace std;

void print_error_and_exit() {
    cout << "Error" << endl;
    exit(1);
}

int main(int argc, char* argv[]) {
    if (argc != 3 || string(argv[1]) != "-e") {
        print_error_and_exit();
    }

    // Split the command into program and arguments
    string command = argv[2];
    stringstream ss(command);
    cout<< "Command: " << command << endl;
    cout<<ss.str()<<endl;
    string program;
    vector<string> args;
    ss >> program;
    string arg;
    while (ss >> arg) {
        args.push_back(arg);
    }

    // Prepare arguments for execvp
    vector<char*> exec_args;
    exec_args.push_back(&program[0]);
    for (auto& a : args) {
        exec_args.push_back(&a[0]);
    }
    exec_args.push_back(nullptr);

    // Fork the process
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        exit(1);
    } else if (pid == 0) {
        // Child process: redirect input/output and execute the program
        dup2(STDIN_FILENO, 0);
        dup2(STDOUT_FILENO, 1);
        execvp(exec_args[0], exec_args.data());
        perror("execvp failed");
        exit(1);
    } else {
        // Parent process: wait for the child to complete
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        } else {
            return 1;
        }
    }

    return 0;
}

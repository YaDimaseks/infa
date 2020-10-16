#include <stdio.h>
#include <process.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <vector>
#include <string>
#include <algorithm>
#include <iostream>
#include <sstream>


using namespace std;

vector<string> parsing(string s, const string& delimeters){
    vector<string> ans;
    for (string::iterator iter = s.begin(); iter != s.end(); iter++) {
        if (delimeters.find(*iter) != std::string::npos) {
            *iter = '\n';
        }
    }
    istringstream ist(s);
    while (getline(ist, s)) {
        istringstream tmp(s);
        string token;
        tmp >> token;
        if (token != "") ans.push_back(token);
    }
    return ans;
}

int main() {
    pid_t pid_main; //PID of the process


    string in;
    printf(">");
    while (getline(cin, in)) {
        int fd[2];
        pipe(fd);
        pid_t pid = fork();
        if (pid != 0) {
            close(fd[0]);
            dup2(fd[1], 1);
        }
        else {

        }

        //парсинг строки по пробелу и табуляции
        vector<string> v = parsing(in, " \t");

        for (auto a : v) {
            cout << a << "\n";
        }
        printf(">");
    }

}
#include <stdio.h>
//#include <process.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <vector>
#include <string>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <pwd.h>


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

string get_homedir() {
    const char *dir;
    if ((dir = getenv("HOME")) == NULL) {
        dir = getpwuid(getuid())->pw_dir;
    }
    string ret = dir;
    return ret;
}

void print_hello(){
    char wd[1000];
    getcwd(wd, sizeof(wd));
    string dir = wd;
    string hello;
    if (dir == "/root")
	    dir = "";
    if(dir > get_homedir()){
        vector<string> dir_vec = parsing(wd, "/");
	hello += "~/";
	for (size_t i = 2; i < dir_vec.size(); i++){
            hello += dir_vec[i];
	    if (i != dir_vec.size() - 1)
		hello += "/";
	}
    }				    
    else if(dir == get_homedir()){
        hello += "~";
    }
    else{
        vector<string> dir_vec = parsing(wd, "/");
	hello += "/";
	for (size_t i = 0; i < dir_vec.size(); i++){
		hello += dir_vec[i];
	if (i != dir_vec.size() - 1)
 		hello += "/";
	}
    }
    gid_t gid = getgid();
    if (long(gid) == 0)
        hello += "!";
    else
        hello += ">";
    cout << hello;
}

class Command{
public:
	vector<string> args;
	Command(const string* input){}
private:

};

int main() {
	print_hello();
	string input;
	getline(cin, input);
	vector<string> v = parsing(input, " \t");
	for (auto a : v) {
		cout << a << "\n";
    	}
    //pid_t pid_main; //PID of the process
    //string in;
    //printf(">");
    //while (getline(cin, in)) {
    //    int fd[2];
    //    pipe(fd);
    //    pid_t pid = fork();
    //    if (pid != 0) {
    //        close(fd[0]);
    //        dup2(fd[1], 1);
    //    }
    //    else {

    //    }

    //    //парсинг строки по пробелу и табуляции
    //    vector<string> v = parsing(in, " \t");

    //    for (auto a : v) {
    //        cout << a << "\n";
    //    }
    //    printf(">");
    //}
}

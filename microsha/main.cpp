#include <stdio.h>
//#include <process.h>
#include <csignal>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/prctl.h>
#include <vector>
#include <string>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <pwd.h>


using namespace std;

vector<string> parsing(string s, const string& delimeters) {
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
    const char* dir;
    if ((dir = getenv("HOME")) == NULL) {
        dir = getpwuid(getuid())->pw_dir;
    }
    string ret = dir;
    return ret;
}

string get_dir() {
    char wd[1000];
    getcwd(wd, sizeof(wd));
    string dir = wd;
    return dir;
}

void print_hello() {
    string dir = get_dir();
    string hello;
    if (dir == "/root")
        dir = "";
    if (dir > get_homedir()) {
        vector<string> dir_vec = parsing(dir, "/");
        hello += "~/";
        for (size_t i = 2; i < dir_vec.size(); i++) {
            hello += dir_vec[i];
            if (i != dir_vec.size() - 1)
                hello += "/";
        }
    }
    else if (dir == get_homedir()) {
        hello += "~";
    }
    else {
        vector<string> dir_vec = parsing(dir, "/");
        hello += "/";
        for (size_t i = 0; i < dir_vec.size(); i++) {
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

class Command {
public:
    //общий вид: command <(>) file1 >(<) file2 
    vector<string> input_args;
    vector<string> command_args;
    vector<pair<string, bool>> files; //пары файл-флаг, где флаг=0 если input, 1 если output
    Command(const string& input) {
        parsing_input(input);
        command_split();
    }
    void print()
    {
        for (auto& arg : input_args)
        {
            cout << cout.width(2) << arg;
        }
        cout << endl;
    }
    int exec() {
        if (is_empty()) {
            perror("Command is empty");
        }
        if (is_cd()) {
            if (command_args.size() == 1)
                exec_cd(get_homedir());
            else if (command_args.size() == 2)
                exec_cd(command_args[1]);
            else
                perror("Too many arguments for command 'cd'");
        }
        if (is_pwd()) {
            exec_pwd();
        }
        else {
            exec_bash_command(command_args);
        }
	return 0;
    }
    bool is_empty() { return command_args.empty(); }
    bool is_cd() { return is_empty() ? false : command_args[0] == "cd"; }
    bool is_time() { return is_empty() ? false : command_args[0] == "time"; }
    bool is_pwd() { return is_empty() ? false : command_args[0] == "pwd"; }
private:
    void parsing_input(string input) { input_args = parsing(input, " \t"); }
    void command_split() { //split команды на command_args и files
        auto in_iter = find(input_args.begin(), input_args.end(), "<");
        auto in_counter = count(input_args.begin(), input_args.end(), "<");
        auto out_iter = find(input_args.begin(), input_args.end(), ">");
        auto out_counter = count(input_args.begin(), input_args.end(), ">");
        try {
            if (in_counter > 1)
                throw '<';
            if (out_counter > 1)
                throw '>';
        }
        catch (char a) {
            cerr << "Too many " << a << endl;
        }
        if (in_iter > out_iter) {
            command_args.assign(input_args.begin(), out_iter);
            files.push_back(make_pair(*(out_iter + 1), 1));
            if (in_iter != input_args.end())
                files.push_back(make_pair(*(in_iter + 1), 0));
        }
        else if (out_iter > in_iter) {
            command_args.assign(input_args.begin(), in_iter);
            files.push_back(make_pair(*(in_iter + 1), 0));
            if (out_iter != input_args.end())
                files.push_back(make_pair(*(out_iter + 1), 1));
        }
        else {
            command_args = input_args;
        }
    }
    void exec_pwd()
    {
        cout << get_dir() << endl;
    }
    void exec_cd(const string& arg)
    {
        int code = chdir(arg.c_str());
        if (code == -1)
        {
            cerr << "No such file or directory" << endl;
        }
    }
    void exec_bash_command(const vector<string>& command_args) {
        vector<char*> v;
        for (size_t i = 0; i < command_args.size(); i++) {
            v.push_back((char*)command_args[i].c_str());
        }
        v.push_back(NULL);
        for (int i = 0; v[i] != NULL; i++) {
            printf("v[%d]='%s'\n", i, v[i]);
        }
	prctl(PR_SET_PDEATHSIG, SIGINT);
        execvp(v[0], &v[0]);
    }
};

int main() {
    string input;
	while(true){
		print_hello();
		getline(cin, input);
		cout << endl;
		Command command(input);
		command.exec();
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

    //    //парсинг строки по пробелу и табул€ции
    //    vector<string> v = parsing(in, " \t");

    //    for (auto a : v) {
    //        cout << a << "\n";
    //    }
    //    printf(">");
    //}
}

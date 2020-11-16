#include <stdio.h>
//#include <process.h>
#include <csignal>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <fstream>
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
    string input_name;
    string output_name;
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
<<<<<<< HEAD
        if (failed) return 1;
=======
	if (failed) return 1;
>>>>>>> 212c9efc5a90867fc207bff9b68b98ba5a54eefe
        if (is_empty()) {
            perror("Command is empty");
        }
	else if (is_cd()) {
            if (command_args.size() == 1)
                exec_cd(get_homedir());
            else if (command_args.size() == 2)
                exec_cd(command_args[1]);
            else
                perror("Too many arguments for command 'cd'");
        }
	else if (is_pwd()) {
            exec_pwd();
        }
        else {
            do_redirect();
            exec_bash_command(command_args);
        }
	return 0;
    }
    bool is_empty() { return command_args.empty(); }
    bool is_cd() { return is_empty() ? false : command_args[0] == "cd"; }
    bool is_time() { return is_empty() ? false : command_args[0] == "time"; }
    bool is_pwd() { return is_empty() ? false : command_args[0] == "pwd"; }
private:
    bool redirect = true;
    bool failed = false;
    void parsing_input(string input) { input_args = parsing(input, " \t"); }
    void command_split() { //split команды на command_args, input_name и output_name
        auto in_iter = find(input_args.begin(), input_args.end(), "<");
        auto in_counter = count(input_args.begin(), input_args.end(), "<");
        auto out_iter = find(input_args.begin(), input_args.end(), ">");
        auto out_counter = count(input_args.begin(), input_args.end(), ">");
<<<<<<< HEAD
        if (in_counter > 1) {
            perror("too many '<'");
            failed = true;
            return;
        }
        if (out_counter > 1) {
            perror("Too many '>'");
            failed = true;
            return;
        }
=======
        if (in_counter > 1){
                perror("too many '<'");
		failed = true;
		return;
	}
        if (out_counter > 1){
        	perror("Too many '>'");
		failed = true;
		return;
	}
>>>>>>> 212c9efc5a90867fc207bff9b68b98ba5a54eefe
        if (in_iter > out_iter) {
            command_args.assign(input_args.begin(), out_iter);
            //files.push_back(make_pair(*(out_iter + 1), 1));
            output_name = *(out_iter + 1);
            if (in_iter != input_args.end())
                //files.push_back(make_pair(*(in_iter + 1), 0));
                input_name = *(in_iter + 1);
        }
        else if (out_iter > in_iter) {
            command_args.assign(input_args.begin(), in_iter);
            //files.push_back(make_pair(*(in_iter + 1), 0));
            input_name = *(in_iter + 1);
            if (out_iter != input_args.end())
                //files.push_back(make_pair(*(out_iter + 1), 1));
                output_name = *(out_iter + 1);
        }
        else {
            command_args = input_args;
            redirect = false;
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
<<<<<<< HEAD
        for (int i = 0; v[i] != NULL; i++) {
            printf("v[%d]='%s'\n", i, v[i]);
        }
	    prctl(PR_SET_PDEATHSIG, SIGINT);
        execvp(v[0], &v[0]);
        perror(v[0]);
=======
	prctl(PR_SET_PDEATHSIG, SIGINT);
        execvp(v[0], &v[0]);
	perror(v[0]);
>>>>>>> 212c9efc5a90867fc207bff9b68b98ba5a54eefe
    }
    int do_redirect() {
        if (!redirect) return 0;
        int fd_in, fd_out;
        if (!input_name.empty()) {
            fd_in = open(input_name.c_str(), O_RDONLY);
            if (fd_in == -1) {
                perror("Can't open input file");
                return 1;
            }
            dup2(fd_in, STDIN_FILENO);
        }
        if (!output_name.empty()) {
            fd_out = open(output_name.c_str(), O_WRONLY | O_TRUNC | O_CREAT, S_IWRITE | S_IREAD);
            if (fd_out == -1) {
                perror("Can't open output file");
                return 1;
            }
            dup2(fd_out, STDOUT_FILENO);
        }
	return 0;
    }
};

class Conveyer {
public:
    // command1 < file1 | command2 | ... | commandn > file2
    vector<Command> commands;
    string input_name;
    string output_name;
    Conveyer(const string& input) {
        vector<string> parsed = parsing(input, "|");
        vector<Command> data;
        for (auto i : parsed)
            commands.push_back(i);
        if (commands.size() > 1) {
            for (auto i = commands.begin(); i != commands.end(); i++) {
                if (!commands[0].output_name.empty() && !(i == commands.end() - 1)) {
                    perror("'>' can only be use in last conponent of conveyer");
                    exit(1);
                }
                if (!commands[0].input_name.empty() && !(i == commands.begin())) {
                    perror("'<' can only be use in first conponent of conveyer");
                    exit(1);
                }
            }
        }
        input_name = commands.front().input_name;
        output_name = commands.back().output_name;
        //fd_in = dup(STDIN_FILENO);
        //fd_out = dup(STDOUT_FILENO);
    }
    ~Conveyer() {}
    int exec() {
        if (commands.empty() || commands[0].is_empty()) return 0;
        if (commands[0].is_time()) {}
        else if (commands.size() == 1) {
            commands[0].exec();
            return 0;
        }
        int cur_fd = 0, prev_fd = -1;
        vector<int[2]> pipes(commands.size() - 1);
        for (auto& fd : pipes) {
            if (pipe2(fd, O_CLOEXEC) != 0) {
                perror("Can't open pipe\n");
            }
        }
        for (int i = 0; i != commands.size(); i++, cur_fd++, prev_fd++) {
            pid_t pid = fork();
            if (pid == 0) {
                if (i == 0) {
                    dup2(pipes[0][1], STDOUT_FILENO);
                    close(pipes[0][0]);
                    commands[i].exec();
                }
                else if (i > 0 && i < commands.size() - 1) {
                    dup2(pipes[prev_fd][0], STDIN_FILENO);
                    close(pipes[prev_fd][1]);
                    dup2(pipes[cur_fd][1], STDOUT_FILENO);
                    close(pipes[cur_fd][0]);
                    commands[i].exec();
                }
                else if (i == commands.size() - 1) {
                    dup2(pipes[prev_fd][0], STDIN_FILENO);
                    close(pipes[prev_fd][1]);
                    commands[i].exec();
                }
            }
            else {
                if (i == commands.size() - 1) {//на последней команде закрываем все пайпы и ждем детей
                    for (auto& i : pipes) {
                        if (i[0] != -1) {
                            close(i[0]);
                            close(i[1]);
                        }
                    }
                    for (int i = 0; i < commands.size(); i++) {
                        wait(NULL);
                    }
                }
            }
        }
        return 0;
    }
private:
    //int fd_in, fd_out;
};

int main() {
    string input;
	while(true){
		print_hello();
		getline(cin, input);
<<<<<<< HEAD
        Conveyer conveyer(input);
        conveyer.exec();
=======
		Command command(input);
		command.exec();
>>>>>>> 212c9efc5a90867fc207bff9b68b98ba5a54eefe
	}
}

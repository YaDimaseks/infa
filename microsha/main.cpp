#include <stdio.h>
#include <csignal>
#include <signal.h>
#include <unistd.h>
#include <iomanip>  
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
#include <sys/time.h>
#include <sys/resource.h>
#include <dirent.h>
#include <wait.h>
#include <pthread.h>

using namespace std;

//разделение по пробелу и табуляции("" не входят)
vector<string> split_space(const string& s) {
	istringstream ist(s);
	vector<string> ans;
	while(ist){
		string token;
		ist >> token;
		if (token != "") ans.push_back(token);
	}
	return ans;
}
//разделение по string("" могут быть)
vector<string> split_by_string(const string& str, const string& delim)
{
	vector<string> tokens;
	size_t prev = 0, pos = 0;
	do
	{
		pos = str.find(delim, prev);
		if (pos == string::npos) pos = str.length();
		string token = str.substr(prev, pos-prev);
		tokens.push_back(token);
		prev = pos + delim.length();
	}
	while (pos < str.length() && prev < str.length());
	return tokens;
}
//возвращает путь к домашней директории из корневой
string get_homedir() {
    const char* dir;
    if ((dir = getenv("HOME")) == NULL) {
        dir = getpwuid(getuid())->pw_dir;
    }
    string ret = dir;
    return ret;
}
//возвращает путь к текущей директории(домашняя обозначается как ~)
string get_dir() {
    char wd[1000];
    getcwd(wd, sizeof(wd));
    string dir = wd;
    return dir;
}
//печать приветственной строки
void print_hello() {
    string dir = get_dir();
    string hello;
    if (dir == "/root")
        dir = "/";
    if (dir > get_homedir()) {
        vector<string> dir_vec = split_by_string(dir, "/");
        dir_vec.erase(dir_vec.begin());
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
        vector<string> dir_vec = split_by_string(dir, "/");
        dir_vec.erase(dir_vec.begin());
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

class Matcher {
public:
    Matcher(const char* name, const char* mask) : name(name), mask(mask) {}

    bool match() {
        if (!try_partial_match())
            return false;

        while (*mask == '*') {
            ++mask;
            while (!try_partial_match() && *name != '\0')
                ++name;
        }

        return is_full_match();
    }

private:
    bool is_full_match() const { return *name == '\0' && *mask == '\0'; }

    bool patrial_match() {
        while (*name != '\0' && (*name == *mask || *mask == '?')) {
            ++name;
            ++mask;
        }

        return is_full_match() || *mask == '*';
    }

    bool try_partial_match() {
        auto tmp = *this;
        if (tmp.patrial_match()) {
            *this = tmp;
            return true;
        }
        return false;
    }

    const char* name;
    const char* mask;
};


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
        convert_args(command_args);
    }

    int exec() {
	    if (failed) exit(1);
	    else if (is_cd()) {
            if (command_args.size() == 1)
                exec_cd(get_homedir());
            else if (command_args.size() == 2)
                exec_cd(command_args[1]);
            else
                cerr << "Too many arguments for command 'cd'" << endl;
        }
	else if (is_pwd()) {
            exec_pwd();
        }
        else {
            if (do_redirect() == 0)
                exec_bash_command(command_args);
        }
	return 0;
    }
    bool is_empty() { return command_args.empty(); }
    bool is_cd() { return is_empty() ? false : command_args[0] == "cd"; }
    bool is_time() { return is_empty() ? false : command_args[0] == "time"; }
    bool is_pwd() { return is_empty() ? false : command_args[0] == "pwd"; }
    void delete_time() {
        if (is_time()) {
            command_args.erase(command_args.begin());
        }
    }
private:
    bool redirect = true;
    
    bool failed = false;
   
    void parsing_input(const string& input) { input_args = split_space(input); }
    //split команды на command_args, input_name и output_name
    void command_split() {
        auto in_iter = find(input_args.begin(), input_args.end(), "<");
        auto in_counter = count(input_args.begin(), input_args.end(), "<");
        auto out_iter = find(input_args.begin(), input_args.end(), ">");
        auto out_counter = count(input_args.begin(), input_args.end(), ">");
        if (in_counter > 1) {
            cerr << "too many '<'" << endl;
            failed = true;
            return;
        }
        if (out_counter > 1) {
            cerr << "Too many '>'";
            failed = true;
            return;
        }
        if (in_iter > out_iter) {
            command_args.assign(input_args.begin(), out_iter);
            output_name = *(out_iter + 1);
            if (in_iter != input_args.end())
                input_name = *(in_iter + 1);
        }
        else if (out_iter > in_iter) {
            command_args.assign(input_args.begin(), in_iter);
            input_name = *(in_iter + 1);
            if (out_iter != input_args.end())
                output_name = *(out_iter + 1);
        }
        else {
            command_args = input_args;
            redirect = false;
        }
    }

    bool is_reg_expr(const string& arg) {
        if (arg.find('*') != string::npos || arg.find('?') != string::npos) {
            return true;
        }
        return false;
    }
    //конвертация всех регулярных выражений в строки
    void convert_args(vector<string>& args) {
        for (int i = 0; i < args.size(); i++) {
            if (!is_reg_expr(args[i])) {
                continue;
            }
            vector<string> splited_arg = split_by_string(args[i], "/");
            vector<string> converted_arg = convert_reg_expr(splited_arg);
            if (converted_arg.empty())
                continue;
            args.erase(args.begin() + i);
            args.insert(args.begin() + i, converted_arg.begin(), converted_arg.end());
        }
    }
    //на вход подается путь, разделенный по слешу
    vector<string> convert_reg_expr(vector<string>& args) {
        vector<string> result;
        string path;
        bool flag = false;
        if (args[0].empty()) {
            path = "/";
            flag = true;
        }
        else {
            path = get_dir() + "/";
        }
        result.push_back("");
        bool is_dir = false;
        if (args[args.size() - 1].empty()) {
            is_dir = true;
        }
        vector<string> reverse_args;
        for (auto it = args.rbegin(); it != args.rend(); it++) {
            if (it->empty()) {
                continue;
            }
            reverse_args.push_back(*it);
        }
        int args_ptr = reverse_args.size() - 1;
        while (args_ptr >= 0) {
            vector<string> temp;
            for (string& i : result) {
                if (flag) {
                    i += '/';
                }
                DIR* dir = opendir((path + i).c_str());
                if (dir != nullptr) {
                    for (dirent* d = readdir(dir); d != nullptr; d = readdir(dir)) {
                        if (d->d_name[0] == '.') {
                            continue;
                        }
                        if (args_ptr > 0) {
                            Matcher m(d->d_name, reverse_args[args_ptr].c_str());
                            if (d->d_type == DT_DIR && m.match()) {
                                temp.push_back(i + d->d_name);
                            }
                        }
                        else {
                            Matcher m(d->d_name, reverse_args[args_ptr].c_str());
                            if ((d->d_type == DT_DIR || (d->d_type == DT_REG && !is_dir)) && m.match()) {
                                temp.push_back(i + d->d_name);
                            }
                        }
                    }
                }
                closedir(dir);
            }
            result = temp;
            args_ptr--;
            flag = true;
        }
        if (result.empty()) {
            return {};
        }
        return result;
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
        if (failed) return;
        vector<char*> v;
        for (size_t i = 0; i < command_args.size(); i++) {
            v.push_back((char*)command_args[i].c_str());
        }
        v.push_back(NULL);
        prctl(PR_SET_PDEATHSIG, SIGINT); // exit process when parent dies
        execvp(v[0], &v[0]);
        cerr << "Execution error" << endl;
    }
  
    int do_redirect() {
        if (!redirect) return 0;
        int fd_in = -1, fd_out = -1;
        if (!input_name.empty()) {
            fd_in = open(input_name.c_str(), O_RDONLY);
            if (fd_in == -1) {
                cerr << "Can't open input file" << endl;
                return 1;
            }
            dup2(fd_in, STDIN_FILENO);
        }
        if (!output_name.empty()) {
            fd_out = open(output_name.c_str(), O_WRONLY | O_TRUNC | O_CREAT, S_IWRITE | S_IREAD);
            if (fd_out == -1) {
                cerr << "Can't open output file" << endl;
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
        vector<string> parsed = split_by_string(input, "|");
        for (auto i : parsed)
            commands.push_back(i);
        if (commands.size() > 1) {
            for (int i = 0; i != commands.size() - 1; i++) {
                if (!commands[i].output_name.empty() && !(i == commands.size() - 1)) {
                    cerr << "'>' can only be use in last conponent of conveyer" << endl;
                    failed = true;
                }
                if (!commands[i].input_name.empty() && !(i == 0)) {
                    cerr << "'<' can only be use in first conponent of conveyer" << endl;
                    failed = true;
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
        if (commands.empty() || commands[0].is_empty() || failed) return 0;
        if (commands[0].is_time()) {
            commands[0].delete_time();
            struct rusage start, stop;
            struct timeval start_real_time, stop_real_time;
            getrusage(RUSAGE_CHILDREN, &start);
            gettimeofday(&start_real_time, NULL);
            ////////
            exec();
            ////////
            gettimeofday(&stop_real_time, NULL);
            getrusage(RUSAGE_CHILDREN, &stop);
            auto real_time_sec = stop_real_time.tv_sec - start_real_time.tv_sec;
            auto real_time_usec = stop_real_time.tv_usec - start_real_time.tv_usec;
            cerr << "real: " << real_time_sec << "." << setfill('0') << setw(3)
                << real_time_usec << endl
                << "system: " << stop.ru_stime.tv_sec - start.ru_stime.tv_sec << "."
                << setfill('0') << setw(3)
                << stop.ru_stime.tv_usec - start.ru_stime.tv_usec << endl
                << "user: " << stop.ru_utime.tv_sec - start.ru_utime.tv_sec << "."
                << setfill('0') << setw(3)
                << stop.ru_utime.tv_usec - start.ru_utime.tv_usec << endl;
            return 0;
        }
        else if (commands.size() == 1) {
            commands[0].exec();
            return 0;
        }
        int cur_fd = 0, prev_fd = -1;
        vector<int[2]> pipes(commands.size() - 1);
        for (auto& fd : pipes) {
            if (pipe2(fd, O_CLOEXEC) != 0) {
                cerr << "Can't open pipe\n" << endl;
                return 1;
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
                //на последней команде закрываем все пайпы и ждем детей
                if (i == commands.size() - 1) {
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
    bool failed = false;
    //int fd_in, fd_out;
};

int signal_ = 0;

void sig_catch(int signal) {
    signal_ = signal;
}

int main() {
	while(true){
        signal(SIGKILL, sig_catch);
        signal(SIGINT, sig_catch);
		print_hello();
        try {
            string input;
            if (!getline(cin, input))
                throw string("EOF");
            if (signal_ == 9)
                break;
            if (signal_ == 2)
                continue;
            Conveyer conveyer(input);
            conveyer.exec();
        }
        catch (string a) {
            cerr << a << " was occured" << endl;
            break;
        }
	}
}

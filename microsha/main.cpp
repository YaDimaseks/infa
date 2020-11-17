#include <stdio.h>
#include <csignal>
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

using namespace std;

vector<string> parsing(string s) {
	istringstream ist(s);
	vector<string> ans;
	while(ist){
		string token;
		ist >> token;
		if (token != "") ans.push_back(token);
	}
	//cout << "parsing" << endl;
	//for (int i = 0; i < ans.size(); i++){
	//	cout << ans[i] << endl;
	//}
	//cout << "end parsing" << endl;

	return ans;
}

vector<string> parsing_by_string(const string& str, const string& delim)
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
    //cout << "get_dir = " << dir << endl;
    string hello;
    if (dir == "/root")
        dir = "";
    if (dir > get_homedir()) {
        vector<string> dir_vec = parsing_by_string(dir, "/");
        dir_vec.erase(dir_vec.begin());
        //cout << "dir_vec = ";
        //for (auto i : dir_vec)
        //    cout << i << ' ';
        //cout << endl;
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
        vector<string> dir_vec = parsing_by_string(dir, "/");
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

void* exevp_my(void* v) {
    vector<char*>* a = (vector<char*>*)v;
    execvp(&v[0], &&v[0]);
}

class Command {
public:
    //����� ���: command <(>) file1 >(<) file2 
    vector<string> input_args;
    vector<string> command_args;
    string input_name;
    string output_name;
    Command(const string& input) {
        parsing_input(input);
        command_split();
        convert_reg_expr();
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
        if (failed) return 1;
	if (failed) return 1;
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
    void delete_time() {
        if (is_time()) {
            command_args.erase(command_args.begin());
        }
    }
private:
    bool redirect = true;
  
    bool failed = false;
   
    void parsing_input(string input) { input_args = parsing(input); }
   
    void command_split() { //split ������� �� command_args, input_name � output_name
        auto in_iter = find(input_args.begin(), input_args.end(), "<");
        auto in_counter = count(input_args.begin(), input_args.end(), "<");
        auto out_iter = find(input_args.begin(), input_args.end(), ">");
        auto out_counter = count(input_args.begin(), input_args.end(), ">");
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
    
    int convert_reg_expr() {
        /*if (command_args.empty())
            return 0;
        int pos = -1;

	    //for( auto i: command_args)
		//    cout << i << " ";
	    //cout << endl;

        for (int i = command_args.size() - 1; i >= 0; i--) {
            if (command_args[i].find('*') != string::npos || command_args[i].find('?') != string::npos) {
               pos = i;
                break;
            }
        }
	    if (pos == -1)
		    return 0;
        cout << pos << endl;
	    string path = command_args[pos];
	    cout << path << endl;
        vector<string> splited_path = parsing_by_string(path, "/");

        for (int i = 0; i < splited_path.size(); i++) {
            printf("splited_path[%d]:%s\n", i, splited_path[i].c_str());
        }

	    vector<string> paths = reg_expr(splited_path);

	    for (auto i:paths)
		    cout << i;

        command_args.erase(command_args.begin() + pos);
	    command_args.insert(command_args.begin() + pos, paths.begin(), paths.end());*/
        return 0;
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
	    prctl(PR_SET_PDEATHSIG, SIGINT);
        pthread_t thr;
        int code = pthread_create(&thr, NULL, execvp_my, &v);
        pthread_join(thr, NULL);
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

class Conveyer {
public:
    // command1 < file1 | command2 | ... | commandn > file2
    vector<Command> commands;
    string input_name;
    string output_name;
    Conveyer(const string& input) {
        vector<string> parsed = parsing_by_string(input, "|");
        //for (int i = 0; i < parsed.size();i++){
	//	cout << parsed[i] << endl;
	//}
	vector<Command> data;
        for (auto i : parsed)
            commands.push_back(i);
        if (commands.size() > 1) {
            for (int i = 0; i != commands.size() - 1; i++) {
                if (!commands[i].output_name.empty() && !(i == commands.size() - 1)) {
                    perror("'>' can only be use in last conponent of conveyer");
                    exit(1);
                }
                if (!commands[i].input_name.empty() && !(i == 0)) {
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
        if (commands[0].is_time()) {
            commands[0].delete_time();
            struct rusage start, stop;
            struct timeval start_real_time, stop_real_time;
            getrusage(RUSAGE_SELF, &start);
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
                if (i == commands.size() - 1) {//�� ��������� ������� ��������� ��� ����� � ���� �����
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
		Conveyer conveyer(input);
        conveyer.exec();
	}
}

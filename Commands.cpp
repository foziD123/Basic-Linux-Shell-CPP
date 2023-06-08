#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <fcntl.h>    /* For O_RDWR */
#include <unistd.h>   /* For open(), creat() */

using namespace std;
#define MAX_CD_ARGUMNETS 2
#define MAX_ARGS 20

const std::string WHITESPACE = " \n\r\t\f\v";

#if 0
#define FUNC_ENTRY() \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT() \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

string _ltrim(const std::string &s) {
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string &s) {
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string &s) {
    return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char *cmd_line, char **args) {
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for (std::string s; iss >> s;) {
        args[i] = (char *) malloc(s.length() + 1);
        memset(args[i], 0, s.length() + 1);
        strcpy(args[i], s.c_str());
        args[++i] = NULL;
    }
    return i;

    FUNC_EXIT()
}

Command::Command(const char *cmd_line) : cmd_line(cmd_line) {

    string cmd_s = _trim(string(cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
    cmd_name = firstWord.c_str();

}

bool _isBackgroundComamnd(const char *cmd_line) {
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char *cmd_line) {
    const string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == string::npos) {
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&') {
        return;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    cmd_line[idx] = ' ';
    // truncate the command line string up to the last non-space character
    cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

// TODO: Add your implementation for classes in Commands.h

SmallShell::SmallShell() : prev_cmd(""), curr_prompt("smash"), prev_dir(""), father_dir(""), curr_job_pid(-1) {

    job_list = new JobsList();
    smash_pid = getpid();
}

SmallShell::~SmallShell() {
    // TODO: add your implementation
}

/**
 * Creates and returns a pointer to Command class which matches the given command line (cmd_line)
 */

Command *SmallShell::CreateCommand(const char *cmd_line) {

    string cmd = string(cmd_line);
    string cmd_s = _trim(string(cmd_line));
    if (cmd.find("|") != string::npos) {
        return new PipeCommand(cmd_line);
    }
    if (cmd.find(">") != string::npos || cmd.find(">>") != string::npos) {
        return new RedirectionCommand(cmd_line);
    }
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
    this->curr_cmd = cmd_line;

    if ("chprompt" == firstWord) {
        return new ChangePromptCommand(cmd_line);
    }
    if ("setcore" == firstWord) {
        return new SetcoreCommand(cmd_line);
    }
    if ("pwd" == firstWord) {
        return new GetCurrDirCommand(cmd_line);
    }
    if ("showpid" == firstWord) {
        return new ShowPidCommand(cmd_line);
    }
    if ("cd" == firstWord) {
        return new ChangeDirCommand(cmd_line);
    }
    if ("jobs" == firstWord) {
        return new JobsCommand(cmd_line);
    }
    if ("fg" == firstWord) {
        return new ForegroundCommand(cmd_line);
    }
    if ("bg" == firstWord) {
        return new BackgroundCommand(cmd_line);
    }
    if ("quit" == firstWord) {
        return new QuitCommand(cmd_line);
    }
    if ("kill" == firstWord) {
        return new KillCommand(cmd_line);
    } else if (firstWord == "") {
        return nullptr;
    }
        //External commands
    else {
        return new ExternalCommand(cmd_line);
    }


    return nullptr;
    // should implement support for external commands

}


void SmallShell::executeCommand(const char *cmd_line) {
    Command *cmd = CreateCommand(cmd_line);
    if (!cmd) {
        return;
    }
    cmd->execute();
}

/* class cmds implementiations*/



ChangePromptCommand::ChangePromptCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {
}

void ChangePromptCommand::execute() {
    SmallShell &s_shell = SmallShell::getInstance();

    char **args = new char *[COMMAND_MAX_ARGS];
    int number_of_words = _parseCommandLine(this->cmd_line, args);
    if (number_of_words == 1) {
        s_shell.change_prompt_helper("smash");
    } else {
        s_shell.change_prompt_helper(args[1]);
    }
}

PipeCommand::PipeCommand(const char *cmd_line) : Command(cmd_line) {
    string cmd = (string) cmd_line;

    int i = 0;
    while (cmd[i] != '|') {
        i++;
    }
    i++;
    if (cmd[i] == '&') {
        error = true;
    } else {
        error = false;
    }

    int j;
    if (error) {
        j = i + 1;
    } else {
        j = i;
    }
    this->cmd_left = _trim(cmd.substr(0, i - 1));
    this->cmd_right = _trim(cmd.substr(j, cmd.size() - j + 1));
}

void PipeCommand::execute() {
    SmallShell &s_shell = SmallShell::getInstance();
    s_shell.job_list->removeFinishedJobs();
    int fd[2];
    int dup_res = 0;
    if (pipe(fd) == -1) {
        perror("smash error: pipe failed");
        return;
    }
    pid_t pid = fork();
    if (pid == -1) {
        perror("smash error: fork failed");
        return;
    }
    if (pid == 0) {
        if (this->error) {
            dup_res = dup2(fd[1], 2);
            if (dup_res == -1) {
                perror("smash error: dup failed");
                return;
            }
            if (close(fd[0]) == -1) {
                perror("smash error: close failed");
                return;
            }
            if (close(fd[1]) == -1) {
                perror("smash error: close failed");
                return;
            }
        } else {
            dup_res = dup2(fd[1], 1);
            if (dup_res == -1) {
                perror("smash error: dup failed");
                return;
            }
            if (close(fd[0]) == -1) {
                perror("smash error: close failed");
                return;
            }
            if (close(fd[1]) == -1) {
                perror("smash error: close failed");
                return;
            }
        }
        s_shell.executeCommand(this->cmd_left.c_str());
        close(dup_res);
        exit(0);
    }
    pid_t pid2 = fork();
    if (pid2 == -1) {
        perror("smash error: fork failed");
        return;
    }
    if (pid2 == 0) {
        dup_res = dup2(fd[0], 0);
        if (dup_res == -1) {
            perror("smash error: dup failed");
            return;
        }
        if (close(fd[0]) == -1) {
            perror("smash error: close failed");
            return;
        }
        if (close(fd[1]) == -1) {
            perror("smash error: close failed");
            return;
        }
        s_shell.executeCommand(this->cmd_right.c_str());
        close(dup_res);
        exit(0);
    }
    if (close(fd[0]) == -1) {
        perror("smash error: close failed");
        return;
    }
    if (close(fd[1]) == -1) {
        perror("smash error: close failed");
        return;
    }
    if (waitpid(pid, nullptr, WUNTRACED) == -1) {
        perror("smash error: waitpid failed");
        return;
    }

    if (waitpid(pid2, nullptr, WUNTRACED) == -1) {
        perror("smash error: waitpid failed");
        return;
    }
}

RedirectionCommand::RedirectionCommand(const char *cmd_line) : Command(cmd_line), std_out(-1),
                                                               invalid(false) {
    string cmd = (string) cmd_line;
    if (cmd.find(">>") != string::npos) {
        is_double = true;
    } else {
        is_double = false;
    }
    int i = 0;
    while (cmd[i] != '>') {
        i++;
    }
    int j;
    if (is_double) {
        j = i + 2;
    } else {
        j = i + 1;
    }
    this->curr_dest = _trim(cmd.substr(j, cmd.size() - j + 1));
    this->curr_cmd = _trim(cmd.substr(0, i));
}

void RedirectionCommand::prologue() {

    this->std_out = dup(1);
    //this->std_err=dup(2);
    if (this->std_out == -1) {
        perror("smash error: dup failed");
        return;
    }
    if (close(1) == -1) {
        close(this->std_out);
        perror("smash error: close failed");
        return;
    }
    int file_discriptor = -1;
    if (!is_double) {
        file_discriptor = open(curr_dest.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0655);
        if (file_discriptor == -1) {
            invalid = true;
            perror("smash error: open failed");
            return;
        }
    } else {
        file_discriptor = open(curr_dest.c_str(), O_CREAT | O_WRONLY | O_APPEND, 0655);
        if (file_discriptor == -1) {
            invalid = true;
            perror("smash error: open failed");
            return;
        }
    }


}

void RedirectionCommand::epilogue() {

    if (close(1) == -1) {
        perror("smash error: close failed");
    }
    if (dup(this->std_out) == -1) {
        perror("smash error: dup failed");
    }
    this->std_out = -1;
}

void RedirectionCommand::execute() {
    this->prologue();
    if (this->invalid) {
        if (dup(this->std_out) == -1) {
            perror("smash error: dup failed");
        }
        this->std_out = -1;
        return;
    }
    SmallShell &s_shell = SmallShell::getInstance();
    s_shell.executeCommand(this->curr_cmd.c_str());

    this->epilogue();
}


SetcoreCommand::SetcoreCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void SetcoreCommand::execute() {
    SmallShell &s_shell = SmallShell::getInstance();

    char **args = new char *[COMMAND_MAX_ARGS];
    int number_of_words = _parseCommandLine(cmd_line, args);
    if (number_of_words != 3) {
        delete[] args;
        cerr << "smash error: setcore: invalid arguments"<< endl;
        return;
    }
    int job_id = 0, core_number = 0;
    int i = 0;
    while (args[1][i] != '\0') {
        if (args[1][i] != '-') {
            if (args[1][i] < '0' || args[1][i] > '9') {
                delete[] args;
                cerr << "smash error: setcore: invalid arguments"<< endl;
                return;
            }
        }
        i++;
    }
    job_id = stoi(args[1]);
    i = 0;
    while (args[2][i] != '\0') {
        if (args[2][i] != '-') {
            if (args[2][i] < '0' || args[2][i] > '9') {
                delete[] args;
                cerr << "smash error: setcore: invalid arguments"<< endl;
                return;
            }
        }
        i++;
    }
    core_number = stoi(args[2]);
    JobEntry *curr = s_shell.job_list->getJobById(job_id);
    if (!curr) {
        delete[] args;
        cerr << "smash error: setcore: job-id " << job_id << " does not exist" << endl;
        return;
    }
    cpu_set_t my_set;
    CPU_ZERO(&my_set);
    CPU_SET(core_number, &my_set);
    if (sched_setaffinity(curr->pid, sizeof(my_set), &my_set) == -1) {
        if(errno == EINVAL || errno ==EPERM) {
            cerr << "smash error: setcore: invalid core number"<<endl;
        }
        else
        {
            perror("smash error: sched_setaffinity failed");
        }
    }
}

ExternalCommand::ExternalCommand(const char *cmd_line) : Command(cmd_line) {}

void ExternalCommand::execute() {
    bool isBackground = _isBackgroundComamnd(cmd_line);
    SmallShell &s_shell = SmallShell::getInstance();
    s_shell.job_list->removeFinishedJobs();
    char **args = new char *[COMMAND_MAX_ARGS];
    ExternalCommand *cmd_obj = new ExternalCommand(cmd_line);
    int x = strlen(cmd_line);
    char *tmp = new char[x];
    strcpy(tmp, cmd_line);
    if (isBackground) {
        _removeBackgroundSign(tmp);
    }
    _parseCommandLine(tmp, args);
    pid_t child_pid = fork();
    if (child_pid < 0) {
        delete[] args;
        perror("smash error: fork failed");
    } else {

        if (child_pid == 0) { //in the child flow
            pid_t p_pid = getppid();
            if (p_pid < 0) {
                delete[] args;
                perror("smash error: getpid failed");
            }
            if (s_shell.smash_pid == p_pid) {
                int p_grp = setpgrp();
                if (p_grp < 0) {
                    delete[] args;
                    perror("smash error: setpgrp failed");
                }
            }
            pid_t curr_pid = getpid();
            if (curr_pid < 0) {
                delete[] args;
                perror("smash error: fork failed");
            }
            char *argv[] = {(char *) "/bin/bash", (char *) "-c", (char *) tmp, nullptr};
            //execl("/bin/bash",,,)
            int x_failing = execv(argv[0], argv);
            if (x_failing == -1) {
                delete[] args;
                perror("smash error: execv failed");
            }
        } else {

            //now in the parent flow

            if (!isBackground) {
                //in the front
                s_shell.curr_job_pid = child_pid;
                int status;
                pid_t tpid = waitpid(child_pid, &status, WUNTRACED);

                if (WIFSTOPPED(status)) {
                    s_shell.job_list->updateMaxStopped();
                    if (s_shell.job_list->getJobByPid(child_pid) == nullptr) {
                        s_shell.job_list->addJob(cmd_obj, child_pid, true, false);
                    }
                }
                if (tpid == -1) {
                    delete[] args;
                    perror("smash error: waitpid failed");
                }
                s_shell.curr_job_pid = -1;
            } else {

                //in background
                s_shell.job_list->updateMaxStopped();
                s_shell.job_list->addJob(cmd_obj, child_pid, false, true);//not stopped ,in bg
                delete cmd_obj;
            }

        }
    }

    delete[] args;
}

ShowPidCommand::ShowPidCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void ShowPidCommand::execute() {
    SmallShell &s_shell = SmallShell::getInstance();
    cout << "smash pid is " << s_shell.smash_pid << endl;
}

GetCurrDirCommand::GetCurrDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void GetCurrDirCommand::execute() {
    //SmallShell &s_shell = SmallShell::getInstance();
    char *mother_dir = getcwd(nullptr, 0);
    if (mother_dir == nullptr) {
        perror("smash error: pwd failed");
    }
    cout << mother_dir << endl;
}

ChangeDirCommand::ChangeDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void ChangeDirCommand::execute() {
    SmallShell &s_shell = SmallShell::getInstance();
    char **args = new char *[COMMAND_MAX_ARGS];
    int number_of_words = _parseCommandLine(cmd_line, args);
    if (number_of_words > MAX_CD_ARGUMNETS) {
        cerr << "smash error: cd: too many arguments\n";
        return;
    }
    if (number_of_words <= 1) {
        cerr << "smash error: cd: too few arguments" << endl;
        return;
    }
    if (strcmp(args[1], "..") == 0) {
        string temp = getcwd(nullptr, 0);

        if (chdir("..") == -1) {
            perror("smash error: chdir failed");
            return;
        } else {
            s_shell.prev_dir = temp;
            return;
        }
    }
    if (strcmp(args[1], "-") == 0) {
        if (s_shell.prev_dir == "") {
            cerr << "smash error: cd: OLDPWD not set" << endl;
            return;
        } else {
            string temp = getcwd(nullptr, 0);
            if (chdir(s_shell.prev_dir.c_str()) == -1) {
                perror("smash error: chdir failed");
                return;
            } else {
                delete[] args;
                s_shell.prev_dir = temp;
                return;
            }
        }
    }
    string temp = getcwd(nullptr, 0);
    if (chdir(args[1]) == -1) {
        delete[] args;
        perror("smash error: chdir failed");
        return;
    } else {
        delete[] args;
        s_shell.prev_dir = temp;
        return;
    }
}

JobsCommand::JobsCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void JobsCommand::execute() {
    SmallShell &s_shell = SmallShell::getInstance();
    s_shell.job_list->printJobsList();
}

ForegroundCommand::ForegroundCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void ForegroundCommand::execute() {
    SmallShell &s_shell = SmallShell::getInstance();
    s_shell.job_list->removeFinishedJobs();
    char **args = new char *[COMMAND_MAX_ARGS];
    int number_of_words = _parseCommandLine(cmd_line, args);

    if (number_of_words > 2) {
        delete[] args;
        cerr << "smash error: fg: invalid arguments" << endl;
        return;
    }

    JobEntry *curr = nullptr;
    int number = 0;

    if (number_of_words == 1) {
        if (s_shell.job_list->job_count == 0) {
            delete[] args;
            cerr << "smash error: fg: jobs list is empty" << endl;
            return;
        }

        s_shell.job_list->updateMaxStopped();
        curr = s_shell.job_list->getJobById(s_shell.job_list->max_job_id);
        //index = int(s_shell.job_list->job_vector.size() - 1);

    } else {
        int i = 0;
        while (args[1][i] != '\0') {
            if (args[1][i] != '-') {
                if (args[1][i] < '0' || args[1][i] > '9') {
                    delete[] args;
                    cerr << "smash error: fg: invalid arguments" << endl;
                    return;
                }
            }
            i++;
        }
        number = atoi(args[1]);
        for (unsigned int i = 0; i < s_shell.job_list->job_vector.size(); i++) {

            if (s_shell.job_list->job_vector[i]->job_id == number) {
                curr = s_shell.job_list->job_vector[i];
                //index = i;
                break;

            }
        }
    }

    if (curr) {
        if (kill(curr->pid, SIGCONT) == -1) {
            perror("smash error: kill failed");
            return;
        }
        cout << curr->cmd_line << " : " << curr->pid;
        cout << endl;
        curr->is_stopped = false;
        curr->is_background = false;
        s_shell.prev_cmd = s_shell.curr_cmd;
        s_shell.curr_cmd = curr->cmd_line;
        s_shell.curr_job_pid = curr->pid;
        //s_shell.job_list->removeJobById(curr->job_id);
        s_shell.job_list->updateMaxStopped();
        //s_shell.job_list->job_count--;
        int status = 0;
        pid_t p = waitpid(curr->pid, &status, WUNTRACED);
        if (p < 0) {
            delete[] args;
            perror("smash error: waitpid failed");
            return;
        }

        if (WIFSTOPPED(status)) {
            curr->is_stopped = true;
        }

        if (WIFEXITED(status) || WTERMSIG(status) == 9) {

            s_shell.job_list->removeJobById(curr->job_id);
            s_shell.job_list->updateMaxStopped();
            s_shell.job_list->job_count--;
            delete curr;
        }

        s_shell.curr_cmd = s_shell.prev_cmd;
        s_shell.curr_job_pid = -1;


    }

    if (curr == nullptr) {
        cerr << "smash error: fg: job-id " << number << " does not exist" << endl;
    }


    delete[] args;
    return;
}

BackgroundCommand::BackgroundCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void BackgroundCommand::execute() {
    SmallShell &s_shell = SmallShell::getInstance();
    s_shell.job_list->removeFinishedJobs();
    char **args = new char *[COMMAND_MAX_ARGS];
    int number_of_words = _parseCommandLine(cmd_line, args);
    if (number_of_words > 2) {
        delete[] args;
        cerr << "smash error: bg: invalid arguments" << endl;
        return;
    }

    JobEntry *curr = nullptr;
    if (number_of_words == 1) {
        if (s_shell.job_list->job_count == 0) {
            delete[] args;
            cerr << "smash error: bg: there is no stopped jobs to resume" << endl;
            return;
        }

        for (unsigned int i = 0; i < s_shell.job_list->job_vector.size(); i++) {
            if (s_shell.job_list->job_vector[i]->job_id == s_shell.job_list->max_job_id_stopped) {
                curr = s_shell.job_list->job_vector[i];
            }
        }

        if (curr == nullptr) {
            delete[] args;
            cerr << "smash error: bg: there is no stopped job to resume" << endl;
            return;
        }
    }
    if (number_of_words == 2) {

        int i = 0;
        while (args[1][i] != '\0') {
            if (args[1][i] < '0' || args[1][i] > '9') {
                int num = atoi(args[1]);
                if (num < 0) {
                    cerr << "smash error: bg: job-id " << num << " does not exist" << endl;
                    delete[] args;
                    return;
                }

                delete[] args;
                cerr << "smash error: bg: invalid arguments" << endl;
                return;
            }
            i++;
        }
        int number = atoi(args[1]);
        curr = s_shell.job_list->getJobById(number);
        if (curr == nullptr) {
            delete[] args;
            cerr << "smash error: bg: job-id " << number << " does not exist" << endl;
            return;
        }
        if (curr->is_stopped == false) {
            delete[] args;
            cerr << "smash error: bg: job-id " << number << " is already running in the background" << endl;
            return;
        }


    }

    curr->is_stopped = false;
    if (kill(curr->pid, SIGCONT) == -1) {
        perror("smash error: kill failed");
        return;
    }
    cout << curr->cmd_line << " : " << curr->pid << endl;
    s_shell.job_list->updateMaxStopped();

    delete[] args;
    return;
}

QuitCommand::QuitCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void QuitCommand::execute() {
    SmallShell &s_shell = SmallShell::getInstance();
    s_shell.job_list->removeFinishedJobs();
    char **args = new char *[COMMAND_MAX_ARGS];
    int number_of_words = _parseCommandLine(cmd_line, args);
    if (number_of_words > 1) {

        if (strcmp(args[1], "kill") != 0) {
            delete[] args;
            delete this;
            exit(0);
        }
        s_shell.job_list->printForQuit();
        s_shell.job_list->killAllJobs();
    }
    delete[] args;
    delete this;
    exit(0);
}

KillCommand::KillCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void KillCommand::execute() {
    SmallShell &s_shell = SmallShell::getInstance();
    s_shell.job_list->removeFinishedJobs();
    char **args = new char *[COMMAND_MAX_ARGS];
    int number_of_words = _parseCommandLine(cmd_line, args);
    if (number_of_words != 3) {
        delete[] args;
        cerr << "smash error: kill: invalid arguments" << endl;
        return;
    }

    int i = 1;
    while (args[1][i] != '\0') {

        if (args[1][i] < '0' || args[1][i] > '9') {
            delete[] args;
            cerr << "smash error: kill: invalid arguments" << endl;
            return;
        }
        i++;
    }
    if (args[1][0] != '-') {
        delete[] args;
        cerr << "smash error: kill: invalid arguments" << endl;
        return;
    }
    i = 0;
    while (args[2][i] != '\0') {
        if (args[2][i] != '-') {
            if (args[2][i] < '0' || args[2][i] > '9') {
                delete[] args;
                cerr << "smash error: kill: invalid arguments" << endl;
                return;
            }
        }
        i++;
    }
    int signal = -(atoi(args[1]));


    if (signal < 0 || signal > 31) {
        delete[] args;
        cerr << "smash error: kill: invalid arguments" << endl;
        return;
    }


    int jobId = (atoi(args[2]));
    if (jobId == 0) {
        delete[] args;
        cerr << "smash error: kill: invalid arguments" << endl;
        return;
    }

    // arguments legal
    JobEntry *curr = s_shell.job_list->getJobById(jobId);
    if (!curr) {
        delete[] args;
        cerr << "smash error: kill: job-id " << jobId << " does not exist" << endl;
        return;
    }
    if (kill(curr->pid, signal) == -1) {
        perror("smash error: kill failed");
    }
    cout << "signal number " << signal << " was sent to pid " << curr->pid << endl;
    delete[] args;
}

// JOBS & JOBENTRY implementations


JobsList::JobsList() : job_count(0), max_job_id(0), max_job_id_stopped(0), job_vector(vector<JobEntry *>()) {}

JobEntry *JobsList::getJobByPid(pid_t pid) {
    SmallShell &smh = SmallShell::getInstance();
    removeFinishedJobs();
    if (this->job_vector.size() < 1) {
        return nullptr;
    }
    for (unsigned int i = 0; i < smh.job_list->job_vector.size(); i++) {
        if (smh.job_list->job_vector[i]->pid == pid) {
            return smh.job_list->job_vector[i];
        }
    }
    return nullptr;
}

void JobsList::addJob(Command *cmd, pid_t pid, bool isstopped, bool is_background) {

    removeFinishedJobs();
    updateMaxStopped();
    string temp = "";
    temp += cmd->cmd_name;
    string temp1 = "";
    temp1 += cmd->cmd_line;
    if (this->job_vector.empty()) {
        JobEntry *job_to_add = new JobEntry(temp, temp1, pid, is_background, 1, time(nullptr), isstopped);
        this->job_vector.push_back(job_to_add);
        this->job_count = 1;
        this->max_job_id = 1;
        if (isstopped) {
            this->max_job_id_stopped = 1;
        }

    } else {
        updateMaxStopped();
        JobEntry *job_to_add = new JobEntry(temp, temp1, pid, is_background, this->max_job_id + 1, time(nullptr),
                                            isstopped);
        this->job_vector.push_back(job_to_add);
        this->job_count++;
        this->max_job_id++;
        if (isstopped) {
            this->max_job_id_stopped = this->max_job_id;
        }
    }
}

void JobsList::printJobsList() {

    SmallShell &smh = SmallShell::getInstance();
    removeFinishedJobs();
    if (this->job_vector.size() < 1) {
        return;
    }
    for (unsigned int i = 0; i < smh.job_list->job_vector.size(); i++) {

        JobEntry *curr = (smh.job_list->job_vector[i]);
        if (curr) {
            cout << "[" << curr->job_id << "] " << curr->cmd_line << " : " << curr->pid << " "
                 << difftime(time(nullptr), curr->time_started) << " secs";
            if (curr->is_stopped) {
                cout << " (stopped)";
            }
        }
        cout << endl;
    }
}

void JobsList::removeFinishedJobs() {
    SmallShell &s = SmallShell::getInstance();
    if (getpid() != s.smash_pid) {
        return;
    }
    if (this->job_vector.size() < 1) {
        return;
    }
    ///int deleted=0;//warning
    for (unsigned int i = 0; i < s.job_list->job_vector.size();) {
        JobEntry *curr = s.job_list->job_vector[i];
        int status = -1;

        int wait_failing = waitpid(curr->pid, &status, WNOHANG);
        if (wait_failing == -1) {
            perror("smash error: waitpid failed");
            return;
        }
        if (wait_failing != 0) {
            s.job_list->job_vector.erase(job_vector.begin() + i);//warning
            s.job_list->job_count--;
        } else {
            i++;
        }
    }
}


void JobsList::updateMaxStopped() {
    //removeFinishedJobs();
    int curr_max = 0;
    int max_id = 0;

    for (unsigned int i = 0; i < this->job_vector.size(); i++) {
        if (this->job_vector[i]->job_id > curr_max && this->job_vector[i]->is_stopped) {
            curr_max = this->job_vector[i]->job_id;

        }
        if (this->job_vector[i]->job_id > max_id) {
            max_id = this->job_vector[i]->job_id;
        }

    }
    this->max_job_id = max_id;
    this->max_job_id_stopped = curr_max;
}

JobEntry *JobsList::getLastJob(int *lastJobId) {
    (*lastJobId) = this->job_vector.back()->job_id;
    return this->job_vector.back();
}

JobEntry *JobsList::getLastStoppedJob(int *jobId) {
    this->updateMaxStopped();
    return this->job_vector[this->max_job_id_stopped];
}

JobEntry *JobsList::getJobById(int jobId) {
    if (this->job_vector.size() == 0) {
        return nullptr;
    }
    for (unsigned int i = 0; i < this->job_vector.size(); i++) {
        if (this->job_vector[i]->job_id == jobId) {
            return job_vector[i];
        }
    }
    return nullptr;
}

void JobsList::removeJobById(int jobId) {
    for (unsigned int i = 0; i < this->job_vector.size(); i++) {
        if (this->job_vector[i]->job_id == jobId) {
            this->job_vector.erase(this->job_vector.begin() + i);
            updateMaxStopped();
        }
    }
}

void JobsList::killAllJobs() {
    while (!this->job_vector.empty()) {
        JobEntry *curr = this->job_vector.front();
        if (kill(curr->pid, SIGKILL) == -1) {
            perror("smash error: kill failed");
            return;
        }
        this->job_vector.erase(this->job_vector.begin());
    }
}

void JobsList::printForQuit() {

    removeFinishedJobs();
    cout << "smash: sending SIGKILL signal to " << this->job_count << " jobs:" << endl;

    for (unsigned int i = 0; i < this->job_vector.size(); i++) {
        cout << this->job_vector[i]->pid << ": " << this->job_vector[i]->cmd_line;
        cout << endl;
    }
}
//helpers



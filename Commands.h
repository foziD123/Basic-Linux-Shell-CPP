#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>

#include <string>
#include <fstream>
#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

using namespace std;

class Command {

public:
    const char *cmd_name;
    const char *cmd_line;

    explicit Command(const char *cmd_line);

    virtual ~Command() = default;

    virtual void execute() = 0;

    //virtual void prepare();
    //virtual void cleanup();
    // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
public:
    explicit BuiltInCommand(const char *cmd_line) : Command(cmd_line) {}

    virtual ~BuiltInCommand() {}
};

class ChangePromptCommand : public BuiltInCommand {
public:
    explicit ChangePromptCommand(const char *cmd_line);

    virtual ~ChangePromptCommand() {}

    void execute() override;
};

class ExternalCommand : public Command {

public:
    explicit ExternalCommand(const char *cmd_line);

    virtual ~ExternalCommand() {}

    void execute() override;
};

class PipeCommand : public Command {
    string cmd_left;
    string cmd_right;
    bool error;
public:
    explicit PipeCommand(const char *cmd_line);

    virtual ~PipeCommand() {}

    void execute() override;
};

class RedirectionCommand : public Command {
   string curr_dest;
   string curr_cmd;
   int std_out;
   bool is_double;
   bool invalid;
public:
    explicit RedirectionCommand(const char *cmd_line);

    virtual ~RedirectionCommand() {}

    void execute() override;
    void prologue() ;
    void epilogue() ;
};

class ChangeDirCommand : public BuiltInCommand {
public:
    explicit ChangeDirCommand(const char *cmd_line);

    virtual ~ChangeDirCommand() {}

    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
public:
    explicit GetCurrDirCommand(const char *cmd_line);

    virtual ~GetCurrDirCommand() {}

    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
public:
    explicit ShowPidCommand(const char *cmd_line);

    virtual ~ShowPidCommand() {}

    void execute() override;
};

class JobsList;

class JobEntry;

class QuitCommand : public BuiltInCommand {
// TODO: Add your data members
public:
    explicit QuitCommand(const char *cmd_line);

    virtual ~QuitCommand() {}

    void execute() override;
};

class JobEntry {
public:
    string cmd_name;
    string cmd_line;
    pid_t pid;
    bool is_background;
    int job_id;
    time_t time_started;
    bool is_stopped;






    JobEntry(string cmd_name,string cmdLine, pid_t process_id, bool is_background, int job_id, time_t time_started,bool isstopped)
            : cmd_name(cmd_name), cmd_line(cmdLine), pid(process_id),is_background(is_background),
            job_id(job_id),time_started(time_started), is_stopped(isstopped){}

    ~JobEntry()=default;
};

class JobsList {
public:
    int job_count;
    int max_job_id;
    int max_job_id_stopped;
    vector<JobEntry *> job_vector;

    // TODO: Add your data members
    JobsList();

    ~JobsList() = default;

    void addJob(Command* cmd, pid_t pid, bool isstopped = false, bool is_background = false);

    void printJobsList();

    void killAllJobs();

    JobEntry* getJobByPid(pid_t pid);

    void removeFinishedJobs();

    void updateMaxStopped();

    JobEntry *getJobById(int jobId);

    void removeJobById(int jobId);

    JobEntry *getLastJob(int *lastJobId);

    JobEntry *getLastStoppedJob(int *jobId);

    void printForQuit();

    // TODO: Add extra methods or modify exisitng ones as needed
};

class JobsCommand : public BuiltInCommand {
public:
    JobsCommand(const char *cmd_line);

    virtual ~JobsCommand() {}

    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    ForegroundCommand(const char *cmd_line);

    virtual ~ForegroundCommand() {}

    void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    BackgroundCommand(const char *cmd_line);

    virtual ~BackgroundCommand() {}

    void execute() override;
};

class TimeoutCommand : public BuiltInCommand {
/* Optional */
// TODO: Add your data members
public:
    explicit TimeoutCommand(char *cmd_line);

    virtual ~TimeoutCommand() {}

    void execute() override;
};

class FareCommand : public BuiltInCommand {
    /* Optional */
    // TODO: Add your data members
public:
    FareCommand(const char *cmd_line);

    virtual ~FareCommand() {}

    void execute() override;
};

class SetcoreCommand : public BuiltInCommand {
public:
    SetcoreCommand(const char *cmd_line);

    virtual ~SetcoreCommand() {}

    void execute() override;
};

class KillCommand : public BuiltInCommand {
    /* Bonus */
    // TODO: Add your data members
public:
    KillCommand(const char *cmd_line);

    virtual ~KillCommand() {}

    void execute() override;
};

class SmallShell {
private:
    SmallShell();

public:
    string prev_cmd;
    string curr_prompt;
    string prev_dir;
    string father_dir;
    pid_t curr_job_pid;
    JobsList *job_list;
    pid_t smash_pid;
    string curr_cmd;

    Command *CreateCommand(const char *cmd_line);

    SmallShell(SmallShell const &) = delete; // disable copy ctor
    void operator=(SmallShell const &) = delete; // disable = operator

    static SmallShell &getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.

        return instance;
    }

    ~SmallShell();

    void executeCommand(const char *cmd_line);

    void change_prompt_helper(string new_prompt) { this->curr_prompt = new_prompt; }
    // TODO: add extra methods as needed
};

#endif //SMASH_COMMAND_H_

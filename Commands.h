#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <string>
#include <sys/types.h>
#include <unistd.h>

#include <time.h>


#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
using namespace std;
typedef int pid_t;

class SmallShell;
class JobsList ;
class Command {

  static SmallShell* current_shell;
  char* args_[COMMAND_MAX_ARGS];
  int args_num_;
  pid_t cmd_pid_;
  char command_name_[COMMAND_ARGS_MAX_LENGTH];
 public:  
  Command(const char* cmd_line);
  virtual ~Command();
  virtual void execute() = 0;
  //virtual void prepare();
  //virtual void cleanup();


  char* getCommandName(){return this->command_name_;}
  void setArgsValues(char** args);
  void setArgsNum(int num);
  int getNumofArg();
  char* getSpecificArg (int arg_appearance);
  void changeSpecificArg (int arg_appearance, char* new_arg);

};

//**********************built-in commands***************************************//
class BuiltInCommand : public Command {
public:
    BuiltInCommand(const char* cmd_line);
    virtual ~BuiltInCommand() {}
};


class ChpromptCommand : public BuiltInCommand {
    // TODO: Add your data members
    string prompt;
    string new_prompt;
public:
    ChpromptCommand(const char* cmd_line);
    virtual ~ChpromptCommand() {}
    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
public:
    ShowPidCommand(const char* cmd_line);
    virtual ~ShowPidCommand() {}
    void execute() override;
};

class pwdCommand : public BuiltInCommand {
public:
    pwdCommand(const char* cmd_line);
    virtual ~pwdCommand() {}
    void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
    char** last_directory;
public:
    ChangeDirCommand(const char* cmd_line, char** plastPwd);
    virtual ~ChangeDirCommand() {}
    void execute() override;
};

class JobsCommand : public BuiltInCommand {
    // TODO: Add your data members
    JobsList *job_list;
public:
    JobsCommand(const char* cmd_line, JobsList* jobs);
    virtual ~JobsCommand() {}
    void execute() override;
};


class ForegroundCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    ForegroundCommand(const char* cmd_line, JobsList* jobs);
    virtual ~ForegroundCommand() {}
    void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
    // TODO: Add your data members
    JobsList* job_list;
public:
    BackgroundCommand(const char* cmd_line, JobsList* jobs);
    virtual ~BackgroundCommand() {}
    void execute() override;

};

class TimeoutCommand : public BuiltInCommand {
/* Optional */
// TODO: Add your data members
public:
    explicit TimeoutCommand(const char* cmd_line);
    virtual ~TimeoutCommand() {}
    void execute() override;
};

class FareCommand : public BuiltInCommand {
    /* Optional */
    // TODO: Add your data members
public:
    FareCommand(const char* cmd_line);
    virtual ~FareCommand() {}
    void execute() override;
};

class SetcoreCommand : public BuiltInCommand {
    /* Optional */
    // TODO: Add your data members
public:
    SetcoreCommand(const char* cmd_line);
    virtual ~SetcoreCommand() {}
    void execute() override;
};

class KillCommand : public BuiltInCommand {
    /* Bonus */
    // TODO: Add your data members
public:
    KillCommand(const char* cmd_line, JobsList* jobs);
    virtual ~KillCommand() {}
    void execute() override;
};



class GetCurrDirCommand : public BuiltInCommand {
public:
    GetCurrDirCommand(const char* cmd_line);
    virtual ~GetCurrDirCommand() {}
    void execute() override;
};

class ExternalCommand : public Command {
 public:
  ExternalCommand(const char* cmd_line);
  virtual ~ExternalCommand() {}
  void execute() override;
};

class PipeCommand : public Command {
  // TODO: Add your data members
 public:
  PipeCommand(const char* cmd_line);
  virtual ~PipeCommand() {}
  void execute() override;
};

class RedirectionCommand : public Command {
 // TODO: Add your data members
 public:
  explicit RedirectionCommand(const char* cmd_line);
  virtual ~RedirectionCommand() {}
  void execute() override;
  //void prepare() override;
  //void cleanup() override;
};









class JobsList;
class QuitCommand : public BuiltInCommand {
// TODO: Add your data members
public:
  QuitCommand(const char* cmd_line, JobsList* jobs);
  virtual ~QuitCommand() {}
  void execute() override;
};


class JobsList {
 public:
  class JobEntry {

  int job_id;
  Command* command;
  bool is_stopped;
  time_t job_starting_time;
  int procces_pid;
  public:

  JobEntry(int id, Command* command,bool is_stopped=false)
          :job_id(id),command(command),is_stopped(is_stopped){}
  int getJobId(){return this->job_id;}
  bool isJobStopped(){return this->is_stopped;}
  time_t getJobStartingTime(){return this->job_starting_time;}
  int getProccesPid(){return this->procces_pid;}
  Command* getCommand(){return this->command;}
  void setJobStatus(bool status){this->is_stopped=status;}
  void stopJob();
  void continueJob();
  };
 // TODO: Add your data members
 private:
  vector<JobEntry*> jobs_list;

 public:
  JobsList();
  ~JobsList();
  void addJob(Command* cmd, bool isStopped = false);
  void printJobsList();
  void killAllJobs();
  void removeFinishedJobs();// to be done
  JobEntry * getJobById(int jobId);
  void removeJobById(int jobId);// to be done
  JobEntry * getLastJob(int* lastJobId);
  JobEntry *getLastStoppedJob(int *jobId);
  // TODO: Add extra methods or modify exisitng ones as needed
  vector<JobEntry*> getJobList(){return this->jobs_list;}
  static int last_job_id;
};
int JobsList::last_job_id=0;



class SmallShell {
private:
    std::string prompt_message_ = "smash";
    const pid_t shell_pid_;
    pid_t fore_pid_;
    JobsList* jobs_list_;
    char* shell_prev_dir_;


    SmallShell();
public:
    Command *CreateCommand(const char* cmd_line);
    SmallShell(SmallShell const&)      = delete; // disable copy ctor
    void operator=(SmallShell const&)  = delete; // disable = operator
    static SmallShell& getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }
    ~SmallShell();
    void executeCommand(const char* cmd_line);


    void setMessage(std::string new_message);
    std::string getMessage();
    const pid_t getShellPid();
    void setForePid(pid_t new_pid);
    JobsList* getJobsList();
    char* getPrevDir();
};

#endif //SMASH_COMMAND_H_

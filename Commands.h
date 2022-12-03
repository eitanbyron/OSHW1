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

enum RedirectionType {kOverride , kAppend};


class SmallShell;
class JobsList;

class Command {

  static SmallShell* current_shell;
  
  int args_num_;
  pid_t cmd_pid_;
  bool is_background_ = false;
  char command_name_[COMMAND_ARGS_MAX_LENGTH];

 public:  
  Command(const char* cmd_line);
  virtual ~Command() = default;
  virtual void execute() = 0;
  //virtual void prepare();
  //virtual void cleanup();
  char* getCommandName(){return this->command_name_;}
  int getNumofArgs();
  void setNumofArgs(int new_num);
  void connectShell(SmallShell* smash);
  pid_t getShellPid();
  pid_t getProccesPid(); // to implement
  void setPid(pid_t process_pid); // to implemnt
  void setPrevDir(char* new_prev_dir);
  char* args_[COMMAND_MAX_ARGS];
  SmallShell* getSmallShell(){return this->current_shell;}
};


//**********************Built-in commands***************************************//
class BuiltInCommand : public Command {
public:
    BuiltInCommand(const char* cmd_line);
    virtual ~BuiltInCommand() {}
};


class ChpromptCommand : public BuiltInCommand {
    // TODO: Add your data members
    string prompt;
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



class GetCurrDirCommand : public BuiltInCommand {
public:
    GetCurrDirCommand(const char* cmd_line);
    virtual ~GetCurrDirCommand() {}
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
    JobsList* jobs_list;
    int job_id;
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

class QuitCommand : public BuiltInCommand {
    JobsList* jobs_list;
public:
    QuitCommand(const char* cmd_line, JobsList* jobs);
    virtual ~QuitCommand() {}
    void execute() override;
};

class KillCommand : public BuiltInCommand {
    /* Bonus */
    // TODO: Add your data members
    JobsList* job_list;
public:
    KillCommand(const char* cmd_line, JobsList* jobs);
    virtual ~KillCommand() {}
    void execute() override;
};


//**********************External and Special commands***************************************//

class ExternalCommand : public Command {
  char* cmd;
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
    RedirectionType type_;
    const char* only_command_;
    const char* output_file;

 public:
  explicit RedirectionCommand(const char* cmd_line);
  virtual ~RedirectionCommand() {}
  void execute() override;
  //void prepare() override;
  //void cleanup() override;
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

class TimeoutCommand : public BuiltInCommand {
/* Optional */
// TODO: Add your data members
public:
    explicit TimeoutCommand(const char* cmd_line);
    virtual ~TimeoutCommand() {}
    void execute() override;
};



class JobsList {
 public:
  class JobEntry {

  int job_id;
  Command* command;
  bool is_stopped;
  time_t job_starting_time; //need to add set for this field 
  int procces_pid;

  public:

  JobEntry(int id, Command* command,bool is_stopped=false)
          :job_id(id),command(command),is_stopped(is_stopped){}
  int getJobId(){return this->job_id;}
  bool isJobStopped(){return this->is_stopped;}
  time_t getJobStartingTime(){return this->job_starting_time;}
  int getProccesPid(){return this->procces_pid;}  

  void setProccesPid(){procces_pid=command->getProccesPid();} 

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
  vector<JobEntry>* getJobsList(){return this->jobs_list;}
  static int max_job_id;
};
int JobsList::max_job_id=0;






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
    void setPrevDir(char* new_prev_dir);
};

#endif //SMASH_COMMAND_H_

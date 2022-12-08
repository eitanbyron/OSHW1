#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <string>
#include <list>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <fstream>


#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
using namespace std;
typedef int pid_t;

enum RedirectionType {kOverride , kAppend};
enum PipeType {kOut =1 , kErr =2};



class SmallShell;
class JobsList;

class Command {

  //static SmallShell* current_shell;
  
  int args_num_;
  pid_t cmd_pid_;
  bool is_background_ = false;
  bool is_pipe_ =false;
  char command_name_[COMMAND_ARGS_MAX_LENGTH];
  time_t procces_starting_time;

 public:  
  static SmallShell* current_shell;

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
  pid_t getProccesPid();
  void setPid(pid_t process_pid);
  void setPrevDir(char* new_prev_dir);
  char* args_[COMMAND_MAX_ARGS];
  void makePipe();
  bool isPipe();
};


//**********************Built-in commands***************************************//
class BuiltInCommand : public Command {
public:
    BuiltInCommand(const char* cmd_line);
    virtual ~BuiltInCommand() {}
};


class ChpromptCommand : public BuiltInCommand {
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
    JobsList* job_list;
public:
    //BackgroundCommand(const_cast* cmd_line , JobsList* job_list):BuiltInCommand(cmd_line),job_list(job_list){}
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
  const char* cmd;
 public:
  ExternalCommand(const char* cmd_line);
  virtual ~ExternalCommand() {}
  void execute() override;
};

class PipeCommand : public Command {
   PipeType type_;
   const char* right_command_;
   const char* left_command_;


 public:
  PipeCommand(const char* cmd_line);
  virtual ~PipeCommand() {}
  void execute() override;
    static pid_t pid_right;
    static pid_t pid_left;
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
    int counter;
    string source;
    string destination;
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

class TimeoutCommand : public Command {
    const char* only_command_;
    time_t end_time;
    Command* the_cmd;
    bool finish;
    int duration;
    string command_str;
public:
    explicit TimeoutCommand(const char* cmd_line);
    virtual ~TimeoutCommand() {}
    void execute() override;
    //getEndTime(){return this->end_time;}

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
          :job_id(id),command(command),is_stopped(is_stopped){
            procces_pid=this->command->getProccesPid();
            time(&(job_starting_time));
          }
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
  vector<JobEntry>* jobs_list;
    
 public:
   int max_job_id;

  JobsList(){jobs_list=new vector<JobEntry>;
             max_job_id=0;}
  ~JobsList();
  void addJob(Command* cmd, bool isStopped = false);
  void printJobsList();
  void killAllJobs();
  void removeFinishedJobs();
  JobEntry * getJobById(int jobId);
  void removeJobById(int jobId);
  JobEntry * getLastJob(int* lastJobId);
  JobEntry *getLastStoppedJob(int *jobId);
  vector<JobEntry>* getJobsList(){return this->jobs_list;}
};
//int JobsList::max_job_id=0;






class SmallShell {
private:
    std::string prompt_message_ = "smash";
    const pid_t shell_pid_;
    pid_t fore_pid_;
    JobsList* jobs_list_;
    char* shell_prev_dir_;
    Command* curr_external;


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
    Command* getCurrExternal();
    void setCurrExternal (Command* new_external);
    pid_t getForePid(){return this->fore_pid_;}
    std::list<TimeoutCommand> timeout_list_;

};

#endif //SMASH_COMMAND_H_

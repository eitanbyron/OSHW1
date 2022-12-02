#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";
enum CmdType {kOrdinary , kRedirection, kPipe};

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

string _ltrim(const std::string& s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s)
{
  return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char* cmd_line, char** args) {
  FUNC_ENTRY()
  int i = 0;
  std::istringstream iss(_trim(string(cmd_line)).c_str());
  for(std::string s; iss >> s; ) {
    args[i] = (char*)malloc(s.length()+1);
    memset(args[i], 0, s.length()+1);
    strcpy(args[i], s.c_str());
    args[++i] = NULL;
  }
  return i;

  FUNC_EXIT()
}

bool _isBackgroundComamnd(const char* cmd_line) {
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char* cmd_line) {
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

CmdType checkCommandType (const char* cmd_line)
{
    string cmd_str(cmd_line);
    size_t check = cmd_str.npos;
    if ((cmd_str.find(">>") != check) || (cmd_str.find('>') != check))
        return kRedirection;
    if ((cmd_str.find("|&") != check) || (cmd_str.find('|') != check))
        return kPipe;
    return kOrdinary;
}

//*************************Command implementation******************************///

int Command::getNumofArg() {
    return this->args_num_;
}

char* Command::getSpecificArg(int arg_appearance) {
    if (arg_appearance > args_num_ -1)
        return nullptr;
    return this->args_[arg_appearance];
}

void Command::connectShell(SmallShell *smash) {
    Command::current_shell = smash;
}

pid_t Command::getShellPid() {
    return current_shell->getShellPid();
}

void Command::setPrevDir(char *new_prev_dir) {
    this->current_shell->setPrevDir(new_prev_dir);
}

Command::Command(const char *cmd_line) {

    this->cmd_pid_ =-1;
    this->is_background_ = _isBackgroundComamnd(cmd_line);
    for(int i=0; i<COMMAND_MAX_ARGS; i++)
        this->args_[i] = nullptr;
    this->args_num_ = _parseCommandLine(cmd_line, args_) + 1;//parse return num of args or num -1?
    if (this->is_background_)
    {
        for (int i =0; i<args_num_; i++)
        {
         if(_isBackgroundComamnd(this->args_[i]))
             _removeBackgroundSign(this->args_[i]);
        }
    }
}


//*************************Built-in commands******************************///
BuiltInCommand::BuiltInCommand(const char *cmd_line) : Command(cmd_line){}

ShowPidCommand::ShowPidCommand(const char *cmd_line) : BuiltInCommand(cmd_line){}

void ShowPidCommand::execute() {
    std::cout << "smash pid is " <<this->getShellPid()<<std::endl;
}

ChangeDirCommand::ChangeDirCommand(const char *cmd_line, char **plastPwd) : BuiltInCommand(cmd_line) {
    last_directory = plastPwd;
}

void ChangeDirCommand::execute() {
    int args_num = this->getNumofArg();
    if (args_num <= 1)
        return;
    if (args_num > 2)
    {
        std::cerr<<"smash error: cd: too many arguments"<<std::endl;
        return;
    }
    char curr_dir[COMMAND_ARGS_MAX_LENGTH];
    getcwd(curr_dir, sizeof(curr_dir)); //getcwd function from unistd.h
    if (this->getSpecificArg(1) == "-") {
        char *prev_dir = *(this->last_directory);
        if (prev_dir == nullptr){
            std::cerr<<"smash error: cd: OLDPWD not set"<<std::endl;
            return;
        }
        else if (chdir(prev_dir) != 0) //chdir function from unistd.h
        {
            perror("smash error: chdir failed");
            return;
        }
        }
        else if (chdir(this->getSpecificArg(1)) != 0) {
            std::cerr << "smash error: chdir failed" << std::endl;
            return;
        }

    char* temp_prev_dir = SmallShell::getInstance().getPrevDir();
    delete [] temp_prev_dir;
    this->setPrevDir(curr_dir);
}


ForegroundCommand::ForegroundCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line) {
    this->jobs_list = jobs;
}


void ForegroundCommand::execute() {
    int args_num = this->getNumofArg();
    if (args_num == 1)
        this-> job_id = -1;
    else if (args_num == 2)
    {
        try {
            this->job_id = stoi(this->getSpecificArg(1));
            if (job_id <= 0)
            {
                std::cerr << "smash error: fg: job-id " << job_id << " does not exist" <<std::endl;
                return;
            }
        }
        catch  (std::invalid_argument &e) {
            std::cerr << "smash error: fg: invalid arguments" << std::endl;
            return;
        }
    }
    else {
        std::cerr << "smash error: fg: invalid arguments" << std::endl;
        return;
    }

    this->jobs_list->removeFinishedJobs();
    JobsList::JobEntry* the_job;

    if (job_id == -1)
       the_job = this->jobs_list->getLastJob(&job_id);
    else
        the_job = this->jobs_list->getJobById(job_id);

    if (the_job)
    {
        int wait_status;
        pid_t command_pid = the_job->getProccesPid();
        // TODO: add function that set the current running external
        this->getSmallShell()->setForePid(command_pid);
        std::cout << the_job->getCommand()->getCommandName() << " : " << the_job->getProccesPid();
        bool stopped_status = the_job->isJobStopped();
        this->jobs_list->removeJobById(job_id);
        if (stopped_status)
        {
           //TODO: boolean check if command isnt pipe.
            {
                if (kill(command_pid,SIGCONT) == -1)
                {
                 perror("smash error: kill failed");
                 return;
                }
            }
            //TODO: boolean check if command is pipe.
            {
                if (killpg(command_pid,SIGCONT) == -1)
                {
                    perror("smash error: killpg failed");
                    return;
                }
            }
        }
        if (waitpid(command_pid, &wait_status, WSTOPPED) == -1)
        {
            perror("smash error: waitpid failed");
            return;
        }
    }
    else
    {
       if (job_id == -1)
           std::cerr << "smash error: fg: jobs list is empty" << std::endl;
       else
           std::cerr << "smash error: fg: jobs list is empty" << std::endl;
    }
}



QuitCommand::QuitCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line) {
    this->jobs_list = jobs;
}

void QuitCommand::execute() {
    bool kill_all =false;
    for (int i=0; i<this->getNumofArg(); i++)
    {
        if (this->getSpecificArg(i) == "kill"){
            kill_all = true;
            break;
        }
    }

    if (kill_all)
    {
        jobs_list->removeFinishedJobs();
        int jobs_num = this->jobs_list->getJobList().size(); 
        std::cout << "smash: sending SIGKILL signal to" <<jobs_num << "jobs:" <<std::endl;
        jobs_list->killAllJobs();
    }
}




ChpromptCommand::ChpromptCommand(const char *cmd_line) :BuiltInCommand(cmd_line),prompt("smash"){}

void ChpromptCommand::execute()
{
  if( getNumofArg()<=1) {
    prompt="smash";
  }else{
    prompt=this->args_[1];
  }
  getSmallShell()->setMessage(prompt);
}

GetCurrDirCommand::GetCurrDirCommand(const char* cmd_line):BuiltInCommand(cmd_line){}

void GetCurrDirCommand::execute()
{
  char buf[COMMAND_ARGS_MAX_LENGTH];
  getcwd(buf,sizeof(buf));
  std::cout<<buf<<std::endl;
}

JobsCommand::JobsCommand(const char* cmd_line , JobsList* job_list):BuiltInCommand(cmd_line),job_list(job_list){}

void JobsCommand::execute()
{
  this->job_list->printJobsList();
}



KillCommand::KillCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line) , job_list(jobs){}

void KillCommand::execute()
{
  if(args_[1]==nullptr || args_[1][0]!= '-' || args_[2]==nullptr || arg[3]==nullptr)
  {
      std::cerr<<"smash error: kill: invalid arguments";
      return;
  } 
  int sig_num;
  try{
     sig_num=stoi(args_[1]);
    }catch(std::invalid_argument &e){
      std::cerr << "smash error: kill: invalid arguments";
      return;
    }
    sig_num=sig_num*(-1);
    if(sig_num<1)
    {
      std::cerr << "smash error: kill: invalid arguments";
      return;
    }
    int job_id;
    try{
     job_id=stoi(args_[2]);
    }catch(std::invalid_argument &e){
      std::cerr << "smash error: kill: invalid arguments";
      return;
    }
    JobsList::JobEntry* job_to_handle=this->job_list->getJobById(job_id);
    if(!job_to_handle)
    {
      cerr<<"smash error: kill: job-id "<<job_id<<" does not exist";
      return;
    }
    if(kill(job_to_handle->getProccesPid(),sig_num)==0)
    {
      cout << "signal number " << sig_num << " was sent to pid " << job_to_handle->getProccesPid() << endl;
    }else{
      perror ("smash error: kill failed");
    }
}

BackgroundCommand::BackgroundCommand(const_cast* cmd_line , JobsList* job_list):BuiltInCommand(cmd_line),job_list(job_list){}

void BackgroundCommand::execute()
{
  if (getNumofArg() > 2)
  {
    std::cerr << "smash error: bg: invalid arguments";
    return;
  }
  int job_id=-1;
  JobsList::JobEntry*  job_to_resume=nullptr;
  if(getNumofArg()==1)
  {
    job_id=this->job_list->getLastStoppedJob()->getJobId();
    job_to_resume=this->job_list->getJobById(job_id);
    if((!job_to_resume) || (job_id==-1))
    {
      std::cerr << "smash error: bg: there is no stopped jobs to resume";
      return;
    }
  }else if(getNumofArg()==2){
    try{
     job_id=stoi(args_[1]);
    }catch(std::invalid_argument &e){
      std::cerr << "smash error: bg: invalid arguments";
      return;
    }
    job_to_resume=this->job_list->getJobById(job_id); 
    if(!job_to_resume)
    {
     std::cerr << "smash error: bg: job-id " << job_id << " does not exist" ;    
     return;
    }else if(!(job_to_resume->isJobStopped())){
      std::cerr << "smash error: bg: job-id "<< job_id <<" is already running in the background";
      return;
    }
  }
 
  cout<<job_to_resume->getCommand()->getCommandName()<<" : "<<job_to_resume->getProccesPid()<<endl;
  if( kill( job_to_resume->getProccesPid(), SIGCONT)!=0 )
  {
      perror ("smash error: kill failed");
      return;
  }
}


//*************************JobsList implementation******************************///
JobsList::JobEntry::JobEntry(int id, Command* command,bool is_stopped=false):job_id(id),command(command),is_stopped(is_stopped){}
  
void JobsList::JobEntry::stopJob()
{
  this->setJobStatus(true);
}

void JobsList::JobEntry::continueJob()
{
  this->setJobStatus(false);
}


void JobsList::addJob(Command* cmd, bool isStopped = false)
{
  removeFinishedJobs();
  int new_job_id=0;
  vector<JobEntry>* job_list_to_update=this->getJobList();
  if(job_list_to_update->empty())
  {
    new_job_id=1;
    this->last_job_id=new_job_id;
  }else{
    int last_job_id=job_list_to_update->rbegin()->getJobId();
    new_job_id=last_job_id+1;
  }
  this->last_job_id++;
  JobsList::JobEntry* new_job= new JobsList::JobEntry(this->last_job_id,cmd,isStopped);
  job_list_to_update->push_back(new_job);
}

void JobsList::killAllJobs()
{
  vector<JobEntry>::iterator job_to_kill;
  vector<JobEntry>* job_list_to_kill=this->getJobList();

  for(job_to_kill=job_list_to_kill->begin()  ; job_to_kill!=job_list_to_kill->end() ;   job_to_kill++)
  {
    if(kill(job_to_kill->getProccesPid(),SIGKILL)==-1)
    {
      perror("smash error: kill failed");
      return;
    }
  }
}

void JobsList::removeFinishedJobs()
{

}

JobsList::JobEntry* JobsList::getJobById(int jobId)
{
  removeFinishedJobs();
  JobEntry* to_return=nullptr;
  vector<JobEntry>::iterator curr=this->getJobList().begin();
  while (curr!=this->getJobList().end())
  {
    int curr_id=curr->getJobId();
    if(curr_id==jobId)
    {
      to_return=&curr;
      return to_return;
    }
    curr++
  }
  return to_return;
}

JobsList::JobEntry *JobsList::getLastStoppedJob(int *jobId)
{
  JobEntry* last_stopped_job=nullptr;
  vector<JobEntry>::iterator curr=this->getJobList().begin();
  vector<JobEntry>::iterator list_end=this->getJobList().end();
  while(curr!=list_end)
  {
    bool is_stopped=curr->isJobStopped();
    if(is_stopped)
    {
      jobId=curr->getJobId();
      last_stopped_job=&*(curr);
    }
    curr++;
  }
  return last_stopped_job;
}

JobsList::JobEntry *JobsList::getLastJob(int *lastJobId)
{
  if(!(this->getJobList()))
  {
    return nullptr;
  }
  lastJobId=this->getJobList().back()->getJobId();//need to check type
  return this->getJobList().back();//need to check return value
}


void JobsList::printJobsList() 
{
  removeFinishedJobs();
  vector<JobEntry>* job_list_to_print=this->getJobList();
  
  vector<JobEntry>::iterator current_job=job_list_to_print->begin();
  //iteration over all the jobs list
  for(current_job  ; current_job != job_list_to_print->end(); current_job++)
  {
      
    time_t time_diffrential=difftime(time(nullptr),current_job->getJobStartingTime());
    if(!(current_job->isJobStopped()))
    {
      count<<"["<<current_job->getJobId()<<"] "<<current_job->getCommand()->getCommandName()<<" : "
            <<current_job->getJobId()<<" "<<time_diffrential<<" secs"<<endl;
    }else{
      cout<<"["<<current_job->getJobId()<<"] "<<current_job->getCommand()->getCommandName()<<" : "
            <<current_job->getJobId()<<" "<<time_diffrential<<" secs"<<" (stopped)"<<endl;
    }
  }
}


//************************* SmallShell implementation******************************///

void SmallShell::setMessage(std::string new_message) {
    this->prompt_message_=new_message;
}

std::string SmallShell::getMessage() {
    return this->prompt_message_;
}

const pid_t SmallShell::getShellPid() {
    return this->shell_pid_;
}

void SmallShell::setForePid(pid_t new_pid) {
   this->fore_pid_=new_pid;
}

JobsList* SmallShell::getJobsList() {
    return this->jobs_list_;
}

char* SmallShell::getPrevDir() {
    return this->shell_prev_dir_;
}

void SmallShell::setPrevDir(char *new_prev_dir) {
    this->shell_prev_dir_=new_prev_dir;
}


SmallShell::SmallShell(): shell_pid_(getpid())  //getpid function from unistd.h
{
    this->fore_pid_=-1;
    this->jobs_list_ = new JobsList();
    this->shell_prev_dir_ = nullptr;
}

SmallShell::~SmallShell() {
    delete this->jobs_list_;
}



/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
    char* args[COMMAND_MAX_ARGS];
    for(int i=0; i<COMMAND_MAX_ARGS; i++)
        args[i] = nullptr;
    int args_num = _parseCommandLine(cmd_line, args);
    if (args[0] == nullptr)
        return nullptr;
    string first_word = args[0];
    switch (checkCommandType(cmd_line)){
        case kOrdinary: {
            if ((first_word == "chprompt") || (first_word == "chprompt&"))
                return new ChpromptCommand(cmd_line);
            if ((first_word == "showpid") || (first_word == "showpid&"))
                return new ShowPidCommand(cmd_line);
            if ((first_word == "pwd") || (first_word == "pwd&"))
                return new GetCurrDirCommand(cmd_line);
            if ((first_word == "cd") || (first_word == "cd&")) {
                char** prev_dir_pointer = &(this->shell_prev_dir_);
                return new ChangeDirCommand(cmd_line, prev_dir_pointer);
            }
            if ((first_word == "jobs") || (first_word == "jobs&"))
                return new JobsCommand(cmd_line, this->getJobsList());
            if ((first_word == "fg") || (first_word == "fg&"))
                return new ForegroundCommand(cmd_line, this->getJobsList());
            if ((first_word == "bg") || (first_word == "bg&"))
                return new BackgroundCommand(cmd_line, this->getJobsList());
            if ((first_word == "quit") || (first_word == "quit&"))
                return new QuitCommand(cmd_line, this->getJobsList());
            if ((first_word == "kill") || (first_word == "kill&"))
                return new KillCommand(cmd_line, this->getJobsList());
            if ((first_word == "timeout") || (first_word == "timeout&"))
                return new TimeoutCommand(cmd_line);
            return new ExternalCommand(cmd_line);
        }
        case kPipe:
            return new RedirectionCommand(cmd_line);
        case kRedirection:
            return new PipeCommand(cmd_line);
    }
  return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
  Command* cmd = CreateCommand(cmd_line);
  if (cmd)
  {
    cmd->connectShell(this);
    cmd->execute();
  }
}


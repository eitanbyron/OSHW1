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
enum SpecialCmd {kRedirection =1, kPipe =2};
enum JobState {kForeground, kBackground};

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


int pipeorredirection (const char* cmd_line)
{
    string cmd_str(cmd_line);
    size_t check = cmd_str.npos;
    if ((cmd_str.find(">>") != check) || (cmd_str.find(">") != check))
        return kRedirection;
    if ((cmd_str.find("|&") != check) || (cmd_str.find("|") != check))
        return kPipe;
    return 0;
}

SmallShell::SmallShell() : shel_Pid(getpid())
{
    this->shell_prev_dir = nullptr;
    this->curr_external_cmd = nullptr;
    this->current_prompt = "smash";
    this->fore_pid = -1;
    this->jobs_list =new JobsList();

    Command::current_shell =this; //???
}

const int SmallShell::getShellpid() {
    return this->shell_pid;
}

void SmallShell::setForePid(int new_pid)
{
    this->fore_pid = new_pid;
}

void SmallShell::setCurrExternal(ExternalCommand* new_cmd)
{
    this->curr_external_cmd = new_cmd;
}

int SmallShell::getForePid(){
    return forepid;
}

ExternalCommand *SmallShell::getCurrExternalCmd()
{
    return curr_external_cmd;
}

SmallShell::~SmallShell() {
    delete this->jobs_list;
    delete [] this->shell_prev_dir
}

Command* SmallShell::CreateCommand(const char *cmd_line) {
    string cmd_s = _trim(string(cmd_line));
    string first_word =cmd_s.substr(0, cmd_s.find_first_of(" \n"));
    //need to add implemation to args in class command
    if (pipeorredirection(cmd_line) == kRedirection){
        return new RedirectionCommand(cmd_line);}
   else if  (pipeorredirection(cmd_line) == kPipe){
        return new PipeCommand(cmd_line);}
   else if (pipeorredirection(cmd_line) == 0)
    {
        if (first_word.compare("chprompt") == 0)
            return new chpromptCommand(cmd_line);
        if (first_word.compare("showpid") == 0)
            return new ShowPidCommand(cmd_line);
        if (first_word.compare("pwd") == 0)
            return new GetCurrDirCommand(cmd_line);
        if (first_word.compare("cd") == 0)
        {
            char** prev_dir_ptr = &(this->shell_prev_dir);
            return new ChangeDirCommand(cmd_line);
        }
        if (first_word.compare("jobs") == 0)
            return new JobsCommand(cmd_line, this->getJobsList());
        if (first_word.compare("fg") == 0)
            return new ForegroundCommand(cmd_line, this->getJobsList());
        if (first_word.compare("bg") == 0)
            return new ForegroundCommand(cmd_line, this->getJobsList());
        if (first_word.compare("quit") == 0)
            return new QuitCommand(cmd_line, this->getJobsList());
        if (first_word.compare("kill") == 0)
            return new ForegroundCommand(cmd_line, this->getJobsList());
        return ExternalCommand(cmd_line);
    }

    return nullptr;
}


void SmallShell::resume(int job_id, JobState state)
{
    this->jobs_list->removeFinishedJobs();
    JobsList::JobEntry* job = jobs_list->getJob(job_id);
    if (job_id == -1)
    {
        if (state == kForeground)
            job = this->jobs_list->getLastJob(&job_id);
        if (state == kBackground)
            job = this->jobs_list->getLastStoppedJob(&job_id);
    }
    if (job != nullptr)
    {
        if (state == kForeground)
        {
            int wait_status;
            this->set

        }
    }

}

ShowPidCommand::ShowPidCommand(const char *cmd_line): BuiltInCommand(cmd_line){
}

void ShowPidCommand::execute() {
    string to_print="smash pid is ";
    std::cout << to_print << this->current_shell->getShellpid()<<std::endl; //getShellpid() implementation??
}

void ChangeDirCommand::ChangeDirCommand(const char *cmd_line, char **plastPwd):BuiltInCommand(cmd_line)
{
    last_directory = plastPwd;
}

void ChangeDirCommand::execute() {
    int args_num = this->getNumofArg();
    if (args_num >2)
    {
        std:cerr <<"smash error: cd: too many arguments"<<std:endl;
        return;
    }
    if (args_num == 1)
        return;
    char* current_prev = get_current_dir();  //get_current_dir() implementation??
    if (args[1] == "-") {
        char *prev_dir = *(this->last_directory);
        if (!prev_dir)
        {
            std:cerr <<"smash error: cd: OLDPWD not set"<<std:endl;
            return;
        }
        if (chdir(prev_dir) != 0)
        {
            std:cerr <<"smash error: cd: chdir failed"<<std:endl;
            delete [] current_prev;
            return;
        }
    }
     if (chdir(args[1]) != 0)
    {
        std:cerr <<"smash error: cd: chdir failed"<<std:endl;
        delete [] current_prev;
        return;
    }

    delete [] this->current_shell->shell_prev_directory; //shellprevdirectory implementation??
    this->current_shell->shell_prev_directory = current_prev;

}


ForegroundCommand::ForegroundCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line)
{
    this->job_list = jobs;  //joblist implementation??
    if(getNumofArg() == 1)
        this->job_id = -1;  //jobid implementation??
    else if (getNumofArg() ==2)
    {
        int temp = stoi(args[1]);
        job_id = temp;
        if (this->jobid <= 0) {
            std:cerr << "smash error: fg: job-id " + to_string(job_id) + " does not exist" << std:endl;
            return;
        }
    }
    else
    {
        std:cerr <<"smash error: fg: invalid arguments"<<std:endl;
        return;
    }
}

void ForegroundCommand::execute() {
    this->current_shell->resume(job_id,kForeground); //resume implementation??
}

QuitCommand::QuitCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line){
    this->jobs_list = jobs;//joblist implementation??
}

void QuitCommand::execute() {
    int args_num = getNumofArg();
    bool  kill_all =false;
    for (int i = 0; i < args_num; i++)
    {
        if (args[i] == "kill")
        {
            kill_all = true;
            break;
        }
    }

    if (kill_all = true)
    {
        job_list->removeFinishedJobs();
        list<JobsList::JobEntry>* temp_list = (job_list->get_list()); //get_list implementation??
        int jobs_num = temp_list->size();
        std:cout <<"smash: sending SIGKILL signal to " + jobs_num + " jobs:"<<std:endl;
        jobs_list->killAllJobs();
    }
    exit (0);
}

chpromptCommand::chpromptCommand(const char *cmd_line) :BuiltInCommand(cmd_line),prompt("smash"){
  if( getNumofArg()<=1) {
    new_prompt=this->prompt;
  }else{
    new_prompt=this->args[1];
  }
}

pwdCommand::pwdCommand(const char* cmd_line):BuiltInCommand(cmd_line){}
void pwdCommand::execute()
{
  char buf[COMMAND_ARGS_MAX_LENGTH];
  getcwd(buf,sizeof(buf));
  std::<<cout<<buf<<std::endl;
}

JobsCommand::JobsCommand(const char* cmd_line , JobsList* job_list):BuiltInCommand(cmd_line),job_list(job_list){}

void JobsCommand::execute()
{
  this->job_list->printJobsList();
}

BackgroundCommand::BackgroundCommand(const_cast* cmd_line , JobsList* job_list):BuiltInCommand(cmd_line),job_list(job_list){}

void BackgroundCommand::execute()
{

  if (getNumofArg() > 2)
  {
    // TODO: handle errors
  }

  int job_id = -1;
  JobsList::JobEntry *job_entry = new JobsList::JobEntry(0, nullptr, false);
  
  if (getNumofArg() == 1)
  {
    job_entry = job_list->getLastStoppedJob(&job_id);
    if (job_entry->isStopped())
    {
      // TODO: print jobentry->cmdLine
      //TODO: resume job entry
    }

  }
  else if (getNumofArg() == 2)
  {
    job_entry = job_list->getJobById(stoi(args[1]));
    
    if (!job_entry)
    {
      // TODO: print error message - job does not exist

    }
    
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
      char kill_err[]="smash error: kill failed";
      perror(kill_err);
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
  JobEntry* to_return;
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




SmallShell::SmallShell() {
// TODO: add your implementation
}

SmallShell::~SmallShell() {
// TODO: add your implementation
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
	// For example:
/*
  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

  if (firstWord.compare("pwd") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }
  else if (firstWord.compare("showpid") == 0) {
    return new ShowPidCommand(cmd_line);
  }
  else if ...
  .....
  else {
    return new ExternalCommand(cmd_line);
  }
  */
  return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
  // TODO: Add your implementation here
  // for example:
  // Command* cmd = CreateCommand(cmd_line);
  // cmd->execute();
  // Please note that you must fork smash process for some commands (e.g., external commands....)
}

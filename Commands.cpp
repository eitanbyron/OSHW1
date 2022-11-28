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


void SmallShell::re

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
    if (arg[1] == "-") {
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
     if (chdir(arg[1]) != 0)
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
        int temp = stoi(arg[1]);
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
    this->current_shell->resume(job_id,1); //resume implementation??
}

QuitCommand::QuitCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line){
    this->jobs_list = jobs;//joblist implementation??
}

void QuitCommand::execute() {
    int args_num = getNumofArg();
    bool  kill_all =false;
    for (int i = 0; i < args_num; i++)
    {
        if (arg[i] == "kill")
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
    new_prompt=this->arg[1];
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

void BackgroundCommand::execute(){
  
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

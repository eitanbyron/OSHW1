#include <unistd.h>
#include <string.h>
#include <iostream>
#include "Commands.h"
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "fcntl.h"

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";
enum CmdType {kOrdinary , kRedirection, kPipe};
pid_t PipeCommand::pid_right = -2;
pid_t PipeCommand::pid_left = -2;

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
TimeoutCommand::TimeoutCommand(const char *cmd_line) : Command(cmd_line) 
{
    this->finish=false;    
}
void TimeoutCommand::execute()
{
    if(this->getNumofArgs()<3)
    {
        std::cerr<<"smash error: timeout: invalid arguments";
        return ;
    }
    try{
        int dur=stoi(args_[1]);
        if(dur<=0)
        {
            std::cerr<<"smash error: timeout: invalid arguments";
            return;
        }else{
            this->duration=dur;
        }
    }catch(std::invalid_argument& e){
        std::cerr<<"smash error: timeout: invalid arguments";
        return;
    }
    this->command_str=this->args_[2];
    //  for (int i = 3; i < getNumofArg(); i++) {
    //      command_str = command_str + " " + arg[i];
    //  }
    this->the_cmd=  SmallShell::getInstance().CreateCommand(this->command_str.c_str());
    if(!the_cmd)return;
   SmallShell::getInstance().timeout_list_.push_back(*this);
    time_t s_time=time(nullptr);
    end_time=s_time+duration;
    alarm(this->duration);
    the_cmd->execute();

}

int Command::getNumofArgs() {
    return this->args_num_;
}

void Command::setNumofArgs(int new_num) {
    args_num_ =new_num;
}

pid_t Command::getShellPid() {
    return SmallShell::getInstance().getShellPid();
}

void Command::makePipe() {
    is_pipe_=true;
}

bool Command::isPipe() {
    return is_pipe_;
}

void Command::setPid(pid_t process_pid) {
    this->cmd_pid_ = process_pid;
}

pid_t Command::getProccesPid() {
    return this->cmd_pid_;
}

void Command::setPrevDir(char *new_prev_dir) {
    SmallShell::getInstance().setPrevDir(new_prev_dir);
}

Command::Command(const char *cmd_line) {
    procces_starting_time=time(nullptr);
    this->cmd_pid_ =-1;
    this->is_background_ = _isBackgroundComamnd(cmd_line);
    for(int i=0; i<COMMAND_MAX_ARGS; i++)
        this->args_[i] = nullptr;
    this->args_num_ = _parseCommandLine(cmd_line, args_);
    if (this->is_background_)
    {
        for (int i =0; i<args_num_; i++) {
            if (_isBackgroundComamnd(this->args_[i])){
                char* s = this->args_[i];
                char c = '&';
                int writer = 0, reader = 0;
                while (s[reader])  {
                    if (s[reader]!=c)
                    {
                        s[writer++] = s[reader];
                    }
                    reader++;
                }
                s[writer]=0;
                this->args_[i] = s;
            }
        }
        }
}


//*************************Built-in commands******************************///
BuiltInCommand::BuiltInCommand(const char *cmd_line) : Command(cmd_line){}

ShowPidCommand::ShowPidCommand(const char *cmd_line) : BuiltInCommand(cmd_line){}

void ShowPidCommand::execute() {
    std::cout << "smash pid is " <<this->getShellPid()<<std::endl;
}

void FareCommand::execute(){
    if(this->getNumofArgs()!=4)
    {
        perror("smash error: fare: invalid arguments");
        return;
    }
    fstream file_to_read;
    file_to_read.open(args_[1],fstream::in | fstream::out);
    if(!file_to_read.is_open())
    {      
        perror("smash error: fare: invalid arguments");
        return;
    }
    this->counter=0;
    this->source=args_[2];
    this->destination=args_[3];
    string new_text="";
    string line;
    while(getline(file_to_read,line))
    {
        new_text.append(line);
        new_text.append("\n");
    }
    file_to_read.close();
    unsigned int pos_in_line=0;
    do{
        pos_in_line=new_text.find(this->source);
        if(pos_in_line<new_text.size()&& pos_in_line>=0)
        {
            new_text.replace(pos_in_line,source.length(),destination);
            this->counter++;
        }
    }while(pos_in_line<new_text.size());
   
    file_to_read.open(args_[1],fstream::in|fstream::out|fstream::trunc);
    file_to_read<<new_text;
    std::cout<<"replaced "<<this->counter<<" instances of the string \""<<this->source<<"\""<<endl;
}

FareCommand::FareCommand(const char* cmd_line):BuiltInCommand(cmd_line){}



ChangeDirCommand::ChangeDirCommand(const char *cmd_line, char **plastPwd) : BuiltInCommand(cmd_line) {
    last_directory = plastPwd;
}


void ChangeDirCommand::execute() {
    int args_num = this->getNumofArgs();
    if (args_[1] == nullptr)
        return;
    string temp_arg= args_[1];
    if (args_num <= 1)
        return;
    if (args_num > 2)
    {
        std::cerr<<"smash error: cd: too many arguments"<<std::endl;
        return;
    }
    char curr_dir[COMMAND_ARGS_MAX_LENGTH];
    getcwd(curr_dir, sizeof(curr_dir)); //getcwd function from unistd.h
    if (temp_arg == "-") {
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
        else if (chdir(this->args_[1]) != 0) {
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
    int args_num = this->getNumofArgs();
    if (args_num == 1)
        this-> job_id = -1;
    else if (args_num == 2)
    {
        try {
            this->job_id = stoi(this->args_[1]);
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
        SmallShell::getInstance().setCurrExternal(the_job->getCommand());
        SmallShell::getInstance().setForePid(command_pid);
        std::cout << the_job->getCommand()->getCommandName() << " : " << the_job->getProccesPid();
        bool stopped_status = the_job->isJobStopped();
        this->jobs_list->removeJobById(job_id);
        if (stopped_status)
        {
           if (!(this->isPipe()))
            {
                if (kill(command_pid,SIGCONT) == -1)
                {
                 perror("smash error: kill failed");
                 return;
                }
            }
            else
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
    if (args_[1] == nullptr)
        return;
    string temp_arg = args_[1];
    for (int i=0; i<this->getNumofArgs(); i++)
    {
        if (temp_arg == "kill"){
            kill_all = true;
            break;
        }
    }

    if (kill_all)
    {
        jobs_list->removeFinishedJobs();
        int jobs_num = this->jobs_list->getJobsList()->size(); 
        std::cout << "smash: sending SIGKILL signal to " <<jobs_num << " jobs:" <<std::endl;
        jobs_list->killAllJobs();
    }
    exit(0);
}




ChpromptCommand::ChpromptCommand(const char *cmd_line) :BuiltInCommand(cmd_line),prompt("smash"){}

void ChpromptCommand::execute()
{
  if( getNumofArgs()<=1) {
    prompt="smash";
  }else{
    prompt=this->args_[1];
  }
    SmallShell::getInstance().setMessage(prompt);
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
  if(args_[1]==nullptr || args_[1][0]!= '-' || args_[2]==nullptr || args_[3]==nullptr)
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

BackgroundCommand::BackgroundCommand(const char* cmd_line , JobsList* job_list):BuiltInCommand(cmd_line),job_list(job_list){}

void BackgroundCommand::execute()
{
  if (getNumofArgs() > 2)
  {
    std::cerr << "smash error: bg: invalid arguments";
    return;
  }
  int job_id=-1;
  JobsList::JobEntry*  job_to_resume=nullptr;
  if(getNumofArgs()==1)
  {
    job_to_resume=this->job_list->getLastStoppedJob(&job_id);
    //job_to_resume=this->job_list->getJobById(job_id);
    if((!job_to_resume) || (job_id==-1))
    {
      std::cerr << "smash error: bg: there is no stopped jobs to resume";
      return;
    }
  }else if(getNumofArgs()==2){
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
//JobsList::JobEntry::JobEntry(int id, Command* command,bool is_stopped=false):job_id(id),command(command),is_stopped(is_stopped){}
  
JobsList::~JobsList() {
    delete this->jobs_list;
}
void JobsList::JobEntry::stopJob()
{
  this->setJobStatus(true);
}

void JobsList::JobEntry::continueJob()
{
  this->setJobStatus(false);
}


void JobsList::addJob(Command* cmd, bool isStopped)
{
  removeFinishedJobs();
  int new_job_id=0;
  vector<JobEntry>* job_list_to_update=this->getJobsList();
  if(job_list_to_update->empty())
  {
    new_job_id=1;
    this->max_job_id=new_job_id;
  }else{
    int last_job_id=job_list_to_update->rbegin()->getJobId();
    new_job_id=last_job_id+1;
  }
  this->max_job_id=new_job_id;
  JobsList::JobEntry* new_job= new JobsList::JobEntry(this->max_job_id,cmd,isStopped);
  job_list_to_update->push_back(*new_job);
}

void JobsList::killAllJobs()
{
  vector<JobEntry>::iterator job_to_kill;
  vector<JobEntry>* job_list_to_kill=this->getJobsList();

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
  if(this->getJobsList()->empty())return;
  vector<JobEntry>::iterator prev=this->getJobsList()->begin();
  vector<JobEntry>::iterator curr=this->getJobsList()->begin();
  vector<JobEntry>::iterator end_of_list=this->getJobsList()->end();
  
  while(curr!=end_of_list)
  {
    pid_t pid_to_check=curr->getProccesPid();
    int check=waitpid(pid_to_check,nullptr,WNOHANG);
    if(check==-1)
    {
      perror("smash error: waitpid failed");
      return;
    }else if(check>0){
      prev=curr;
      curr++;
      this->getJobsList()->erase(prev);
      continue;
    }
    prev=curr;
    curr++;
  }
  
  this->getLastJob(&max_job_id);

}

void JobsList::removeJobById(int jobId)
{
  vector<JobEntry>::iterator curr=this->getJobsList()->begin();
  vector<JobEntry>::iterator end_of_list=this->getJobsList()->end();
  int max_id=0;
  for( ; curr!=end_of_list ; curr++)
  {
    if(curr->getJobId()==jobId)
    {
      if(curr->getJobId()==this->max_job_id)
      {
        this->max_job_id=max_id;
      }
      this->getJobsList()->erase(curr);
      delete &curr;
      break;
    }
    max_id=curr->getJobId();
  }
}



JobsList::JobEntry* JobsList::getJobById(int jobId)
{
  removeFinishedJobs();
  JobEntry* to_return=nullptr;
  vector<JobEntry>::iterator curr=this->getJobsList()->begin();
  while (curr!=this->getJobsList()->end())
  {
    int curr_id=curr->getJobId();
    if(curr_id==jobId)
    {
      to_return=&*(curr);
      return to_return;
    }
    curr++;
  }
  return to_return;
}

JobsList::JobEntry *JobsList::getLastStoppedJob(int *jobId)
{
  JobEntry* last_stopped_job=nullptr;
  vector<JobEntry>::iterator curr=this->getJobsList()->begin();
  vector<JobEntry>::iterator list_end=this->getJobsList()->end();
  while(curr!=list_end)
  {
    bool is_stopped=curr->isJobStopped();
    if(is_stopped)
    {
      *jobId=curr->getJobId();
      last_stopped_job=&*(curr);
    }
    curr++;
  }
  return last_stopped_job;
}

JobsList::JobEntry *JobsList::getLastJob(int *lastJobId)
{
  if(!(this->getJobsList()))
  {
    return nullptr;
  }
  
  *lastJobId=this->getJobsList()->end()->getJobId();//need to check type
  return &(this->getJobsList()->back());//need to check return value
}


void JobsList::printJobsList() 
{
  removeFinishedJobs();

      cout<<"checking again"<<endl;

  vector<JobEntry>* job_list_to_print=this->getJobsList();
  
  vector<JobEntry>::iterator current_job=job_list_to_print->begin();
  //iteration over all the jobs list
  for( ; current_job != job_list_to_print->end(); current_job++)
  {
      
    time_t time_diffrential=difftime(time(nullptr),current_job->getJobStartingTime());
    if(!(current_job->isJobStopped()))
    {
      cout<<"["<<current_job->getJobId()<<"] "<<current_job->getCommand()->getCommandName()<<" : "
            <<current_job->getJobId()<<" "<<time_diffrential<<" secs"<<endl;
    }else{
      cout<<"["<<current_job->getJobId()<<"] "<<current_job->getCommand()->getCommandName()<<" : "
            <<current_job->getJobId()<<" "<<time_diffrential<<" secs"<<" (stopped)"<<endl;
    }
  }
}


//*************************External Commands****************************************///

ExternalCommand::ExternalCommand(const char* cmd_line): Command(cmd_line) , cmd(cmd_line){}

void ExternalCommand::execute()
{
    
  bool is_bg=_isBackgroundComamnd(this->cmd);
  string cmd_s(this->cmd);
  if(is_bg)
  {
    cmd_s.pop_back();
  }

  const char* temp_cmd=cmd_s.data();
  strcpy(this->bash_command,temp_cmd);

    

  pid_t curr_pid=fork();
  if(curr_pid==-1)
  {
    perror("smash error: fork failed");
    return;
  }

  
  if(curr_pid==0) 
  {
    if(setpgrp()==-1)
    {
      perror("smash error: setpgrp failed");
      return;
    }

    char dir[]="/bin/bash";
    char c[]="-c";
    char* argv[4]={dir,c,bash_command,nullptr};

    //_removeBackgroundSign(argv[2]);
   //    cout<<argv[0]<<" "<<argv[1]<<" "<<argv[2]<<" "<<"salad"<<endl;

   //cout<<args_[0]<<"  "<<args_[1]<<"chill"<<endl;
    if(execv(argv[0],argv) ==-1)
    {
      perror("smash error: execv failed");
      return;
    }
  }else{ 
    if(is_bg)
    {
      this->setPid(curr_pid);
        SmallShell::getInstance().getJobsList()->addJob(this);
    }else{
        SmallShell::getInstance().setCurrExternal(this);
        SmallShell::getInstance().setForePid(curr_pid);
      
      if(waitpid(curr_pid,nullptr,WSTOPPED)==-1)
      {
        perror("smash error: waitpid failed");
      }
    }
  }

  //TODO: UPDATE CURRENT EXTERNAL IN THE SHELL

}

//*************************special Commands******************************///

RedirectionCommand::RedirectionCommand(const char *cmd_line) : Command(cmd_line) {
    int args_num = this-> getNumofArgs();
    string s1 = "";
    string s2 ="";
    bool second = false;
    for (int i = 0; i < args_num; i++)
    {
        string temp_arg1 = args_[i];
        if ((temp_arg1 != ">" ) && (temp_arg1 != ">>"))
        {
            if (!second) {
                s1 = s1.append(temp_arg1);
                s1 = s1.append(" ");
            }
            else
            {
                s2 = s2.append(temp_arg1);
                s2 = s2.append(" ");
            }
        }
        else {
            if (args_[i+1] == nullptr)
                break;
            string temp_arg2 = args_[i+1];
            if (temp_arg1 == ">") {
                if (temp_arg2 == ">")
                    type_ = kAppend;
                else
                    type_ = kOverride;
                second = true;
            }
            if (temp_arg1 == ">>") {
                type_ = kAppend;
                second = true;
            }
        }
    }
    only_command_ = s1.c_str();
    output_file = s2.c_str();
}


void RedirectionCommand::execute() {
    int fd_dst;
    if (type_ == kAppend)
        fd_dst = open(output_file, O_APPEND | O_CREAT | O_WRONLY, 0655);
    if (type_ == kOverride)
        fd_dst = open(output_file, O_TRUNC | O_CREAT | O_WRONLY, 0655);
    if (fd_dst == -1) {
        perror("smash error: open failed");
        return;
    }
    if (args_[0] == nullptr)
        return;
    string command_name =args_[0];
    bool is_simple = ((command_name == "chprompt") || (command_name == "showpid") || (command_name == "pwd") ||
            (command_name == "cd") || (command_name == "jobs") || (command_name == "fg") || (command_name == "bg") ||
            (command_name == "quit") || (command_name == "cat") || (command_name == "timeput") || (command_name == "kill"));
    if (is_simple) {
        int temp_fd = dup(1);
        if (temp_fd == -1){
            perror("smash error: dup failed");
            return;
    }
        if (dup2(fd_dst, STDOUT_FILENO) == -1)
        {
            perror("smash error: dup2 failed");
            return;
        }
        SmallShell::getInstance().executeCommand(args_[0]);
        if (dup2(temp_fd, STDOUT_FILENO) == -1)
        {
            perror("smash error: dup2 failed");
            return;
        }
    }
    else {
        pid_t command_pid = fork();
        if (command_pid == -1)
        {
            perror("smash error: fork failed");
            return;
        }
        if (command_pid == 0)
        {
            if (setpgrp() == -1)
            {
                perror("smash error: setpgrp failed");
                return;
            }
            if (close(STDOUT_FILENO) == -1)
            {
                perror("smash error: close failed");
                return;
            }
            if (dup2(fd_dst, STDOUT_FILENO) == -1)
            {
                perror("smash error: dup2 failed");
                return;
            }
            if (close(fd_dst) == -1)
            {
                perror("smash error: close failed");
                return;
            }
            SmallShell::getInstance().executeCommand(args_[0]);
            exit(0);
        }
        else
            if (waitpid(command_pid, nullptr, WSTOPPED) == -1) {
                perror("smash error: waitpid failed");
                return;
            }
    }
    if (close(fd_dst) == -1)
    {
        perror("smash error: close failed");
        return;
    }
}

void sigcont(int sig){
    pid_t leftpid=PipeCommand::pid_left;
    pid_t rightpid=PipeCommand::pid_right;
    if(rightpid > 0){
        if(kill(rightpid,SIGCONT)==-1){
            perror("smash error: kill failed");
            return;
        }
    }
    if(leftpid > 0 ){
        if(kill(leftpid,SIGCONT)==-1){
            perror("smash error: kill failed");
            return;
        }
    }

    if(waitpid(-1, nullptr,0)){
        perror("smash error: waitpid failed");
        return;
        }
}

PipeCommand::PipeCommand(const char *cmd_line) : Command(cmd_line){
    this->makePipe();
    int args_num = this-> getNumofArgs();
    string s1 = "";
    string s2 ="";
    bool second = false;
    for (int i = 0; i < args_num; i++)
    {
        if ((args_[i] == nullptr) || (args_[i+1] == nullptr))
            return;
        string temp_arg1 =args_[i];
        string temp_arg2 = args_[i+1];
        if ((temp_arg1 != "|" )&& (temp_arg1 != "&") && (temp_arg1 != "|&"))
        {
            if (!second) {
                s1 = s1.append(temp_arg1);
                s1 = s1.append(" ");
            }
            else
            {
                s2 = s2.append(temp_arg1);
                s2 = s2.append(" ");
            }
        }
        else {
            if (temp_arg1 == "|") {
                if (temp_arg2 == "&")
                    type_ = kErr;
                else
                    type_ = kOut;
                second = true;
            }
            if (temp_arg1 == "|&") {
                type_ = kErr;
                second = true;
            }
        }
    }
    left_command_ = s1.c_str();
    right_command_ =s2.c_str();
}


void PipeCommand::execute() {
    JobsList *the_job_list =  SmallShell::getInstance().getJobsList();
    the_job_list->removeFinishedJobs();
    Command *leftcommand;
    Command *rightcommand;

    pid_t main_pid = fork();
    if (main_pid == -1) {
        perror("smash error: fork failed");
        return;
    } else if (main_pid != 0) {
        this->setPid(main_pid);
        SmallShell::getInstance().setForePid(this->getProccesPid());
        SmallShell::getInstance().setCurrExternal(this);
        waitpid(this->getProccesPid(), nullptr, WUNTRACED);
        JobsList *the_job_list1 =  SmallShell::getInstance().getJobsList();
        the_job_list1->removeFinishedJobs();
    } else {
        if (setpgrp() == -1) {
            perror("smash error: setpgrp failed");
            return;

        }
        int pipefd[2];
        if (pipe(pipefd) == -1) {
            perror("smash error: pipe failed");
            return;
        }
        pid_t temp_pid = fork();
        if (temp_pid == -1) {
            perror("smash error: fork failed");
            return;
        }
        if (temp_pid == 0) {
            if (setpgrp() == -1) {
                perror("smash error: setpgrp failed");
                return;
            }
            if (close(type_) == -1) {
                perror("smash error: close failed");
                return;

            }
            if (dup2(pipefd[1], type_) == -1) {
                perror("smash error: dup2 failed");
                return;

            }
            if (close(pipefd[0]) == -1) {
                perror("smash error: close failed");
                return;

            }
            if (close(pipefd[1]) == -1) {
                perror("smash error: close failed");
                return;

            }
            const char *left_string_char = left_command_;
            leftcommand =  SmallShell::getInstance().CreateCommand(left_string_char);
            if (leftcommand) {
                leftcommand->execute();
            }
            pid_t left_command_pid = leftcommand->getProccesPid();
            while (waitpid(left_command_pid, nullptr, WUNTRACED) > -1) {}

            if (close(type_) == -1) {
                perror("smash error: close failed");
                return;
            }
            exit(0);
        }
        pid_t temp_pid1 = fork();
        if (temp_pid1 == -1) {
            perror("smash error: fork failed");
            return;
        } else if (temp_pid1 == 0) {
            if (setpgrp() == -1) {
                perror("smash error: setpgrp failed");
                return;
            }
            if (close(STDIN_FILENO) == -1) {
                perror("smash error: close failed");
                return;

            }
            if (dup2(pipefd[0], STDIN_FILENO) == -1) {
                perror("smash error: dup2 failed");
                return;
            }
            if (close(pipefd[1])) {
                perror("smash error: close failed");
                return;
            }
            if (close(pipefd[0]) == -1) {
                perror("smash error: close failed");
                return;
            }

            const char *right_string_char = right_command_;

            rightcommand = SmallShell::getInstance().CreateCommand(right_string_char);
            if (rightcommand) {
                rightcommand->execute();
            }
            pid_t rightcommandpid = rightcommand->getProccesPid();
            while (waitpid(rightcommandpid, nullptr, WUNTRACED) > -1) {}

            if (close(STDIN_FILENO) == -1) {
                perror("smash error: close failed");
                return;
            }
            exit(0);
        }
        PipeCommand::pid_left = temp_pid1;
        PipeCommand::pid_right = temp_pid;
        if (close(pipefd[0]) == -1) {
            perror("smash error: close failed");
            return;
        }
        if (close(pipefd[1]) == -1) {
            perror("smash error: close failed");
            return;
        }
        if (signal(SIGCONT, sigcont) == SIG_ERR) {
            perror("smash error: close failed");
            return;
        }
        for (int i = 0; i < 4; i++) {
            int tempstatus;
            wait(&tempstatus);
        }
        exit(0);
    }
}




// TimeoutCommand::TimeoutCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {
//     if (this->args_[1])
//     {
//         try{
//             duration_ = stoi(args_[1]);
//         }
//         catch  (std::invalid_argument &e) {
//             duration_=-1;
//         }
//     }
//     else
//         duration_ = -1;
//     if (args_[2]) {
//         string s = args_[2];
//         for (int i = 3; i < this->getNumofArgs(); i++) {
//             s.append(" ");
//             s.append(args_[i]);
//         }
//         only_command_ = s.c_str();
//     }
//     else
//         only_command_ = nullptr;
// }

// void TimeoutCommand::execute() {
//     if ((duration_ <= 0) || (!only_command_) || (this->getNumofArgs() <3)) {
//         std::cerr << "smash error: timeout: invalid arguments";
//         return;
//     }

//     //TODO: FINISH THE FUNCTION

// }




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

Command* SmallShell::getCurrExternal() {
    return this->curr_external;
}

void SmallShell::setCurrExternal(Command *new_external) {
    this->curr_external = new_external;
}


SmallShell::SmallShell(): shell_pid_(getpid())  //getpid function from unistd.h
{
    this->fore_pid_=-1;
    this->jobs_list_ = new JobsList();
    this->shell_prev_dir_ = nullptr;
    this->curr_external= nullptr;
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
    _parseCommandLine(cmd_line, args);
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
            return new PipeCommand(cmd_line);
        case kRedirection:
            return new RedirectionCommand(cmd_line);
    }
  return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
  Command* cmd = CreateCommand(cmd_line);
  if (cmd)
  {
    cmd->execute();
  }
}


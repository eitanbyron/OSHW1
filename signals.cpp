#include <iostream>
//#include <signal.h>
#include "signals.h"
#include <signal.h>

#include "Commands.h"
using namespace std;

void ctrlZHandler(int sig_num) {
  cout<<"smash: got ctrl-Z"<<endl;
  SmallShell& inst_shell=SmallShell::getInstance();
  pid_t pid_to_stop=inst_shell.getForePid();
  if(pid_to_stop>0)
  {
    inst_shell.getJobsList()->removeFinishedJobs();
    inst_shell.getJobsList()->addJob(inst_shell.getCurrExternal(),true);
    if(kill(pid_to_stop,SIGSTOP)!=0)
    {
      perror("smash error: kill failed");
      return;
    }
    inst_shell.setForePid(-1);
    inst_shell.setCurrExternal(nullptr);
    cout<<"smash: process "<<pid_to_stop<<" was stopped"<<endl;
  }

}

void ctrlCHandler(int sig_num) {
  cout<<"smash: got ctrl-C"<<endl;
  SmallShell& inst_shell=SmallShell::getInstance();
  pid_t pid_to_kill=inst_shell.getForePid();
  if(pid_to_kill>0)
  {
    if(kill(pid_to_kill,SIGKILL)!=0)
    {
      perror ("smash error: kill failed");
      return;
    }
    inst_shell.setForePid(-1);
    inst_shell.setCurrExternal(nullptr);
    cout<<"smash: process "<<pid_to_kill<<" was killed"<<endl;

  }

}

void alarmHandler(int sig_num) {
  cout<<"smash: got an alarm"<<endl;
 // SmallShell& inst = SmallShell::getInstance();


}

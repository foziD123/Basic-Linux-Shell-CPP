#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;


void ctrlZHandler(int sig_num) {
    cout<<"smash: got ctrl-Z"<<endl;
    SmallShell &s_shell = SmallShell::getInstance();
    s_shell.job_list->removeFinishedJobs();
    int job_pid = s_shell.curr_job_pid;
    if(job_pid==-1)
    {
        return;
    }
    int kill_failing=kill(job_pid,SIGSTOP);
    if(kill_failing==-1)
    {
        perror("smash error: kill failed");
    }
     cout<<"smash: process "<<job_pid<<" was stopped"<<endl;
/*
     if(job_pid==s_shell.job_list->max_job_id)
     {
         s_shell.job_list->max_job_id--;
     }
*/
    ExternalCommand* cmd_obj= new ExternalCommand(s_shell.curr_cmd.c_str());
    if(s_shell.job_list->getJobByPid(job_pid)== nullptr)
    {    //cout <<"updatebefore 2 max_id is:"<<s_shell.job_list->max_job_id<<endl;//all good

       // cout<<"fffffffffff"<<s_shell.job_list->job_vector.size()<<endl;

        s_shell.job_list->updateMaxStopped();
        s_shell.job_list->addJob(cmd_obj,job_pid,true,false);
        s_shell.job_list->updateMaxStopped();
       // cout <<"updateafter 1 max_id is:"<<s_shell.job_list->max_job_id<<endl;

    }
    else{


        s_shell.job_list->getJobByPid(job_pid)->is_stopped=true;
        s_shell.job_list->getJobByPid(job_pid)->is_background= false;
    }
    delete cmd_obj;


    s_shell.curr_job_pid = -1;
    if(job_pid==s_shell.job_list->max_job_id)
    {
        s_shell.job_list->max_job_id--;
    }

}

void ctrlCHandler(int sig_num) {
    cout<<"smash: got ctrl-C\n";
    SmallShell &s_shell = SmallShell::getInstance();

    int job_pid = s_shell.curr_job_pid;
    if(job_pid==-1)
    {
    } else{
    int kill_failing=kill(job_pid,SIGKILL);
    if(kill_failing==-1)
    {
        perror("smash error: killpg failed");
    }
    cout<<"smash: process " <<job_pid<< " was killed\n";
    s_shell.curr_job_pid=-1;}
}

void alarmHandler(int sig_num) {
  // TODO: Add your implementation
}


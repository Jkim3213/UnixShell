#include <cstdlib>
#include <string>
#include <cstdio>
#include <unistd.h>
#include <vector>
#include <sys/types.h>
#include <pwd.h>
#include <sys/wait.h>
#include <algorithm>
#include <cstring>
#include <signal.h>
#include <array>
#include <getopt.h>
#include <fcntl.h>
#include <sys/stat.h>

using std::string;
using std::vector;
using std::array;

pid_t pid, ppid;//current and parentkind of
int laststatus = 0;


//vectors
vector< vector<string> >jobslist;
vector<vector<int> > pidsofbacks;//holcs pids of background process. first int is the process leader 

int parse(string input);//cuts up the input and puts them in to a vector
int eval(vector<string> clean);//takes the cleaned vector and evaluates the args
string getcurrentdir();//gets the current working directory, also if home replace with tilde
int runprog(vector<string> io, vector< vector<string> > pargs);//runs the program;
char *convert(const std::string & s);//converts string into char*
void close_pipe(int pipefd [2]);//closes pipes
int jobhandler(vector<string> currentjob, int status); //handles our jobs
int jobupdate(string cpid, string stat);//updates jobs status for builtins
int strtosig(string csig); //converts a string signal to an int signal
void checkio(vector<string> &io, bool &out, bool &err, bool &stin, bool &stout, bool &sterr);//checks io vector for trunc and append and sets appropriate booleans
int updatestatus(int status, int pid, vector<string> temppros, bool bg);//updates the status when programs change status
int getbgpid(int bpid);//returns the catologued gpid

//BUILT-INS
int isbuiltin(string prog);//returns bool if program is a builtin
int runbuilt(int ind, vector<string> prog);//tries to run the builtin

int exit(vector<string> args);//probably want to wait until all children are dead
int cd(vector<string> args);//takes args for chdir
int help(vector<string> args);//maybe can take argument for help
int bg(vector<string> args);
int xport(vector<string> args);
int fg(vector<string> args);
int jobs(vector< vector<string> > jobslist);
int kill(vector<string> args);
//
void my_handler(int signo) {
} // my_handler
  

int main(int argc, char* argv[]){
  //  pid = getpid();

  signal(SIGINT,  my_handler);//ignore C-c
  signal(SIGQUIT, my_handler);//ignore C-backslash
  signal(SIGTSTP, my_handler);//ignore C-z
  signal(SIGTTIN, my_handler);//ignore
  signal(SIGTTOU, my_handler);//ignore
  signal(SIGCHLD, my_handler);//ignore

  
  setvbuf(stdout, nullptr, _IONBF, 0);
  setvbuf(stderr, nullptr, _IONBF, 0);

  //open messafe
  printf("\n"
	 "  _/          _/            _/\n"
	 " _/          _/    _/_/    _/    _/_/_/    _/_/    _/_/_/  _/_/      _/_/\n"
	 "_/    _/    _/  _/_/_/_/  _/  _/        _/    _/  _/    _/    _/  _/_/_/_/\n"
	 " _/  _/  _/    _/        _/  _/        _/    _/  _/    _/    _/  _/\n"
	 "  _/  _/        _/_/_/  _/    _/_/_/    _/_/    _/    _/    _/    _/_/_/\n"
	 "\n"
	 "\n"
	 "   _/                      _/  _/_/_/_/_/  _/_/_/      _/              _/        _/\n"
	 "_/_/_/_/    _/_/        _/_/          _/        _/  _/  _/    _/_/_/  _/_/_/    _/\n"
	 " _/      _/    _/        _/        _/      _/_/    _/  _/  _/_/      _/    _/  _/\n"
	 "_/      _/    _/        _/      _/            _/  _/  _/      _/_/  _/    _/\n"
	 " _/_/    _/_/          _/    _/        _/_/_/      _/    _/_/_/    _/    _/  _/\n");
	 
  printf("\nType exit to quit\n\n");

  bool exit = 0;//maybe when logout, not really for this program
  
  while(!exit){// will have condition while array is not logout
    string input;
    const int BUFF_SIZE = 1024; // size of data buffer
    char buffer [BUFF_SIZE];    // data buffer
    int n = 0;                  // number of bytes read


    printf("1730sh:%s/$ ", getcurrentdir().c_str());
    //preoblem may lay in buffer
    if((n = read(STDIN_FILENO, buffer, BUFF_SIZE)) > 0){
      //      input = (string)buffer;//the real problem lies here
      input.assign(buffer, n - 1);//n - 1 bc \n key
    }
    
    if(parse(input) == -1){
      fprintf(stderr, "%s\n", "-1730sh: Oops, an error has occurred.");
    }
  } // while
   


  return EXIT_SUCCESS;
}


int parse(string input){

  input += " ";
  vector<string> clean;
  //looks for words surrounded by spaces and pushes them into clean vector
  for(unsigned int i = 0; i < input.length(); i++){
    if(input.at(i) != ' '){
      unsigned int j = i + 1;
      unsigned int length = 1;
      if(input.at(i) == '"'){//looking at double quotes
	while(j < input.length()){
	  if(input.at(j) == '"' && input.at(j-1) != '\\'){
	    --length;
	    break;
	  }
	  else if(input.at(j) == '"' && input.at(j-1) == '\\'){
	    input.erase(j - 1, 1);
	  }
	  else{
	    ++j;
	    ++length;
	  }
	}//while
	++i;
      }//if
      else{
	while(input.at(j) != ' ' && j < input.length()){
	  ++j;
	  ++length;
	}
      }//else
      clean.push_back(input.substr(i,length));
      i = j;
    }//if

  }//for

  return eval(clean);
}//parse


int eval(vector<string> clean){
  /*  
  for(string arg : copy)
    printf("%s\n", arg.c_str());
  */

  if(clean.size() == 0){
    return 0;
  }

  string stdin = "STDIN_FILENO";
  string stdout = "STDOUT_FILENO";
  string stderr = "STDERR_FILENO";

  for(unsigned int i = 0; i < clean.size() - 1; i++){
    //printf("%s\n", clean.at(i).c_str());
    if(clean.at(i).compare("<") == 0){
      stdin = clean.at(i + 1);
    }
    if(clean.at(i).compare(">") ==0){
      stdout = clean.at(i + 1) + " (truncate)";
    }
    if(clean.at(i).compare(">>") == 0){
      stdout = clean.at(i + 1) + " (append)";
    }
    if(clean.at(i).compare("e>") == 0){
      stderr = clean.at(i + 1) + " (truncate)";
    }
    if(clean.at(i).compare("e>>") == 0){
      stderr = clean.at(i + 1) + " (append)";
    }
  }//for jobs

  vector<string> io = {stdin, stdout, stderr};

  vector< vector<string> > pargs;
  unsigned int pindex = 0;//index of processes
  unsigned int k = 0;
  while(k < clean.size()){
    int eindex = 0;//index of elements
    vector<string> args;

    while(k < clean.size() && clean.at(k) != "|"){
      if(clean.at(k).compare("<") == 0 || clean.at(k).compare(">") == 0 || clean.at(k).compare(">>") == 0 || clean.at(k).compare("e>") == 0 || clean.at(k).compare("e>>") == 0){
	k += 2;
      }
      else{
	args.push_back(clean.at(k));
	++eindex;
	++k;
      }//else
    }//inside while
    ++k;
    ++pindex;
    pargs.push_back(args);
  }//loop through to get pipes and args now to print

 
 
  return runprog(io, pargs);
}


string getcurrentdir(){
  char cwd[1024];
  string cfixed;
  string home;

  if (getcwd(cwd, sizeof(cwd)) != NULL){
    cfixed = (string)cwd;
  }
  else{
    perror("getcwd() error");
    return "?";
  }

  struct passwd *pw = getpwuid(getuid());
  home = (string)pw->pw_dir;

  if(home.compare(cfixed.substr(0, home.length())) == 0){
    cfixed.replace(0, home.length(), "~");
  }
  
  return cfixed;
}


int runprog(vector<string> io, vector< vector<string> > pargs){

  bool outapp, errapp;//if true append, if not truncate
  bool stin = 0, stout = 0, sterr = 0;
  bool bg;//true if program should run in background
  checkio(io, outapp, errapp, stin, stout, sterr);
  vector<string> last = pargs.at(pargs.size() - 1);
  if(last.at(last.size() - 1).compare("&") == 0){
    bg = 1;
    last.pop_back();
    pargs.at(pargs.size() - 1) = last;
  }
  // printf("o%d e%d b%d\n", outapp, errapp, bg);

  vector<string> sargs(pargs.at(0));//the first process// will be inside loop
  vector<string> temppros; // the temporary process for each job used for job table
  
  int leadpid;

  int is;//dictates which built in to run
  if((is = isbuiltin(sargs.at(0))) >= 0){//the first command
    if(runbuilt(is, sargs) == -1){
      fprintf(stderr, "Error: Builtin: %s: %s\n", sargs.at(0).c_str(), strerror(errno));
    }
  }//if builtin do not fork
  else{
    vector<array<int, 2> > pipes;
    vector<int> pidsof;//to be pushed into pids of backs
    for(unsigned int i = 0; i < pargs.size(); i++){//fork for piped processes
      if (i != pargs.size() - 1) {
	int pipefd [2];
	if (pipe(pipefd) == -1) {
	  fprintf(stderr, "pipe: %s\n", strerror(errno));
	  return(0);
	} // if
	pipes.push_back({pipefd[0], pipefd[1]});
      } // if

      if((pid = fork()) < 0){
	fprintf(stderr, "FORK ERROR\n");
      }
      else if(pid == 0){//first process
	
	if(bg){
	  if(i == 0){
	    leadpid = getpid();
	  }
	  if(setpgid(pid, leadpid) == -1)
	    fprintf(stderr, "setpgid: %s\n", strerror(errno));
	}
	vector<char*> cargs;
	std::transform(pargs.at(i).begin(), pargs.at(i).end(), std::back_inserter(cargs), convert);
	cargs.push_back((char*)0);

	if (i != 0) {//not at first
	  if (dup2(pipes.at(i - 1)[0], STDIN_FILENO) == -1) {
	    fprintf(stderr, "dup2: %s\n", strerror(errno));
	  } // if
	} // if

	else{//at first process

	  int fd = open(io.at(0).c_str(), O_RDONLY, 0);
	  if(stin){
	    fd = 0;
	  }
	  int pipin[2];
	  if(pipe(pipin) == -1){
	    fprintf(stderr, "pipe: %s\n", strerror(errno));
	  }
	  if (dup2(fd, STDIN_FILENO) == -1){//read from file
            fprintf(stderr, "dup2: %s\n", strerror(errno));
          } // if
	  close_pipe(pipin);
	}//else

	if (i != pargs.size() - 1) {//not at end
	  if (dup2(pipes.at(i)[1], STDOUT_FILENO) == -1) {
	    fprintf(stderr, "dup2: %s\n", strerror(errno));
	    return 0;
	  } // if
	} // if
	else{//at last process
	  int fd;
	  if(stout){
            fd = 1;
          }
          else{
	    if(outapp)
	      fd = open(io.at(1).c_str(),O_CREAT | O_WRONLY | O_APPEND,0755);
	    else
	      fd = open(io.at(1).c_str(),O_CREAT | O_WRONLY | O_TRUNC,0755);
	  }
	  	  
          int pipout[2];
          if(pipe(pipout) == -1){
            fprintf(stderr, "pipe: %s\n", strerror(errno));
          }
          if (dup2(fd, STDOUT_FILENO) == -1){//read from file
            fprintf(stderr, "dup2: %s\n", strerror(errno));
          } // if
	  
          close_pipe(pipout);

	  int efd;
          if(sterr){
            efd = 2;
          }
          else{
            if(errapp)
              efd = open(io.at(2).c_str(),O_CREAT | O_WRONLY | O_APPEND,0755);
            else
              efd = open(io.at(2).c_str(),O_CREAT | O_WRONLY | O_TRUNC,0755);
          }

          int piperr[2];
          if(pipe(piperr) == -1){
            fprintf(stderr, "pipe: %s\n", strerror(errno));
          }
          if(dup2(efd, STDERR_FILENO) == -1){//read from file
            fprintf(stderr, "dup2: %s\n", strerror(errno));
          } // if

          close_pipe(piperr);

        }//else


	// close all pipes created so far
	for (unsigned int i = 0; i < pipes.size(); ++i) {
	  close_pipe(pipes.at(i).data());
	} // for

	if(execvp(cargs[0], &cargs[0]) == -1){
	  string ermsg = "-1730sh: " + sargs[0];
	  fprintf(stderr, "%s: %s\n", ermsg.c_str(), strerror(errno));
	}

	for (unsigned int i = 0; i < cargs.size(); i++ )//UPDATE: No memory leaks occur when testing so unsure if necessary to change. unfortunately skipped atm by execvp, i think
	  delete [] cargs[i];

	exit(22);
      }//process terminates loop dies in this fork
      else{
	pidsof.push_back(pid);
	if(bg){
	  if(i == 0){
	    leadpid = getpid();
	  }
	  if(setpgid(pid, leadpid) == -1)
            fprintf(stderr, "setpgid: %s\n", strerror(errno));
	}
      }

    }//for
    pidsofbacks.push_back(pidsof);
        
    // close all pipes
    for (unsigned int i = 0; i < pipes.size(); ++i) {
      close_pipe(pipes.at(i).data());
    } // for baby

    if(pid > 0){//parent
      //make group of all process last process pid, only if background

      //setting up running process
      if(!bg)
	temppros.push_back(std::to_string(pid));
      else
	temppros.push_back(std::to_string(getbgpid(pid)));
      temppros.push_back("Running");
      string arguments;
      for(unsigned int j = 0; j < pargs.size(); j++){//add all process to job
	vector<string> a = pargs.at(j);	
	for(string b : a){
	  arguments = arguments + b + " ";
	}
	if(j < pargs.size() -1)
	  arguments += "| ";
      }	    
      temppros.push_back(arguments);
      if(bg){
	temppros.at(2) += " &";
      }

      jobhandler(temppros, 0);
      //if statement like, If this last process(which is the leader)'s group id is the same as parents group id, that means it is foreground. else WNOHANG, -1 so can kill zombies. Set the last process as a group leader

      int status = 0;
      int bpid = 0;//pid of potential zombie backgrounders
      if(getpgid(pid) == getpgrp() && !bg){//foreground 
	if(waitpid(pid, &status, WUNTRACED) == -1)//waits for freshest child to come home
	  fprintf(stderr, "waitpid(): %s", strerror(errno));
	updatestatus(status, pid, temppros, 0);
      }

    

      int bstatus;
      int bgpid;
      bpid = waitpid(-1, &bstatus, WUNTRACED | WNOHANG);//wait for background process if they exist
      bgpid = getbgpid(bpid);

      if(bgpid == bpid){//if it is the leader 
	for(unsigned int p = 0; p < jobslist.size(); p++){
	  if(jobslist.at(p).at(0).compare(std::to_string(bpid)) == 0){
	    temppros = jobslist.at(p);
	    break;
	  }
	}
	updatestatus(bstatus, bpid, temppros, 1);
      }
      //waitpid(0, nullptr, WNOHANG);//clear defunct          

    }//parent if

  }//else if forked

  
  return 0;
}//runprog

int getbgpid(int bpid){
  for(unsigned int i = 0; i < pidsofbacks.size(); i++){
    vector<int> a = pidsofbacks.at(i);
    for(unsigned int j = 0; j < a.size(); j++){
      if(bpid == a.at(j))
	return a.at(0);
    }
  }
  return 0;
}//getbgpid

int updatestatus(int status, int tpid, vector<string> temppros, bool bg){
  string statstr;

  if(WIFEXITED(status)){
    laststatus = (int)WEXITSTATUS(status);
    if(!bg)
      printf("Process %d exited with status = %d\n", tpid, (int)WEXITSTATUS(status));
    statstr = "Exited (" + std::to_string(laststatus) + ")";
  }
  else if(WIFSIGNALED(status)){
    if(!bg)
      printf("Process %d terminated with signal = %d\n", tpid, (int)WTERMSIG(status));
    if(WCOREDUMP(status) && !bg)
      printf("(core dumped)\n");
    statstr = "Exited (" + (string)strsignal((int)WTERMSIG(status)) + ")";
  }
  if(WIFSTOPPED(status)){
    if(!bg)
      printf("Process %d stopped with signal = %d\n", tpid, (int)WSTOPSIG(status));
    statstr = "Stopped";
  }
  if(WIFCONTINUED(status)){
    if(!bg)
      printf("Process %d has been continued\n", tpid);
    statstr = "Continued";
  }


  temppros.erase(temppros.begin() + 1);//erase current stauts
  temppros.insert(temppros.begin() + 1, statstr);//add new status
  jobhandler(temppros, status);//will add to the jobslist
  
  return 0;
}//update status

void checkio(vector<string> &io, bool &outapp, bool &errapp, bool &stin, bool &stout, bool &sterr){
  int alength = 9;
  int tlength = 11;//" (truncate)"
  if(io.at(0).compare("STDIN_FILENO") == 0){
    stin = 1;
  }
  if(io.at(1).compare("STDOUT_FILENO") == 0){
    stout = 1;
  }
  else if(io.at(1).substr(io.at(1).length() - alength, alength).compare(" (append)") == 0){//out
    outapp = 1;
    io.at(1) = io.at(1).substr(0, io.at(1).length() - alength);
  }
  else{
    outapp = 0;
    io.at(1) = io.at(1).substr(0, io.at(1).length() - tlength);
  }

  if(io.at(2).compare("STDERR_FILENO") == 0){
    sterr = 1;
  }
  else if(io.at(2).substr(io.at(2).length() - alength, alength).compare(" (append)") == 0){//err
    errapp = 1;
    io.at(2) = io.at(2).substr(0, io.at(2).length() - alength);
  }
  else{
    errapp = 0;
    io.at(2) = io.at(2).substr(0, io.at(2).length() - tlength);
  }
}//check io

/*
  jobHandler
  @param vector<string> currentjob
  @return void
  --> Handles our jobs.
 */
int jobhandler(vector<string> currentjob, int status){
  int countjob = 0;

  //check if the program is already present
  if(!((int)WEXITSTATUS(status) == 22)){
    for(vector<string> checker : jobslist){
      if(currentjob.at(0).compare(checker.at(0)) == 0){
	//the job already exists;
	//replace her
	jobslist.erase(jobslist.begin() + countjob);
	jobslist.insert(jobslist.begin() + countjob, currentjob);
	return 0;
      } 
      countjob++;
    }
    jobslist.push_back(currentjob);//if new job
    return 0;
  }//if
  else{
    jobslist.pop_back();
  }
  return -1;//if exit 22
}//jobhandler



void close_pipe(int pipefd [2]) {
  if (close(pipefd[0]) == -1) {
    perror("close");
    exit(EXIT_FAILURE);
  } // if
  if (close(pipefd[1]) == -1) {
    perror("close");
    exit(EXIT_FAILURE);
  } // if
} // close_pipe

char *convert(const std::string & s){
  char *pc = new char[s.size()+1];
  std::strcpy(pc, s.c_str());
  return pc; 
}//http://stackoverflow.com/questions/7048888/stdvectorstdstring-to-char-array



int isbuiltin(string prog){
  string listbuilts[8] = {"exit", "cd", "help", "bg",
			  "export", "fg", "jobs", "kill"};

  for(int i = 0; i < 8; i++){
    if(prog.compare(listbuilts[i]) == 0){
      return i;
    }
  }
  return -1;
}

int runbuilt(int ind, vector<string> prog){

  switch (ind){
  case 0://exit
    return exit(prog);
  case 1://cd
    return cd(prog);
  case 2://help
    return help(prog);
  case 3://bg
    return bg(prog);
  case 4://xport
    return xport(prog);
  case 5://fg
    return fg(prog);
  case 6://jobs
    return jobs(jobslist);
  case 7:
    return kill(prog);
  default:
    return -1;
  }
  
}//runbuilt

int exit(vector<string> args){


  //should be also if no argument, then supply with last process value
  if(args.size() > 1)
    exit(stoi(args.at(1)));
  else if(args.size() == 1){
    exit(laststatus);
  }
  else
    exit(0);

  return -1;
}
int cd(vector<string> args){

  string path;
  struct passwd *pw = getpwuid(getuid());//to get home

  if(args.size() == 1 || args.at(1).compare("~") == 0){
    path = (string)pw->pw_dir;
  }
  else
    path = args.at(1);

  if(chdir(path.c_str()) == -1){
    perror("cd");
    return -1;
  }

  return 0;
}
int help(vector<string> args){

  if(args.size() == 1){
    printf("$ exit [N] - Cause the shell to exit with a status of N. If N is omitted,\n"
	   " the exit status is that of the last job executed\n\n");
    printf("$ cd [PATH] - Change the current directory to PATH.\n"
	   " The environmental variable HOME is the default PATH.\n\n");
    printf("$ help - Display helpful information about builtin commands.\n\n");
    printf("$ bg JID - Resume the stopped job JID in the background, as if it had been started with &.\n\n");
    printf("$ export NAME[=WORD] - NAME is automatically included in the environment\n"
	   " of subsequently executed jobs.\n\n");
    printf("$ fg JID - Resume job JID in the foreground, and make it the current job.\n\n");
    printf("$ jobs - List current jobs.\n\n");
    printf("$ kill [-s SIGNAL] PID - The kill utility sends the specied signal\n"
	   " to the specied process or process group PID (seekill(2))\n\n");

    return 0;
  }

  for(unsigned int i = 1; i < args.size(); i++){
    int c; 
    if((c = isbuiltin(args.at(i))) == -1){
      fprintf(stderr, "%s is not a built-in command.\n"
	      "Try $help to see all commands.\n\n", args.at(i).c_str());
    }
    switch (c){
    case 0://exit
      printf("$ exit [N] - Cause the shell to exit with a status of N. If N is omitted,\n"
	     " the exit status is that of the last job executed\n\n");
      break;
    case 1://cd
      printf("$ cd [PATH] - Change the current directory to PATH.\n"
	     " The environmental variable HOME is the default PATH.\n\n");
      break;
    case 2://help
      printf("$ help - Display helpful information about builtin commands.\n\n");
      break;
    case 3://bg
      printf("$ bg JID - Resume the stopped job JID in the background, as if it had been started with &.\n\n");
      break;
    case 4://xport
      printf("$ export NAME[=WORD] - NAME is automatically included in the environment\n"
	     " of subsequently executed jobs.\n\n");
      break;
    case 5://fg
      printf("$ fg JID - Resume job JID in the foreground, and make it the current job.\n\n");
      break;
    case 6://jobs
      printf("$ jobs - List current jobs.\n\n");
      break;
    case 7://kill
      printf("$ kill [-s SIGNAL] PID - The kill utility sends the specied signal\n"
	     " to the specied process or process group PID (seekill(2))\n\n");
      break;
    }//switch


  }//for

  return 0;
}//help
  

int bg(vector<string> args){//background
  //wrong
  if(kill(stoi(args[1]), SIGCONT) == -1){
    return -1;
  }
  jobupdate(args[1], "Running");
  
  return 0;
  
}

int xport(vector<string> args){


  return -1;
}

int fg(vector<string> args){


  return -1;
}
/*
  jobs
  @param vector <string> args
  @return int
  --> prints our jobs
 */
int jobs(vector< vector<string> > jobslist){
  //format print
  for(unsigned int i = 0; i < jobslist.size(); i++){
    printf("%-4s %-16s %s\n", jobslist.at(i).at(0).c_str(), jobslist.at(i).at(1).c_str(), jobslist.at(i).at(2).c_str());
  }
  return 0;
}

int kill(vector<string> pargs){

  vector<char*> args;
  std::transform(pargs.begin(), pargs.end(), std::back_inserter(args), convert);
  args.push_back((char*)0);

  int c, flag, signal, loc;
  string csignal;

  while((c = getopt(args.size(), &args[0], "s:")) != -1){//our problem lies here. Will iterate size -1 times and breaks
    switch(c){
    case 's':
      flag = 1;
      loc = 2;
      csignal = (string)optarg;
      break;
    case 'c':
      printf("helo\n");
      break;
    }
  }
 

  if((signal = strtosig(csignal)) == -1){
    for (unsigned int i = 0; i < args.size(); i++ )
      delete [] args[i];
    return -1;
  }
  if(!flag){
    signal = 15; //SIGTERM signal
    loc = 1;
  }
  if(kill(stoi(pargs[loc]), signal) == -1){
    for (unsigned int i = 0; i < args.size(); i++ )
      delete [] args[i];
    return -1;
  }

  for (unsigned int i = 0; i < args.size(); i++ )
    delete [] args[i];

  return 0;
}

int strtosig(string csignal){
  int idx;
  int flag;
  string signalbuilt[19] = {"SIGHUP", "SIGINT", "SIGQUIT" , "SIGILL",
			    "SIGABRT", "SIGFPE", "SIGKILL", "SIGSEGV",
			    "SIGPIPE" , "SIGALRM", "SIGTERM", "SIGUSR1",
			    "SIGUSR2", "SIGCHLD", "SIGCONT", "SIGSTOP",
			    "SIGTSTP" , "SIGTTIN", "SIGTTOU"};
  for(unsigned int i = 0; i<19; i++){
    if(csignal.compare(signalbuilt[i]) == 0){
      idx = i;
      flag = 1;
      break;
    }
  }
  if(!flag){
    return -1;
  }
  switch(idx){
  case 1:
    return 1;
  case 2:
    return 2;
  case 3:
    return 3;
  case 4:
    return 4;
  case 5:
    return 6;
  case 6: 
    return 8;
  case 7: 
    return 9;
  case 8: 
    return 11;
  case 9:
    return 13;
  case 10: 
    return 14;
  case 11:
    return 15;
  case 12:
    return 10;
  case 13:
    return 12;
  case 14:
    return 17;
  case 15:
    return 18;
  case 16:
    return 19;
  case 17:
    return 20;
  case 18:
    return 21;
  case 19:
    return 22;  
  }
  return -1;
}

int jobupdate(string cpid, string status){
  for(unsigned int i = 0; i < jobslist.size(); i++){
    if(cpid.compare(jobslist.at(i).at(0)) == 0){
      jobslist.at(i).at(1) = status;
      return 0;
    }
  }

  return -1;
}





//Question List:
//

#include "builtins.h"

using namespace std;


int com_ls(vector<string>& tokens) {
  // if no directory is given, use the local directory
  if (tokens.size() < 2) {
    tokens.push_back(".");
  }

  // open the directory
  DIR* dir = opendir(tokens[1].c_str());

  // catch an errors opening the directory
  if (!dir) {
    // print the error from the last system call with the given prefix
    perror("ls error: ");

    // return error
    return 1;
  }

  // output each entry in the directory
  for (dirent* current = readdir(dir); current; current = readdir(dir)) {
    cout << current->d_name << endl;
  }

  // return success
  return 0;
}


int com_cd(vector<string>& tokens) {
	const char*	directory = tokens[1].c_str();
	DIR* dir;
	if (directory) dir = opendir(directory);
	if (!dir){
		perror("error cd");
	}
	else {
		chdir(directory);
	}
  return 0;
}


int com_pwd(vector<string>& tokens) {
  // HINT: you should implement the actual fetching of the current directory in
  // pwd(), since this information is also used for your prompt
  cout << pwd() << endl; // delete when implemented
  return 0;
}


int com_alias(vector<string>& tokens) {
  // TODO: YOUR CODE GOES HERE
  cout << "alias called" << endl; // delete when implemented
  return 0;
}


int com_unalias(vector<string>& tokens) {
  // TODO: YOUR CODE GOES HERE
  cout << "unalias called" << endl; // delete when implemented
  return 0;
}


int com_echo(vector<string>& tokens) {
	for (int i = 1; i < tokens.size(); i++){
		cout << tokens[i] << " ";
	}
	cout << endl;
  return 0;
}


int com_exit(vector<string>& tokens) {
  exit(0);
  return 0;
}


int com_history(vector<string>& tokens) {
	HIST_ENTRY** hist = history_list();

	for (int i = 0; i < where_history(); i++){
		cout << i << " " << hist[i]->line << endl;
	}

  return 0;
}

string pwd() {
	char buff[PATH_MAX + 1];
  getcwd(buff, PATH_MAX + 1);
  return buff;
}

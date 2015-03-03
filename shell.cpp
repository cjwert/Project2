#include <cstdlib>
#include <iostream>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <readline/readline.h>
#include <sys/stat.h>
#include <readline/history.h>

#include "builtins.h"

// Potentially useful #includes (either here or in builtins.h):
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/errno.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <regex>

using namespace std;

//function prototypes
int execute_background_command(vector<string>&);

// The characters that readline will use to delimit words
const char* const WORD_DELIMITERS = " \t\n\"\\'`@><=;|&{(";

// An external reference to the execution environment
extern char** environ;

// Define 'command' as a type for built-in commands
typedef int (*command)(vector<string>&);

// A mapping of internal commands to their corresponding functions
map<string, command> builtins;

// Variables local to the shell
map<string, string> localvars;

char *convert(const string & s)
{
   char *pc = new char[s.size()+1];
   strcpy(pc, s.c_str());
   return pc;
}

// Handles external commands, redirects, and pipes.
int execute_external_command(vector<string> tokens) {
  int ret_val = -1;
  int status;

  pid_t childProcess = fork();
  if (childProcess == 0){ //child process case
    vector<char*> vc;
    transform(tokens.begin(), tokens.end(), std::back_inserter(vc), convert);
    vc.push_back(NULL);
    char **command = &vc[0];
    ret_val = execvp(command[0], command);
    if (ret_val < 0) {
      cerr << "ERROR: execvp failed\n";
      exit(1);
    }
    exit(childProcess);
  } else { //this is the parent process case
    pid_t finish = wait(&status);
  }
  return ret_val;
}


// Return a string representing the prompt to display to the user. It needs to
// include the current working directory and should also use the return value to
// indicate the result (success or failure) of the last command.
string get_prompt(int return_value) {
  if (return_value == 0){
    return pwd() + " :) $ ";
  } else {
    return pwd() + " :( $ ";
  }
  
  //return "prompt > "; // replace with your own code
}


// Return one of the matches, or NULL if there are no more.
char* pop_match(vector<string>& matches) {
  if (matches.size() > 0) {
    const char* match = matches.back().c_str();

    // Delete the last element
    matches.pop_back();

    // We need to return a copy, because readline deallocates when done
    char* copy = (char*) malloc(strlen(match) + 1);
    strcpy(copy, match);

    return copy;
  }

  // No more matches
  return NULL;
}


// Generates environment variables for readline completion. This function will
// be called multiple times by readline and will return a single cstring each
// time.
char* environment_completion_generator(const char* text, int state) {
  // A list of all the matches;
  // Must be static because this function is called repeatedly
  static vector<string> matches;

  // If this is the first time called, construct the matches list with
  // all possible matches
  if (state == 0) {
    // TODO: YOUR CODE GOES HERE
  }

  // Return a single match (one for each time the function is called)
  return pop_match(matches);
}


// Generates commands for readline completion. This function will be called
// multiple times by readline and will return a single cstring each time.
char* command_completion_generator(const char* text, int state) {
  // A list of all the matches;
  // Must be static because this function is called repeatedly
  static vector<string> matches;

  // If this is the first time called, construct the matches list with
  // all possible matches
  if (state == 0) {
    // TODO: YOUR CODE GOES HERE
  }

  // Return a single match (one for each time the function is called)
  return pop_match(matches);
}


// This is the function we registered as rl_attempted_completion_function. It
// attempts to complete with a command, variable name, or filename.
char** word_completion(const char* text, int start, int end) {
  char** matches = NULL;

  if (start == 0) {
    rl_completion_append_character = ' ';
    matches = rl_completion_matches(text, command_completion_generator);
  } else if (text[0] == '$') {
    rl_completion_append_character = ' ';
    matches = rl_completion_matches(text, environment_completion_generator);
  } else {
    rl_completion_append_character = '\0';
    // We get directory matches for free (thanks, readline!)
  }

  return matches;
}


// Transform a C-style string into a C++ vector of string tokens, delimited by
// whitespace.
vector<string> tokenize(const char* line) {
  vector<string> tokens;
  string token;

  // istringstream allows us to treat the string like a stream
  istringstream token_stream(line);

  while (token_stream >> token) {
    tokens.push_back(token);
  }

  // Search for quotation marks, which are explicitly disallowed
  for (size_t i = 0; i < tokens.size(); i++) {

    if (tokens[i].find_first_of("\"'`") != string::npos) {
      cerr << "\", ', and ` characters are not allowed." << endl;

      tokens.clear();
    }
  }

  return tokens;
}

// Executes a line of input by either calling execute_external_command or
// directly invoking the built-in command.
int execute_line(vector<string>& tokens, map<string, command>& builtins) {
  int return_value = 0;
  if (tokens.at(tokens.size() - 1) == "&"){
    return execute_background_command(tokens);
  }
  if (tokens.size() != 0) {
    map<string, command>::iterator cmd = builtins.find(tokens[0]);

    if (cmd == builtins.end()) {
      return_value = execute_external_command(tokens);
    } else {
      return_value = ((*cmd->second)(tokens));
    }
  }
  return return_value;
}

int execute_background_command(vector<string>& tokens) {
  int ret_val = -1;
  pid_t childProcess = fork();
  if (childProcess == 0) {
    tokens.pop_back(); //remove &
    ret_val = execute_line(tokens, builtins);
  }else{
    cout << "bg count?" << endl;
  }
  return ret_val;
}


// Substitutes any tokens that start with a $ with their appropriate value, or
// with an empty string if no match is found.
void variable_substitution(vector<string>& tokens) {
  vector<string>::iterator token;

  for (token = tokens.begin(); token != tokens.end(); ++token) {

    if (token->at(0) == '$') {
      string var_name = token->substr(1);

      if (getenv(var_name.c_str()) != NULL) {
        *token = getenv(var_name.c_str());
      } else if (localvars.find(var_name) != localvars.end()) {
        *token = localvars.find(var_name)->second;
      } else {
        *token = "";
      }
    }
  }
}


// Examines each token and sets an env variable for any that are in the form
// of key=value.
void local_variable_assignment(vector<string>& tokens) {
  vector<string>::iterator token = tokens.begin();

  while (token != tokens.end()) {
    string::size_type eq_pos = token->find("=");

    // If there is an equal sign in the token, assume the token is var=value
    if (eq_pos != string::npos) {
      string name = token->substr(0, eq_pos);
      string value = token->substr(eq_pos + 1);

      localvars[name] = value;

      token = tokens.erase(token);
    } else {
      ++token;
    }
  }
}

int file_redirect(vector<string>& tokens) {
	int return_val;
	bool redirect_flag = false;
	for (int i = 0; i < tokens.size(); i++){
		if (tokens[i] == "<") {
			char* outputFile = (char*)tokens[i+1].c_str();
			int out = open(outputFile, O_RDONLY);
			int savestdin = dup(0);
			dup2(out, 0);
			tokens.pop_back();
			tokens.pop_back();
			return_val = execute_line(tokens, builtins);
			dup2(savestdin, 0);
			close(out);
			redirect_flag = true;
			break;
		}
		else if (tokens[i] == ">") {			//overwrite
			char* outputFile = (char*)tokens[i+1].c_str();
			int out = open(outputFile, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
			int savestdout = dup(1);
			dup2(out, 1);
			tokens.pop_back();
			tokens.pop_back();
			return_val = execute_line(tokens, builtins);
			dup2(savestdout, 1);
			close(out);
			redirect_flag = true;
			break;
		}
		else if (tokens[i] == ">>") {			//append
			char* outputFile = (char*)tokens[i+1].c_str();
			int out = open(outputFile, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
			int savestdout = dup(1);
			dup2(out, 1);
			tokens.pop_back();
			tokens.pop_back();
			return_val = execute_line(tokens, builtins);
			dup2(savestdout, 1);
			close(out);
			redirect_flag = true;
			break;
		}
	}
	if (redirect_flag){
  	return return_val;
	} else {
		return execute_line(tokens, builtins);
	}
}


// The main program
int main() {
  // Populate the map of available built-in functions
  builtins["ls"] = &com_ls;
  builtins["cd"] = &com_cd;
  builtins["pwd"] = &com_pwd;
  builtins["alias"] = &com_alias;
  builtins["unalias"] = &com_unalias;
  builtins["echo"] = &com_echo;
  builtins["exit"] = &com_exit;
  builtins["history"] = &com_history;

  // Specify the characters that readline uses to delimit words
  rl_basic_word_break_characters = (char *) WORD_DELIMITERS;

  // Tell the completer that we want to try completion first
  rl_attempted_completion_function = word_completion;

  // The return value of the last command executed
  int return_value = 0;

  // Loop for multiple successive commands 
  while (true) {

    // Get the prompt to show, based on the return value of the last command
    string prompt = get_prompt(return_value);

    // Read a line of input from the user
    char* line = readline(prompt.c_str());

    // If the pointer is null, then an EOF has been received (ctrl-d)
    if (!line) {
      break;
    }

	  string lineAsString = line;

	  if (line[0] == '!'){
      if (line[1] == '!'){
        strcpy(line, history_get(where_history())->line);
        cout << line << endl;
      }
      else if (isdigit(line[1])){
        int i = 1;
        string buffer = "";
        while (isdigit(line[i])){
          buffer = buffer + line[i];
          i++;
        }
        int history_index = stoi(buffer);
        
        if (history_index > where_history()-1) {
          cerr << "Requested index is larger than max in history." << endl;
          *line = '\0';
        } else {
          strcpy(line, history_get(history_index + 1)->line);
          cout << line << endl;
        }
      }
    }

    // If the command is non-empty, attempt to execute it
    if (line[0]) {

      // Add this command to readline's history
      // string lineAsString = line;
      if (lineAsString.compare("history") != 0) {
        add_history(line);
      }

      // Break the raw input line into tokens
      vector<string> tokens = tokenize(line);

      // Handle local variable declarations
      local_variable_assignment(tokens);

      // Substitute variable references
      variable_substitution(tokens);
			
			//file redirection and line execution
			return_value = file_redirect(tokens);
			//cout << endl << endl << where_history() << endl;	//this is for my testing
    }

    // Free the memory for the input string
    free(line);
  }

  return 0;
}

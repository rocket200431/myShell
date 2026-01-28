#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>

#define MAX_INPUT_SIZE 1024
#define MAX_ARGS 64
#define MAX_PIPE_COMMANDS 5  // Max 4 pipes mean 5 commands
#define ROLL_NO "myRollNo" 

/*
  line The raw input string from the user.
  args An array of strings to store the parsed arguments.
 */
void parse_input(char *line, char **args) {
    int i = 0;
    args[i] = strtok(line, " \t\n");
    while (args[i] != NULL && i < MAX_ARGS - 1) {
        i++;
        args[i] = strtok(NULL, " \t\n");
    }
    args[i] = NULL;
}
/**
   Executes external commands.
   Forks a new process to run the command using execvp.
   The parent process waits for the child to complete args The command and its arguments.
 */
void execute_external_command(char **args) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        return;
    } else if (pid == 0) {
        // Child process
        if (execvp(args[0], args) == -1) {
            perror(args[0]);
            exit(EXIT_FAILURE);
        }
    } else {
        // Parent process
        int status;
        waitpid(pid, &status, 0);
    }
}

/*
   Built-in handler for the 'pwd' command.
   Prints the current working directory.
 */
void handle_pwd() {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
    } else {
        perror("getcwd() error");
    }
}

/*
   Built-in handler for the 'cd' command.
   Changes the current directory.
   args The arguments array, where args[1] is the target directory.
 */
void handle_cd(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "cd: expected argument to \"cd\"\n");
    } else {
        if (chdir(args[1]) != 0) {
            perror("chdir() error");
        }
    }
}

/*
   Built-in handler for the 'mkdir' command.
   Creates a new directory with default permissions (0755).
   args The arguments array, where args[1] is the directory name.
 */
void handle_mkdir(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "mkdir: missing operand\n");
    } else {
        // 0755 permissions: rwx for owner, r-x for group and others
        if (mkdir(args[1], 0755) != 0) {
            perror("mkdir() error");
        }
    }
}

/*
   Built-in handler for the 'rmdir' command.
   Removes an empty directory.
   args The arguments array, where args[1] is the directory name.
 */
void handle_rmdir(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "rmdir: missing operand\n");
    } else {
        if (rmdir(args[1]) != 0) {
            perror("rmdir() error");
        }
    }
}
/*
   Helper function for 'ls -l' to print file details.
   filename - The name of the file to detail.
 */
void print_file_details(const char* filename) {
    struct stat fileStat;
    if(stat(filename, &fileStat) < 0) {
        perror("stat");
        return;
    }

    // File type and permissions
    printf( (S_ISDIR(fileStat.st_mode)) ? "d" : "-");
    printf( (fileStat.st_mode & S_IRUSR) ? "r" : "-");
    printf( (fileStat.st_mode & S_IWUSR) ? "w" : "-");
    printf( (fileStat.st_mode & S_IXUSR) ? "x" : "-");
    printf( (fileStat.st_mode & S_IRGRP) ? "r" : "-");
    printf( (fileStat.st_mode & S_IWGRP) ? "w" : "-");
    printf( (fileStat.st_mode & S_IXGRP) ? "x" : "-");
    printf( (fileStat.st_mode & S_IROTH) ? "r" : "-");
    printf( (fileStat.st_mode & S_IWOTH) ? "w" : "-");
    printf( (fileStat.st_mode & S_IXOTH) ? "x" : "-");

    // Number of links
    printf(" %2lu", fileStat.st_nlink);

    // Owner and group
    struct passwd *pw = getpwuid(fileStat.st_uid);
    struct group  *gr = getgrgid(fileStat.st_gid);
    printf(" %s %s", pw->pw_name, gr->gr_name);

    // File size
    printf(" %8ld", fileStat.st_size);

    // Modification time
    char time_buf[80];
    strftime(time_buf, sizeof(time_buf), "%b %d %H:%M", localtime(&fileStat.st_mtime));
    printf(" %s", time_buf);

    // Filename
    printf(" %s\n", filename);
}
/*
   Built-in handler for the 'ls' command.
   Supports 'ls' and 'ls -l'.
   args The arguments array to check for the '-l' option.
 */
void handle_ls(char **args) {
    DIR *d;
    struct dirent *dir;
    int long_format = 0;

    if (args[1] != NULL && strcmp(args[1], "-l") == 0) {
        long_format = 1;
    }

    d = opendir(".");
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            // Do not list hidden files (starting with '.')
            if (dir->d_name[0] == '.') {
                continue;
            }
            if (long_format) {
                print_file_details(dir->d_name);
            } else {
                printf("%s\n", dir->d_name);
            }
        }
        closedir(d);
    } else {
        perror("opendir() error");
    }
}
/*
   Built-in handler for the 'cp' command.
   Copies file1 to file2 only if file1's modification time is more recent than file2's.
   args The arguments array, args[1] is source, args[2] is destination.
 */
void handle_cp(char **args) {
    if (args[1] == NULL || args[2] == NULL) {
        fprintf(stderr, "cp: missing source or destination file operand\n");
        return;
    }

    struct stat stat_src, stat_dest;
    int dest_exists = 1;

    // Get stats for source file
    if (stat(args[1], &stat_src) != 0) {
        perror(args[1]);
        return;
    }

    // Get stats for destination file. It's okay if it doesn't exist.
    if (stat(args[2], &stat_dest) != 0) {
        dest_exists = 0;
    }

    // Proceed with copy if destination doesn't exist or source is newer
    if (!dest_exists || difftime(stat_src.st_mtime, stat_dest.st_mtime) > 0) {
        FILE *src = fopen(args[1], "rb");
        if (!src) {
            perror(args[1]);
            return;
        }

        FILE *dest = fopen(args[2], "wb");
        if (!dest) {
            perror(args[2]);
            fclose(src);
            return;
        }

        char buffer[4096];
        size_t bytes;
        while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
            fwrite(buffer, 1, bytes, dest);
        }

        fclose(src);
        fclose(dest);
        printf("Copied '%s' to '%s'\n", args[1], args[2]);
    } else {
        printf("'%s' is not newer than '%s'. No copy performed.\n", args[1], args[2]);
    }
}
/*
   Parses a single command line, identifying arguments and redirection files.
   line The command line string to parse. This string will be modified (tokenized).
   args An array to be populated with command arguments.
   input_file A pointer to a string that will hold the input redirection filename.
   output_file A pointer to a string that will hold the output redirection filename.
 */
void parse_line(char *line, char **args, char **input_file, char **output_file) {
    int i = 0;
    *input_file = NULL;
    *output_file = NULL;

    char *token = strtok(line, " \t\n");
    while (token != NULL) {
        if (strcmp(token, "<") == 0) {
            token = strtok(NULL, " \t\n");
            if (token) *input_file = token;
        } else if (strcmp(token, ">") == 0) {
            token = strtok(NULL, " \t\n");
            if (token) *output_file = token;
        } else {
            args[i++] = token;
        }
        token = strtok(NULL, " \t\n");
    }
    args[i] = NULL;
}

/*
   Executes a single command, handling I/O redirection.
   args The command and its arguments.
   input_file The name of the file for input redirection, or NULL.
   output_file The name of the file for output redirection, or NULL.
 */
void execute_command(char **args, char *input_file, char *output_file) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        return;
    }

    if (pid == 0) { // Child process
        // Handle input redirection
        if (input_file != NULL) {
            int in_fd = open(input_file, O_RDONLY);
            if (in_fd < 0) {
                perror("open input file");
                exit(EXIT_FAILURE);
            }
            dup2(in_fd, STDIN_FILENO); // Redirect stdin
            close(in_fd);
        }

        // Handle output redirection
        if (output_file != NULL) {
            // Create file with permissions rw-r--r--
            int out_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (out_fd < 0) {
                perror("open output file");
                exit(EXIT_FAILURE);
            }
            dup2(out_fd, STDOUT_FILENO); // Redirect stdout
            close(out_fd);
        }

        if (execvp(args[0], args) == -1) {
            perror(args[0]);
            exit(EXIT_FAILURE);
        }
    } else { // Parent process
        wait(NULL);
    }
}
/*
   Executes a series of commands connected by pipes.
   input_line The full command line containing pipes.
 */
void execute_piped_commands(char *input_line) {
    char *commands[MAX_PIPE_COMMANDS];
    int num_commands = 0;

    // Split the input line into commands based on the pipe '|' delimiter
    char *token = strtok(input_line, "|");
    while (token != NULL && num_commands < MAX_PIPE_COMMANDS) {
        commands[num_commands++] = token;
        token = strtok(NULL, "|");
    }

    int pipe_fds[2];
    int in_fd = 0; // Stdin for the first command

    for (int i = 0; i < num_commands; i++) {
        char *args[MAX_ARGS];
        // Note: Redirection within pipes is not supported in this simplified version.
        char *input_file = NULL;
        char *output_file = NULL;

        // Parse each individual command for arguments
        parse_line(commands[i], args, &input_file, &output_file);

        if (pipe(pipe_fds) < 0) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if (pid == 0) { // Child process
            // Redirect input
            // If it's not the first command, read from the previous pipe.
            if (in_fd != 0) {
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
            }

            // Redirect output
            // If it's not the last command, write to the current pipe.
            if (i < num_commands - 1) {
                dup2(pipe_fds[1], STDOUT_FILENO);
            }
            
            // Close unused pipe ends
            close(pipe_fds[0]);
            close(pipe_fds[1]);

            // Execute the command
            if (execvp(args[0], args) == -1) {
                perror(args[0]);
                exit(EXIT_FAILURE);
            }
        } else { // Parent process
            wait(NULL);
            
            // Close the write end of the pipe in the parent
            close(pipe_fds[1]);
            
            // Close the previous read fd if it's not stdin
            if (in_fd != 0) {
                close(in_fd);
            }
            
            // The read end of the current pipe becomes the input for the next command
            in_fd = pipe_fds[0];
        }
    }
}

// Expands '<' and '>' with spaces around them, so "ls>out" -> "ls > out"
void normalize_redirects(char *dst, size_t dstsz, const char *src) {
    size_t j = 0;
    for (size_t i = 0; src[i] != '\0' && j + 3 < dstsz; ++i) {
        if (src[i] == '<' || src[i] == '>') {
            if (j + 3 >= dstsz) break; // avoid overflow
            dst[j++] = ' ';
            dst[j++] = src[i];
            dst[j++] = ' ';
        } else {
            dst[j++] = src[i];
        }
    }
    dst[j] = '\0';
}

int main() {
    char input_line[MAX_INPUT_SIZE];
    char prompt[100];

    // Create the custom prompt
    snprintf(prompt, sizeof(prompt), "%s_Shell) ", ROLL_NO);

    while (1) {
        printf("%s", prompt);
        fflush(stdout);

        // Read user input
        if (fgets(input_line, sizeof(input_line), stdin) == NULL) {
            printf("\n");
            break; // Handle Ctrl+D (EOF)
        }

        // Remove trailing newline character
        input_line[strcspn(input_line, "\r\n")] = 0;
        
        // If input is empty, continue
        if(strlen(input_line) == 0) {
            continue;
        }

        // Check for pipes first, as it's a more complex execution path
        if (strchr(input_line, '|')) {
            execute_piped_commands(input_line);
        } else {
            // Handle single commands (with potential I/O redirection)
            char *args[MAX_ARGS];
            char *input_file = NULL;
            char *output_file = NULL;

            // Create a mutable copy for parsing
            char expanded[2 * MAX_INPUT_SIZE];
            normalize_redirects(expanded, sizeof(expanded), input_line);
            parse_line(expanded, args, &input_file, &output_file);

            if (args[0] == NULL) {
                continue;
            }

            // Handle built-in commands
            if (strcmp(args[0], "exit") == 0) {
                break;
            } else if (strcmp(args[0], "cd") == 0) {
                if (args[1] == NULL) {
                    fprintf(stderr, "cd: expected argument\n");
                } else {
                    if (chdir(args[1]) != 0) {
                        perror("cd");
                    }
                }
            } else if (strcmp(args[0], "pwd") == 0) {
                 char cwd[1024];
                 if (getcwd(cwd, sizeof(cwd)) != NULL) {
                     printf("%s\n", cwd);
                 } else {
                     perror("pwd");
                 }
            } else {
                // Execute external command
                execute_command(args, input_file, output_file);
            }
        }
    }
    return 0;
}
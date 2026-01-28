#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    // Compile 23CS01011_shell.c
    if (system("gcc myShell.c -o shell") != 0) {
        fprintf(stderr, "Compilation failed!\n");
        return 1;
    }

    // Run the compiled shell
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    }

    if (pid == 0) {
        execl("./shell", "myShell", NULL);
        perror("execl");
        exit(1);
    } else {
        waitpid(pid, NULL, 0);
    }

    return 0;
}
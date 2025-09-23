#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MAX_COMMAND_LENGTH 1024
#define MAX_ARGS 64
#define MAX_HISTORY 100

// Global variables
char *history_commands[MAX_HISTORY];
int history_count = 0;
char current_dir[1024];

// Function declarations
void execute_command(char *command);
void parse_command(char *command, char **args);
void add_to_history(const char *command);
void display_history();
void change_directory(char **args);
void print_working_directory();
void clear_screen();
void handle_exit();

// Built-in commands
int is_builtin_command(char *command)
{
    char *builtins[] = {"cd", "exit", "pwd", "history", "clear", NULL};

    for (int i = 0; builtins[i] != NULL; i++)
    {
        if (strcmp(command, builtins[i]) == 0)
        {
            return 1;
        }
    }
    return 0;
}

// Execute built-in commands
void execute_builtin(char **args)
{
    if (strcmp(args[0], "cd") == 0)
    {
        change_directory(args);
    }
    else if (strcmp(args[0], "exit") == 0)
    {
        handle_exit();
    }
    else if (strcmp(args[0], "pwd") == 0)
    {
        print_working_directory();
    }
    else if (strcmp(args[0], "history") == 0)
    {
        display_history();
    }
    else if (strcmp(args[0], "clear") == 0)
    {
        clear_screen();
    }
}

// Change directory implementation
void change_directory(char **args)
{
    if (args[1] == NULL)
    {
        // No argument, go to home directory
        char *home = getenv("HOME");
        if (home != NULL)
        {
            if (chdir(home) != 0)
            {
                perror("cd");
            }
        }
    }
    else
    {
        if (chdir(args[1]) != 0)
        {
            perror("cd");
        }
    }

    // Update current directory
    if (getcwd(current_dir, sizeof(current_dir)) == NULL)
    {
        perror("getcwd");
    }
}

// Print working directory
void print_working_directory()
{
    if (getcwd(current_dir, sizeof(current_dir)) != NULL)
    {
        printf("%s\n", current_dir);
    }
    else
    {
        perror("pwd");
    }
}

// Clear screen
void clear_screen()
{
    printf("\033[H\033[J"); // ANSI escape codes to clear screen
}

// Exit handler
void handle_exit()
{
    // Clean up history
    for (int i = 0; i < history_count; i++)
    {
        free(history_commands[i]);
    }
    printf("Goodbye!\n");
    exit(0);
}

// Add command to history
void add_to_history(const char *command)
{
    if (history_count < MAX_HISTORY)
    {
        history_commands[history_count] = strdup(command);
        history_count++;
    }
    else
    {
        // Shift history if full
        free(history_commands[0]);
        for (int i = 1; i < MAX_HISTORY; i++)
        {
            history_commands[i - 1] = history_commands[i];
        }
        history_commands[MAX_HISTORY - 1] = strdup(command);
    }

    // Also add to readline history for better line editing
    add_history(command);
}

// Display command history
void display_history()
{
    printf("Command History:\n");
    for (int i = 0; i < history_count; i++)
    {
        printf("%d: %s\n", i + 1, history_commands[i]);
    }
}

// Parse command into arguments
void parse_command(char *command, char **args)
{
    char *token;
    int i = 0;

    token = strtok(command, " \t\n");
    while (token != NULL && i < MAX_ARGS - 1)
    {
        args[i] = token;
        i++;
        token = strtok(NULL, " \t\n");
    }
    args[i] = NULL;
}

// Execute external commands using fork() and execvp()
void execute_external_command(char **args)
{
    pid_t pid;
    int status;

    pid = fork();

    if (pid == 0)
    {
        // Child process
        if (execvp(args[0], args) == -1)
        {
            perror("Command not found");
            exit(EXIT_FAILURE);
        }
    }
    else if (pid < 0)
    {
        // Fork failed
        perror("fork");
    }
    else
    {
        // Parent process
        do
        {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
}

// Handle command sequencing with semicolon
void handle_command_sequence(char *command)
{
    char *sequence[MAX_COMMAND_LENGTH];
    char *token;
    int i = 0;

    // Split commands by semicolon
    token = strtok(command, ";");
    while (token != NULL)
    {
        sequence[i] = token;
        i++;
        token = strtok(NULL, ";");
    }
    sequence[i] = NULL;

    // Execute each command in sequence
    for (int j = 0; j < i; j++)
    {
        char *trimmed_command = sequence[j];
        // Remove leading/trailing whitespace
        while (*trimmed_command == ' ' || *trimmed_command == '\t')
        {
            trimmed_command++;
        }

        if (strlen(trimmed_command) > 0)
        {
            execute_command(trimmed_command);
        }
    }
}

// Handle background execution with ampersand
int handle_background_execution(char *command)
{
    // Check if command ends with &
    int len = strlen(command);
    int background = 0;

    // Remove trailing whitespace and check for &
    while (len > 0 && (command[len - 1] == ' ' || command[len - 1] == '\t' || command[len - 1] == '\n'))
    {
        len--;
    }

    if (len > 0 && command[len - 1] == '&')
    {
        background = 1;
        command[len - 1] = '\0';

        // Remove any remaining trailing whitespace
        while (len > 1 && (command[len - 2] == ' ' || command[len - 2] == '\t'))
        {
            command[len - 2] = '\0';
            len--;
        }
    }

    return background;
}

// Main command execution function
void execute_command(char *command)
{
    if (command == NULL || strlen(command) == 0)
    {
        return;
    }

    // Add to history
    add_to_history(command);

    // Check for command sequencing
    if (strstr(command, ";") != NULL)
    {
        handle_command_sequence(command);
        return;
    }

    // Handle background execution
    int background = handle_background_execution(command);

    char *args[MAX_ARGS];
    parse_command(command, args);

    if (args[0] == NULL)
    {
        return; // Empty command
    }

    if (is_builtin_command(args[0]))
    {
        execute_builtin(args);
    }
    else
    {
        if (background)
        {
            // Background execution
            pid_t pid = fork();

            if (pid == 0)
            {
                // Child process for background execution
                if (execvp(args[0], args) == -1)
                {
                    perror("Command not found");
                    exit(EXIT_FAILURE);
                }
            }
            else if (pid < 0)
            {
                perror("fork");
            }
            else
            {
                printf("[Background PID: %d]\n", pid);
            }
        }
        else
        {
            // Foreground execution
            execute_external_command(args);
        }
    }
}

// Main shell loop
int main()
{
    char *command;

    printf("Welcome to Simple Shell!\n");
    printf("Type 'exit' to quit or 'help' for available commands.\n\n");

    // Initialize current directory
    if (getcwd(current_dir, sizeof(current_dir)) == NULL)
    {
        strcpy(current_dir, "unknown");
    }

    while (1)
    {
        // Display prompt with current directory
        printf("\033[1;32mshell:%s$\033[0m ", current_dir);

        // Read command using readline for better line editing
        command = readline("");

        if (command == NULL)
        {
            printf("\n");
            handle_exit(); // Handle Ctrl+D
        }

        if (strlen(command) > 0)
        {
            execute_command(command);
        }

        free(command);
    }

    return 0;
}
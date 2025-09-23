#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <direct.h>

#define MAX_COMMAND_LENGTH 1024
#define MAX_ARGS 64
#define MAX_HISTORY 100

// Global variables
char history_commands[MAX_HISTORY][MAX_COMMAND_LENGTH];
int history_count = 0;
char current_dir[MAX_COMMAND_LENGTH];

// Function declarations
void execute_command(char *command);
void parse_command(char *command, char **args);
void add_to_history(const char *command);
void display_history();
void change_directory(char **args);
void print_working_directory();
void clear_screen();
void handle_exit();
char *read_input();

// Simple input function without readline
char *read_input()
{
    static char buffer[MAX_COMMAND_LENGTH];
    printf("shell:%s$ ", current_dir);
    fflush(stdout);

    if (fgets(buffer, MAX_COMMAND_LENGTH, stdin) == NULL)
    {
        return NULL; // Ctrl+Z or error
    }

    // Remove newline character
    buffer[strcspn(buffer, "\n")] = 0;
    return buffer;
}

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

// Change directory implementation (Windows)
void change_directory(char **args)
{
    if (args[1] == NULL)
    {
        // No argument, go to home directory
        char *home = getenv("USERPROFILE");
        if (home != NULL)
        {
            if (_chdir(home) != 0)
            {
                perror("cd");
            }
        }
    }
    else
    {
        if (_chdir(args[1]) != 0)
        {
            perror("cd");
        }
    }

    // Update current directory
    if (_getcwd(current_dir, sizeof(current_dir)) == NULL)
    {
        perror("getcwd");
    }
}

// Print working directory
void print_working_directory()
{
    if (_getcwd(current_dir, sizeof(current_dir)) != NULL)
    {
        printf("%s\n", current_dir);
    }
    else
    {
        perror("pwd");
    }
}

// Clear screen (Windows)
void clear_screen()
{
    system("cls");
}

// Exit handler
void handle_exit()
{
    printf("Goodbye!\n");
    exit(0);
}

// Add command to history
void add_to_history(const char *command)
{
    if (history_count < MAX_HISTORY)
    {
        strncpy(history_commands[history_count], command, MAX_COMMAND_LENGTH - 1);
        history_commands[history_count][MAX_COMMAND_LENGTH - 1] = '\0';
        history_count++;
    }
    else
    {
        // Shift history if full
        for (int i = 1; i < MAX_HISTORY; i++)
        {
            strcpy(history_commands[i - 1], history_commands[i]);
        }
        strncpy(history_commands[MAX_HISTORY - 1], command, MAX_COMMAND_LENGTH - 1);
        history_commands[MAX_HISTORY - 1][MAX_COMMAND_LENGTH - 1] = '\0';
    }
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

// Execute external commands (Windows)
void execute_external_command(char **args)
{
    // Build command string for system() call
    char command[MAX_COMMAND_LENGTH] = "";
    for (int i = 0; args[i] != NULL; i++)
    {
        strcat(command, args[i]);
        if (args[i + 1] != NULL)
        {
            strcat(command, " ");
        }
    }

    int result = system(command);
    if (result == -1)
    {
        perror("system");
    }
}

// Handle command sequencing with semicolon
void handle_command_sequence(char *command)
{
    char sequence[MAX_COMMAND_LENGTH][MAX_COMMAND_LENGTH];
    char *token;
    int i = 0;

    // Split commands by semicolon
    char command_copy[MAX_COMMAND_LENGTH];
    strcpy(command_copy, command);
    token = strtok(command_copy, ";");
    while (token != NULL && i < MAX_COMMAND_LENGTH)
    {
        strcpy(sequence[i], token);
        i++;
        token = strtok(NULL, ";");
    }

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

// Handle background execution (simplified for Windows)
int handle_background_execution(char *command)
{
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
    char command_copy[MAX_COMMAND_LENGTH];
    strcpy(command_copy, command);
    parse_command(command_copy, args);

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
            // Background execution (start detached process)
            char bg_command[MAX_COMMAND_LENGTH + 10];
            snprintf(bg_command, sizeof(bg_command), "start /B %s", command);
            system(bg_command);
            printf("[Background process started]\n");
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
    printf("Welcome to Simple Shell (Windows Version)!\n");
    printf("Type 'exit' to quit.\n\n");

    // Initialize current directory
    if (_getcwd(current_dir, sizeof(current_dir)) == NULL)
    {
        strcpy(current_dir, "unknown");
    }

    while (1)
    {
        char *command = read_input();

        if (command == NULL)
        {
            printf("\n");
            handle_exit(); // Handle Ctrl+Z
        }

        if (strlen(command) > 0)
        {
            execute_command(command);
        }
    }

    return 0;
}
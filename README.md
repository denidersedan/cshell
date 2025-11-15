cshell

cshell is a custom Unix shell written in C. It supports essential shell functionality, including command execution, full job control, command history, and a set of custom file system navigation tools.

Features

    Standard UNIX Commands: Executes external commands (e.g., ls, grep, vim) using fork and exec.

    Job Control: Full support for background (&) and foreground processes.

        jobs: List all background jobs.

        fg %<id>: Bring a job to the foreground.

        bg %<id>: Resume a stopped job in the background.

    Command History:

        history: Display all previously entered commands.

    Custom File System Tools:

        peek [dir]: Displays a recursive tree view of a directory's contents.

        seek [flags] [path]: Searches for files.

            -n <pattern>: Filter by file name.

            -s <max_size>: Filter by maximum file size in bytes.

        warp <dir>: A custom "fast navigation" command to quickly jump to directories (presumably bookmarked or from history).

    Core Built-ins:

        cd <dir>: Change the current directory.

        pwd: Print the working directory.

        help: Display the help message.

        exit: Exit the shell.

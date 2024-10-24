#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

#define ERR_FATAL "error: fatal\n"
#define ERR_CD_ARGS "error: cd: bad arguments\n"
#define ERR_CD_FAIL "error: cd: cannot change directory to "
#define ERR_EXECV "error: cannot execute "

void err(char *str)
{
	while (*str)
		write(2, str++, 1);
}

int cd(char **av, int i)
{
	if (i != 2)
	{
		err(ERR_CD_ARGS);
		return (1);
	}
	if (chdir(av[1]) == -1)
	{
		err(ERR_CD_FAIL);
		err(av[1]);
		err("\n");
		return (1);
	}
	return (0);
}

void set_pipe(int has_pipe, int *fd, int end)
{
	if (has_pipe)
	{
		if (dup2(fd[end], end) == -1 || close(fd[0]) == -1 || close(fd[1]) == -1)
		{
			err(ERR_FATAL);
			exit(1);
		}
	}
}

int execute(char **av, int i, char **env, int *fd_in)
{
	int fd[2], pid, status;
	int has_pipe = av[i] && !strcmp(av[i], "|");

	if (!strcmp(*av, "cd"))
		return cd(av, i);
	if (has_pipe && pipe(fd) == -1)
	{
		err(ERR_FATAL);
		exit(1);
	}
	if ((pid = fork()) == -1)
	{
		err(ERR_FATAL);
		exit(1);
	}

	// Child Process
	if (pid == 0)
	{
		// Null-terminate the current command
		av[i] = NULL;

		// Only redirect stdin from a previous pipe if fd_in is valid
		if (fd_in && *fd_in != -1)
		{
			if (dup2(*fd_in, 0) == -1)
			{
				err(ERR_FATAL);
				exit(1);
			}
			close(*fd_in);
		}

		// If there is a pipe, redirect output to the write-end of the pipe
		set_pipe(has_pipe, fd, 1);

		// Execute the command
		execve(*av, av, env);
		err(ERR_EXECV), err(*av), err("\n");
		exit(1);
	}

	// Parent process: wait for the child to finish
	waitpid(pid, &status, 0);

	// Close the write-end of the pipe
	if (has_pipe)
		close(fd[1]);

	// If there was input from a previous pipe, close it
	if (fd_in && *fd_in != -1)
		close(*fd_in);

	// Pass the read-end of the pipe to the next command
	if (has_pipe)
		*fd_in = fd[0];
	else
		*fd_in = -1;

	return WIFEXITED(status) && WEXITSTATUS(status);
}

int main(int ac, char **av, char **env)
{
	(void)ac;
	int i = 0, status = 0, fd_in = -1;

	while (av[i])
	{
		// Skip any semicolons
		av += i + 1;
		i = 0;

		// Find the next `|` or `;`
		while (av[i] && strcmp(av[i], "|") && strcmp(av[i], ";"))
			i++;

		// We found a command to execute
		if (i > 0)
			status = execute(av, i, env, &fd_in);

		// If it's a semicolon, reset the pipe
		if (av[i] && !strcmp(av[i], ";"))
			fd_in = -1;
	}
	return status;
}

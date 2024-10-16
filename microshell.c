#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

void	err(char *str)
{
	while (*str)
		write (2, str++, 1);

}

void	close_fds(int *fd)
{
	if (fd[0] != -1) close(fd[0]);
	if (fd[1] != -1) close(fd[1]);
}

int		cd(char **av)
{
	if (!av[1] || av[2])
	{
		err("error: cd: bad arguments\n");
		return (1);
	}
	if (chdir(av[1]) == -1)
	{
		err("error: cd: cannot change directory to ");
		err(av[1]);
		err("\n");
		return (1);
	}
	return (0);
}

// This function handles the execution of a command and manages piping between processes.
int		exec_cmd(char **av, char **env, int *infd, int has_pipe, int *pipefd)
{
	int pid, status;
	if (has_pipe && pipe(pipefd) == -1)
	{
		err("error: fatal\n");
		exit (1);
	}
	if ((pid = fork()) == -1)
	{
		err("error: fatal\n");
		exit(1);
	}
	if (pid == 0)
	{
		if (infd && *infd != -1 && dup2(*infd, 0) == -1)
		{
			err("error: fatal\n");
			exit(1);
		}
		if (has_pipe && dup2(pipefd[1], 1) == -1)
		{
			err("error: fatal\n");
			exit (1);
		}	
		close_fds(infd);
		close_fds(pipefd);
		execve(av[0], av, env);
		err("error: cannot execute ");
        err(av[0]);
        err("\n");
        exit(1);
	}
	if (infd && *infd != -1) close (*infd);
	if (has_pipe) close(pipefd[1]);
	if (!has_pipe) waitpid(pid, &status, 0);
	return (pid);
}

int	main(int ac, char **av, char **env)
{
	int i = 1, status = 0, has_pipe = 0, infd = -1, pipefd[2];
	char **cmd_start = NULL;
	(void)ac;
	while (av[i])
	{
		cmd_start = &av[i];
		while (av[i] && strcmp(av[i], "|") && strcmp(av[i], ";"))
			i++;
		has_pipe = (av[i] && strcmp(av[i], "|") == 0);
		if (av[i])
			av[i++] = NULL;
		if (cmd_start[0])
		{
			if (!strcmp(cmd_start[0], "cd"))
			{
				if (cd(cmd_start))
					status = 1;
			}
			else
			{
				exec_cmd(cmd_start, env, &infd, has_pipe, pipefd);
				if (has_pipe)
					infd = pipefd[0];
			}
		}
		if (!has_pipe)
			infd = -1;
	}
	while (waitpid(-1, &status, 0) != -1);
	return (status);
}

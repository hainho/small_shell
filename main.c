#include "smallsh.h"

void catchchld(int signo);
void goback(int signo);

sigjmp_buf position;

int main()
{

    act2.sa_handler = catchchld;
    act2.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &act2, &oact2);

    char prompt[MAXBUF];

    if (sigsetjmp(position, 1) == 0)
    {
        act1.sa_handler = goback;
        sigaction(SIGINT, &act1, &oact1);
    }
    if (getcwd(prompt, MAXBUF) == NULL)
        perror("getcwd error");
    while (userin(prompt) != EOF)
    {
        procline();
        if (getcwd(prompt, MAXBUF) == NULL)
            perror("getcwd error");
    }
    sigaction(SIGINT, &oact1, NULL);
    sigaction(SIGCHLD, &oact2, NULL);

    return 0;
}

void catchchld(int signo)
{
    int status;
    waitpid(-1, &status, WNOHANG);
}

void goback(int signo)
{
    int status;
    while (waitpid(-1, &status, WNOHANG) != -1)
    {
    }
    printf("\n");
    siglongjmp(position, 1);
}
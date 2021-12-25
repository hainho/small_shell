#include "smallsh.h"
static char inpbuf[MAXBUF];
static char tokbuf[2 * MAXBUF];
static char *ptr = inpbuf;
static char *tok = tokbuf;
static char special[] = {' ', '\t', '&', ';', '\n', '\0'};

int userin(char *p)
{
    int c, count;
    ptr = inpbuf;
    tok = tokbuf;

    //prompt 출력
    printf("%s$ ", p);
    count = 0;

    //한 글자씩 읽어옴
    while (1)
    {
        //getchar로 한글자 읽어 오고 끝이면 리턴
        if ((c = getchar()) == EOF)
            return EOF;

        // 아직 덜 읽어왔으면 저장하고 count 증가
        if (count < MAXBUF)
            inpbuf[count++] = c;

        // 다 읽어왔으면 \0 으로 끝 표시해주고 리턴
        if (c == '\n' && count < MAXBUF)
        {
            inpbuf[count] = '\0';
            return count;
        }

        // 범위 초과한 경우 입력 다시 받기 위한 메시지 출력, count 초기화
        if (c == '\n' || count >= MAXBUF)
        {
            printf("smallsh: input line too long\n");
            count = 0;
            printf("%s", p);
        }
    }
}

// 입력한 거 토큰 단위로 끊어서 가져옴
int gettok(char **outptr)
{
    int type;

    //스트링 배열의 첫번째 스트링이 시작점 가리키도록 함
    *outptr = tok;

    //공백이나 탭이면 뛰어넘어서 저장
    while (*ptr == ' ' || *ptr == '\t')
        ptr++;
    *tok++ = *ptr;

    // 개행 & ; 중 하나면 스킵 아니면 일반 arg 입력 받음
    switch (*ptr++)
    {
    case '\n':
        type = EOL;
        break;
    case '&':
        type = AMPERSAND;
        break;
    case ';':
        type = SEMICOLON;
        break;
    default:
        type = ARG;
        while (inarg(*ptr))
            *tok++ = *ptr++;
    }

    //
    *tok++ = '\0';
    return type;
}

int inarg(char c)
{
    char *wrk;

    // special에 속하는 문자 쭉
    for (wrk = special; *wrk; wrk++)
    {
        if (c == *wrk)
            return 0;
    }
    return 1;
}

void procline()
{
    char *arg[MAXARG + 1];
    int toktype, type;
    int narg = 0;
    for (;;)
    {
        // gettok 으로 읽어온 타입에 따라서 처리
        switch (toktype = gettok(&arg[narg]))
        {

        //arg 인겨우 하나씩 증가시키며 파싱함
        case ARG:
            if (narg < MAXARG)
                narg++;
            break;
        // 셋 중 하나인 경우 특정한 커멘드의 종료 이므로
        case EOL:
        case SEMICOLON:
        case AMPERSAND:
            if (toktype == AMPERSAND)
                type = BACKGROUND;
            else
                type = FOREGROUND;
            if (narg != 0)
            {
                arg[narg] = NULL;
                runcommand(arg, type);
            }
            if (toktype == EOL)
                return;
            narg = 0;
            break;
        }
    }
}

int runcommand(char **cline, int where)
{
    pid_t pid;
    int status;

    int idx = 0;
    while (cline[idx] != NULL)
    {
        if (strcmp(cline[idx], "|") == 0)
        {
            int fd[2];
            if (pipe(fd) == -1)
            {
                perror("pipe call");
            }
            cline[idx] = NULL;
            runpipe(cline, WRITEPIPE, fd);
            runpipe(cline + idx + 1, READPIPE, fd);
            waitpid(-1, &status, 0);
            waitpid(-1, &status, 0);
            return 0;
        }
        idx++;
    }

    if (where == WRITEPIPE)
    {
    }

    if (strcmp(cline[0], "cd") == 0)
    {
        runcd(cline);
    }
    else if (strcmp(cline[0], "exit") == 0)
    {
        exit(0);
    }
    else
    {
        switch (pid = fork())
        {
        case -1:
            perror("smallsh");
            return -1;
        case 0:
            sigaction(SIGINT, &oact1, NULL);
            checkdup(cline);
            execvp(*cline, cline);
            perror(*cline);
            exit(1);
        }
        if (where == BACKGROUND)
        {
            printf("[Process id] %d\n", pid);
            return 0;
        }
        if (waitpid(pid, &status, 0) == -1)
            return -1;
        else
            return status;
    }

    return 0;
}

int runpipe(char **cline, int where, int *fd)
{
    pid_t pid;
    int status;

    if (strcmp(cline[0], "cd") == 0)
    {
        runcd(cline);
    }
    else if (strcmp(cline[0], "exit") == 0)
    {
        exit(0);
    }
    else
    {
        switch (pid = fork())
        {
        case -1:
            perror("smallsh");
            return -1;
        case 0:
            sigaction(SIGINT, &oact1, NULL);
            if (where == WRITEPIPE)
            {
                close(fd[0]);
                dup2(fd[1], 1);
            }
            else if (where == READPIPE)
            {
                close(fd[1]);
                dup2(fd[0], 0);
            }
            checkdup(cline);
            execvp(*cline, cline);
            perror(*cline);
            exit(1);
        }
        if (where == WRITEPIPE)
        {
            close(fd[1]);
        }
    }
    return 0;
}

void runcd(char **cline)
{
    int len = 0;
    char **temp = cline;

    while (*temp++)
    {
        len++;
    }
    if (len == 2)
    {
        if (chdir(cline[1]) == -1)
        {
            printf("chdir fail");
        }
    }
    else if (len == 1)
    {
        if (chdir("/") == -1)
        {
            printf("chdir fail");
        }
    }
    else
    {
        printf("usage : %s <directory>\n", *cline);
    }
}

int checkdup(char **cline)
{
    int len = 0;
    int idx = 0;

    while (cline[idx])
    {
        if (strcmp(cline[idx], ">") == 0)
        {
            idx++;
            if (cline[idx])
            {
                int fd = open(cline[idx], O_RDWR | O_CREAT, NULL);
                if (fd == -1)
                {
                    printf("file open error\n");
                    return -1;
                }
                dup2(fd, 1);
                idx++;
                while (1)
                {
                    cline[idx - 2] = cline[idx];
                    if (cline[idx] == NULL)
                    {
                        break;
                    }
                    idx++;
                }
                if (idx == 2)
                {
                    while (1)
                    {
                        char c;
                        if ((c = getchar()) == EOF)
                        {
                            break;
                        }
                        inpbuf[0] = c;
                        if (write(1, inpbuf, 1) == -1)
                        {
                            break;
                        }
                    }
                }

                return 0;
            }
            else
            {
                printf("usage : > <file>\n");
                return -1;
            }
        }
        idx++;
    }
    return 1;
}
#include "csapp.h"

int main(void)
{
    char *buf, *p;
    char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
    int n1 = 0, n2 = 0;

    // buf에서는 환경 변수에 저장된 값을 받아 사용함
    // QUERY_STRING을 cgiargs로 overwrite 했기 때문에 
    // buf에 first=30&second=20과 같은 값이 들어감
    if((buf = getenv("QUERY_STRING")) != NULL) {
        p = strchr(buf, '&');
        *p = '\0';
        // strcpy(arg1, buf);
        // strcpy(arg2, p+1);
        // n1 = atoi(arg1);
        // n2 = atoi(arg2);
        // sscanf(buf, )
        sscanf(buf, "first=%d", &n1);
        sscanf(p+1, "second=%d", &n2);
    }

    sprintf(content, "QUERY_STRING=%s", buf);
    sprintf(content, "Welcome to add.com: ");
    sprintf(content, "%sThe Internet addition portal.\r\n<p>", content);
    sprintf(content, "%sThe answer is : %d + %d = %d\r\n<p>", content, n1, n2, n1 + n2);
    sprintf(content, "%sThanks for visiting!\r\n", content);

    printf("Content-length: %d\r\n", (int)strlen(content));
    printf("Content-type: text/html\r\n\r\n");
    printf("%s", content);
    fflush(stdout);
    exit(0);
}
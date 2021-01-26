#include <stdlib.h>

int main() {
    setenv("QUERY_STRING", "asdasd", 0);
    printf("%s", getenv("QUERY_STRING"));
    setenv("QUERY_STRING", "ㅁㄴㅇ", 1);
    printf("%s", getenv("QUERY_STRING"));
    setenv("QUERY_STRING", "ㅁㄴㅇ", 0);
    printf("%s", getenv("QUERY_STRING"));
    setenv("QUERY_STRING", "ㅁㄴㅇ", 0);
    printf("%s", getenv("QUERY_STRING"));
}
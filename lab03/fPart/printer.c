#include <stdio.h>

#define GREEN "\e[1;92m"
#define YELLOW "\e[1;93m"
#define CYAN "\e[1;96m"
#define RESET "\033[0m"

void printError(char *errMsg)
{
    printf(YELLOW "%s\n" RESET, errMsg);
}

void printOkMessage(char *msg)
{
    printf(GREEN "%s\n" RESET, msg);
}

void printSeparator()
{
    printOkMessage("\n====================================|\n");
}

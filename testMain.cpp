#include "qqbot.h"
#include <windows.h> /// We don't want to include it. But here we have to.

int main()
{
    QQClient q;
    int ret=q.login();
    printf("QQClient login returns %d\n",ret);
    while(1)
    {
        QQMessage m=q.getNextMessage();
        printf("QQMessage received.\n");
        Sleep(1000);
    }
    return 0;
}

#include "qqbot.h"
#include <windows.h> /// We don't want to include it. But here we have to.

int main()
{
    QQClient q;
    int ret=q.login();
    printf("QQClient login returns %d\n",ret);
    auto vec=q.getFriendList();
    printf("Get %d friends.\n",(int)vec.size());
    for(auto& x:vec)
    {
        printf("%s %s\n",x.markname.c_str(),x.nickname.c_str());
    }
    return 0;
}

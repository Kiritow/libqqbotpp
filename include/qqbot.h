#include <string>

class QQClient
{
public:
    QQClient();
    ~QQClient();

    int login();
private:
    class _impl;
    _impl* _p;
};

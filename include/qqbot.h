#include <string>

class QQClient
{
public:
    QQClient();
    ~QQClient();

    int loginWithQRCode();
    int loginWithCookie();

    /// Will try Cookie first, then QRCode.
    int login();
private:
    class _impl;
    _impl* _p;
};

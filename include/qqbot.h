#include <string>
#include <vector>

class QQFriend
{
public:
    long long uin;
    std::string markname;
    std::string nickname;
    bool vip;
    int viplevel;

    int categories;
};

class QQMessage
{
public:
    std::string content;
    int sender_uin;
    int receiver_qq;
    int timestamp;

    std::string poll_type;

    /// Other content.
    int msg_type;

    QQMessage();
    ~QQMessage();
    bool isReady() const;

    friend class QQClient;
private:
    class _impl;
    _impl* _p;
};

class QQClient
{
public:
    QQClient();
    ~QQClient();

    int loginWithQRCode();
    int loginWithCookie();

    /// Will try Cookie first, then QRCode.
    int login();

    QQMessage getNextMessage();
    std::vector<QQFriend> getFriendList();
private:
    class _impl;
    _impl* _p;
};

#include "qqbot.h"
#include "NetworkWrapper.h"
#include <cpplib/cpplib#gcolor>
#include "windows.h"
using namespace std;

class QQClient::_impl
{
public:
    int GetQRCode();
    int OpenQRCode();

    string qrsig;
    string qrcode_filename;
};

#define USERAGENT "Mozilla/5.0 (Windows NT 6.3; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/61.0.3163.79 Safari/537.36"

QQClient::QQClient()
{
    _p=new _impl;
}

QQClient::~QQClient()
{
    if(_p)
    {
        delete _p;
        _p=nullptr;
    }
}

#define ShowError(fmt,...) cprint(Color::red,Color::black);printf(fmt,##__VA_ARGS__);cprint()
#define ShowMsg(fmt,...) cprint(Color::yellow,Color::black);printf(fmt,##__VA_ARGS__);cprint()

int QQClient::_impl::GetQRCode()
{
    HTTPConnection t;
    t.setUserAgent(USERAGENT);
    t.setURL("https://ssl.ptlogin2.qq.com/ptqrshow?appid=501004106&e=0&l=M&s=5&d=72&v=4&t=0.1");
    t.setDataOutputFile(qrcode_filename);
    t.setCookieOutputFile("tmp/cookie1.txt");
    t.perform();
    if(t.getResponseCode()!=200)
    {
        ShowError("Failed to download QRCode. HTTP Response Code: %d\n",t.getResponseCode());
        return -1;
    }

    vector<Cookie> vec=t.getCookies();
    for(auto& c:vec)
    {
        if(c.name=="qrsig")
        {
            qrsig=c.value;
            break;
        }
    }

    OpenQRCode();

    ShowMsg("QRCode is downloaded. Please scan it with your smart phone.\n");

    return 0;
}

int QQClient::_impl::OpenQRCode()
{
    int ret=(int)ShellExecute(NULL,"open",qrcode_filename.c_str(),NULL,NULL,SW_SHOWMAXIMIZED);
    if(ret<32)
    {
        ret=GetLastError();
        ShowError("Failed to open qrcode. GetLastError:%d\n",ret);
        return -1;
    }
    return 0;
}

int QQClient::login()
{
    /// Notice that we must write '\\' instead of '/' as we are running on Windows.
    _p->qrcode_filename="tmp\\qrcode.png";
    if(_p->GetQRCode()<0)
    {
        printf("Failed to get QRCode.\n");
        return -1;
    }

    return 0;
}

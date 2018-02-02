#include "qqbot.h"
#include "NetworkWrapper.h"
#include <cpplib/cpplib#gcolor>
#include "util.h"
#include "StringLoader.h"
#include <windows.h>
using namespace std;

class QQClient::_impl
{
public:
    int GetQRCode();
    int OpenQRCode();
    int GetQRScanStatus();

    string qrsig;
    string qrcode_filename;
    StringLoader mp;
};

#define USERAGENT "Mozilla/5.0 (Windows NT 6.3; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/61.0.3163.79 Safari/537.36"
#define URL_1 "https://ssl.ptlogin2.qq.com/ptqrshow?appid=501004106&e=0&l=M&s=5&d=72&v=4&t=0.1"
#define URL_2R "https://ssl.ptlogin2.qq.com/ptqrlogin?" \
                    "ptqrtoken={1}&webqq_type=10&remember_uin=1&login2qq=1&aid=501004106&" \
                    "u1=https%3A%2F%2Fw.qq.com%2Fproxy.html%3Flogin2qq%3D1%26webqq_type%3D10&" \
                    "ptredirect=0&ptlang=2052&daid=164&from_ui=1&pttype=1&dumy=&fp=loginerroralert&0-0-157510&" \
                    "mibao_css=m_webqq&t=undefined&g=1&js_type=0&js_ver=10184&login_sig=&pt_randsalt=3"
#define REF_2 "https://ui.ptlogin2.qq.com/cgi-bin/login?daid=164&target=self&style=16&mibao_css=m_webqq&appid=501004106&enable_qlogin=0&no_verifyimg=1 &s_url=http%3A%2F%2Fw.qq.com%2Fproxy.html&f_url=loginerroralert &strong_login=1&login_state=10&t=20131024001"


QQClient::QQClient()
{
    _p=new _impl;
    if(_p)
    {
        _p->mp=StringLoader("strings_utf8.txt");
    }
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

void ReplaceTag(std::string& src,const std::string& tag,const std::string& replaceto)
{
    size_t x;
    while((x=src.find(tag))!=string::npos)
    {
        src.replace(x,tag.size(),replaceto);
    }
}

void _DoStrParse(std::string& out, int current_cnt, const std::string& current_str)
{
    char buf[16];
    sprintf(buf,"{%d}",current_cnt);
    string tag(buf);

    ReplaceTag(out,tag,current_str);
}

template<typename... Args>
void _DoStrParse(std::string& out,int current_cnt,const std::string& current_str,Args&&... args)
{
    _DoStrParse(out,current_cnt,current_str);
    current_cnt++;

    _DoStrParse(out,current_cnt,args...);
}

template<typename... Args>
std::string StrParse(const std::string& src,Args&&... args)
{
    int cnt=1;
    std::string s(src);
    _DoStrParse(s,cnt,args...);
    return s;
}

int QQClient::_impl::GetQRCode()
{
    HTTPConnection t;
    t.setUserAgent(USERAGENT);
    t.setURL(URL_1);
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

/// Hash function for generating ptqrtoken
string hash33(const std::string& s)
{
    int e = 0;
    int sz=s.size();
    for (int i = 0; i<sz; ++i)
    {
        e += (e << 5) + (int)s[i];
    }
    int result=2147483647 & e;
    char buff[16];
    sprintf(buff,"%d",result);
    return string(buff);
}

int QQClient::_impl::GetQRScanStatus()
{
    char buff[1024];
    memset(buff,0,1024);

    HTTPConnection t;
    t.setUserAgent(USERAGENT);
    t.setURL(StrParse(URL_2R,hash33(qrsig)));
    t.setReferer(REF_2);
    t.setCookieInputFile("tmp/cookie1.txt");
    t.setCookieOutputFile("tmp/cookie2.txt");
    t.setDataOutputBuffer(buff,1024);
    t.perform();

    if(t.getResponseCode()!=200)
    {
        ShowError("Failed to get QRScan status. Response Code: %d\n",t.getResponseCode());
        return -2;
    }

    string response=UTF8ToGBK(buff);

    ShowMsg("Buffer content:\n%s\n",response.c_str());

    if(strstr(buff,mp["qrcode_scanning"].c_str()))
    {
        return 0;
    }
    else if(strstr(buff,mp["qrcode_valid"].c_str()))
    {
        return 0;
    }
    else if(strstr(buff,mp["qrcode_invalid"].c_str()))
    {
        return -1;
    }
    else if(strstr(buff,"http")) /// Now the API returns https instead of http.
    {
        return 1;
    }
    else
    {
        return -3;
    }
}

int QQClient::login()
{
    ShowMsg("[Starting] QRCode\n");
    /// Notice that we must write '\\' instead of '/' as we are running on Windows.
    _p->qrcode_filename="tmp\\qrcode.png";
    if(_p->GetQRCode()<0)
    {
        ShowError("Failed to get qrcode.\n");
        return -1;
    }
    ShowMsg("[OK] QRCode.\n");

    ShowMsg("[Starting] ScanStatus\n");
    int status=0;
    while((status=_p->GetQRScanStatus())==0)
    {
        ShowMsg("Waiting for next check.\n");
        Sleep(2000);
    }
    if(status<0)
    {
        ShowError("Failed to get scan status. (or qrcode is invalid.)\n");
        return -2;
    }
    ShowMsg("[OK] ScanStatus.\n");
    return 0;
}

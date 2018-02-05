#include "qqbot.h"
#include "NetworkWrapper.h"
#include <algorithm>
#include <functional>
#include "util.h"
#include "StringLoader.h"
#include <windows.h>
#include <thread>
#include <mutex>
#include <vector>
#include <condition_variable>
#include "json.hpp"
/// Delete if compile error here.
#include <cpplib/cpplib#gcolor>
using namespace std;

class QQMessage::_impl
{
public:
    int status;
};

QQMessage::QQMessage() : _p(new _impl)
{
    if(_p)
    {
        _p->status=-1;
    }
}

QQMessage::~QQMessage()
{
    if(_p)
    {
        delete _p;
        _p=nullptr;
    }
}

bool QQMessage::isReady() const
{
    return _p&&_p->status==1;
}

class QQClient::_impl
{
public:
    int GetQRCode();
    int OpenQRCode();
    int CloseQRCode();
    int GetQRScanStatus();
    int GetPtWebQQ();
    int GetVfWebQQ();
    int GetPSessionID_UIN();

    QQMessage GetNextMessage();

    string qrsig;
    string qrcode_filename;
    /// https://ptlogin2.web2.qq.com/check_sig?pttype=1&uin=...
    string login_url;
    string ptwebqq;
    string vfwebqq;
    string psessionid;
    int uin;

    StringLoader mp;

    thread td;
    vector<QQMessage> mvec;
    mutex mvec_lock;
};

#define USERAGENT "Mozilla/5.0 (Windows NT 6.3; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/61.0.3163.79 Safari/537.36"
#define URL_1 "https://ssl.ptlogin2.qq.com/ptqrshow?appid=501004106&e=0&l=M&s=5&d=72&v=4&t=0.1"
#define URL_2R "https://ssl.ptlogin2.qq.com/ptqrlogin?" \
                    "ptqrtoken={1}&webqq_type=10&remember_uin=1&login2qq=1&aid=501004106&" \
                    "u1=https%3A%2F%2Fw.qq.com%2Fproxy.html%3Flogin2qq%3D1%26webqq_type%3D10&" \
                    "ptredirect=0&ptlang=2052&daid=164&from_ui=1&pttype=1&dumy=&fp=loginerroralert&0-0-157510&" \
                    "mibao_css=m_webqq&t=undefined&g=1&js_type=0&js_ver=10184&login_sig=&pt_randsalt=3"
#define REF_2 "https://ui.ptlogin2.qq.com/cgi-bin/login?daid=164&target=self&style=16&mibao_css=m_webqq&appid=501004106&enable_qlogin=0&no_verifyimg=1 &s_url=http%3A%2F%2Fw.qq.com%2Fproxy.html&f_url=loginerroralert &strong_login=1&login_state=10&t=20131024001"
#define REF_3 "http://s.web2.qq.com/proxy.html?v=20130916001&callback=1&id=1"
#define URL_4R "http://s.web2.qq.com/api/getvfwebqq?ptwebqq={1}&clientid=53999199&psessionid=&t=0.1"
#define REF_4 "http://s.web2.qq.com/proxy.html?v=20130916001&callback=1&id=1"
#define URL_5 "http://d1.web2.qq.com/channel/login2"
#define REF_5 "http://d1.web2.qq.com/proxy.html?v=20151105001&callback=1&id=2"
#define URL_POLL "https://d1.web2.qq.com/channel/poll2"
#define REF_POLL "https://d1.web2.qq.com/proxy.html?v=20151105001&callback=1&id=2"

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

#ifdef cpplib_gcolor
#define ShowError(fmt,...) cprint(Color::red,Color::black);printf(fmt,##__VA_ARGS__);cprint()
#define ShowMsg(fmt,...) cprint(Color::yellow,Color::black);printf(fmt,##__VA_ARGS__);cprint()
#else
#define ShowError(fmt,...) printf("[Message] ");printf(fmt,##__VA_ARGS__)
#define ShowMsg(fmt,...) printf("[Error] ");printf(fmt,##__VA_ARGS__)
#endif

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

static int CALLBACK _global_enumwindows_delegator(HWND hwnd,LPARAM p)
{
    auto pfn=(function<int(HWND)>*)p;
    return (*pfn)(hwnd);
}

int QQClient::_impl::CloseQRCode()
{
    int target_pid=-1;
    function<int(HWND)> fn=[&](HWND hwnd)
    {
        char buff[1024];
        memset(buff,0,1024);
        GetWindowText(hwnd,buff,1024);
        if(strstr(buff,"qrcode.png"))
        {
            DWORD pid;
            GetWindowThreadProcessId(hwnd,&pid);
            target_pid=pid;
            /// stop enum
            return 0;
        }
        else
        {
            return 1;
        }
    };
    EnumWindows(_global_enumwindows_delegator,(LPARAM)&fn);
    if(target_pid<0)
    {
        ShowError("Failed to close qrcode window.\n");
        /// but this is not a critical error.
    }
    else
    {
        HANDLE hand=OpenProcess(PROCESS_TERMINATE,FALSE,target_pid);
        if(hand==NULL)
        {
            ShowError("Failed to open process.\n");
        }
        else
        {
            if(!TerminateProcess(hand,0))
            {
                ShowError("Failed to terminate process with process id: %d\n",target_pid);
            }
            else
            {
                ShowMsg("Process terminated. QRCode window might be closed.\n");
            }
        }
    }
    return 0;
}

/// Hash function for generating ptqrtoken
static string hash33(const std::string& s)
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
        CloseQRCode();

        char xbuf[1024];
        memset(xbuf,0,1024);
        char* p=strstr(buff,"http");
        char* q=strstr(p,"'");
        strncpy(xbuf,p,q-p);
        login_url=xbuf;

        return 1;
    }
    else
    {
        return -3;
    }
}

int QQClient::_impl::GetPtWebQQ()
{
    HTTPConnection t;
    t.setVerbos(true);
    t.setUserAgent(USERAGENT);
    t.setURL(login_url);
    t.setReferer(REF_3);
    t.setCookieInputFile("tmp/cookie2.txt");
    t.setCookieOutputFile("tmp/cookie3.txt");
    t.perform();

    if(t.getResponseCode()!=302)
    {
        ShowError("Failed to get ptwebqq. Response code: %d\n",t.getResponseCode());
    }

    vector<Cookie> vec=t.getCookies();
    vector<Cookie>::iterator iter=find_if(vec.begin(),vec.end(),[](const Cookie& c){return c.name=="ptwebqq";});
    if(iter!=vec.end())
    {
        ptwebqq=iter->value;
    }
    else
    {
        ShowError("Failed to find ptwebqq in cookie.\n");
        return -1;
    }

    return 0;
}

int QQClient::_impl::GetVfWebQQ()
{
    char buff[1024];
    memset(buff,0,1024);

    HTTPConnection t;
    t.setUserAgent(USERAGENT);
    t.setURL(StrParse(URL_4R,ptwebqq));
    t.setReferer(REF_4);
    t.setCookieInputFile("tmp/cookie3.txt");
    t.setCookieOutputFile("tmp/cookie4.txt");
    t.setDataOutputBuffer(buff,1024);
    t.perform();

    if(t.getResponseCode()!=200)
    {
        ShowError("Failed to get vfwebqq. Response Code: %d\n",t.getResponseCode());
        return -1;
    }

    printf("Buffer Received: %s\n",buff);

    nlohmann::json j=nlohmann::json::parse(buff);
    try
    {
        vfwebqq=j["result"]["vfwebqq"];
    }
    catch(...)
    {
        return -2;
    }

    return 0;
}

int QQClient::_impl::GetPSessionID_UIN()
{
    char buff[1024];
    memset(buff,0,1024);

    HTTPConnection t;
    t.setVerbos(true);
    t.setUserAgent(USERAGENT);
    t.setMethod(HTTPConnection::Method::Post);
    t.setURL(URL_5);
    t.setReferer(REF_5);
    t.setDataOutputBuffer(buff,1024);
    t.setCookieInputFile("tmp/cookie4.txt");
    t.setCookieOutputFile("tmp/cookie5.txt");
    nlohmann::json j;
    j["ptwebqq"]=ptwebqq;
    j["clientid"]=53999199;
    j["psessionid"]="";
    j["status"]="online";
    string jstr="r="+j.dump();
    t.setPostData(jstr);
    t.perform();

    if(t.getResponseCode()!=200)
    {
        ShowError("Failed to get psessionid. Response Code: %d\n",t.getResponseCode());
        return -1;
    }

    ShowMsg("Response buff: %s\n",buff);

    j=nlohmann::json::parse(buff);
    try
    {
        psessionid=j["result"]["psessionid"];
        uin=j["result"]["uin"];
    }
    catch(...)
    {
        return -2;
    }

    return 0;
}

QQMessage QQClient::_impl::GetNextMessage()
{
    char buff[4096];
    memset(buff,0,4096);

    HTTPConnection t;
    t.setUserAgent(USERAGENT);
    t.setURL(URL_POLL);
    t.setReferer(REF_POLL);

    nlohmann::json j;
    j["ptwebqq"]=ptwebqq;
    j["clientid"]=53999199;
    j["psessionid"]=psessionid;
    j["key"]="";

    string jstr="r="+j.dump();

    t.setPostData(jstr);
    t.setCookieInputFile("tmp/cookie5.txt");
    t.setDataOutputBuffer(buff,4096);

    t.perform();

    ShowMsg("Message raw buff:\n%s\n",buff);

    nlohmann::json x=nlohmann::json::parse(buff);

    try
    {
        auto& f=x["result"];
        int sz=f.size();
        for(int i=0;i<sz;i++)
        {
            auto& msg=f[i];
            QQMessage m;
            m.poll_type=msg["poll_type"];

            auto& msgobj=msg["value"];

            m.timestamp=msgobj["time"];
            m.sender_uin=msgobj["from_uin"];
            m.receiver_qq=msgobj["to_uin"];
            mvec.push_back(m);
        }

        /// Sort of stupid method.
        QQMessage xm=mvec.at(0);
        mvec.erase(mvec.begin());
        return xm;
    }
    catch(exception& e)
    {
        ShowError("Exception caught: %s\n",e.what());
        QQMessage m;
        return m;
    }
}

int QQClient::loginWithQRCode()
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

    ShowMsg("[Starting] ptwebqq\n");
    if(_p->GetPtWebQQ()<0)
    {
        ShowError("Failed to get ptwebqq.\n");
        return -3;
    }
    ShowMsg("[OK] ptwebqq\n");

    ShowMsg("[Starting] vfwebqq\n");
    if(_p->GetVfWebQQ()<0)
    {
        ShowError("Failed to get vfwebqq.\n");
        return -4;
    }
    ShowMsg("[OK] vfwebqq\n");

    ShowMsg("[Starting] psessionid,uin\n");
    if(_p->GetPSessionID_UIN()<0)
    {
        ShowError("Failed to get psessionid,uin.\n");
        return -5;
    }
    ShowMsg("[OK] psessionid,uin\n");

    ShowMsg("[OK] Successfully logged in.\n");

    ShowMsg("[Starting] message receiver\n");

    ShowMsg("[OK] message receiver.\n");
    return 0;
}

int QQClient::login()
{
    /// loginWithCookie is not implemented yet.
    return loginWithQRCode();
}

QQMessage QQClient::getNextMessage()
{
    return _p->GetNextMessage();
}

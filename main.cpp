/** @author ligang
  * @date 2013-09-13
**/
#include "stdio.h"
#include "cmdline.h"
#include "tinyxml2.h"
using namespace std;
using namespace tinyxml2;

cmdline::parser g_parser;
string g_mode;
string g_protocolName;
string g_fileInput;
string g_fileOutput;
XMLDocument* g_DocOutput;
bool g_setid;

unsigned int g_maxUserID;

void RegisterCmd();
void CheckArgs(int argc,char **argv);
void Add();
void Remove();
void SetID();
void Save();

string GetIEBaseFileName(string protocolName);
unsigned int GetMaxUserID(XMLDocument* pDoc);
unsigned int Max(unsigned int a, unsigned int b);
unsigned int GetUnsignedIntUserID(string strUserid);
string ToUpper(const string str);
string GetIEBaseFileName(string protocolName);

int main(int argc,char **argv)
{
    RegisterCmd();
    CheckArgs(argc, argv);

    if (g_mode == "add")
    {
        Add();
    }
    else if (g_mode == "remove")
    {
        Remove();
    }

    if(g_setid)
    {
        SetID();
    }

    Save();

    return 0;
}

void RegisterCmd()
{
    g_parser.add<string>("mode", 'm', "add, remove or none", true, "", cmdline::oneof<string>("add", "remove", "none"));
    g_parser.add<string>("protocol", 'p', "protocol name", false, "none");
    g_parser.add<string>("read", 'r', "file to read,  default: Order_IEDecode.xml ", false, "Order_IEDecode.xml");
    g_parser.add<string>("write", 'w', "file to write, default: Order_IEDecode.xml ", false, "Order_IEDecode.xml");
    g_parser.add("setid", 's', "set userid");
}
void CheckArgs(int argc,char **argv)
{
    g_parser.parse_check(argc, argv);
    g_mode = g_parser.get<string>("mode");
    g_protocolName = ToUpper( g_parser.get<string>("protocol") );
    g_fileInput = g_parser.get<string>("read");
    g_fileOutput = g_parser.get<string>("write");
    g_setid = g_parser.exist("setid");
}


void Add()
{
    string iebaseFileName = GetIEBaseFileName(g_protocolName);
    XMLDocument* pDoc = new XMLDocument();

    if (pDoc->LoadFile(iebaseFileName.c_str()) != XML_SUCCESS)
    {
        printf("ERROR: can't open %s\n", iebaseFileName.c_str());
        exit(0);
    }
    XMLElement* pEle = pDoc->FirstChildElement("Protocol");
    if(pEle == NULL)
    {
        printf("ERROR: can't find <Protocol> in %s\n", iebaseFileName.c_str());
        exit(0);
    }

    XMLDocument* pDocOrder = new XMLDocument();
    if(pDocOrder->LoadFile(g_fileInput.c_str()) != XML_SUCCESS)
    {
        printf("ERROR: can't open %s\n", g_fileInput.c_str());
        exit(0);
    }

    g_maxUserID = GetMaxUserID(pDocOrder);

    XMLElement* pNewEle = pDocOrder->NewElement( "Protocol" );
    pDocOrder->FirstChildElement("IEDecode")->LinkEndChild(pNewEle);
    pNewEle->LinkEndChild(pEle);

    printf("%s has been add\n", g_protocolName.c_str());

    pNewEle->SetAttribute("name", pEle->Attribute("name"));
    pNewEle->SetAttribute("id", pEle->Attribute("id"));

    pEle->SetValue("OrderMsg");
    pEle->DeleteAttribute("xmlns");
    pEle->DeleteAttribute("id");
    pEle->DeleteAttribute("name");
    pEle->DeleteAttribute("show");
    pEle->DeleteAttribute("sequence");

    XMLElement* pMsg = pEle->FirstChildElement("Message");
    while(pMsg)
    {
        XMLElement* pIE = pMsg->FirstChildElement("IE");
        while(pIE)
        {
            string ieName = pIE->Attribute("name");
            string ieID = pIE->Attribute("id");
            pIE->DeleteAttribute("sequence");
            pIE->DeleteAttribute("show");
            pIE->DeleteAttribute("name");
            pIE->DeleteAttribute("id");
            pIE->DeleteAttribute("tagid");
            char ch[8];
            sprintf(ch, "0x%04x", ++g_maxUserID);
            pIE->SetAttribute("userid", ch);
            pIE->SetAttribute("withtime", "0");
            pIE->SetAttribute("name", ieName.c_str());
            pIE->SetAttribute("id", ieID.c_str());
            pIE = pIE->NextSiblingElement("IE");
        }
        string msgName = pMsg->Attribute("name");
        string msgID = pMsg->Attribute("id");
        pMsg->DeleteAttribute("sequence");
        pMsg->DeleteAttribute("show");
        pMsg->DeleteAttribute("name");
        pMsg->DeleteAttribute("id");
        pMsg->DeleteAttribute("tagid");
        pMsg->SetAttribute("name", msgName.c_str());
        pMsg->SetAttribute("id", msgID.c_str());
        pMsg = pMsg->NextSiblingElement("Message");
    }

    g_DocOutput = pDocOrder;
}

void Remove()
{
    XMLDocument* pDocOrder = new XMLDocument();
    if (pDocOrder->LoadFile(g_fileInput.c_str()) != XML_SUCCESS)
    {
        printf("ERROR: can't open file (%s)\n", g_fileInput.c_str());
    }

    XMLElement* pRoot = pDocOrder->FirstChildElement("IEDecode");
    XMLElement* pPro = pRoot->FirstChildElement("Protocol");;
    if(pPro == NULL)
    {
        printf("ERROR: can't find <Protocol> in %s\n", g_fileInput.c_str());
        exit(0);
    }
    while(pPro)
    {
        if (pPro->Attribute("name") == g_protocolName)
        {
            pRoot->DeleteChild(pPro);
            printf("%s has been removed\n", g_protocolName.c_str());
        }
        pPro = pPro->NextSiblingElement("Protocol");
    }

    g_DocOutput = pDocOrder;
}

void SetID()
{
    XMLElement* pRoot = g_DocOutput->FirstChildElement("IEDecode");
    XMLElement* pPro = pRoot->FirstChildElement("Protocol");
    if(pPro == NULL)
    {
        printf("ERROR: can't find <Protocol> in %s\n", g_fileInput.c_str());
        exit(0);
    }
    while(pPro)
    {
        XMLElement* pMsg = pPro->FirstChildElement("OrderMsg")->FirstChildElement("Message");
        while(pMsg)
        {
            XMLElement* pIE = pMsg->FirstChildElement("IE");
            while(pIE)
            {
                char ch[8];
                sprintf(ch, "0x%04x", g_maxUserID++);
                pIE->SetAttribute("userid", ch);
                pIE = pIE->NextSiblingElement("IE");
            }
            pMsg = pMsg->NextSiblingElement("Message");
        }
        pPro = pPro->NextSiblingElement("Protocol");
    }

    printf("userid has been reset\n");
}



void StringReplace(string &str, const string strSrc, const string strDst)
{
    int pos = str.find(strSrc);
    while(pos != -1)
    {
        str.replace(pos, strSrc.size(), strDst);
        pos = str.find(strSrc, pos);
    }
}

string ToUpper(const string str)
{
    string strCopy = str;
    transform(strCopy.begin(), strCopy.end(), strCopy.begin(), ::toupper);
    return strCopy;
}

string GetIEBaseFileName(string protocolName)
{
    StringReplace(protocolName, " ", "_");
    return "IEBase_xml/" + protocolName +"_IEBase.xml";
}

unsigned int GetMaxUserID(XMLDocument* pDoc)
{
    unsigned int max = 0;
    XMLElement* pRoot = pDoc->FirstChildElement("IEDecode");
    XMLElement* pPro = pRoot->FirstChildElement("Protocol");

    while(pPro)
    {
        XMLElement* pMsg = pPro->FirstChildElement("OrderMsg")->FirstChildElement("Message");
        while(pMsg)
        {
            XMLElement* pIE = pMsg->FirstChildElement("IE");
            while(pIE)
            {
               string id = pIE->Attribute("userid");
               max = Max(max, GetUnsignedIntUserID(id));
               pIE = pIE->NextSiblingElement("IE");
            }
            pMsg = pMsg->NextSiblingElement("Message");
        }
        pPro = pPro->NextSiblingElement("Protocol");
    }

    return max;
}

unsigned int GetUnsignedIntUserID(string strUserid)
{
    unsigned int id = 0;
    sscanf(strUserid.c_str(), "0x%04x", &id);
    return id;
}

unsigned int Max(unsigned int a, unsigned int b)
{
    if(a > b)
        return a;
    else
        return b;
}

void Save()
{
    g_DocOutput->SaveFile(g_fileOutput.c_str());
}

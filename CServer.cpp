#include "CServer.h"

CServer::CServer()
{
    m_server = NULL;
    m_business = NULL;
}

int CServer::Init(CBusiness* business, const Buffer& ip, short port)
{
    if (business == NULL)return -1;
    int ret = 0;
    m_business = business;
    ret = m_process.SetEntryFunction(&CBusiness::BusinessProcass,m_business,&m_process);
    if (ret != 0)return -2;
    ret = m_process.CreateSubProcess();
    if (ret != 0)return -3;
    ret = m_pool.Start(2);
    if (ret != 0)return -4;
    ret = m_epoll.Create(2);
    if (ret != 0)return -5;
    m_server = new CSocket();
    if (m_server == NULL)return -6;
    ret = m_server->Init(CSocketParam(ip, port, SOCK_ISSERVER | SOCK_ISIP | SOCK_ISREUSE));
    if (ret != 0)return -7;
    ret = m_epoll.Add(m_server->getInt(), EpollData((void*)m_server));
    if (ret != 0)return -8;
    for (size_t i = 0; i < m_pool.Size(); i++)
    {
        ret = m_pool.AddTask(&CServer::ThreadFunc, this); 
        if (ret != 0)return -9;
    }
    return 0;
}

int CServer::Run()
{
    while (m_server != NULL)
    {
        usleep(10);
    }
    return 0;
}

int CServer::Close()
{
    if (m_server)
    {
        CSocketBase* sock = m_server;
        m_server = NULL;
        m_epoll.Del(sock->getInt());
        delete sock;
    }
    m_epoll.Close();
    m_process.SendFD(-1);
    m_pool.Close();
    return 0;
}

int CServer::ThreadFunc()
{
    TRACEI("epoll =%d  server=%p", (int)m_epoll, m_server);
    int ret = 0;
    EPEvents events;
    while (m_epoll != -1 && m_server != NULL)
    {
        ssize_t size = m_epoll.WaitEvents(events);
        if (size < 0)break;
        if (size > 0)
        {
            TRACEI("size=%d event=%08X", size, events[0].events);
            for (ssize_t i = 0; i < size; i++)
            {
                if (events[i].events & EPOLLERR)
                {
                    if (events[i].data.ptr != m_server)
                    {
                        break;
                    }
                }
                else if (events[i].data.ptr == m_server && events[i].events & EPOLLIN)
                {
                    if (m_server)
                    {
                        CSocketBase* pCLient = NULL;
                        ret = m_server->Link(&pCLient);
                        if (ret != 0)continue;
                        ret = m_process.SendSocket(pCLient->getInt(),*pCLient);
                        TRACEI("SendSocktet=%d", ret);
                        int s = pCLient->getInt();
                        delete pCLient;
                        if (ret != 0)
                        {
                            TRACEE("send client %d failed!", s);
                            continue;
                        }
                    }
                }
            }
        }
    }
    TRACEI("·þÎñÆ÷ÖÕÖ¹£¡");
    return 0;
}

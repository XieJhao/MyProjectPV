#include "Epoll.h"

int CEpoll::Create(unsigned count)
{
    if (m_epoll != -1)
    {
        return -1;
    }
    m_epoll = epoll_create(count);
    if (m_epoll == -1)
    {
        return -2;
    }
    return 0;
}

ssize_t CEpoll::WaitEvents(EPEvents& events, int timeout)
{
    if (m_epoll == -1)
    {
        return -1;
    }
    EPEvents evs(EVENT_SIZE);
    int ret = epoll_wait(m_epoll,evs.data(),(int)evs.size(),timeout);
    if (ret == -1)
    {
        if (errno == EINTR || errno == EAGAIN)
        {
            return 0;
        }
        return -2;
    }
    if (ret > (int)events.size())
    {
        events.resize(ret);
    }
    memcpy(events.data(),evs.data(),sizeof(epoll_event)*ret);
    return ret;
}

int CEpoll::Add(int fd, const EpollData& data, uint32_t events)
{
    if (m_epoll == -1)
    {
        return -1;
    }
    epoll_event ev;
    ev.events = events;
    ev.data = data;
    int ret = epoll_ctl(m_epoll, EPOLL_CTL_ADD, fd, &ev);
    if (ret == -1)
    {
        return -2;
    }
    return 0;
}

int CEpoll::Modify(int fd, uint32_t events, const EpollData& data)
{
    if (m_epoll == -1)
    {
        return -1;
    }
    epoll_event ev = { events,data };
    int ret = epoll_ctl(m_epoll, EPOLL_CTL_MOD, fd, &ev);
    if (ret == -1)
    {
        return -2;
    }
    return 0;
}

int CEpoll::Del(int fd)
{
    if (m_epoll == -1)
    {
        return -1;
    }
    int ret = epoll_ctl(m_epoll, EPOLL_CTL_DEL, fd, NULL);
    if (ret == -1)
    {
        return -2;
    }
    return 0;
}

void CEpoll::Close()
{
    if (m_epoll != -1)
    {
        int fd = m_epoll;
        m_epoll = -1;
        close(fd);
    }
}

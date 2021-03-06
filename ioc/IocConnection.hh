#ifndef Pds_IocConnection_hh
#define Pds_IocConnection_hh
#include <unistd.h>
#include<string>
#include<list>
#include<vector>

namespace Pds {
  class IocControl;

  class IocConnection {
  public:
    IocConnection(std::string host, uint32_t host_ip, uint16_t port, IocControl *cntl);
    ~IocConnection() { close(_sock); _sock = -1; };
    static IocConnection *get_connection(std::string host, uint32_t host_ip,
                                         uint16_t port, IocControl *cntl);
    void configure(unsigned run, unsigned stream);
    void transmit(const char *s);
    void transmit(std::string s);
    static void transmit_all(std::string s);
    static void clear_all(void);
    static std::list<IocConnection*> _connections;
    static int check_all(void);
    std::string host() { return _host; };
    int getIndex() { _damage.push_back(0); return _idx++; };
    int damage_status(int idx);
  private:
    int _sock;
    std::string _host;
    uint32_t _host_ip;
    uint16_t _port;
    IocControl *_cntl;
    int _idx;
    int _damage_req;
    int _conn_req;
    int _init_conn_req;
    int _wait_conn_req;
    int _num_conn_up;
    unsigned _run;
    unsigned _stream;
    std::vector<int> _damage;
  };
};

#endif

#ifndef PDSCOLLECTIONSERVER_HH
#define PDSCOLLECTIONSERVER_HH

#include "pds/service/NetServer.hh"

namespace Pds {

class CollectionServerManager;

class CollectionServer: public NetServer {
public:
  CollectionServer(CollectionServerManager& manager,
		   int id,
		   int interface,
		   unsigned maxpayload,
		   unsigned datagrams);

  CollectionServer(CollectionServerManager& manager,
		   int id,
		   const Ins& bcast,
		   unsigned maxpayload,
		   unsigned datagrams);

  ~CollectionServer();

private:
  // Implements Server
  virtual int commit(char* dg, char* payload, int size, const Ins& src); 
  virtual int handleError(int value);
  virtual char* payload() {return _payload;}

private:
  CollectionServerManager& _manager;
  char* _payload;
};

}
#endif

#include "connection/caravan.h"
#include <errno.h>

caravan::ClusterManager::ClusterManager() {
}


caravan::Machine::Machine(MachineID id, unsigned int port) {
  port_ = port;
  socket_fd_set_ = new fd_set();
  FD_ZERO(socket_fd_set_);
  machine_id_ = id;
  isSocketInitialized_ = false;
}

caravan::Machine::~Machine() {
  // Free sockets
  for ( std::map< MachineID, Socket * >::const_iterator iter = sockets_out_.begin(); iter != sockets_out_.end(); ++iter ) {
    if (iter->second->Connected) {
      // Make closing the socket do a hard disconnect FOR NOW
      // TODO: Make disconnects graceful
      linger lingerOption;
      lingerOption.l_onoff = 1;
      lingerOption.l_linger = 0;
      if (setsockopt(iter->second->Handle, SOL_SOCKET,  SO_LINGER, (char *)&lingerOption, sizeof(linger)) < 0) {
        perror("setsockopt() failed linger on iterator");
      }
    }
    close(iter->second->Handle);
    free(iter->second);

  }

  for ( std::map< MachineID, Socket * >::const_iterator iter = sockets_in_.begin(); iter != sockets_in_.end(); ++iter ) {
    if (iter->second->Connected) {
      // Make closing the socket do a hard disconnect FOR NOW
      // TODO: Make disconnects graceful
      linger lingerOption;
      lingerOption.l_onoff = 1;
      lingerOption.l_linger = 0;
      if (setsockopt(iter->second->Handle, SOL_SOCKET,  SO_LINGER, (char *)&lingerOption, sizeof(linger)) < 0) {
        perror("setsockopt() failed linger on iterator");
      }
    }
    close(iter->second->Handle);
    free(iter->second);
  }

  if (isSocketInitialized_) {
    linger lingerOption;
    lingerOption.l_onoff = 1;
    lingerOption.l_linger = 0;
    // Make closing the socket do a hard disconnect FOR NOW
    // TODO: Make disconnects graceful
    if (fcntl(inSocketHandle_, F_GETFD) == 0) {
      if (setsockopt(inSocketHandle_, SOL_SOCKET,  SO_LINGER, (char *)&lingerOption, sizeof(linger)) < 0) {
        perror("setsockopt() failed destruct");
      }
    }
    close(inSocketHandle_);
  }
  FD_ZERO(socket_fd_set_);
  delete socket_fd_set_;
}

bool caravan::Machine::IsConnectedTo(MachineID otherID) {
  Socket* outSocket = sockets_out_[otherID];
  if (outSocket == NULL) {
    return false;
  }
  return outSocket->Connected;
}

bool caravan::Machine::IsConnectedFrom(MachineID otherID) {
  Socket* inSocket = sockets_in_[otherID];
  if (inSocket == NULL) {
    return false;
  }
  return inSocket->Connected;
}

void caravan::Machine::AddMachine(MachineID otherID, const string& ipAddress, unsigned int port) {
  Socket *socketInfo = new Socket();
  socketInfo->IPAddress = string(ipAddress.c_str());

  socketInfo->RemoteEndPoint.sin_family = AF_INET;
  socketInfo->RemoteEndPoint.sin_port = htons(port);

  int errorCode = inet_pton(AF_INET, ipAddress.c_str(), &socketInfo->RemoteEndPoint.sin_addr);
  if (0 > errorCode)
  {
    perror("error: AF_INET is not a valid address family");
  }
  else if (0 == errorCode)
  {
    perror("A valid IP Address was not passed in");
  }
  sockets_out_[otherID] = socketInfo;

}


void caravan::Machine::SendMessage(MachineID destination, const Slice *message) {
  int outSocket = sockets_out_[destination]->Handle;
  send(outSocket, message->data(), message->size(), 0);
}

void caravan::Machine::InitializeListenerSocket() {
  struct sockaddr_in serverEndPoint;
  inSocketHandle_ = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if(-1 == inSocketHandle_)
  {
    perror("can not create socket");
  }

  memset(&serverEndPoint, 0, sizeof(serverEndPoint));

  serverEndPoint.sin_family = AF_INET;
  serverEndPoint.sin_port = htons(port_);
  serverEndPoint.sin_addr.s_addr = INADDR_ANY;


  if(-1 == bind(inSocketHandle_,(struct sockaddr *)&serverEndPoint, sizeof(serverEndPoint)))
  {
    printf("error bind failed");
    close(inSocketHandle_);
  }

  // Make socket reusable so that if a socket if this address is closed and reopened
  // in the same process, no errors occur
  int on = 1;
  if (setsockopt(inSocketHandle_, SOL_SOCKET,  SO_REUSEADDR, (char *)&on, sizeof(on)) < 0) {
    perror("setsockopt() failed!");
  }

  if(-1 == listen(inSocketHandle_, 10))
  {
    printf("error listen failed");
    close(inSocketHandle_);
  }


  int result = listen(inSocketHandle_, 10);
  if (result != 0) {
    perror("Listen failed");
  }
}

void caravan::Machine::Listen() {
  sockaddr_in clientAddress;
  socklen_t addressLength = sizeof(clientAddress);
  int newSocketHandle = accept(inSocketHandle_, (sockaddr *)&clientAddress, &addressLength);
  int buf[1];
  memset(buf, 0, sizeof(buf));
  if (newSocketHandle <= 0) {
    perror("No new socket accepted");
  } 
  else {
    // Receive machine ID
    int result = recv(newSocketHandle, buf, sizeof(buf), 0);
    if (result <= 0) {
      perror("Handshake failed");
    }
    // Make nonblocking
    int opts = fcntl(newSocketHandle,F_GETFL);
    if (opts < 0) {
	    perror("fcntl(F_GETFL)");
    }
    opts = (opts | O_NONBLOCK);
    if (fcntl(newSocketHandle,F_SETFL,opts) < 0) {
	    perror("fcntl(F_SETFL)");
    }
    FD_SET(newSocketHandle,socket_fd_set_);
    char *newIPAddress = inet_ntoa(clientAddress.sin_addr);
    int otherID = buf[0];
    Socket* newSocket = new Socket();
    newSocket->Handle = newSocketHandle;
    newSocket->IPAddress = newIPAddress;
    newSocket->Connected = true;
    sockets_in_[otherID] = newSocket;
  }
}

caravan::Message *caravan::Machine::ReceiveMessage() {
  char buf[255];
  memset(buf, 0, 255);
  for ( std::map< MachineID, Socket * >::const_iterator iter = sockets_in_.begin(); iter != sockets_in_.end(); ++iter ) {
    if (iter->second->Connected) {
      if (FD_ISSET(iter->second->Handle,socket_fd_set_)) {
        int result = recv(iter->second->Handle, buf, 255, 0);
        if (result > 0) {
          Message *msg = new Message(buf, 255, iter->first);
          return msg;
        }
      }
    }
  }
  return NULL;
}

void caravan::Machine::StartListening() {
  // TODO: Check if we are already listening
  stopListening_ = false;
  pthread_create(&listenerThread_, NULL, &Machine::ListenerLoop, (void *)this);
  // TODO: CORRECTLY block until listener thread is running.
  sleep(1);
}

void caravan::Machine::StopListening() {
  stopListening_ = true;
  // TODO: Make accept call have a timeout and don't DO THIS!!
  pthread_cancel(listenerThread_);
 // pthread_join(listenerThread_, NULL);
}

bool caravan::Machine::Connect(MachineID machineID) {
  Socket *outSocket = sockets_out_[machineID];
  if (outSocket->Connected) {
    perror("Socket is already connected");
    return false;
  }

  outSocket->Handle = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  int on = 1;
  if (setsockopt(outSocket->Handle, SOL_SOCKET,  SO_REUSEADDR, (char *)&on, sizeof(on)) < 0) {
    perror("setsockopt() failed");
  }


  if (-1 == outSocket->Handle)
  {
    perror("cannot create socket");
    return false;
  }

  if (-1 == connect(outSocket->Handle, (struct sockaddr *)&outSocket->RemoteEndPoint, sizeof(outSocket->RemoteEndPoint)))
  {
    perror("connect failed");
    close(outSocket->Handle);
    return false;
  }
  // This is necessary because there is a time delay between when the client thinks it's connect and the server
  // allocates and accepts the socket. This is true even though connect waits for the tcp handshake to complete
  sleep(1);
  int buf[1];
  buf[0] = (int)machine_id_;
  send(outSocket->Handle, buf, sizeof(buf), 0);
  sleep(1);
  outSocket->Connected = true;
  return true;
}



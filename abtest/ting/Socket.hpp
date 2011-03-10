/* The MIT License:

Copyright (c) 2009-2011 Ivan Gagis

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE. */

// ting 0.6.0
// Homepage: http://code.google.com/p/ting

/**
 * @file Socket.hpp
 * @author Ivan Gagis <igagis@gmail.com>
 * @brief Main header file of the socket network library.
 * This is the main header file of socket network library, cross platfrom C++ Sockets wrapper.
 */

#pragma once


#include <string>
#include <sstream>


#if defined(__WIN32__) || defined(WIN32)
#ifndef __WIN32__
#define __WIN32__
#endif
#ifndef WIN32
#define WIN32
#endif

#include <winsock2.h>
#include <windows.h>

#else //assume *nix
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>

#endif



#include "Singleton.hpp"
#include "Exc.hpp"
#include "types.hpp"
#include "WaitSet.hpp"
#include "debug.hpp"
#include "Thread.hpp"
#include "utils.hpp"



/**
 * @brief the main namespace of ting library.
 * All the declarations of ting library are made inside this namespace.
 */
namespace ting{



/**
 * @brief Basic socket class.
 * This is a base class for all socket types such as TCP sockets or UDP sockets.
 */
class Socket : public Waitable{
protected:
#ifdef __WIN32__
	typedef SOCKET T_Socket;

	inline static T_Socket DInvalidSocket(){
		return INVALID_SOCKET;
	}

	inline static int DSocketError(){
		return SOCKET_ERROR;
	}

	inline static int DEIntr(){
		return WSAEINTR;
	}
	
	inline static int DEAgain(){
		return WSAEWOULDBLOCK;
	}
#else //assume *nix
	typedef int T_Socket;

	inline static T_Socket DInvalidSocket(){
		return -1;
	}

	inline static T_Socket DSocketError(){
		return -1;
	}

	inline static int DEIntr(){
		return EINTR;
	}

	inline static int DEAgain(){
		return EAGAIN;
	}
#endif

#ifdef __WIN32__
	WSAEVENT eventForWaitable;
#endif

	T_Socket socket;

	Socket() :
#ifdef __WIN32__
			eventForWaitable(WSA_INVALID_EVENT),
#endif
			socket(DInvalidSocket())
	{
//		TRACE(<< "Socket::Socket(): invoked " << this << std::endl)
	}



	//same as std::auto_ptr
	Socket& operator=(const Socket& s){
//		TRACE(<< "Socket::operator=(): invoked " << this << std::endl)
		if(this == &s)//detect self-assignment
			return *this;

		//first, assign as Waitable, it may throw an exception
		//if the waitable is added to some waitset
		this->Waitable::operator=(s);

		this->Close();
		this->socket = s.socket;

#ifdef __WIN32__
		this->eventForWaitable = s.eventForWaitable;
		const_cast<Socket&>(s).eventForWaitable = WSA_INVALID_EVENT;
#endif

		const_cast<Socket&>(s).socket = DInvalidSocket();
		return *this;
	}



public:
	/**
	 * @brief Basic exception class.
	 * This is a basic exception class of the library. All other exception classes are derived from it.
	 */
	class Exc : public ting::Exc{
	public:
		/**
		 * @brief Exception constructor.
		 * @param message Pointer to the exception message null-terminated string. Constructor will copy the string into objects internal memory buffer.
		 */
		Exc(const std::string& message = std::string()) :
				ting::Exc((std::string("[Socket::Exc] ") + message).c_str())
		{}
	};


	Socket(const Socket& s) :
			Waitable(),
			//NOTE: operator=() will call Close, so the socket should be in invalid state!!!
			//Therefore, init variables to invalid values.
#ifdef __WIN32__
			eventForWaitable(WSA_INVALID_EVENT),
#endif
			socket(DInvalidSocket())
	{
//		TRACE(<< "Socket::Socket(copy): invoked " << this << std::endl)
		this->operator=(s);
	}

	virtual ~Socket(){
		this->Close();
	}



	/**
	 * @brief Tells whether the socket is opened or not.
	 * @return Returns true if the socket is opened or false otherwise.
	 */
	inline bool IsValid()const{
		return this->socket != DInvalidSocket();
	}



	/**
	 * @brief Tells whether the socket is opened or not.
	 * @return inverse of IsValid().
	 */
	inline bool IsNotValid()const{
		return !this->IsValid();
	}



	/**
	 * @brief Closes the socket disconnecting it if necessary.
	 */
	void Close(){
//		TRACE(<< "Socket::Close(): invoked " << this << std::endl)
		if(this->IsValid()){
			ASSERT(!this->IsAdded()) //make sure the socket is not added to WaitSet

#ifdef __WIN32__
			//Closing socket in Win32.
			//refer to http://tangentsoft.net/wskfaq/newbie.html#howclose for details
			shutdown(this->socket, SD_BOTH);
			closesocket(this->socket);

			this->CloseEventForWaitable();
#else //assume linux/unix
			close(this->socket);
#endif
		}
		this->ClearAllReadinessFlags();
		this->socket = DInvalidSocket();
	}



	/**
	 * @brief Returns local port this socket is bound to.
	 * @return local port number to which this socket is bound,
	 *         0 means that the socket is not bound.
	 */
	u16 GetLocalPort(){
		if(!this->IsValid())
			throw Socket::Exc("Socket::GetLocalPort(): socket is not valid");

		sockaddr_in addr;

#ifdef __WIN32__
		int len = sizeof(addr);
#else//assume linux/unix
		socklen_t len = sizeof(addr);
#endif

		if(getsockname(
				this->socket,
				reinterpret_cast<sockaddr*>(&addr),
				&len
			) < 0)
		{
			throw Socket::Exc("Socket::GetLocalPort(): getsockname() failed");
		}

		return u16(ntohs(addr.sin_port));
	}



#ifdef __WIN32__
private:
	//override
	HANDLE GetHandle(){
		//return event handle
		return this->eventForWaitable;
	}

	//override
	bool CheckSignalled(){
		WSANETWORKEVENTS events;
		memset(&events, 0, sizeof(events));
		if(WSAEnumNetworkEvents(this->socket, this->eventForWaitable, &events) != 0){
			throw Socket::Exc("Socket::CheckSignalled(): WSAEnumNetworkEvents() failed");
		}

		if((events.lNetworkEvents & (FD_READ | FD_ACCEPT | FD_CLOSE)) != 0){
			this->SetCanReadFlag();
		}

		if(events.lNetworkEvents & FD_WRITE){
			this->SetCanWriteFlag();
		}

		return this->Waitable::CheckSignalled();
	}

protected:
	inline void CreateEventForWaitable(){
		ASSERT(this->eventForWaitable == WSA_INVALID_EVENT)
		this->eventForWaitable = WSACreateEvent();
		if(this->eventForWaitable == WSA_INVALID_EVENT){
			throw Socket::Exc("Socket::CreateEventForWaitable(): could not create event (Win32) for implementing Waitable");
		}
	}

	inline void CloseEventForWaitable(){
		ASSERT(this->eventForWaitable != WSA_INVALID_EVENT)
		WSACloseEvent(this->eventForWaitable);
		this->eventForWaitable = WSA_INVALID_EVENT;
	}

	inline void SetWaitingEventsForWindows(long flags){
		ASSERT_INFO(this->IsValid() && (this->eventForWaitable != WSA_INVALID_EVENT), "HINT: Most probably, you are trying to remove the _closed_ socket from WaitSet. If so, you should first remove the socket from WaitSet and then call Close() method.")

		if(WSAEventSelect(
				this->socket,
				this->eventForWaitable,
				flags
			) != 0)
		{
			throw Socket::Exc("Socket::SetWaitingEventsForWindows(): could not associate event (Win32) with socket");
		}
	}



#else
private:
	//override
	int GetHandle(){
		return this->socket;
	}
#endif
};//~class Socket



/**
 * @brief a structure which holds IP address
 */
class IPAddress{
public:
	u32 host;///< IP address
	u16 port;///< IP port number

	inline IPAddress() :
			host(0),
			port(0)
	{}

	/**
	 * @brief Create IP address specifying exact ip address and port number.
	 * @param h - IP address. For example, 0x100007f represents "127.0.0.1" IP address value.
	 * @param p - IP port number.
	 */
	inline IPAddress(u32 h, u16 p) :
			host(h),
			port(p)
	{}

	/**
	 * @brief Create IP address specifying exact ip address as 4 bytes and port number.
	 * The ip adress can be specified as 4 separate byte values, for example:
	 * @code
	 * ting::IPAddress ip(127, 0, 0, 1, 80); //"127.0.0.1" port 80
	 * @endcode
	 * @param h1 - 1st triplet of IP address.
	 * @param h2 - 2nd triplet of IP address.
	 * @param h3 - 3rd triplet of IP address.
	 * @param h4 - 4th triplet of IP address.
	 * @param p - IP port number.
	 */
	inline IPAddress(u8 h1, u8 h2, u8 h3, u8 h4, u16 p) :
			host(u32(h1) + (u32(h2) << 8) + (u32(h3) << 16) + (u32(h4) << 24)),
			port(p)
	{}

	/**
	 * @brief Create IP address specifying ip address as string and port number.
	 * @param ip - IP address null-terminated string. Example: "127.0.0.1".
	 * @param p - IP port number.
	 */
	inline IPAddress(const char* ip, u16 p) :
			host(IPAddress::ParseString(ip)),
			port(p)
	{}

	/**
	 * @brief compares two IP addresses for equality.
	 * @param ip - IP address to compare with.
	 * @return true if hosts and ports of the two IP addresses are equal accordingly.
	 * @return false otherwise.
	 */
	inline bool operator==(const IPAddress& ip){
		return (this->host == ip.host) && (this->port == ip.port);
	}
private:
	static inline void ThrowInvalidIP(){
		throw Socket::Exc("IPAddress::ParseString(): string is not a valid IP address");
	}

	//parse IP address from string
	static u32 ParseString(const char* ip){
		//TODO: there already is an IP parsing function in BSD sockets, consider using it here
		if(!ip)
			throw Socket::Exc("IPAddress::ParseString(): pointer passed as argument is 0");

		u32 h = 0;//parsed host
		const char *curp = ip;
		for(unsigned t = 0; t < 4; ++t){
			unsigned digits[3];
			unsigned numDgts;
			for(numDgts = 0; numDgts < 3; ++numDgts){
				if( *curp == '.' || *curp == 0 ){
					if(numDgts==0)
						ThrowInvalidIP();
					break;
				}else{
					if(*curp < '0' || *curp > '9')
						ThrowInvalidIP();
					digits[numDgts] = unsigned(*curp) - unsigned('0');
				}
				++curp;
			}

			if(t < 3 && *curp != '.')//unexpected delimiter or unexpected end of string
				ThrowInvalidIP();
			else if(t == 3 && *curp != 0)
				ThrowInvalidIP();

			unsigned xxx = 0;
			for(unsigned i = 0; i < numDgts; ++i){
				unsigned ord = 1;
				for(unsigned j = 1; j < numDgts - i; ++j)
				   ord *= 10;
				xxx += digits[i] * ord;
			}
			if(xxx > 255)
				ThrowInvalidIP();

			h |= (xxx << (8 * t));

			++curp;
		}
		return h;
	}
};//~class IPAddress



/**
 * @brief Socket library singletone class.
 * This is a Socket library singletone class. Creating an object of this class initializes the library
 * while destroying this object deinitializes it. So, the convenient way of initializing the library
 * is to create an object of this class on the stack. Thus, when the object goes out of scope its
 * destructor will be called and the library will be deinitialized automatically.
 * This is what C++ RAII is all about ;-).
 */
class SocketLib : public Singleton<SocketLib>{
public:
	SocketLib(){
#ifdef __WIN32__
		WORD versionWanted = MAKEWORD(2,2);
		WSADATA wsaData;
		if(WSAStartup(versionWanted, &wsaData) != 0 )
			throw Socket::Exc("SocketLib::SocketLib(): Winsock 2.2 initialization failed");
#else //assume linux/unix
		// SIGPIPE is generated when a remote socket is closed
		void (*handler)(int);
		handler = signal(SIGPIPE, SIG_IGN);
		if(handler != SIG_DFL)
			signal(SIGPIPE, handler);
#endif
	}

	~SocketLib(){
#ifdef __WIN32__
		// Clean up windows networking
		if(WSACleanup() == SOCKET_ERROR)
			if(WSAGetLastError() == WSAEINPROGRESS){
				WSACancelBlockingCall();
				WSACleanup();
			}
#else //assume linux/unix
		// Restore the SIGPIPE handler
		void (*handler)(int);
		handler = signal(SIGPIPE, SIG_DFL);
		if(handler != SIG_IGN)
			signal(SIGPIPE, handler);
#endif
	}


	/**
	 * @brief Resolve host IP by its name.
	 * This function resolves host IP address by its name. If it fails resolving the IP address it will throw Socket::Exc.
	 * @param hostName - null-terminated string representing host name. Example: "www.somedomain.com".
	 * @param port - IP port number which will be placed in the resulting IPAddress structure.
	 * @return filled IPAddress structure.
	 */
	IPAddress GetHostByName(const char *hostName, u16 port){
		if(!hostName)
			throw Socket::Exc("SocketLib::GetHostByName(): pointer passed as argument is 0");

		IPAddress addr;
		addr.host = inet_addr(hostName);
		if(addr.host == INADDR_NONE){
			struct hostent *hp;
			hp = gethostbyname(hostName);
			if(hp){
				memcpy(&(addr.host), hp->h_addr, sizeof(addr.host)/* hp->h_length */);
			}else{
				throw Socket::Exc("SocketLib::GetHostByName(): gethostbyname() failed");
			}
		}
		addr.port = port;
		return addr;
	}
};//~class SocketLib



/**
 * @brief a class which represents a TCP socket.
 */
class TCPSocket : public Socket{
	friend class TCPServerSocket;
public:
	/**
	 * @brief Constructs an invalid TCP socket object.
	 */
	TCPSocket(){
//		TRACE(<< "TCPSocket::TCPSocket(): invoked " << this << std::endl)
	};

	/**
	 * @brief A copy constructor.
	 * Copy constructor creates a new socket object which refers to the same socket as s.
	 * After constructor completes the s becomes invalid.
	 * In other words, the behavior of copy constructor is similar to one of std::auto_ptr class from standard C++ library.
	 * @param s - other TCP socket to make a copy from.
	 */
	//copy constructor
	TCPSocket(const TCPSocket& s) :
			Socket(s)
	{
//		TRACE(<< "TCPSocket::TCPSocket(copy): invoked " << this << std::endl)
	}

	/**
	 * @brief A constructor which automatically calls TCPSocket::Open() method.
	 * This constructor creates a socket and calls its TCPSocket::Open() method.
	 * So, it creates an already opened socket.
	 * @param ip - IP address to 'connect to/listen on'.
	 * @param disableNaggle - enable/disable Naggle algorithm.
	 */
	TCPSocket(const IPAddress& ip, bool disableNaggle = false){
		this->Open(ip, disableNaggle);
	}

	/**
	 * @brief Assignment operator, works similar to std::auto_ptr::operator=().
	 * After this assignment operator completes this socket object refers to the socket the s objejct referred, s become invalid.
	 * It works similar to std::auto_ptr::operator=() from standard C++ library.
	 * @param s - socket to assign from.
	 */
	TCPSocket& operator=(const TCPSocket& s){
		this->Socket::operator=(s);
		return *this;
	}

	/**
	 * @brief Connects the socket.
	 * This method connects the socket to remote TCP server socket.
	 * @param ip - IP address.
	 * @param disableNaggle - enable/disable Naggle algorithm.
	 */
	void Open(const IPAddress& ip, bool disableNaggle = false){
		if(this->IsValid())
			throw Socket::Exc("TCPSocket::Open(): socket already opened");

		//create event for implementing Waitable
#ifdef __WIN32__
		this->CreateEventForWaitable();
#endif

		this->socket = ::socket(AF_INET, SOCK_STREAM, 0);
		if(this->socket == DInvalidSocket()){
#ifdef __WIN32__
			this->CloseEventForWaitable();
#endif
			throw Socket::Exc("TCPSocket::Open(): Couldn't create socket");
		}

		//Connecting to remote host
		sockaddr_in sockAddr;
		memset(&sockAddr, 0, sizeof(sockAddr));
		sockAddr.sin_family = AF_INET;
		sockAddr.sin_addr.s_addr = ip.host;
		sockAddr.sin_port = htons(ip.port);

		// Connect to the remote host
		if(connect(
				this->socket,
				reinterpret_cast<sockaddr *>(&sockAddr),
				sizeof(sockAddr)
			) == DSocketError())
		{
			this->Close();
			throw Socket::Exc("TCPSocket::Open(): Couldn't connect to remote host");
		}

		//Disable Naggle algorithm if required
		if(disableNaggle)
			this->DisableNaggle();

		//Set the socket to non-blocking mode
#if defined(O_NONBLOCK)
		{
			fcntl(this->socket, F_SETFL, O_NONBLOCK);
		}
#elif defined(__WIN32__)
		{
			u_long mode = 1;
			ioctlsocket(this->socket, FIONBIO, &mode);
		}
#else
#error "How do we set non-blocking mode on other operating systems?"
#endif

		this->ClearAllReadinessFlags();
	}



	/**
	 * @brief Send data to connected socket.
	 * Sends data on connected socket. This method does not guarantee that the whole
	 * buffer will be sent completely, it will return the number of bytes actually sent.
	 * @param buf - pointer to the buffer with data to send.
	 * @param offset - offset inside the buffer from where to start sending the data.
	 * @return the number of bytes actually sent.
	 */
	unsigned Send(const ting::Buffer<u8>& buf, unsigned offset = 0){
		if(!this->IsValid())
			throw Socket::Exc("TCPSocket::Send(): socket is not opened");

		this->ClearCanWriteFlag();

		//Keep sending data until it is sent or an error occurs
		int res;
		while(true){
			res = send(
					this->socket,
					reinterpret_cast<const char*>(&buf[offset]),
					buf.Size() - offset,
					0
				);
			if(res == DSocketError()){
#ifdef __WIN32__
				int errorCode = WSAGetLastError();
#else //linux/unix
				int errorCode = errno;
#endif
				if(errorCode == DEIntr()){
					continue;
				}if(errorCode == DEAgain()){
					//can't send more bytes, return 0 bytes sent
					res = 0;
				}else{
					std::stringstream ss;
					ss << "TCPSocket::Send(): send() failed, error code = " << errorCode << ": ";
#ifdef _MSC_VER //if MSVC compiler
					{
						const unsigned msgbufSize = 0xff;
						char msgbuf[msgbufSize];
						strerror_s(msgbuf, msgbufSize, errorCode);
						msgbuf[msgbufSize - 1] = 0;//make sure the string is null-terminated
						ss << msgbuf;
					}
#else
					ss << strerror(errorCode);
#endif
					throw Socket::Exc(ss.str());
				}
			}
			break;
		}//~while

		ASSERT(res >= 0)
		return unsigned(res);
	}



	/**
	 * @brief Send data to connected socket.
	 * Sends data on connected socket. This method blocks until all data is completely sent.
	 * @param buf - the buffer with data to send.
	 */
	void SendAll(const ting::Buffer<u8>& buf){
		if(!this->IsValid())
			throw Socket::Exc("TCPSocket::Send(): socket is not opened");

		DEBUG_CODE(int left = int(buf.Size());)
		ASSERT(left >= 0)

		unsigned offset = 0;

		while(true){
			int res = this->Send(buf, offset);
			DEBUG_CODE(left -= res;)
			ASSERT(left >= 0)
			offset += res;
			if(offset == buf.Size()){
				break;
			}
			//give 30ms to allow data from send buffer to be sent
			Thread::Sleep(30);
		}

		ASSERT(left == 0)
	}



	/**
	 * @brief Receive data from connected socket.
	 * Receives data available on the socket.
	 * If there is no data available this function does not block, instead it returns 0,
	 * indicating that 0 bytes were received.
	 * If previous WaitSet::Wait() indicated that socket is ready for reading
	 * and TCPSocket::Recv() returns 0, then connection was closed by peer.
	 * @param buf - pointer to the buffer where to put received data.
	 * @param offset - offset inside the buffer where to start putting data from.
	 * @return the number of bytes written to the buffer.
	 */
	unsigned Recv(ting::Buffer<u8>& buf, unsigned offset = 0){
		//the 'can read' flag shall be cleared even if this function fails to avoid subsequent
		//calls to Recv() because it indicates that there's activity.
		//So, do it at the beginning of the function.
		this->ClearCanReadFlag();

		if(!this->IsValid())
			throw Socket::Exc("TCPSocket::Send(): socket is not opened");

		int len;

		while(true){
			len = recv(
					this->socket,
					reinterpret_cast<char*>(&buf[offset]),
					buf.Size() - offset,
					0
				);
			if(len == DSocketError()){
#ifdef __WIN32__
				int errorCode = WSAGetLastError();
#else //linux/unix
				int errorCode = errno;
#endif
				if(errorCode == DEIntr()){
					continue;
				}if(errorCode == DEAgain()){
					//no data available, return 0 bytes received
					len = 0;
				}else{
					std::stringstream ss;
					ss << "TCPSocket::Recv(): recv() failed, error code = " << errorCode << ": ";
#ifdef _MSC_VER //if MSVC compiler
					{
						const unsigned msgbufSize = 0xff;
						char msgbuf[msgbufSize];
						strerror_s(msgbuf, msgbufSize, errorCode);
						msgbuf[msgbufSize - 1] = 0;//make sure the string is null-terminated
						ss << msgbuf;
					}
#else
					ss << strerror(errorCode);
#endif
					throw Socket::Exc(ss.str());
				}
			}
			break;
		}//~while

		ASSERT(len >= 0)
		return unsigned(len);
	}

	/**
	 * @brief Get local IP address and port.
	 * @return IP address and port of the local socket.
	 */
	IPAddress GetLocalAddress(){
		if(!this->IsValid())
			throw Socket::Exc("Socket::GetLocalPort(): socket is not valid");

		sockaddr_in addr;

#ifdef __WIN32__
		int len = sizeof(addr);
#else//assume linux/unix
		socklen_t len = sizeof(addr);
#endif

		if(getsockname(
				this->socket,
				reinterpret_cast<sockaddr*>(&addr),
				&len
			) < 0)
		{
			throw Socket::Exc("Socket::GetLocalPort(): getsockname() failed");
		}

		return IPAddress(
				u32(ntohl(addr.sin_addr.s_addr)),
				u16(ntohs(addr.sin_port))
			);
	}

	/**
	 * @brief Get remote IP address and port.
	 * @return IP address and port of the peer socket.
	 */
	IPAddress GetRemoteAddress(){
		if(!this->IsValid())
			throw Socket::Exc("TCPSocket::GetRemoteAddress(): socket is not valid");

		sockaddr_in addr;

#ifdef __WIN32__
		int len = sizeof(addr);
#else//assume linux/unix
		socklen_t len = sizeof(addr);
#endif

		if(getpeername(
				this->socket,
				reinterpret_cast<sockaddr*>(&addr),
				&len
			) < 0)
		{
			throw Socket::Exc("TCPSocket::GetRemoteAddress(): getpeername() failed");
		}

		return IPAddress(
				u32(ntohl(addr.sin_addr.s_addr)),
				u16(ntohs(addr.sin_port))
			);
	}

private:
	void DisableNaggle(){
		if(!this->IsValid())
			throw Socket::Exc("TCPSocket::DisableNaggle(): socket is not opened");

		int yes = 1;
		setsockopt(this->socket, IPPROTO_TCP, TCP_NODELAY, (char*)&yes, sizeof(yes));
	}



#ifdef __WIN32__
private:
	//override
	void SetWaitingEvents(u32 flagsToWaitFor){
		long flags = FD_CLOSE;
		if((flagsToWaitFor & Waitable::READ) != 0){
			flags |= FD_READ;
		}
		if((flagsToWaitFor & Waitable::WRITE) != 0){
			flags |= FD_WRITE;
		}
		this->SetWaitingEventsForWindows(flags);
	}
#endif

};//~class TCPSocket



/**
 * @brief a class which represents a TCP server socket.
 * TCP server socket is the socket which can listen for new connections
 * and accept them creating an ordinary TCP socket for it.
 */
class TCPServerSocket : public Socket{
	bool disableNaggle;//this flag indicates if accepted sockets should be created with disabled Naggle
public:
	/**
	 * @brief Creates an invalid (unopened) TCP server socket.
	 */
	TCPServerSocket() :
			disableNaggle(false)
	{}

	/**
	 * @brief A copy constructor.
	 * Copy constructor creates a new socket object which refers to the same socket as s.
	 * 	After constructor completes the s becomes invalid.
	 * 	In other words, the behavior of copy constructor is similar to one of std::auto_ptr class from standard C++ library.
	 * 	@param s - other TCP socket to make a copy from.
	 */
	//copy constructor
	TCPServerSocket(const TCPServerSocket& s) :
			Socket(s),
			disableNaggle(s.disableNaggle)
	{}

	/**
	 * @brief Assignment operator, works similar to std::auto_ptr::operator=().
	 * After this assignment operator completes this socket object refers to the socket the s objejct referred, s become invalid.
	 * It works similar to std::auto_ptr::operator=() from standard C++ library.
	 * @param s - socket to assign from.
	 */
	TCPServerSocket& operator=(const TCPServerSocket& s){
		this->disableNaggle = s.disableNaggle;
		this->Socket::operator=(s);
		return *this;
	}

	/**
	 * @brief A constructor which automatically calls TCPServerSocket::Open() method.
	 * This constructor creates a socket and calls its TCPServerSocket::Open() method.
	 * So, it creates an already opened socket listening on the specified port.
	 * @param port - IP port number to listen on.
	 * @param disableNaggle - enable/disable Naggle algorithm for all accepted connections.
	 */
	TCPServerSocket(u16 port, bool disableNaggle = false){
		this->Open(port, disableNaggle);
	}

	/**
	 * @brief Connects the socket or starts listening on it.
	 * This method starts listening on the socket for incoming connections.
	 * @param port - IP port number to listen on.
	 * @param disableNaggle - enable/disable Naggle algorithm for all accepted connections.
	 * @param queueLength - the maximum length of the queue of pending connections.
	 */
	void Open(u16 port, bool disableNaggle = false, u16 queueLength = 50){
		if(this->IsValid())
			throw Socket::Exc("TCPServerSocket::Open(): socket already opened");

		this->disableNaggle = disableNaggle;

#ifdef __WIN32__
		this->CreateEventForWaitable();
#endif

		this->socket = ::socket(AF_INET, SOCK_STREAM, 0);
		if(this->socket == DInvalidSocket()){
#ifdef __WIN32__
			this->CloseEventForWaitable();
#endif
			throw Socket::Exc("TCPServerSocket::Open(): Couldn't create socket");
		}

		sockaddr_in sockAddr;
		memset(&sockAddr, 0, sizeof(sockAddr));
		sockAddr.sin_family = AF_INET;
		sockAddr.sin_addr.s_addr = INADDR_ANY;
		sockAddr.sin_port = htons(port);

		// allow local address reuse
		{
			int yes = 1;
			setsockopt(this->socket, SOL_SOCKET, SO_REUSEADDR, (char*)&yes, sizeof(yes));
		}

		// Bind the socket for listening
		if(bind(
				this->socket,
				reinterpret_cast<sockaddr*>(&sockAddr),
				sizeof(sockAddr)
			) == DSocketError())
		{
			this->Close();
			throw Socket::Exc("TCPServerSocket::Open(): Couldn't bind to local port");
		}

		if(listen(this->socket, int(queueLength)) == DSocketError()){
			this->Close();
			throw Socket::Exc("TCPServerSocket::Open(): Couldn't listen to local port");
		}

		//Set the socket to non-blocking mode for accept()
#if defined(O_NONBLOCK)
		{
			fcntl(this->socket, F_SETFL, O_NONBLOCK);
		}
#elif defined(__WIN32__)
		{
			u_long mode = 1;
			ioctlsocket(this->socket, FIONBIO, &mode);
		}
#else
#error "How do we set non-blocking mode on other operating systems?"
#endif
	}

	/**
	 * @brief Accepts one of the pending connections, non-blocking.
	 * Accepts one of the pending connections and returns a TCP socket object which represents
	 * either a valid connected socket or an invalid socket object.
	 * This function does not block if there is no any pending connections, it just returns invalid
	 * socket object in this case. One can periodically check for incoming connections by calling this method.
	 * @return TCPSocket object. One can later check if the returned socket object
	 *         is valid or not by calling Socket::IsValid() method on that object.
	 *         - if the socket is valid then it is a newly connected socket, further it can be used to send or receive data.
	 *         - if the socket is invalid then there was no any connections pending, so no connection was accepted.
	 */
	TCPSocket Accept(){
		if(!this->IsValid())
			throw Socket::Exc("TCPServerSocket::Accept(): the socket is not opened");

		this->ClearCanReadFlag();

		sockaddr_in sockAddr;

#ifdef __WIN32__
		int sock_alen = sizeof(sockAddr);
#else //linux/unix
		socklen_t sock_alen = sizeof(sockAddr);
#endif

		TCPSocket sock;//allocate a new socket object

		sock.socket = ::accept(
				this->socket,
				reinterpret_cast<sockaddr*>(&sockAddr),
#ifdef USE_GUSI_SOCKETS
				(unsigned int *)&sock_alen
#else
				&sock_alen
#endif
			);

		if(sock.socket == DInvalidSocket())
			return sock;//no connections to be accepted, return invalid socket

#ifdef __WIN32__
		sock.CreateEventForWaitable();
#endif

		//set blocking mode
#ifdef __WIN32__
		{
			//passing a zero value, socket mode set to block on
			u_long mode = 0;
			ioctlsocket(sock.socket, FIONBIO, &mode);
		}
#elif defined(O_NONBLOCK)
		{
			int flags = fcntl(sock.socket, F_GETFL, 0);
			fcntl(sock.socket, F_SETFL, flags & (~O_NONBLOCK));
		}
#else
#error "do not know how to set blocking mode to socket"
#endif

		if(this->disableNaggle)
			sock.DisableNaggle();

		return sock;//return a newly created socket
	}//~Accept()



#ifdef __WIN32__
private:
	//override
	void SetWaitingEvents(u32 flagsToWaitFor){
		long flags = FD_CLOSE;
		if((flagsToWaitFor & Waitable::READ) != 0){
			flags |= FD_ACCEPT;
		}
		this->SetWaitingEventsForWindows(flags);
	}
#endif
};//~class TCPServerSocket



class UDPSocket : public Socket{
public:
	UDPSocket(){}



	UDPSocket(const UDPSocket& s) :
			Socket(s)
	{}



	UDPSocket& operator=(const UDPSocket& s){
		this->Socket::operator=(s);
		return *this;
	}


	/**
	 * @brief Open the socket.
	 * This mthod opens the socket, this socket can further be used to send or receive data.
	 * After the socket is opened it becomes a valid socket and Socket::IsValid() will return true for such socket.
	 * After the socket is closed it becomes invalid.
	 * In other words, a valid socket is an opened socket.
	 * In case of errors this method throws Socket::Exc.
	 * @param port - IP port number on which the socket will listen for incoming datagrams.
	 * This is useful for server-side sockets, for client-side sockets use UDPSocket::Open().
	 */
	void Open(u16 port){
		if(this->IsValid())
			throw Socket::Exc("UDPSocket::Open(): the socket is already opened");

#ifdef __WIN32__
		this->CreateEventForWaitable();
#endif

		this->socket = ::socket(AF_INET, SOCK_DGRAM, 0);
		if(this->socket == DInvalidSocket()){
#ifdef __WIN32__
			this->CloseEventForWaitable();
#endif
			throw Socket::Exc("UDPSocket::Open(): ::socket() failed");
		}

		//Bind locally, if appropriate
		if(port != 0){
			struct sockaddr_in sockAddr;
			memset(&sockAddr, 0, sizeof(sockAddr));
			sockAddr.sin_family = AF_INET;
			sockAddr.sin_addr.s_addr = INADDR_ANY;
			sockAddr.sin_port = htons(port);

			// Bind the socket for listening
			if(::bind(
					this->socket,
					reinterpret_cast<struct sockaddr*>(&sockAddr),
					sizeof(sockAddr)
				) == DSocketError())
			{
				this->Close();
				throw Socket::Exc("UDPSocket::Open(): could not bind to local port");
			}
		}
#ifdef SO_BROADCAST
		//Allow LAN broadcasts with the socket
		{
			int yes = 1;
			setsockopt(this->socket, SOL_SOCKET, SO_BROADCAST, (char*)&yes, sizeof(yes));
		}
#endif

		this->ClearAllReadinessFlags();
	}


	inline void Open(){
		this->Open(0);
	}



	//returns number of bytes sent, should be less or equal to size.
	unsigned Send(const ting::Buffer<u8>& buf, const IPAddress& destinationIP){
		if(!this->IsValid())
			throw Socket::Exc("UDPSocket::Send(): socket is not opened");

		this->ClearCanWriteFlag();

		sockaddr_in sockAddr;
		int sockLen = sizeof(sockAddr);

		sockAddr.sin_addr.s_addr = destinationIP.host;
		sockAddr.sin_port = htons(destinationIP.port);
		sockAddr.sin_family = AF_INET;
		int res = ::sendto(
				this->socket,
				reinterpret_cast<const char*>(buf.Begin()),
				buf.Size(),
				0,
				reinterpret_cast<struct sockaddr*>(&sockAddr),
				sockLen
			);

		if(res == DSocketError())
			throw Socket::Exc("UDPSocket::Send(): sendto() failed");

		ASSERT(buf.Size() <= unsigned(ting::DMaxInt()))
		ASSERT_INFO(res <= int(buf.Size()), "res = " << res)

		return res;
	}



	//returns number of bytes received
	unsigned Recv(ting::Buffer<u8>& buf, IPAddress &out_SenderIP){
		if(!this->IsValid())
			throw Socket::Exc("UDPSocket::Recv(): socket is not opened");

		//the flag shall be cleared even if this function fails to avoid subsequent
		//calls to Recv() because it indicates that there's activity.
		//So, do it at the beginning of the function.
		this->ClearCanReadFlag();

		sockaddr_in sockAddr;

#ifdef __WIN32__
		int sockLen = sizeof(sockAddr);
#else //linux/unix
		socklen_t sockLen = sizeof(sockAddr);
#endif

		int res = ::recvfrom(
				this->socket,
				reinterpret_cast<char*>(buf.Begin()),
				buf.Size(),
				0,
				reinterpret_cast<sockaddr*>(&sockAddr),
				&sockLen
			);

		if(res == DSocketError())
			throw Socket::Exc("UDPSocket::Recv(): recvfrom() failed");

		ASSERT(buf.Size() <= unsigned(ting::DMaxInt()))
		ASSERT_INFO(res <= int(buf.Size()), "res = " << res)

		out_SenderIP.host = ntohl(sockAddr.sin_addr.s_addr);
		out_SenderIP.port = ntohs(sockAddr.sin_port);
		return res;
	}



#ifdef __WIN32__
private:
	//override
	void SetWaitingEvents(u32 flagsToWaitFor){
		long flags = FD_CLOSE;
		if((flagsToWaitFor & Waitable::READ) != 0){
			flags |= FD_READ;
		}
		if((flagsToWaitFor & Waitable::WRITE) != 0){
			flags |= FD_WRITE;
		}
		this->SetWaitingEventsForWindows(flags);
	}
#endif
};//~class UDPSocket



}//~namespace


/*
 * @mainpage ting::Socket library
 *
 * @section sec_about About
 * <b>tin::Socket</b> is a simple cross platfrom C++ wrapper above sockets networking API designed for games.
 *
 * @section sec_getting_started Getting started
 * @ref page_usage_tutorial "library usage tutorial" - quickstart tutorial
 */

/*
 * @page page_usage_tutorial ting::Socket usage tutorial
 *
 * TODO: write usage tutorial
 */

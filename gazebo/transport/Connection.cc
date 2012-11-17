/*
 * Copyright 2011 Nate Koenig
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/
#include <boost/lexical_cast.hpp>

#include "common/Console.hh"
#include "msgs/msgs.hh"

#include "transport/IOManager.hh"
#include "transport/Connection.hh"

using namespace gazebo;
using namespace transport;

unsigned int Connection::idCounter = 0;
IOManager *Connection::iomanager = NULL;

//////////////////////////////////////////////////
Connection::Connection()
{
  if (iomanager == NULL)
    iomanager = new IOManager();

  this->socket = new boost::asio::ip::tcp::socket(iomanager->GetIO());

  iomanager->IncCount();
  this->id = idCounter++;

  this->connectMutex = new boost::mutex();
  this->writeMutex = new boost::recursive_mutex();
  this->readMutex = new boost::recursive_mutex();
  this->acceptor = NULL;
  this->readQuit = false;
  this->writeQueue.clear();
  this->writeCount = 0;

  this->localURI = std::string("http://") + this->GetLocalHostname() + ":" +
                   boost::lexical_cast<std::string>(this->GetLocalPort());
  this->localAddress = this->GetLocalEndpoint().address().to_string();
}

//////////////////////////////////////////////////
Connection::~Connection()
{
  this->ProcessWriteQueue();
  this->writeQueue.clear();
  this->Shutdown();

  delete this->writeMutex;
  this->writeMutex = NULL;

  delete this->readMutex;
  this->readMutex = NULL;

  if (iomanager)
  {
    iomanager->DecCount();
    if (iomanager->GetCount() == 0)
    {
      this->idCounter = 0;
      delete iomanager;
      iomanager = NULL;
    }
  }
}

//////////////////////////////////////////////////
bool Connection::Connect(const std::string &_host, unsigned int _port)
{
  boost::mutex::scoped_lock lock(*this->connectMutex);

  std::string service = boost::lexical_cast<std::string>(_port);

  int httpIndex = _host.find("http://");
  std::string host = _host;
  if (httpIndex != static_cast<int>(std::string::npos))
    host = _host.substr(7, _host.size() - 7);

  // Resolve the host name into an IP address
  boost::asio::ip::tcp::resolver::iterator end;
  boost::asio::ip::tcp::resolver resolver(iomanager->GetIO());
  boost::asio::ip::tcp::resolver::query query(host, service,
      boost::asio::ip::resolver_query_base::numeric_service);
  boost::asio::ip::tcp::resolver::iterator endpoint_iter;
  try
  {
    endpoint_iter = resolver.resolve(query);
  }
  catch(...)
  {
    gzerr << "Unable to resolve uri[" << host << ":" << _port << "]\n";
    return false;
  }

  this->connectError = false;
  this->remoteURI.clear();

  // Use async connect so that we can use a custom timeout. This is useful
  // when trying to detect network errors.
  this->socket->async_connect(*endpoint_iter++,
      boost::bind(&Connection::OnConnect, this,
        boost::asio::placeholders::error, endpoint_iter));

  this->connectCondition.wait(lock);

  if (this->connectError)
  {
    return false;
  }

  if (this->remoteURI.empty())
  {
    gzerr << "Unable to connect to host[" << host << ":" << _port << "]\n";
    return false;
  }

  return true;
}

//////////////////////////////////////////////////
void Connection::Listen(unsigned int port, const AcceptCallback &accept_cb)
{
  this->acceptCB = accept_cb;

  this->acceptor = new boost::asio::ip::tcp::acceptor(iomanager->GetIO());
  boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), port);
  this->acceptor->open(endpoint.protocol());
  this->acceptor->set_option(
      boost::asio::ip::tcp::acceptor::reuse_address(true));
  this->acceptor->bind(endpoint);
  this->acceptor->listen();

  this->acceptConn = ConnectionPtr(new Connection());

  this->acceptor->async_accept(*this->acceptConn->socket,
      boost::bind(&Connection::OnAccept, this,
                  boost::asio::placeholders::error));
}

//////////////////////////////////////////////////
void Connection::OnAccept(const boost::system::error_code &e)
{
  // Call the accept callback if there isn't an error
  if (!e)
  {
    // First start a new acceptor
    this->acceptCB(this->acceptConn);

    this->acceptConn = ConnectionPtr(new Connection());

    this->acceptor->async_accept(*this->acceptConn->socket,
        boost::bind(&Connection::OnAccept, this,
          boost::asio::placeholders::error));
  }
  else
  {
    // Probably the connection was closed. No need to report an error since
    // this can happen duing a shutdown.
    if (e.value() != ECANCELED)
      gzerr << e.message() << "Value[" << e.value() << "]" << std::endl;
  }
}

//////////////////////////////////////////////////
void Connection::StartRead(const ReadCallback & /*_cb*/)
{
  gzerr << "\n\n\n\n DONT USE \n\n\n\n";
}

//////////////////////////////////////////////////
void Connection::StopRead()
{
  this->readQuit = true;
}

//////////////////////////////////////////////////
void Connection::EnqueueMsg(const std::string &_buffer, bool _force)
{
  // Don't enqueue empty messages
  if (_buffer.empty())
  {
    return;
  }

  std::ostringstream header_stream;

  header_stream << std::setw(HEADER_LENGTH) << std::hex << _buffer.size();

  if (header_stream.str().empty() ||
      header_stream.str().size() != HEADER_LENGTH)
  {
    // Something went wrong, inform the caller
    boost::system::error_code error(boost::asio::error::invalid_argument);
    gzerr << "Connection::Write error[" << error.message() << "]\n";
    return;
  }

  /*if (_force)
  {
    this->writeMutex->lock();
    boost::asio::streambuf *buffer = new boost::asio::streambuf;
    std::ostream os(buffer);
    os << header_stream.str() << _buffer;

    std::size_t written = 0;
    written = boost::asio::write(*this->socket, buffer->data());
    if (written != buffer->size())
      gzerr << "Didn't write all the data\n";

    delete buffer;
    this->writeMutex->unlock();
  }
  else
  {
  */
    this->writeMutex->lock();
    this->writeQueue.push_back(header_stream.str());
    this->writeQueue.push_back(_buffer);
    this->writeMutex->unlock();
  // }

    if (_force)
    {
      this->ProcessWriteQueue();
    }
}

/////////////////////////////////////////////////
void Connection::ProcessWriteQueue()
{
  if (!this->IsOpen())
  {
    return;
  }

  this->writeMutex->lock();

  // async_write should only be called when the last async_write has
  // completed. Therefore we have to check the writeCount attribute
  if (this->writeQueue.size() == 0 || this->writeCount > 0)
  {
    this->writeMutex->unlock();
    return;
  }

  boost::asio::streambuf *buffer = new boost::asio::streambuf;
  std::ostream os(buffer);

  for (unsigned int i = 0; i < this->writeQueue.size(); i++)
  {
    os << this->writeQueue[i];
  }
  this->writeQueue.clear();
  this->writeCount++;

  // Write the serialized data to the socket. We use
  // "gather-write" to send both the head and the data in
  // a single write operation
  // Note: This seems to cause a memory leak.
  /*boost::asio::async_write(*this->socket, buffer->data(),
    boost::bind(&Connection::OnWrite, shared_from_this(),
    boost::asio::placeholders::error, buffer));
    */

  try
  {
    boost::asio::write(*this->socket, buffer->data());
  }
  catch(...)
  {
    this->Shutdown();
  }

  this->writeCount--;
  delete buffer;
  this->writeMutex->unlock();
}

//////////////////////////////////////////////////
std::string Connection::GetLocalURI() const
{
  return this->localURI;
}

//////////////////////////////////////////////////
std::string Connection::GetRemoteURI() const
{
  return this->remoteURI;
}

//////////////////////////////////////////////////
void Connection::OnWrite(const boost::system::error_code &e,
                         boost::asio::streambuf *_buffer)
{
  this->writeMutex->lock();
  delete _buffer;
  this->writeCount--;
  this->writeMutex->unlock();

  if (e)
  {
    // gzerr << "onWrite error[" << e.message() << "]\n";
    // It will reach this point if the remote connection disconnects.
    this->Shutdown();
  }
}

//////////////////////////////////////////////////
void Connection::Shutdown()
{
  this->ProcessWriteQueue();

  int iters = 0;
  while (this->writeCount > 0 && iters < 50)
  {
    common::Time::MSleep(10);
    iters++;
  }

  this->shutdown();
  // this->StopRead();

  this->Cancel();

  if (this->socket && this->socket->is_open())
  {
    this->Close();
    boost::system::error_code ec;
    this->socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
  }

  delete this->socket;
  this->socket = NULL;
}

//////////////////////////////////////////////////
bool Connection::IsOpen() const
{
  bool result = this->socket && this->socket->is_open();
  try
  {
    this->GetRemoteURI();
  }
  catch(...)
  {
    result = false;
  }

  return result;
}


//////////////////////////////////////////////////
void Connection::Close()
{
  if (this->socket && this->socket->is_open())
  {
    this->ProcessWriteQueue();
    try
    {
      this->socket->close();
    }
    catch(boost::system::system_error &e)
    {
      // This warning message is unnecessary...
      // gzwarn << "Error closing socket[" << this->id << "] ["
             // << e.what() << "]\n";
    }
  }

  if (this->acceptor && this->acceptor->is_open())
  {
    try
    {
      this->acceptor->close();
    }
    catch(boost::system::system_error &e)
    {
      gzwarn <<"Error closing acceptor[" << this->id << "]\n";
    }

    delete this->acceptor;
    this->acceptor = NULL;
  }
}

//////////////////////////////////////////////////
void Connection::Cancel()
{
  if (this->acceptor)
  {
    try
    {
      this->acceptor->cancel();
    }
    catch(boost::system::system_error &e)
    {
      gzwarn << "Connection::Cancel Error[" << e.what() << "]\n";
      // Left empty on purpose
    }
    delete this->acceptor;
    this->acceptor = NULL;
  }

  if (this->socket && this->socket->is_open())
  {
    try
    {
      this->socket->cancel();
    }
    catch(...)
    {
      this->socket->close();
    }
  }
}

//////////////////////////////////////////////////
bool Connection::Read(std::string &data)
{
  bool result = false;
  char header[HEADER_LENGTH];
  std::vector<char> incoming;

  std::size_t incoming_size;
  boost::system::error_code error;

  this->readMutex->lock();

  // First read the header
  this->socket->read_some(boost::asio::buffer(header), error);
  if (error)
  {
    gzerr << "Connection[" << this->id << "] Closed during Read\n";
    throw boost::system::system_error(error);
  }

  // Parse the header to get the size of the incoming data packet
  incoming_size = this->ParseHeader(header);
  if (incoming_size > 0)
  {
    incoming.resize(incoming_size);

    std::size_t len = 0;
    do
    {
      // Read in the actual data
      len += this->socket->read_some(boost::asio::buffer(&incoming[len],
            incoming_size - len), error);
    } while (len < incoming_size && !error && !this->readQuit);

    if (len != incoming_size)
    {
      gzerr << "Did not read everying. Read[" << len
        << "] Needed[" << incoming_size << "]\n";
    }

    if (error)
      throw boost::system::system_error(error);

    data = std::string(&incoming[0], incoming.size());
    result = true;
  }

  this->readMutex->unlock();
  return result;
}

//////////////////////////////////////////////////
std::string Connection::GetLocalAddress() const
{
  return this->localAddress;
}

//////////////////////////////////////////////////
unsigned int Connection::GetLocalPort() const
{
  try
  {
    if (this->socket && this->socket->is_open())
      return this->socket->local_endpoint().port();
    else if (this->acceptor)
      return this->acceptor->local_endpoint().port();
  }
  catch(...)
  {
  }

  return 0;
}

//////////////////////////////////////////////////
std::string Connection::GetRemoteAddress() const
{
  return this->remoteAddress;
}

//////////////////////////////////////////////////
unsigned int Connection::GetRemotePort() const
{
  if (this->socket && this->socket->is_open())
    return this->socket->remote_endpoint().port();
  else
    return 0;
}



//////////////////////////////////////////////////
std::size_t Connection::ParseHeader(const std::string &header)
{
  std::size_t data_size = 0;
  std::istringstream is(header);

  if (!(is >> std::hex >> data_size))
  {
    // Header doesn't seem to be valid. Inform the caller
    boost::system::error_code error(boost::asio::error::invalid_argument);
  }

  return data_size;
}

//////////////////////////////////////////////////
void Connection::ReadLoop(const ReadCallback &cb)
{
  std::string data;

  this->readQuit = false;
  while (!this->readQuit)
  {
    try
    {
      if (this->socket->available() >= HEADER_LENGTH)
      {
        if (this->Read(data))
        {
          (cb)(data);
        }
      }
      else
      {
        common::Time::MSleep(10);
        continue;
      }
    }
    catch(std::exception &e)
    {
      // The connection closed
      break;
    }
  }
}

//////////////////////////////////////////////////
boost::asio::ip::tcp::endpoint Connection::GetLocalEndpoint() const
{
  boost::asio::ip::tcp::resolver resolver(iomanager->GetIO());
  boost::asio::ip::tcp::resolver::query query(boost::asio::ip::host_name(), "");
  boost::asio::ip::tcp::resolver::iterator iter = resolver.resolve(query);
  boost::asio::ip::tcp::resolver::iterator end;
  boost::asio::ip::tcp::endpoint ep;

  while (iter != end)
    ep = *iter++;

  return ep;
}

//////////////////////////////////////////////////
boost::asio::ip::tcp::endpoint Connection::GetRemoteEndpoint() const
{
  boost::asio::ip::tcp::endpoint ep;
  if (this->socket)
    ep = this->socket->remote_endpoint();

  return ep;
}

//////////////////////////////////////////////////
std::string Connection::GetHostname(boost::asio::ip::tcp::endpoint ep)
{
  boost::asio::ip::tcp::resolver resolver(iomanager->GetIO());
  boost::asio::ip::tcp::resolver::iterator iter = resolver.resolve(ep);
  boost::asio::ip::tcp::resolver::iterator end;

  std::string name;

  while (iter != end)
  {
    name = (*iter).host_name();
    ++iter;
  }

  return name;
}

//////////////////////////////////////////////////
std::string Connection::GetRemoteHostname() const
{
  return this->GetHostname(this->GetRemoteEndpoint());
}

//////////////////////////////////////////////////
std::string Connection::GetLocalHostname() const
{
  return this->GetHostname(this->GetLocalEndpoint());
}

//////////////////////////////////////////////////
void Connection::OnConnect(const boost::system::error_code &_error,
    boost::asio::ip::tcp::resolver::iterator _endPointIter)
{
  boost::mutex::scoped_lock(*this->connectMutex);

  if (_error == 0)
  {
    this->remoteURI = std::string("http://") + this->GetRemoteHostname()
      + ":" + boost::lexical_cast<std::string>(this->GetRemotePort());

    if (this->socket && this->socket->is_open())
    {
      this->remoteAddress =
        this->socket->remote_endpoint().address().to_string();
    }
    else
    {
      this->connectError = true;
      gzerr << "Invalid socket connection\n";
    }

    this->connectCondition.notify_one();
  }
  else if (_endPointIter != boost::asio::ip::tcp::resolver::iterator() &&
           this->socket)
  {
    this->socket->close();
    boost::asio::ip::tcp::endpoint endPoint = *_endPointIter;

    this->socket->async_connect(endPoint,
        boost::bind(&Connection::OnConnect, this,
          boost::asio::placeholders::error, ++_endPointIter));
  }
  else
  {
    this->connectError = true;
    this->connectCondition.notify_one();
  }
}

/*
 * This file is part of the ZoneMinder Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "catch2/catch.hpp"

#include "zm_comms.h"
#include <array>

TEST_CASE("ZM::Pipe basics") {
  ZM::Pipe pipe;

  SECTION("setBlocking on non-opened") {
    REQUIRE(pipe.setBlocking(true) == false);
    REQUIRE(pipe.setBlocking(false) == false);
  }

  REQUIRE(pipe.open() == true);

  REQUIRE(pipe.isOpen() == true);
  REQUIRE(pipe.isClosed() == false);
  REQUIRE(pipe.getReadDesc() != -1);
  REQUIRE(pipe.getWriteDesc() != -1);

  SECTION("double open") {
    REQUIRE(pipe.open() == true); // is this expected?
  }

  SECTION("close") {
    REQUIRE(pipe.close() == true);

    REQUIRE(pipe.isOpen() == false);
    REQUIRE(pipe.isClosed() == true);
    REQUIRE(pipe.getReadDesc() == -1);
    REQUIRE(pipe.getWriteDesc() == -1);

    SECTION("double close") {
      REQUIRE(pipe.close() == true);
    }

    SECTION("setBlocking on closed") {
      REQUIRE(pipe.setBlocking(true) == false);
      REQUIRE(pipe.setBlocking(false) == false);
    }
  }

  SECTION("setBlocking") {
    REQUIRE(pipe.setBlocking(true) == true);
    REQUIRE(pipe.setBlocking(false) == true);
  }
}

TEST_CASE("ZM::Pipe read/write") {
  ZM::Pipe pipe;

  std::array<char, 3> msg = {'a', 'b', 'c'};
  std::array<char, msg.size()> rcv{};

  SECTION("read/write on non-opened pipe") {
    REQUIRE(pipe.write(msg.data(), msg.size()) == -1);
    REQUIRE(pipe.read(rcv.data(), rcv.size()) == -1);
  }

  SECTION("read/write on opened pipe") {
    REQUIRE(pipe.open() == true);

    REQUIRE(pipe.write(msg.data(), msg.size()) == msg.size());
    REQUIRE(pipe.read(rcv.data(), rcv.size()) == msg.size());

    REQUIRE(rcv == msg);
  }

  SECTION("read/write on closed pipe") {
    REQUIRE(pipe.open() == true);
    REQUIRE(pipe.close() == true);

    REQUIRE(pipe.write(msg.data(), msg.size()) == -1);
    REQUIRE(pipe.read(rcv.data(), rcv.size()) == -1);
  }
}

TEST_CASE("ZM::SockAddrInet") {
  ZM::SockAddrInet addr;
  REQUIRE(addr.getAddrSize() == sizeof(sockaddr_in));

  SECTION("resolve") {
    addr.resolve(80, "");
    REQUIRE(addr.getDomain() == AF_INET);

    SECTION("newSockAddr from resolved addr") {
      ZM::SockAddr *addr2 = ZM::SockAddr::newSockAddr(&addr);
      REQUIRE(addr2->getDomain() == AF_INET);
      REQUIRE(addr2->getAddrSize() == sizeof(sockaddr_in));
    }
  }
}

TEST_CASE("ZM::SockAddrUnix") {
  ZM::SockAddrUnix addr;
  REQUIRE(addr.getAddrSize() == sizeof(sockaddr_un));

  SECTION("resovle") {
    addr.resolve("/", "");
    REQUIRE(addr.getDomain() == AF_UNIX);

    SECTION("newSockAddr from resolved addr") {
      ZM::SockAddr *addr2 = ZM::SockAddr::newSockAddr(&addr);
      REQUIRE(addr2->getDomain() == AF_UNIX);
      REQUIRE(addr2->getAddrSize() == sizeof(sockaddr_un));
    }
  }
}

TEST_CASE("ZM::UdpInetSocket basics") {
  ZM::UdpInetSocket socket;
  REQUIRE(socket.isClosed() == true);
  REQUIRE(socket.isOpen() == false);
  REQUIRE(socket.isConnected() == false);
  REQUIRE(socket.isDisconnected() == false);

  SECTION("bind with host and port") {
    REQUIRE(socket.bind(nullptr, "1234") == true);
    REQUIRE(socket.isOpen() == true);
    REQUIRE(socket.isDisconnected() == true);
    REQUIRE(socket.isClosed() == false);
    REQUIRE(socket.isConnected() == false);

    SECTION("close") {
      REQUIRE(socket.close() == true);
      REQUIRE(socket.isClosed() == true);
      REQUIRE(socket.isOpen() == false);
      REQUIRE(socket.isConnected() == false);
      REQUIRE(socket.isDisconnected() == false);
    }
  }

  SECTION("bind with port") {
    REQUIRE(socket.bind("1234") == true);
  }

  SECTION("bind with host and port number") {
    REQUIRE(socket.bind(nullptr, 1234) == true);
  }

  SECTION("bind with port number") {
    REQUIRE(socket.bind(1234) == true);
  }
}

TEST_CASE("ZM::UdpInetSocket send/recv") {
  ZM::UdpInetSocket srv_socket;
  ZM::UdpInetSocket client_socket;

  std::array<char, 3> msg = {'a', 'b', 'c'};
  std::array<char, msg.size()> rcv{};

  SECTION("send/recv on unbound socket") {
    REQUIRE(client_socket.send(msg.data(), msg.size()) == -1);
    REQUIRE(srv_socket.recv(rcv.data(), rcv.size()) == -1);
  }

  SECTION("send/recv") {
    REQUIRE(srv_socket.bind("127.0.0.1", "1234") == true);
    REQUIRE(srv_socket.isOpen() == true);

    REQUIRE(client_socket.connect("127.0.0.1", "1234") == true);
    REQUIRE(client_socket.isConnected() == true);

    REQUIRE(client_socket.send(msg.data(), msg.size()) == msg.size());
    REQUIRE(srv_socket.recv(rcv.data(), rcv.size()) == msg.size());

    REQUIRE(rcv == msg);
  }
}

TEST_CASE("ZM::UdpUnixSocket basics") {
  std::string sock_path = "/tmp/zm.unittest.sock";
  unlink(sock_path.c_str()); // make sure the socket file does not exist

  ZM::UdpUnixSocket socket;
  REQUIRE(socket.isClosed() == true);
  REQUIRE(socket.isOpen() == false);
  REQUIRE(socket.isConnected() == false);
  REQUIRE(socket.isDisconnected() == false);

  SECTION("bind") {
    REQUIRE(socket.bind(sock_path.c_str()) == true);
    REQUIRE(socket.isOpen() == true);
    REQUIRE(socket.isDisconnected() == true);
    REQUIRE(socket.isClosed() == false);
    REQUIRE(socket.isConnected() == false);

    SECTION("close") {
      REQUIRE(socket.close() == true);
      REQUIRE(socket.isClosed() == true);
      REQUIRE(socket.isOpen() == false);
      REQUIRE(socket.isConnected() == false);
      REQUIRE(socket.isDisconnected() == false);
    }
  }

  SECTION("connect to unbound socket") {
    REQUIRE(socket.connect(sock_path.c_str()) == false);
  }
}

TEST_CASE("ZM::UdpUnixSocket send/recv") {
  std::string sock_path = "/tmp/zm.unittest.sock";
  unlink(sock_path.c_str()); // make sure the socket file does not exist

  ZM::UdpUnixSocket srv_socket;
  ZM::UdpUnixSocket client_socket;

  std::array<char, 3> msg = {'a', 'b', 'c'};
  std::array<char, msg.size()> rcv{};

  SECTION("send/recv on unbound socket") {
    REQUIRE(client_socket.send(msg.data(), msg.size()) == -1);
    REQUIRE(srv_socket.recv(rcv.data(), rcv.size()) == -1);
  }

  SECTION("send/recv") {
    REQUIRE(srv_socket.bind(sock_path.c_str()) == true);
    REQUIRE(srv_socket.isOpen() == true);

    REQUIRE(client_socket.connect(sock_path.c_str()) == true);
    REQUIRE(client_socket.isConnected() == true);

    REQUIRE(client_socket.send(msg.data(), msg.size()) == msg.size());
    REQUIRE(srv_socket.recv(rcv.data(), rcv.size()) == msg.size());

    REQUIRE(rcv == msg);
  }
}

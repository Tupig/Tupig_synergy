/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "net/IDataSocket.h"

#include "net/SocketException.h"

//
// IDataSocket
//

void IDataSocket::close()
{
  // Default bodies exist so the class can be used as a concrete base on old
  // compilers; calling them means a derived class failed to override.
  throw SocketException(QStringLiteral("IDataSocket::close called on abstract base"));
}

void *IDataSocket::getEventTarget() const
{
  throw SocketException(QStringLiteral("IDataSocket::getEventTarget called on abstract base"));
}

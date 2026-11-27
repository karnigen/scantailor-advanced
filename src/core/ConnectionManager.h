// SPDX-FileCopyrightText: 2020 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2020 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QtCore/QArgument>
#include <functional>
#include <list>

class ConnectionManager {
 public:
  class ScopedBlock {
   public:
    explicit ScopedBlock(ConnectionManager& parent);

    ~ScopedBlock();

   private:
    ConnectionManager& m_parent;
    bool m_needSetup;
  };

  explicit ConnectionManager(const std::function<void()>& setupConnectionsFunc);

  void addConnection(const QMetaObject::Connection& connection);

  ScopedBlock getScopedBlock();

  void setupConnections() const;

 private:
  bool removeConnections();

  std::function<void()> m_setupConnectionsFunc;
  std::list<QMetaObject::Connection> m_connectionList;
};

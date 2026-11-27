// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QObject>
#include <cstddef>
#include <iterator>
#include <list>
#include <memory>
#include <unordered_map>
#include <utility>

#include "EditableSpline.h"
#include "PropertySet.h"

class EditableZoneSet : public QObject {
  Q_OBJECT
 private:
  using ZoneItem = std::pair<EditableSpline::Ptr, std::shared_ptr<PropertySet>>;
  using ZoneItemsInOrder = std::list<ZoneItem>;
  using ZoneItemsBySpline = std::unordered_map<EditableSpline::Ptr, ZoneItemsInOrder::iterator>;

 public:
  class const_iterator;

  class Zone {
    friend class EditableZoneSet::const_iterator;

   public:
    Zone() = default;

    const EditableSpline::Ptr& spline() const { return m_iter->first; }

    const std::shared_ptr<PropertySet>& properties() const { return m_iter->second; }

   private:
    explicit Zone(ZoneItemsInOrder::const_iterator it) : m_iter(it) {}

    ZoneItemsInOrder::const_iterator m_iter;
  };


  class const_iterator {
    friend class EditableZoneSet;

   public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = Zone;
    using difference_type = std::ptrdiff_t;
    using pointer = const Zone*;
    using reference = const Zone&;

    const_iterator() = default;

    reference operator*() const { return m_zone; }

    pointer operator->() const { return &m_zone; }

    const_iterator& operator++() {
      ++m_zone.m_iter;
      return *this;
    }

    const_iterator operator++(int) {
      const_iterator prev(*this);
      ++(*this);
      return prev;
    }

    const_iterator& operator--() {
      --m_zone.m_iter;
      return *this;
    }

    const_iterator operator--(int) {
      const_iterator prev(*this);
      --(*this);
      return prev;
    }

    friend bool operator==(const const_iterator& lhs, const const_iterator& rhs) {
      return lhs.m_zone.m_iter == rhs.m_zone.m_iter;
    }

    friend bool operator!=(const const_iterator& lhs, const const_iterator& rhs) { return !(lhs == rhs); }

   private:
    explicit const_iterator(ZoneItemsInOrder::const_iterator it) : m_zone(it) {}

    Zone m_zone;
  };

  using iterator = const_iterator;

  EditableZoneSet();

  const_iterator begin() const { return iterator(m_zoneItemsInOrder.begin()); }

  const_iterator end() const { return iterator(m_zoneItemsInOrder.end()); }

  const PropertySet& defaultProperties() const { return m_defaultProps; }

  void setDefaultProperties(const PropertySet& props);

  void addZone(const EditableSpline::Ptr& spline);

  void addZone(const EditableSpline::Ptr& spline, const PropertySet& props);

  void removeZone(const EditableSpline::Ptr& spline);

  void commit();

  std::shared_ptr<PropertySet> propertiesFor(const EditableSpline::Ptr& spline);

  std::shared_ptr<const PropertySet> propertiesFor(const EditableSpline::Ptr& spline) const;

 signals:

  void committed();

 private:
  void insertZone(const EditableSpline::Ptr& spline, std::shared_ptr<PropertySet> properties);

  ZoneItemsInOrder m_zoneItemsInOrder;
  ZoneItemsBySpline m_zoneItemsBySpline;
  PropertySet m_defaultProps;
};

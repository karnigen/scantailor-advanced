// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "EditableZoneSet.h"

EditableZoneSet::EditableZoneSet() = default;

void EditableZoneSet::setDefaultProperties(const PropertySet& props) {
  m_defaultProps = props;
}

void EditableZoneSet::addZone(const EditableSpline::Ptr& spline) {
  insertZone(spline, std::make_shared<PropertySet>(m_defaultProps));
}

void EditableZoneSet::addZone(const EditableSpline::Ptr& spline, const PropertySet& props) {
  insertZone(spline, std::make_shared<PropertySet>(props));
}

void EditableZoneSet::insertZone(const EditableSpline::Ptr& spline, std::shared_ptr<PropertySet> properties) {
  auto [it, inserted] = m_zoneItemsBySpline.try_emplace(spline, m_zoneItemsInOrder.end());
  if (!inserted) {
    return;
  }

  try {
    it->second = m_zoneItemsInOrder.emplace(m_zoneItemsInOrder.end(), spline, std::move(properties));
  } catch (...) {
    m_zoneItemsBySpline.erase(it);
    throw;
  }
}

void EditableZoneSet::removeZone(const EditableSpline::Ptr& spline) {
  const auto it = m_zoneItemsBySpline.find(spline);
  if (it != m_zoneItemsBySpline.end()) {
    m_zoneItemsInOrder.erase(it->second);
    m_zoneItemsBySpline.erase(it);
  }
}

void EditableZoneSet::commit() {
  emit committed();
}

std::shared_ptr<PropertySet> EditableZoneSet::propertiesFor(const EditableSpline::Ptr& spline) {
  const auto it = m_zoneItemsBySpline.find(spline);
  if (it != m_zoneItemsBySpline.end()) {
    return it->second->second;
  } else {
    return nullptr;
  }
}

std::shared_ptr<const PropertySet> EditableZoneSet::propertiesFor(const EditableSpline::Ptr& spline) const {
  const auto it = m_zoneItemsBySpline.find(spline);
  if (it != m_zoneItemsBySpline.end()) {
    return it->second->second;
  } else {
    return nullptr;
  }
}

// SPDX-FileCopyrightText: 2026 project contributors
// SPDX-License-Identifier: GPL-3.0-only

#include "PageSizePresets.h"

#include <QFile>

#include <cmath>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

#include <toml++/toml.h>

namespace page_sizes {
namespace {
constexpr double kSizeMatchToleranceMm = 0.5;
}  // namespace

std::vector<Preset> load() {
  QFile file(QStringLiteral(":/page_sizes.toml"));
  if (!file.open(QIODevice::ReadOnly)) {
    throw std::runtime_error("Unable to read the built-in page size catalog: " + file.errorString().toStdString());
  }

  const QByteArray contents = file.readAll();
  if (file.error() != QFileDevice::NoError) {
    throw std::runtime_error("Unable to read the built-in page size catalog: " + file.errorString().toStdString());
  }

  toml::table document;
  try {
    document = toml::parse(std::string_view(contents.constData(), static_cast<std::size_t>(contents.size())));
  } catch (const toml::parse_error& parseError) {
    throw std::runtime_error("Invalid built-in page size catalog: " + std::string(parseError.what()));
  }

  const toml::array* sizes = document["paper_size"].as_array();
  if (!sizes) {
    throw std::runtime_error("The built-in page size catalog has no paper_size array.");
  }

  std::vector<Preset> presets;
  presets.reserve(sizes->size());
  for (const toml::node& node : *sizes) {
    const toml::table* entry = node.as_table();
    if (!entry) {
      throw std::runtime_error("The built-in page size catalog contains an invalid entry.");
    }

    const toml::node* nameNode = entry->get("name");
    const toml::node* widthNode = entry->get("width_mm");
    const toml::node* heightNode = entry->get("height_mm");
    const auto name = nameNode ? nameNode->value<std::string>() : std::nullopt;
    const auto width = widthNode ? widthNode->value<double>() : std::nullopt;
    const auto height = heightNode ? heightNode->value<double>() : std::nullopt;
    if (!name || name->empty() || !width || !height || !std::isfinite(*width) || !std::isfinite(*height)
        || *width <= 0.0 || *height <= 0.0) {
      throw std::runtime_error("The built-in page size catalog contains an invalid paper size.");
    }

    presets.push_back({QString::fromStdString(*name), QSizeF(*width, *height)});
  }
  return presets;
}

bool sizesMatch(const QSizeF& lhs, const QSizeF& rhs) {
  return std::abs(lhs.width() - rhs.width()) <= kSizeMatchToleranceMm
         && std::abs(lhs.height() - rhs.height()) <= kSizeMatchToleranceMm;
}

QString matchingName(const QSizeF& sizeMm) {
  static const std::vector<Preset> presets(load());
  for (const Preset& preset : presets) {
    if (sizesMatch(sizeMm, preset.sizeMm)
        || sizesMatch(sizeMm, QSizeF(preset.sizeMm.height(), preset.sizeMm.width()))) {
      return preset.name;
    }
  }
  return {};
}
}  // namespace page_sizes

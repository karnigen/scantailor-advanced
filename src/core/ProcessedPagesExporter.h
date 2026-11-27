// SPDX-FileCopyrightText: 2026 ScanTailor Advanced contributors
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_CORE_PROCESSEDPAGESEXPORTER_H_
#define SCANTAILOR_CORE_PROCESSEDPAGESEXPORTER_H_

#include <QString>
#include <QStringList>

#include <functional>

class OutputFileNameGenerator;
class PageSequence;

class ProcessedPagesExporter {
 public:
  using ProgressCallback = std::function<bool(int completed, int total)>;

  static QStringList supportedImageFormats();

  static bool exportImages(const PageSequence& pages,
                           const OutputFileNameGenerator& fileNameGenerator,
                           const QString& outputDirectory,
                           const QString& format,
                           const ProgressCallback& progress,
                           QString* errorMessage);

  static bool exportPdf(const PageSequence& pages,
                        const OutputFileNameGenerator& fileNameGenerator,
                        const QString& outputFilePath,
                        const ProgressCallback& progress,
                        QString* errorMessage);
};

#endif  // SCANTAILOR_CORE_PROCESSEDPAGESEXPORTER_H_

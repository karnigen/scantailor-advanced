// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_CORE_OPENGLSUPPORT_H_
#define SCANTAILOR_CORE_OPENGLSUPPORT_H_

#include <QString>

class OpenGLSupport {
 public:
  /**
   * \brief Returns true if OpenGL support is present and provides the necessary features.
   */
  static bool supported();

  /**
   * \brief Returns the device name (GL_RENDERER) associated with the default OpenGL context.
   *
   * \note This function has to be called from the GUI thread only.
   */
  static QString deviceName();
};


#endif

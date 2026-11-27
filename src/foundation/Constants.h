// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_FOUNDATION_CONSTANTS_H_
#define SCANTAILOR_FOUNDATION_CONSTANTS_H_

namespace constants {
extern const double PI;

extern const double SQRT_2;

/**
 * angle_rad = angle_deg * RED2RAD
 */
extern const double DEG2RAD;

/**
 * angle_deg = angle_rad * RAD2DEG
 */
extern const double RAD2DEG;

/**
 * mm = inch * INCH2MM
 */
extern const double INCH2MM;

/**
 * inch = mm * MM2INCH
 */
extern const double MM2INCH;

/**
 * dots_per_meter = dots_per_inch * DPI2DPM
 */
extern const double DPI2DPM;

/**
 * dots_per_inch = dots_per_meter * DPM2DPI
 */
extern const double DPM2DPI;
}  // namespace constants
#endif

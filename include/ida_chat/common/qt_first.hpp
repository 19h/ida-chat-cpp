/**
 * @file qt_first.hpp
 * @brief Include Qt headers BEFORE any IDA headers.
 *
 * This header MUST be included at the very top of any .cpp file that
 * uses both Qt and IDA headers, to avoid symbol conflicts.
 *
 * Both Qt and IDA define:
 *   - qstrlen()
 *   - qstrncmp()
 *   - qsnprintf()
 *   - qvsnprintf()
 *   - qstrdup()
 *   - qstrncpy()
 *
 * By including Qt headers first, the Qt versions are defined, and
 * we can suppress the redefinition errors when IDA headers are included.
 */

#pragma once

// Include the main Qt headers that define the conflicting symbols
// (qbytearrayalgorithms.h is pulled in transitively)
#include <QString>
#include <QByteArray>
#include <QObject>
#include <QWidget>

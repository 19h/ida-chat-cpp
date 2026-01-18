/**
 * @file qt_fwd.hpp
 * @brief Forward declarations for Qt classes, compatible with QT_NAMESPACE.
 *
 * IDA uses QT_NAMESPACE=QT, so all Qt classes are in the QT:: namespace.
 * This header provides forward declarations that work whether or not
 * QT_NAMESPACE is defined.
 */

#pragma once

// When QT_NAMESPACE is defined, Qt wraps everything in that namespace
// and provides "using namespace QT;" so both QT::QWidget and QWidget work.
// But forward declarations need to be in the right namespace.

#ifdef QT_NAMESPACE
#define QT_FWD_NAMESPACE_BEGIN namespace QT_NAMESPACE {
#define QT_FWD_NAMESPACE_END }
#else
#define QT_FWD_NAMESPACE_BEGIN
#define QT_FWD_NAMESPACE_END
#endif

QT_FWD_NAMESPACE_BEGIN

// Core
class QObject;
class QString;
class QByteArray;
class QTimer;
class QThread;
class QMutex;
class QWaitCondition;
class QEvent;

// GUI
class QWidget;
class QFrame;
class QLabel;
class QLineEdit;
class QTextEdit;
class QPlainTextEdit;
class QPushButton;
class QRadioButton;
class QButtonGroup;
class QScrollArea;
class QStackedWidget;
class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QSpacerItem;
class QPainter;
class QColor;
class QFont;
class QPixmap;
class QImage;
class QIcon;
class QKeyEvent;
class QMouseEvent;
class QWheelEvent;
class QResizeEvent;
class QPaintEvent;
class QCloseEvent;

QT_FWD_NAMESPACE_END

// Make names available globally when namespace is used
#ifdef QT_NAMESPACE
using QT_NAMESPACE::QObject;
using QT_NAMESPACE::QString;
using QT_NAMESPACE::QByteArray;
using QT_NAMESPACE::QTimer;
using QT_NAMESPACE::QThread;
using QT_NAMESPACE::QMutex;
using QT_NAMESPACE::QWaitCondition;
using QT_NAMESPACE::QEvent;
using QT_NAMESPACE::QWidget;
using QT_NAMESPACE::QFrame;
using QT_NAMESPACE::QLabel;
using QT_NAMESPACE::QLineEdit;
using QT_NAMESPACE::QTextEdit;
using QT_NAMESPACE::QPlainTextEdit;
using QT_NAMESPACE::QPushButton;
using QT_NAMESPACE::QRadioButton;
using QT_NAMESPACE::QButtonGroup;
using QT_NAMESPACE::QScrollArea;
using QT_NAMESPACE::QStackedWidget;
using QT_NAMESPACE::QVBoxLayout;
using QT_NAMESPACE::QHBoxLayout;
using QT_NAMESPACE::QGridLayout;
using QT_NAMESPACE::QSpacerItem;
using QT_NAMESPACE::QPainter;
using QT_NAMESPACE::QColor;
using QT_NAMESPACE::QFont;
using QT_NAMESPACE::QPixmap;
using QT_NAMESPACE::QImage;
using QT_NAMESPACE::QIcon;
using QT_NAMESPACE::QKeyEvent;
using QT_NAMESPACE::QMouseEvent;
using QT_NAMESPACE::QWheelEvent;
using QT_NAMESPACE::QResizeEvent;
using QT_NAMESPACE::QPaintEvent;
using QT_NAMESPACE::QCloseEvent;
#endif

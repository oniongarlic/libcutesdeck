#ifndef CUTESDECK_GLOBAL_H
#define CUTESDECK_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(CUTESDECK_LIBRARY)
#  define CUTESDECK_EXPORT Q_DECL_EXPORT
#else
#  define CUTESDECK_EXPORT Q_DECL_IMPORT
#endif

#endif // CUTESDECK_GLOBAL_H

#pragma once

#include <QtGlobal>

#if defined(CORE_LIBRARY)
#  define CORE_EXPORT Q_DECL_EXPORT
#elif defined(CORE_STATIC_LIBRARY)
#  define CORE_EXPORT
#else
#  define CORE_EXPORT Q_DECL_IMPORT
#endif

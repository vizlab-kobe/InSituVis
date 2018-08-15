#pragma once
#if defined( KVS_SUPPORT_OSMESA )
#include <kvs/osmesa/Screen>
namespace InSituVis { typedef kvs::osmesa::Screen Screen; }
#elif defined( KVS_SUPPORT_EGL )
#include <kvs/egl/Screen>
namespace InSituVis { typedef kvs::egl::Screen Screen; }
#else
#error "KVS with KVS_SUPPORT_OSMESA or KVS_SUPPORT_EGL needs to be used for InSituVis."
#endif

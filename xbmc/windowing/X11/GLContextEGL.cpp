/*
 *      Copyright (C) 2005-2014 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#include "system.h"

// always define GL_GLEXT_PROTOTYPES before include gl headers
#if !defined(GL_GLEXT_PROTOTYPES)
  #define GL_GLEXT_PROTOTYPES
#endif

#include <clocale>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>

#include "GLContextEGL.h"
#include "utils/log.h"

#define EGL_NO_CONFIG (EGLConfig)0

CGLContextEGL::CGLContextEGL(Display *dpy) : CGLContext(dpy)
{
  m_extPrefix = "EGL_";
  m_eglDisplay = EGL_NO_DISPLAY;
  m_eglSurface = EGL_NO_SURFACE;
  m_eglContext = EGL_NO_CONTEXT;
  m_eglConfig = EGL_NO_CONFIG;
}

CGLContextEGL::~CGLContextEGL()
{
  Destroy();
}

bool CGLContextEGL::Refresh(bool force, int screen, Window glWindow, bool &newContext)
{
  m_sync.cont = 0;

  // refresh context
  if (m_eglContext && !force)
  {
    if (m_eglSurface == EGL_NO_SURFACE)
    {
      m_eglSurface = eglCreateWindowSurface(m_eglDisplay, m_eglConfig, glWindow, NULL);
      if (m_eglSurface == EGL_NO_SURFACE)
      {
        CLog::Log(LOGERROR, "failed to create EGL window surface %d\n", eglGetError());
        return false;
      }
    }

    CLog::Log(LOGDEBUG, "CWinSystemX11::RefreshEGLContext: refreshing context");
    eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglMakeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface, m_eglContext);
    return true;
  }

  // create context
  bool retVal = false;

  if (m_eglDisplay == EGL_NO_DISPLAY)
  {
    m_eglDisplay = eglGetDisplay((EGLNativeDisplayType)m_dpy);
    if (m_eglDisplay == EGL_NO_DISPLAY)
    {
      CLog::Log(LOGERROR, "failed to get egl display\n");
      return false;
    }
    if (!eglInitialize(m_eglDisplay, NULL, NULL))
    {
      CLog::Log(LOGERROR, "failed to initialize egl display\n");
      return false;
    }
  }

  XVisualInfo vMask;
  XVisualInfo *vInfo = nullptr;
  int availableVisuals    = 0;
  vMask.screen = screen;
  XWindowAttributes winAttr;

  if (XGetWindowAttributes(m_dpy, glWindow, &winAttr))
  {
    vMask.visualid = XVisualIDFromVisual(winAttr.visual);
    vInfo = XGetVisualInfo(m_dpy, VisualScreenMask | VisualIDMask, &vMask, &availableVisuals);
    if (!vInfo)
    {
      CLog::Log(LOGWARNING, "Failed to get VisualInfo of visual 0x%x", (unsigned) vMask.visualid);
    }
    else if(!IsSuitableVisual(vInfo))
    {
      CLog::Log(LOGWARNING, "Visual 0x%x of the window is not suitable, looking for another one...",
                (unsigned) vInfo->visualid);
      XFree(vInfo);
      vInfo = nullptr;
    }
  }
  else
    CLog::Log(LOGWARNING, "Failed to get window attributes");

  if (vInfo)
  {
    CLog::Log(LOGNOTICE, "Using visual 0x%x", (unsigned) vInfo->visualid);

    if (m_eglContext)
    {
      eglMakeCurrent(m_eglContext, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
      eglDestroyContext(m_eglDisplay, m_eglContext);
      m_eglContext = EGL_NO_CONTEXT;

      if (m_eglSurface)
      {
        eglDestroySurface(m_eglDisplay, m_eglSurface);
        m_eglSurface = EGL_NO_SURFACE;
      }
      eglTerminate(m_eglDisplay);
      m_eglDisplay = EGL_NO_DISPLAY;
      XSync(m_dpy, FALSE);
      newContext = true;
    }

    m_eglDisplay = eglGetDisplay((EGLNativeDisplayType)m_dpy);
    if (m_eglDisplay == EGL_NO_DISPLAY)
    {
      CLog::Log(LOGERROR, "failed to get egl display");
      return false;
    }
    if (!eglInitialize(m_eglDisplay, NULL, NULL))
    {
      CLog::Log(LOGERROR, "failed to initialize egl\n");
      return false;
    }

    if (!eglBindAPI(EGL_OPENGL_API))
    {
      CLog::Log(LOGERROR, "failed to initialize egl");
      XFree(vInfo);
      return false;
    }

    if (m_eglConfig == EGL_NO_CONFIG)
    {
      m_eglConfig = GetEGLConfig(m_eglDisplay, vInfo);
    }

    if (m_eglConfig == EGL_NO_CONFIG)
    {
      CLog::Log(LOGERROR, "failed to get eglconfig for visual id");
      XFree(vInfo);
      return false;
    }

    if (m_eglSurface == EGL_NO_SURFACE)
    {
      m_eglSurface = eglCreateWindowSurface(m_eglDisplay, m_eglConfig, glWindow, NULL);
      if (m_eglSurface == EGL_NO_SURFACE)
      {
        CLog::Log(LOGERROR, "failed to create EGL window surface %d", eglGetError());
        XFree(vInfo);
        return false;
      }
    }

    EGLint contextAttributes[] =
    {
      EGL_CONTEXT_CLIENT_VERSION, 2,
      EGL_NONE
    };
    m_eglContext = eglCreateContext(m_eglDisplay, m_eglConfig, EGL_NO_CONTEXT, contextAttributes);
    if (m_eglContext == EGL_NO_CONTEXT)
    {
      CLog::Log(LOGERROR, "failed to create EGL context\n");
      return false;
    }

    if (!eglMakeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface, m_eglContext))
    {
      CLog::Log(LOGERROR, "Failed to make context current %p %p %p\n", m_eglDisplay, m_eglSurface, m_eglContext);
      return false;
    }
    XFree(vInfo);
    retVal = true;
  }
  else
  {
    CLog::Log(LOGERROR, "EGL Error: vInfo is NULL!");
  }

  eglGetSyncValuesCHROMIUM = (PFNEGLGETSYNCVALUESCHROMIUMPROC)eglGetProcAddress("eglGetSyncValuesCHROMIUM");
  return retVal;
}

void CGLContextEGL::Destroy()
{
  if (m_eglContext)
  {
    glFinish();
    eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(m_eglDisplay, m_eglContext);
    m_eglContext = EGL_NO_CONTEXT;
  }

  if (m_eglSurface)
  {
    eglDestroySurface(m_eglDisplay, m_eglSurface);
    m_eglSurface = EGL_NO_SURFACE;
  }

  if (m_eglDisplay)
  {
    eglTerminate(m_eglDisplay);
    m_eglDisplay = EGL_NO_DISPLAY;
  }
}

void CGLContextEGL::Detach()
{
  if (m_eglContext)
  {
    glFinish();
    eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  }
  if (m_eglSurface)
  {
    eglDestroySurface(m_eglDisplay, m_eglSurface);
    m_eglSurface = EGL_NO_SURFACE;
  }
}

bool CGLContextEGL::IsSuitableVisual(XVisualInfo *vInfo)
{
  EGLConfig config = GetEGLConfig(m_eglDisplay, vInfo);
  if (config == EGL_NO_CONFIG)
  {
    CLog::Log(LOGERROR, "Failed to determine egl config for visual info");
    return false;
  }
  EGLint value;

  if (!eglGetConfigAttrib(m_eglDisplay, config, EGL_RED_SIZE, &value) || value < 8)
    return false;
  if (!eglGetConfigAttrib(m_eglDisplay, config, EGL_GREEN_SIZE, &value) || value < 8)
    return false;
  if (!eglGetConfigAttrib(m_eglDisplay, config, EGL_BLUE_SIZE, &value) || value < 8)
    return false;

  return true;
}

EGLConfig CGLContextEGL::GetEGLConfig(EGLDisplay eglDisplay, XVisualInfo *vInfo)
{
  EGLint numConfigs;

  if (!eglGetConfigs(eglDisplay, nullptr, 0, &numConfigs))
  {
    CLog::Log(LOGERROR, "Failed to query number of egl configs");
    return EGL_NO_CONFIG;
  }
  if (numConfigs == 0)
  {
    CLog::Log(LOGERROR, "No suitable egl configs found");
    return EGL_NO_CONFIG;
  }

  EGLConfig *eglConfigs;
  eglConfigs = (EGLConfig*)malloc(numConfigs * sizeof(EGLConfig));
  if (!eglConfigs)
  {
    CLog::Log(LOGERROR, "eglConfigs malloc failed");
    return EGL_NO_CONFIG;
  }
  EGLConfig eglConfig = EGL_NO_CONFIG;
  if (!eglGetConfigs(eglDisplay, eglConfigs, numConfigs, &numConfigs))
  {
    CLog::Log(LOGERROR, "Failed to query egl configs");
    goto Exit;
  }
  for (EGLint i = 0;i < numConfigs;++i)
  {
    EGLint value;
    if (!eglGetConfigAttrib(eglDisplay, eglConfigs[i], EGL_NATIVE_VISUAL_ID, &value))
    {
      CLog::Log(LOGERROR, "Failed to query EGL_NATIVE_VISUAL_ID for egl config.");
      break;
    }
    if (value == (EGLint)vInfo->visualid) {
      eglConfig = eglConfigs[i];
      break;
    }
  }

Exit:
  free(eglConfigs);
  return eglConfig;
}

void CGLContextEGL::SetVSync(bool enable)
{
  eglSwapInterval(m_eglDisplay, enable ? 1 : 0);
}

void CGLContextEGL::SwapBuffers()
{
  if ((m_eglDisplay == EGL_NO_DISPLAY) || (m_eglSurface == EGL_NO_SURFACE))
    return;

  uint64_t ust1, ust2;
  uint64_t msc1, msc2;
  uint64_t sbc1, sbc2;

  eglGetSyncValuesCHROMIUM(m_eglDisplay, m_eglSurface, &ust1, &msc1, &sbc1);

  eglSwapBuffers(m_eglDisplay, m_eglSurface);

  eglGetSyncValuesCHROMIUM(m_eglDisplay, m_eglSurface, &ust2, &msc2, &sbc2);

  if ((msc1 - m_sync.msc1) > 2)
  {
    m_sync.cont = 0;
    CLog::Log(LOGDEBUG, "CGLContextEGL::SwapBuffers: reset: msc: %ld, last-msc: %ld", msc1, m_sync.msc1);
  }

  struct timespec nowTs;
  uint64_t now;
  clock_gettime(CLOCK_MONOTONIC, &nowTs);
  now = nowTs.tv_sec * 1000000000 + nowTs.tv_nsec;

  // we want to block in SwapBuffers
  // if a vertical retrace occurs 5 times in a row outside
  // of this function, we take action
  if (m_sync.cont < 5)
  {
    if ((msc1 - m_sync.msc1) == 2)
    {
      m_sync.cont = 0;
    }
    else if ((msc1 - m_sync.msc1) == 1)
    {
      m_sync.interval = (ust1 - m_sync.ust1) / (msc1 - m_sync.msc1);
      m_sync.cont++;
    }
  }
  else if ((m_sync.cont == 5) && (msc2 == msc1))
  {
    // if no vertical retrace has occurred in eglSwapBuffers,
    // sleep until next vertical retrace
    uint64_t sleeptime = m_sync.interval - (now / 1000 - ust2);
    usleep(sleeptime);
    m_sync.cont++;
    msc2++;
  }
  else if ((m_sync.cont > 5) && (msc2 == m_sync.msc2))
  {
    // sleep until next vertical retrace
    // this avoids blocking outside of this function
    uint64_t sleeptime = m_sync.interval - (now / 1000 - ust2);
    usleep(sleeptime);
    msc2++;
  }

  m_sync.ust1 = ust1;
  m_sync.ust2 = ust2;
  m_sync.msc1 = msc1;
  m_sync.msc2 = msc2;
  m_sync.sbc2 = sbc2;

}

uint64_t CGLContextEGL::GetFrameLatencyAdjustment()
{
  struct timespec nowTs;
  uint64_t now;
  clock_gettime(CLOCK_MONOTONIC, &nowTs);
  now = nowTs.tv_sec * 1000000000 + nowTs.tv_nsec;
  now /= 1000;

  uint64_t interval = (m_sync.cont > 5) ? m_sync.interval : m_sync.ust2 - m_sync.ust1;
  if (interval == 0)
    return 0;

  if (now < m_sync.ust2)
  {
    return 0;
  }

  uint64_t ret = now - m_sync.ust2;
  while (ret > interval)
    ret -= interval;

  return ret;
}

void CGLContextEGL::QueryExtensions()
{
  std::string extensions = eglQueryString(m_eglDisplay, EGL_EXTENSIONS);
  m_extensions = std::string(" ") + extensions + " ";

  CLog::Log(LOGDEBUG, "EGL_EXTENSIONS:%s", m_extensions.c_str());
}

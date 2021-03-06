/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoSyncAndroid.h"
#include "WinSystemAndroidGLESContext.h"
#include "utils/log.h"
#include "threads/SingleLock.h"
#include "platform/android/activity/XBMCApp.h"

std::unique_ptr<CWinSystemBase> CWinSystemBase::CreateWinSystem()
{
  std::unique_ptr<CWinSystemBase> winSystem(new CWinSystemAndroidGLESContext());
  return winSystem;
}

bool CWinSystemAndroidGLESContext::InitWindowSystem()
{
  if (!CWinSystemAndroid::InitWindowSystem())
  {
    return false;
  }

  if (!m_pGLContext.CreateDisplay(m_nativeDisplay,
                                  EGL_OPENGL_ES2_BIT,
                                  EGL_OPENGL_ES_API))
  {
    return false;
  }

  const EGLint contextAttribs[] =
  {
    EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE
  };

  if (!m_pGLContext.CreateContext(contextAttribs))
  {
    return false;
  }

  return true;
}

bool CWinSystemAndroidGLESContext::CreateNewWindow(const std::string& name,
                                               bool fullScreen,
                                               RESOLUTION_INFO& res)
{
  m_pGLContext.DestroySurface();

  if (!CWinSystemAndroid::CreateNewWindow(name, fullScreen, res))
  {
    return false;
  }

  if (!m_pGLContext.CreateSurface(m_nativeWindow))
  {
    return false;
  }

  if (!m_pGLContext.BindContext())
  {
    return false;
  }

  if (!m_delayDispReset)
  {
    CSingleLock lock(m_resourceSection);
    // tell any shared resources
    for (std::vector<IDispResource *>::iterator i = m_resources.begin(); i != m_resources.end(); ++i)
      (*i)->OnResetDisplay();
  }

  return true;
}

bool CWinSystemAndroidGLESContext::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
{
  CRenderSystemGLES::ResetRenderSystem(newWidth, newHeight);
  return true;
}

bool CWinSystemAndroidGLESContext::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays)
{
  CreateNewWindow("", fullScreen, res);
  CRenderSystemGLES::ResetRenderSystem(res.iWidth, res.iHeight);
  return true;
}

void CWinSystemAndroidGLESContext::SetVSyncImpl(bool enable)
{
  // We use Choreographer for timing
  m_pGLContext.SetVSync(false);
}

void CWinSystemAndroidGLESContext::PresentRenderImpl(bool rendered)
{
  if (m_delayDispReset && m_dispResetTimer.IsTimePast())
  {
    m_delayDispReset = false;
    CSingleLock lock(m_resourceSection);
    // tell any shared resources
    for (std::vector<IDispResource *>::iterator i = m_resources.begin(); i != m_resources.end(); ++i)
      (*i)->OnResetDisplay();
  }
  // Ignore EGL_BAD_SURFACE: It seems to happen during/after mode changes, but
  // we can't actually do anything about it
  if (rendered && !m_pGLContext.TrySwapBuffers() && eglGetError() != EGL_BAD_SURFACE)
  {
    CEGLUtils::LogError("eglSwapBuffers failed");
    throw std::runtime_error("eglSwapBuffers failed");
  }
  CXBMCApp::get()->WaitVSync(1000);
}

EGLDisplay CWinSystemAndroidGLESContext::GetEGLDisplay() const
{
  return m_pGLContext.GetEGLDisplay();
}

EGLSurface CWinSystemAndroidGLESContext::GetEGLSurface() const
{
  return m_pGLContext.GetEGLSurface();
}

EGLContext CWinSystemAndroidGLESContext::GetEGLContext() const
{
  return m_pGLContext.GetEGLContext();
}

EGLConfig  CWinSystemAndroidGLESContext::GetEGLConfig() const
{
  return m_pGLContext.GetEGLConfig();
}

std::unique_ptr<CVideoSync> CWinSystemAndroidGLESContext::GetVideoSync(void *clock)
{
  std::unique_ptr<CVideoSync> pVSync(new CVideoSyncAndroid(clock));
  return pVSync;
}


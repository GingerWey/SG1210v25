//-----------------------------------------------------------------------------
/*
 File        : GFormPlatform.h
 Version     : V1.00
 By          : Wey. Silver Grid

 Description : Platform abstraction layer for GForm system.
               Wraps std::mutex (simulator) vs osMutexId (MCU) behind a
               uniform Lock + ScopedLock interface.

 Date        : 2026.06.24
*/
//-----------------------------------------------------------------------------
#ifndef GUI_GFORM_PLATFORM_H
#define GUI_GFORM_PLATFORM_H

//-----------------------------------------------------------------------------
// Platform detection: __vmSIMULATOR__ is defined by the Visual Studio project.
// On real device (Keil MDK / ARMCC), it is not defined.
//-----------------------------------------------------------------------------

#if !defined(__vmSIMULATOR__)
  // ── Real device: CMSIS-RTOS v2 mutex ──────────────────────────────────
  #include <cmsis_os2.h>
  #include <cmsis_os.h>

  namespace gform {
  namespace platform {

  class Lock {
  public:
      Lock() {
          static const osMutexAttr_t attr = {
              nullptr,                             // name
              osMutexRecursive | osMutexPrioInherit,
              nullptr,                             // cb_mem
              0U                                    // cb_size
          };
          m_mutex = osMutexNew(&attr);
      }
      ~Lock() {
          if (m_mutex) osMutexDelete(m_mutex);
      }

      void Acquire() {
          if (m_mutex) osMutexWait(m_mutex, osWaitForever);
      }
      void Release() {
          if (m_mutex) osMutexRelease(m_mutex);
      }

      Lock(const Lock&)            = delete;
      Lock& operator=(const Lock&) = delete;

  private:
      osMutexId_t m_mutex = nullptr;
  };

  }  // namespace platform
  }  // namespace gform

#else
  // ── Simulator: no-op lock (single-threaded, avoids static init crash) ─
  namespace gform {
  namespace platform {

  class Lock {
  public:
      void Acquire() { }
      void Release() { }
  };

  }  // namespace platform
  }  // namespace gform

#endif  // __vmSIMULATOR__

//-----------------------------------------------------------------------------
// RAII scoped lock — usable from both platforms
//-----------------------------------------------------------------------------
namespace gform {

class ScopedLock {
public:
    explicit ScopedLock(platform::Lock& lock) : m_lock(lock) {
        m_lock.Acquire();
    }
    ~ScopedLock() {
        m_lock.Release();
    }
    ScopedLock(const ScopedLock&)            = delete;
    ScopedLock& operator=(const ScopedLock&) = delete;

private:
    platform::Lock& m_lock;
};

}  // namespace gform

//-----------------------------------------------------------------------------
#endif  // GUI_GFORM_PLATFORM_H

#!/usr/bin/env python

# pot_cap.py
# 2016-09-26
# Public Domain

import time
import pigpio

class reader:

   def __init__(self, pi, gpio, drain_ms=1.0, timeout_s=1.0):


      self.pi = pi
      self.gpio = gpio
      self.drain_ms = drain_ms
      self.timeout_s = timeout_s
      self._timeout_us = timeout_s * 1000000.0


      self._sid = pi.store_script(
         b't sta v0 m p0 r t sub v0 div 2 sta p3 add v0 sta p2 inr p1 ')

      s = pigpio.PI_SCRIPT_INITING

      while s == pigpio.PI_SCRIPT_INITING:
         s, p = self.pi.script_status(self._sid)
         time.sleep(0.001)

      self._cb = pi.callback(gpio, pigpio.EITHER_EDGE, self._cbf)

   def _cbf(self, g, l, t):
      """
      Record the tick when the GPIO becomes high.
      """
      if l == 1:
         self._end = t

   def read(self):
      """
      Triggers and returns a reading.

      A tuple of the reading status (True for a good reading,
      False for a timeout or outlier), the reading, and the
      range are returned.

      The reading is the number of microseconds taken for the
      capacitor to charge.  The range is a measure of how
      accurately the start of recharge was measured as +/-
      microseconds.
      """

      timeout = time.time() + self.timeout_s

      self.pi.write(self.gpio, 0)

      time.sleep(self.drain_ms/1000.0)

      while (self.pi.read(self.gpio) != 0) and (time.time() < timeout):
         time.sleep(0.001)

      self._end = None

      self.pi.run_script(self._sid, [self.gpio, 0])

      while time.time() < timeout:
         s, p = self.pi.script_status(self._sid)
         if p[1]:
            break
         time.sleep(0.001)

      # p[2] is start charge tick from script
      # p[3] is +/- range from script

      if time.time() < timeout:

         _start = p[2]
         if _start < 0:
            _start += (1<<32)

         while self._end is None and time.time() < timeout:
            time.sleep(0.001)

         if self._end is not None:
            diff = pigpio.tickDiff(_start, self._end)
            # Discard obvious outliers
            if (diff < self._timeout_us) and (p[3] < 6):
               return True, diff, p[3]
            else:
               return False, diff, p[3]

      return False, 0, 0

   def cancel(self):
      """
      Cancels the reader and releases resources.
      """
      self.pi.delete_script(self._sid)
      self._cb.cancel()



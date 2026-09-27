from py_yapt import ReadbackTarget, ReadbackState
from yapt.screenshot_writer import write_readback_data
from PySide6.QtCore import QDate, QDateTime, QStandardPaths
import os
import traceback


class PendingScreenshot:
    def __init__(self, file_path, readback, state):
        self.file_path = file_path
        self.readback = readback
        self.last_logged_state = state
        self.ticks_waited = 0
        self.time_waited = 0.0


class ScreenshotController:

    MAX_PENDING_SCREENSHOTS = 8
    DEFAULT_INTERVAL_SECONDS = 1.0
    POLL_LOG_INTERVAL_TICKS = 60

    def __init__(self, app):
        self._app = app
        self._pending = []
        self._status_listeners = set()

        self._continuous = False
        self._interval = self.DEFAULT_INTERVAL_SECONDS
        self._time_since_last_capture = 0.0

        self._output_directory = self._default_output_directory()
        self._name_prefix = "screenshot"
        self._capture_index = 0

        self._is_ticking = False

    def shutdown(self):
        self.stop_continuous()
        self._stop_ticking()
        #the readbacks reference renderer owned resources, so they have to go before the renderer does
        for pending in self._pending:
            pending.readback.release()
        self._pending.clear()
        self._status_listeners.clear()

    #status listeners are called with a single string describing what the controller just did
    def add_status_listener(self, l):
        self._status_listeners.add(l)

    def remove_status_listener(self, l):
        self._status_listeners.discard(l)

    def notify_status_listeners(self, message):
        for l in list(self._status_listeners):
            l(message)

    #settings
    def get_interval(self):
        return self._interval

    def set_interval(self, interval_in_seconds):
        self._interval = max(interval_in_seconds, 0.0)

    def get_output_directory(self):
        return self._output_directory

    def set_output_directory(self, directory):
        self._output_directory = directory

    def get_name_prefix(self):
        return self._name_prefix

    def set_name_prefix(self, prefix):
        self._name_prefix = prefix

    def get_pending_count(self):
        return len(self._pending)

    #capturing
    def take_screenshot(self):
        self._issue_readback()

    def is_continuous_running(self):
        return self._continuous

    def start_continuous(self):
        if self._continuous:
            return
        self._continuous = True
        #capture immediately so that the first screenshot doesn't have to wait out a full interval
        self._time_since_last_capture = self._interval
        self._start_ticking()
        self._log(f"continuous screenshots started (every {self._interval:.3f}s)")

    def stop_continuous(self):
        if not self._continuous:
            return
        self._continuous = False
        self._log("continuous screenshots stopped")

    def tick(self, dt):
        self._poll_pending(dt)

        if self._continuous:
            self._time_since_last_capture += dt
            if self._time_since_last_capture >= self._interval:
                #subtract instead of clearing so that the capture rate doesn't drift with the tick rate
                self._time_since_last_capture -= self._interval
                #...but don't let the accumulator run away if the interval is shorter than a single tick
                self._time_since_last_capture = min(self._time_since_last_capture, self._interval)
                self._issue_readback()

        #nothing left to drive, stop listening until the next capture is requested
        if not self._continuous and len(self._pending) == 0:
            self._stop_ticking()

    #internals
    @staticmethod
    def _default_output_directory():
        #the desktop location follows windows folder redirection (e.g. onedrive), unlike a plain ~/Desktop
        desktop = QStandardPaths.writableLocation(QStandardPaths.DesktopLocation)
        if not desktop:
            desktop = os.path.join(os.path.expanduser("~"), "Desktop")
        date = QDate.currentDate().toString("yyyy-MM-dd")
        return os.path.normpath(os.path.join(desktop, "YaptScreenshots", date))

    def _start_ticking(self):
        if self._is_ticking:
            return
        self._app.add_tick_listener(self.tick)
        self._is_ticking = True

    def _stop_ticking(self):
        if not self._is_ticking:
            return
        self._app.remove_tick_listener(self.tick)
        self._is_ticking = False

    def _issue_readback(self):
        if len(self._pending) >= self.MAX_PENDING_SCREENSHOTS:
            self._log(f"skipped a capture, {len(self._pending)} readbacks are still pending")
            return

        renderer = self._app.get_renderer()
        if renderer == None:
            return

        readback = renderer.readback(ReadbackTarget.FinalColor)
        file_path = self._build_file_path()
        state = readback.getState()
        self._capture_index += 1
        self._pending.append(PendingScreenshot(file_path, readback, state))
        self._start_ticking()
        self._log(f"issued readback for '{os.path.basename(file_path)}' (state: {state})")

    def _poll_pending(self, dt):
        polled = []
        still_pending = []
        for pending in self._pending:
            pending.ticks_waited += 1
            pending.time_waited += dt

            state = pending.readback.getState()
            polled.append((pending, state))
            if state != ReadbackState.Ready and state != ReadbackState.Freed and state != ReadbackState.Failed:
                still_pending.append(pending)

        #drop the finished ones before reporting anything, so that listeners see the state as it is after the poll
        self._pending = still_pending

        for pending, state in polled:
            name = os.path.basename(pending.file_path)

            if state == ReadbackState.Ready:
                self._log(f"'{name}' ready after {pending.ticks_waited} ticks ({pending.time_waited:.3f}s)")
                self._store_image(pending)
            elif state == ReadbackState.Freed:
                self._log(f"'{name}' was freed before it became ready, dropping it")
            elif state == ReadbackState.Failed:
                self._log(f"'{name}' failed, dropping it")
            else:
                state_changed = state != pending.last_logged_state
                if state_changed or (pending.ticks_waited % self.POLL_LOG_INTERVAL_TICKS) == 0:
                    self._log(f"'{name}' polled, state: {state}, waited {pending.ticks_waited} ticks ({pending.time_waited:.3f}s)")
                    pending.last_logged_state = state

    def _store_image(self, pending):
        name = os.path.basename(pending.file_path)
        try:
            data = pending.readback.getData()
            if data == None:
                self._log(f"'{name}' has no data to write")
            else:
                description = write_readback_data(data, pending.file_path)
                self._log(f"wrote '{name}' ({description})")
        except Exception as e:
            self._log(f"failed to write '{name}': {e}")
            traceback.print_exc()
        finally:
            #the readback holds on to a gpu resource, so let go of it whether or not the write worked out
            pending.readback.release()

    def _build_file_path(self):
        timestamp = QDateTime.currentDateTime().toString("yyyyMMdd_hhmmsszzz")
        file_name = f"{self._name_prefix}_{self._capture_index:04d}_{timestamp}.png"
        return os.path.join(self._output_directory, file_name)

    def _log(self, message):
        print(f"[screenshot] {message}")
        self.notify_status_listeners(message)

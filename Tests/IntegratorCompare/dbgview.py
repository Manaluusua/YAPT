# Minimal OutputDebugStringA listener (DBWIN protocol), prints captured messages.
import ctypes, ctypes.wintypes as wt, sys, time

k32 = ctypes.WinDLL('kernel32', use_last_error=True)
PAGE_READWRITE=0x4; FILE_MAP_READ=0x4
INVALID_HANDLE_VALUE = wt.HANDLE(-1)

k32.CreateFileMappingW.restype = wt.HANDLE
k32.MapViewOfFile.restype = ctypes.c_void_p
k32.CreateEventW.restype = wt.HANDLE
k32.OpenEventW.restype = wt.HANDLE

buf_ready = k32.CreateEventW(None, False, True, "DBWIN_BUFFER_READY")
data_ready = k32.CreateEventW(None, False, False, "DBWIN_DATA_READY")
if not buf_ready or not data_ready:
    print("cannot create DBWIN events", ctypes.get_last_error()); sys.exit(1)

hmap = k32.CreateFileMappingW(INVALID_HANDLE_VALUE, None, PAGE_READWRITE, 0, 4096, "DBWIN_BUFFER")
if not hmap:
    print("cannot create DBWIN_BUFFER", ctypes.get_last_error()); sys.exit(1)
view = k32.MapViewOfFile(hmap, FILE_MAP_READ, 0, 0, 4096)

timeout = float(sys.argv[1]) if len(sys.argv) > 1 else 120.0
end = time.time() + timeout
print("[dbgview] listening", flush=True)
while time.time() < end:
    r = k32.WaitForSingleObject(data_ready, 500)
    if r != 0:
        k32.SetEvent(buf_ready)
        continue
    pid = ctypes.c_uint32.from_address(view).value
    msg = ctypes.string_at(view + 4)
    try: msg = msg.decode('utf-8', 'replace')
    except Exception: msg = repr(msg)
    print(f"[{pid}] {msg.rstrip()}", flush=True)
    k32.SetEvent(buf_ready)

"""Creates the EWMH window-manager atoms JUCE looks up only if they exist, then stays connected.

Bare Xvfb has no window manager, so those atoms are missing and opening a real window fails with BadAtom. Run this in
the background on the Xvfb display before muon_editor_cpu; it must stay connected, because Xvfb resets (and drops
the atoms) when its last client leaves.
    Xvfb :99 &  DISPLAY=:99 python3 x11_atoms.py &  sleep 2;  DISPLAY=:99 muon_editor_cpu 10
"""
import ctypes
import time

NAMES = """
_NET_WM_WINDOW_TYPE _NET_WM_WINDOW_TYPE_NORMAL _NET_WM_WINDOW_TYPE_DIALOG _NET_WM_WINDOW_TYPE_TOOLTIP
_NET_WM_WINDOW_TYPE_POPUP_MENU _NET_WM_WINDOW_TYPE_COMBO _NET_WM_STATE _NET_WM_STATE_SKIP_TASKBAR
_NET_WM_STATE_ABOVE _NET_WM_STATE_HIDDEN _NET_WM_STATE_FULLSCREEN _NET_WM_STATE_MAXIMIZED_VERT
_NET_WM_STATE_MAXIMIZED_HORZ _NET_WM_PID _NET_WM_NAME _NET_WM_ICON _NET_WM_PING _NET_FRAME_EXTENTS
_NET_ACTIVE_WINDOW _NET_SUPPORTED _NET_WM_USER_TIME _NET_WORKAREA _NET_CURRENT_DESKTOP _MOTIF_WM_HINTS
WM_PROTOCOLS WM_DELETE_WINDOW WM_STATE WM_CHANGE_STATE UTF8_STRING XdndAware XdndEnter XdndLeave XdndPosition
XdndStatus XdndDrop XdndFinished XdndSelection XdndTypeList XdndActionList XdndActionDescription
XdndActionCopy XdndActionPrivate CLIPBOARD TARGETS JUCE_SEL _XSETTINGS_SETTINGS KWM_WIN_DECORATION
WM_TAKE_FOCUS _KDE_NET_WM_WINDOW_TYPE_OVERRIDE _NET_WM_ALLOWED_ACTIONS _NET_WM_MOVERESIZE _WIN_HINTS
_XEMBED_INFO
""".split()

x = ctypes.cdll.LoadLibrary("libX11.so.6")
x.XOpenDisplay.restype = ctypes.c_void_p
x.XInternAtom.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int]
display = x.XOpenDisplay(None)
if not display:
    raise SystemExit("no X display")
for name in NAMES:
    x.XInternAtom(display, name.encode(), 0)
x.XFlush(display)
time.sleep(24 * 3600)

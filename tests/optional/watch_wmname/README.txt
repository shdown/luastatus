This thing watches X11 root window's WM_NAME property, like dwm does it, and
prints it to stdout, so that we can test the "dwm" barlib without launching dwm.

It requires libX11.

The tests are currently done manually.

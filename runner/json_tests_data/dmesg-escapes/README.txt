Unprintable characters in kernel logs, when read via /dev/kmsg, are
escaped as \xNN where NN the character's ascii code in hex. Those are
expected to be decoded to results, if they really are
printable. Kernel's idea of nonprintable is "c < 0x20 || c > 127".

The same applies to regexes from igt_emit_ignore_dmesg_regex(), which
also go through /dev/kmsg; \x5c must be decoded before the regex is
compiled so that the error line it matches is ignored.

# smbios (vendored)

`smbios.h` is the single-header distribution of Leon Freist's smbios library (version 0.1.0, commit `2eda79b`),
copied unchanged from its `header_only/smbios.h`. It is used internally only and not installed.

To update it, regenerate the header in the smbios repository (`python3 tools/amalgamate.py`), copy it here and update
the commit above. Do not edit `smbios.h` directly.

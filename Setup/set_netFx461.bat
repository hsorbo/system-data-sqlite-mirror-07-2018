@ECHO OFF

::
:: set_netFx461.bat --
::
:: Written by Joe Mistachkin.
:: Released to the public domain, use at your own risk!
::

IF NOT DEFINED ISNETFX2 (
  SET ISNETFX2=False
)

IF NOT DEFINED VCRUNTIME (
  SET VCRUNTIME=2015_VSU3
)

IF NOT DEFINED CONFIGURATION (
  SET CONFIGURATION=Release
)

IF NOT DEFINED PLATFORM (
  SET PLATFORM=Win32
)

IF NOT DEFINED PROCESSOR (
  SET PROCESSOR=x86
)

IF NOT DEFINED YEAR (
  SET YEAR=2015
)

IF NOT DEFINED FRAMEWORK (
  SET FRAMEWORK=netFx461
)

:end_of_file

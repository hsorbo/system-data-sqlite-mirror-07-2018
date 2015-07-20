@ECHO OFF

::
:: set_2015.bat --
::
:: Written by Joe Mistachkin.
:: Released to the public domain, use at your own risk!
::

SET NETFX20ONLY=
SET NETFX35ONLY=
SET NETFX40ONLY=
SET NETFX45ONLY=
SET NETFX451ONLY=
SET NETFX46ONLY=1

REM
REM HACK: Evidently, using MSBuild with Visual Studio 2015 requires some
REM       extra magic to make it recognize the "v140" platform toolset.
REM
SET BUILD_ARGS=/property:VisualStudioVersion=14.0

VERIFY > NUL

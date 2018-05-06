@ECHO OFF

::
:: set_common.bat --
::
:: Written by Joe Mistachkin.
:: Released to the public domain, use at your own risk!
::

IF NOT DEFINED APPID (
  SET APPID={{02E43EC2-6B1C-45B5-9E48-941C3E1B204A}
)

IF NOT DEFINED URL (
  SET URL=https://system.data.sqlite.org/
)

IF NOT DEFINED PUBLICKEY (
  SET PUBLICKEY=db937bc2d44ff139
)

IF NOT DEFINED BUILD_CONFIGURATIONS (
  IF DEFINED BUILD_DEBUG (
    SET BUILD_CONFIGURATIONS=Debug DebugNativeOnly Release ReleaseNativeOnly
  ) ELSE (
    SET BUILD_CONFIGURATIONS=Release ReleaseNativeOnly
  )
)

IF NOT DEFINED TEST_CONFIGURATIONS (
  IF DEFINED TEST_DEBUG (
    SET TEST_CONFIGURATIONS=Debug Release
  ) ELSE (
    SET TEST_CONFIGURATIONS=Release
  )
)

IF NOT DEFINED BAKE_CONFIGURATIONS (
  IF DEFINED BAKE_DEBUG (
    SET BAKE_CONFIGURATIONS=Debug DebugNativeOnly Release ReleaseNativeOnly
  ) ELSE (
    SET BAKE_CONFIGURATIONS=Release ReleaseNativeOnly
  )
)

IF NOT DEFINED RELEASE_CONFIGURATIONS (
  IF DEFINED RELEASE_DEBUG (
    SET RELEASE_CONFIGURATIONS=Debug DebugNativeOnly Release ReleaseNativeOnly
  ) ELSE (
    SET RELEASE_CONFIGURATIONS=Release ReleaseNativeOnly
  )
)

IF NOT DEFINED PLATFORMS (
  SET PLATFORMS=Win32 x64
)

IF NOT DEFINED PROCESSORS (
  SET PROCESSORS=x86 x64
)

IF NOT DEFINED FRAMEWORK2005 (
  SET FRAMEWORK2005=netFx20
)

IF NOT DEFINED FRAMEWORK2008 (
  SET FRAMEWORK2008=netFx35
)

IF NOT DEFINED FRAMEWORK2010 (
  SET FRAMEWORK2010=netFx40
)

IF NOT DEFINED FRAMEWORK2012 (
  SET FRAMEWORK2012=netFx45
)

IF NOT DEFINED FRAMEWORK2013 (
  SET FRAMEWORK2013=netFx451
  REM SET FRAMEWORK2013=netFx452
)

IF NOT DEFINED FRAMEWORK2015 (
  SET FRAMEWORK2015=netFx46
  REM SET FRAMEWORK2015=netFx461
  REM SET FRAMEWORK2015=netFx462
)

IF NOT DEFINED FRAMEWORK2017 (
  SET FRAMEWORK2017=netFx47
  REM SET FRAMEWORK2015=netFx471
  REM SET FRAMEWORK2015=netFx472
)

IF NOT DEFINED FRAMEWORKNETSTANDARD20 (
  SET FRAMEWORKNETSTANDARD20=netStandard20
)

IF DEFINED YEARS GOTO end_of_file

IF NOT DEFINED NOVS2005 (
  IF DEFINED VS2005SP (
    SET YEARS=%YEARS% 2005
  )
)

IF NOT DEFINED NOVS2008 (
  IF DEFINED VS2008SP (
    SET YEARS=%YEARS% 2008
  )
)

IF NOT DEFINED NOVS2010 (
  IF DEFINED VS2010SP (
    SET YEARS=%YEARS% 2010
  )
)

IF NOT DEFINED NOVS2012 (
  IF DEFINED VS2012SP (
    SET YEARS=%YEARS% 2012
  )
)

IF NOT DEFINED NOVS2013 (
  IF DEFINED VS2013SP (
    SET YEARS=%YEARS% 2013
  )
)

IF NOT DEFINED NOVS2015 (
  IF DEFINED VS2015SP (
    SET YEARS=%YEARS% 2015
  )
)

IF NOT DEFINED NOVS2017 (
  IF DEFINED VS2017SP (
    SET YEARS=%YEARS% 2017
  )
)

IF NOT DEFINED NONETSTANDARD20 (
  SET YEARS=%YEARS% NetStandard20
)

:end_of_file

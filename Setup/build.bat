@ECHO OFF

::
:: build.bat --
::
:: Wrapper Tool for MSBuild
::
:: Written by Joe Mistachkin.
:: Released to the public domain, use at your own risk!
::

SETLOCAL

REM SET __ECHO=ECHO
REM SET __ECHO2=ECHO
REM SET __ECHO3=ECHO
IF NOT DEFINED _AECHO (SET _AECHO=REM)
IF NOT DEFINED _CECHO (SET _CECHO=REM)
IF NOT DEFINED _VECHO (SET _VECHO=REM)

%_AECHO% Running %0 %*

SET DUMMY2=%3

IF DEFINED DUMMY2 (
  GOTO usage
)

SET ROOT=%~dp0\..
SET ROOT=%ROOT:\\=\%

%_VECHO% Root = '%ROOT%'

SET CONFIGURATION=%1

IF DEFINED CONFIGURATION (
  CALL :fn_UnquoteVariable CONFIGURATION
) ELSE (
  %_AECHO% No configuration specified, using default...
  SET CONFIGURATION=Release
)

%_VECHO% Configuration = '%CONFIGURATION%'

SET PLATFORM=%2

IF DEFINED PLATFORM (
  CALL :fn_UnquoteVariable PLATFORM
) ELSE (
  %_AECHO% No platform specified, using default...
  SET PLATFORM=Win32
)

%_VECHO% Platform = '%PLATFORM%'

SET BASE_CONFIGURATION=%CONFIGURATION%
SET BASE_CONFIGURATION=%BASE_CONFIGURATION:ManagedOnly=%
SET BASE_CONFIGURATION=%BASE_CONFIGURATION:NativeOnly=%

%_VECHO% BaseConfiguration = '%BASE_CONFIGURATION%'

SET TOOLS=%~dp0
SET TOOLS=%TOOLS:~0,-1%

%_VECHO% Tools = '%TOOLS%'

IF EXIST "%TOOLS%\set_%CONFIGURATION%_%PLATFORM%.bat" (
  CALL :fn_ResetErrorLevel

  %_AECHO% Running "%TOOLS%\set_%CONFIGURATION%_%PLATFORM%.bat"...
  %__ECHO3% CALL "%TOOLS%\set_%CONFIGURATION%_%PLATFORM%.bat"

  IF ERRORLEVEL 1 (
    ECHO File "%TOOLS%\set_%CONFIGURATION%_%PLATFORM%.bat" failed.
    GOTO errors
  )
)

IF NOT DEFINED NOUSER (
  IF EXIST "%TOOLS%\set_user_%USERNAME%_%BASE_CONFIGURATION%_%PLATFORM%.bat" (
    CALL :fn_ResetErrorLevel

    %_AECHO% Running "%TOOLS%\set_user_%USERNAME%_%BASE_CONFIGURATION%_%PLATFORM%.bat"...
    %__ECHO3% CALL "%TOOLS%\set_user_%USERNAME%_%BASE_CONFIGURATION%_%PLATFORM%.bat"

    IF ERRORLEVEL 1 (
      ECHO File "%TOOLS%\set_user_%USERNAME%_%BASE_CONFIGURATION%_%PLATFORM%.bat" failed.
      GOTO errors
    )
  )

  IF EXIST "%TOOLS%\set_user_%USERNAME%_%BASE_CONFIGURATION%.bat" (
    CALL :fn_ResetErrorLevel

    %_AECHO% Running "%TOOLS%\set_user_%USERNAME%_%BASE_CONFIGURATION%.bat"...
    %__ECHO3% CALL "%TOOLS%\set_user_%USERNAME%_%BASE_CONFIGURATION%.bat"

    IF ERRORLEVEL 1 (
      ECHO File "%TOOLS%\set_user_%USERNAME%_%BASE_CONFIGURATION%.bat" failed.
      GOTO errors
    )
  )

  IF EXIST "%TOOLS%\set_user_%USERNAME%.bat" (
    CALL :fn_ResetErrorLevel

    %_AECHO% Running "%TOOLS%\set_user_%USERNAME%.bat"...
    %__ECHO3% CALL "%TOOLS%\set_user_%USERNAME%.bat"

    IF ERRORLEVEL 1 (
      ECHO File "%TOOLS%\set_user_%USERNAME%.bat" failed.
      GOTO errors
    )
  )
)

IF NOT DEFINED MSBUILD (
  SET MSBUILD=MSBuild.exe
)

%_VECHO% MsBuild = '%MSBUILD%'

IF NOT DEFINED CSC (
  SET CSC=csc.exe
)

%_VECHO% Csc = '%CSC%'

REM
REM TODO: When the next version of Visual Studio is released, this section
REM       may need updating.
REM
IF DEFINED NETFX20ONLY (
  %_AECHO% Forcing the use of the .NET Framework 2.0...
  SET YEAR=2005
  CALL :fn_CheckFrameworkDir v2.0.50727
  GOTO setup_buildToolDir
)

IF DEFINED NETFX35ONLY (
  %_AECHO% Forcing the use of the .NET Framework 3.5...
  SET YEAR=2008
  CALL :fn_CheckFrameworkDir v3.5
  GOTO setup_buildToolDir
)

IF DEFINED NETFX40ONLY (
  %_AECHO% Forcing the use of the .NET Framework 4.0...
  SET YEAR=2010
  CALL :fn_CheckFrameworkDir v4.0.30319
  GOTO setup_buildToolDir
)

IF DEFINED NETFX45ONLY (
  %_AECHO% Forcing the use of the .NET Framework 4.5...
  SET YEAR=2012
  CALL :fn_CheckFrameworkDir v4.0.30319
  GOTO setup_buildToolDir
)

IF DEFINED NETFX451ONLY (
  %_AECHO% Forcing the use of the .NET Framework 4.5.1...
  SET YEAR=2013
  CALL :fn_CheckFrameworkDir v4.0.30319
  CALL :fn_CheckMsBuildDir 12.0
  GOTO setup_buildToolDir
)

IF DEFINED NETFX452ONLY (
  %_AECHO% Forcing the use of the .NET Framework 4.5.2...
  SET YEAR=2013
  CALL :fn_CheckFrameworkDir v4.0.30319
  CALL :fn_CheckMsBuildDir 12.0
  GOTO setup_buildToolDir
)

REM
REM TODO: When the next version of MSBuild is released, this section may need
REM       updating.
REM
IF NOT DEFINED MSBUILDDIR (
  CALL :fn_CheckMsBuildDir 12.0
  IF DEFINED MSBUILDDIR (
    SET YEAR=2013
  )
)

REM
REM TODO: When the next version of Visual Studio is released, this section
REM       may need updating.
REM
IF NOT DEFINED FRAMEWORKDIR (
  CALL :fn_CheckFrameworkDir v4.0.30319
  IF DEFINED FRAMEWORKDIR (
    SET YEAR=2010
  )
)

IF NOT DEFINED FRAMEWORKDIR (
  CALL :fn_CheckFrameworkDir v3.5
  IF DEFINED FRAMEWORKDIR (
    SET YEAR=2008
  )
)

IF NOT DEFINED FRAMEWORKDIR (
  CALL :fn_CheckFrameworkDir v2.0.50727
  IF DEFINED FRAMEWORKDIR (
    SET YEAR=2005
  )
)

:setup_buildToolDir

IF DEFINED BUILDTOOLDIR (
  %_AECHO% Forcing the use of build tool directory "%BUILDTOOLDIR%"...
) ELSE (
  CALL :fn_CheckBuildToolDir
  CALL :fn_VerifyBuildToolDir
)

%_VECHO% Year = '%YEAR%'
%_VECHO% FrameworkDir = '%FRAMEWORKDIR%'
%_VECHO% MsBuildDir = '%MSBUILDDIR%'
%_VECHO% BuildToolDir = '%BUILDTOOLDIR%'

IF NOT DEFINED BUILDTOOLDIR (
  ECHO.
  ECHO No directory containing MSBuild could be found.
  ECHO.
  ECHO Please install the .NET Framework or set the "FRAMEWORKDIR"
  ECHO environment variable to the location where it is installed.
  ECHO.
  GOTO errors
)

CALL :fn_ResetErrorLevel

%__ECHO2% PUSHD "%ROOT%"

IF ERRORLEVEL 1 (
  ECHO Could not change directory to "%ROOT%".
  GOTO errors
)

SET PATH=%BUILDTOOLDIR%;%PATH%

%_VECHO% Path = '%PATH%'

IF NOT DEFINED SOLUTION (
  %_AECHO% Building all projects...
  SET SOLUTION=.\SQLite.NET.%YEAR%.MSBuild.sln
)

IF NOT EXIST "%SOLUTION%" (
  %_AECHO% Building all projects...
  SET SOLUTION=.\SQLite.NET.%YEAR%.sln
)

%_VECHO% Solution = '%SOLUTION%'

IF NOT DEFINED TARGET (
  SET TARGET=Rebuild
)

%_VECHO% Target = '%TARGET%'

IF NOT DEFINED TEMP (
  ECHO Temporary directory must be defined.
  GOTO errors
)

%_VECHO% Temp = '%TEMP%'

IF NOT DEFINED LOGDIR (
  SET LOGDIR=%TEMP%
)

%_VECHO% LogDir = '%LOGDIR%'

IF NOT DEFINED LOGPREFIX (
  SET LOGPREFIX=System.Data.SQLite.Build
)

%_VECHO% LogPrefix = '%LOGPREFIX%'

IF NOT DEFINED LOGSUFFIX (
  SET LOGSUFFIX=Unknown
)

%_VECHO% LogSuffix = '%LOGSUFFIX%'

IF DEFINED LOGGING GOTO skip_setLogging
IF DEFINED NOLOG GOTO skip_setLogging

SET LOGGING="/logger:FileLogger,Microsoft.Build.Engine;Logfile=%LOGDIR%\%LOGPREFIX%_%CONFIGURATION%_%PLATFORM%_%YEAR%_%LOGSUFFIX%.log;Verbosity=diagnostic"

:skip_setLogging

IF NOT DEFINED NOPROPS (
  IF EXIST Externals\Eagle\bin\EagleShell.exe (
    IF DEFINED INTEROP_EXTRA_PROPS_FILE (
      REM
      REM HACK: This is used to work around a limitation of Visual Studio 2005
      REM       and 2008 that prevents the "InheritedPropertySheets" attribute
      REM       value from working correctly when it refers to a property that
      REM       evaluates to an empty string.
      REM
      %__ECHO% Externals\Eagle\bin\EagleShell.exe -evaluate "set fileName {SQLite.Interop/props/include.vsprops}; set data [readFile $fileName]; regsub -- {	InheritedPropertySheets=\"\"} $data {	InheritedPropertySheets=\"$^(INTEROP_EXTRA_PROPS_FILE^)\"} data; writeFile $fileName $data"

      IF ERRORLEVEL 1 (
        ECHO Property file modification of "SQLite.Interop\props\include.vsprops" failed.
        GOTO errors
      ) ELSE (
        ECHO Property file modification successful.
      )
    )
  ) ELSE (
    ECHO WARNING: Property file modification skipped, Eagle binaries are not available.
  )
) ELSE (
  ECHO WARNING: Property file modification skipped, disabled via NOPROPS environment variable.
)

IF NOT DEFINED NOTAG (
  IF EXIST Externals\Eagle\bin\EagleShell.exe (
    %__ECHO% Externals\Eagle\bin\EagleShell.exe -file Setup\sourceTag.eagle SourceIdMode SQLite.Interop\src\win\interop.h

    IF ERRORLEVEL 1 (
      ECHO Source tagging of "SQLite.Interop\src\win\interop.h" failed.
      GOTO errors
    )

    %__ECHO% Externals\Eagle\bin\EagleShell.exe -file Setup\sourceTag.eagle SourceIdMode System.Data.SQLite\SQLitePatchLevel.cs

    IF ERRORLEVEL 1 (
      ECHO Source tagging of "System.Data.SQLite\SQLitePatchLevel.cs" failed.
      GOTO errors
    )
  ) ELSE (
    ECHO WARNING: Source tagging skipped, Eagle binaries are not available.
  )
) ELSE (
  ECHO WARNING: Source tagging skipped, disabled via NOTAG environment variable.
)

%_VECHO% Logging = '%LOGGING%'
%_VECHO% BuildArgs = '%BUILD_ARGS%'
%_VECHO% MsBuildArgs = '%MSBUILD_ARGS%'

IF NOT DEFINED NOBUILD (
  %__ECHO% "%MSBUILD%" "%SOLUTION%" "/target:%TARGET%" "/property:Configuration=%CONFIGURATION%" "/property:Platform=%PLATFORM%" %LOGGING% %BUILD_ARGS% %MSBUILD_ARGS%

  IF ERRORLEVEL 1 (
    ECHO Build failed.
    GOTO errors
  )
) ELSE (
  ECHO WARNING: Build skipped, disabled via NOBUILD environment variable.
)

%__ECHO2% POPD

IF ERRORLEVEL 1 (
  ECHO Could not restore directory.
  GOTO errors
)

GOTO no_errors

:fn_CheckFrameworkDir
  IF DEFINED NOFRAMEWORKDIR GOTO :EOF
  SET FRAMEWORKVER=%1
  %_AECHO% Checking for .NET Framework "%FRAMEWORKVER%"...
  IF NOT DEFINED FRAMEWORKVER GOTO :EOF
  IF DEFINED NOFRAMEWORK64 (
    %_AECHO% Forced into using 32-bit version of MSBuild from Microsoft.NET...
    SET FRAMEWORKDIR=%windir%\Microsoft.NET\Framework\%FRAMEWORKVER%
    CALL :fn_VerifyFrameworkDir
    GOTO :EOF
  )
  IF NOT "%PROCESSOR_ARCHITECTURE%" == "x86" (
    %_AECHO% The operating system appears to be 64-bit.
    IF EXIST "%windir%\Microsoft.NET\Framework64\%FRAMEWORKVER%" (
      IF EXIST "%windir%\Microsoft.NET\Framework64\%FRAMEWORKVER%\%MSBUILD%" (
        IF EXIST "%windir%\Microsoft.NET\Framework64\%FRAMEWORKVER%\%CSC%" (
          %_AECHO% Using 64-bit version of MSBuild from Microsoft.NET...
          SET FRAMEWORKDIR=%windir%\Microsoft.NET\Framework64\%FRAMEWORKVER%
          CALL :fn_VerifyFrameworkDir
          GOTO :EOF
        ) ELSE (
          %_AECHO% Missing 64-bit version of "%CSC%".
        )
      ) ELSE (
        %_AECHO% Missing 64-bit version of "%MSBUILD%".
      )
    ) ELSE (
      %_AECHO% Missing 64-bit version of .NET Framework "%FRAMEWORKVER%".
    )
  ) ELSE (
    %_AECHO% The operating system appears to be 32-bit.
  )
  %_AECHO% Using 32-bit version of MSBuild from Microsoft.NET...
  SET FRAMEWORKDIR=%windir%\Microsoft.NET\Framework\%FRAMEWORKVER%
  CALL :fn_VerifyFrameworkDir
  GOTO :EOF

:fn_VerifyFrameworkDir
  IF DEFINED NOFRAMEWORKDIR GOTO :EOF
  IF NOT DEFINED FRAMEWORKDIR (
    %_AECHO% .NET Framework directory is not defined.
    GOTO :EOF
  )
  IF DEFINED FRAMEWORKDIR IF NOT EXIST "%FRAMEWORKDIR%" (
    %_AECHO% .NET Framework directory does not exist, unsetting...
    CALL :fn_UnsetVariable FRAMEWORKDIR
    GOTO :EOF
  )
  IF DEFINED FRAMEWORKDIR IF NOT EXIST "%FRAMEWORKDIR%\%MSBUILD%" (
    %_AECHO% File "%MSBUILD%" not in .NET Framework directory, unsetting...
    CALL :fn_UnsetVariable FRAMEWORKDIR
    GOTO :EOF
  )
  IF DEFINED FRAMEWORKDIR IF NOT EXIST "%FRAMEWORKDIR%\%CSC%" (
    %_AECHO% File "%CSC%" not in .NET Framework directory, unsetting...
    CALL :fn_UnsetVariable FRAMEWORKDIR
    GOTO :EOF
  )
  %_AECHO% .NET Framework directory "%FRAMEWORKDIR%" verified.
  GOTO :EOF

:fn_CheckMsBuildDir
  IF DEFINED NOMSBUILDDIR GOTO :EOF
  SET MSBUILDVER=%1
  %_AECHO% Checking for MSBuild "%MSBUILDVER%"...
  IF NOT DEFINED MSBUILDVER GOTO :EOF
  IF DEFINED NOMSBUILD64 (
    %_AECHO% Forced into using 32-bit version of MSBuild from Program Files...
    GOTO set_msbuild_x86
  )
  IF "%PROCESSOR_ARCHITECTURE%" == "x86" GOTO set_msbuild_x86
  %_AECHO% The operating system appears to be 64-bit.
  %_AECHO% Using 32-bit version of MSBuild from Program Files...
  SET MSBUILDDIR=%ProgramFiles(x86)%\MSBuild\%MSBUILDVER%\bin
  GOTO set_msbuild_done
  :set_msbuild_x86
  %_AECHO% The operating system appears to be 32-bit.
  %_AECHO% Using native version of MSBuild from Program Files...
  SET MSBUILDDIR=%ProgramFiles%\MSBuild\%MSBUILDVER%\bin
  :set_msbuild_done
  CALL :fn_VerifyMsBuildDir
  GOTO :EOF

:fn_VerifyMsBuildDir
  IF DEFINED NOMSBUILDDIR GOTO :EOF
  IF NOT DEFINED MSBUILDDIR (
    %_AECHO% MSBuild directory is not defined.
    GOTO :EOF
  )
  IF DEFINED MSBUILDDIR IF NOT EXIST "%MSBUILDDIR%" (
    %_AECHO% MSBuild directory does not exist, unsetting...
    CALL :fn_UnsetVariable MSBUILDDIR
    GOTO :EOF
  )
  IF DEFINED MSBUILDDIR IF NOT EXIST "%MSBUILDDIR%\%MSBUILD%" (
    %_AECHO% File "%MSBUILD%" not in MSBuild directory, unsetting...
    CALL :fn_UnsetVariable MSBUILDDIR
    GOTO :EOF
  )
  IF DEFINED MSBUILDDIR IF NOT EXIST "%MSBUILDDIR%\%CSC%" (
    %_AECHO% File "%CSC%" not in MSBuild directory, unsetting...
    CALL :fn_UnsetVariable MSBUILDDIR
    GOTO :EOF
  )
  %_AECHO% MSBuild directory "%MSBUILDDIR%" verified.
  GOTO :EOF

:fn_CheckBuildToolDir
  %_AECHO% Checking for build tool directories...
  IF DEFINED MSBUILDDIR GOTO set_msbuild_tools
  IF DEFINED FRAMEWORKDIR GOTO set_framework_tools
  %_AECHO% No build tool directories found.
  GOTO :EOF
  :set_msbuild_tools
  %_AECHO% Using MSBuild directory "%MSBUILDDIR%"...
  CALL :fn_CopyVariable MSBUILDDIR BUILDTOOLDIR
  GOTO :EOF
  :set_framework_tools
  %_AECHO% Using .NET Framework directory "%FRAMEWORKDIR%"...
  CALL :fn_CopyVariable FRAMEWORKDIR BUILDTOOLDIR
  GOTO :EOF

:fn_VerifyBuildToolDir
  IF NOT DEFINED BUILDTOOLDIR (
    %_AECHO% Build tool directory is not defined.
    GOTO :EOF
  )
  IF DEFINED BUILDTOOLDIR IF NOT EXIST "%BUILDTOOLDIR%" (
    %_AECHO% Build tool directory does not exist, unsetting...
    CALL :fn_UnsetVariable BUILDTOOLDIR
    GOTO :EOF
  )
  IF DEFINED BUILDTOOLDIR IF NOT EXIST "%BUILDTOOLDIR%\%MSBUILD%" (
    %_AECHO% File "%MSBUILD%" not in build tool directory, unsetting...
    CALL :fn_UnsetVariable BUILDTOOLDIR
    GOTO :EOF
  )
  IF DEFINED BUILDTOOLDIR IF NOT EXIST "%BUILDTOOLDIR%\%CSC%" (
    %_AECHO% File "%CSC%" not in build tool directory, unsetting...
    CALL :fn_UnsetVariable BUILDTOOLDIR
    GOTO :EOF
  )
  %_AECHO% Build tool directory "%BUILDTOOLDIR%" verified.
  GOTO :EOF

:fn_UnquoteVariable
  IF NOT DEFINED %1 GOTO :EOF
  SETLOCAL
  SET __ECHO_CMD=ECHO %%%1%%
  FOR /F "delims=" %%V IN ('%__ECHO_CMD%') DO (
    SET VALUE=%%V
  )
  SET VALUE=%VALUE:"=%
  REM "
  ENDLOCAL && SET %1=%VALUE%
  GOTO :EOF

:fn_CopyVariable
  IF NOT DEFINED %1 GOTO :EOF
  IF "%2" == "" GOTO :EOF
  SETLOCAL
  SET __ECHO_CMD=ECHO %%%1%%
  FOR /F "delims=" %%V IN ('%__ECHO_CMD%') DO (
    SET VALUE=%%V
  )
  ENDLOCAL && SET %2=%VALUE%
  GOTO :EOF

:fn_UnsetVariable
  IF NOT "%1" == "" (
    SET %1=
    CALL :fn_ResetErrorLevel
  )
  GOTO :EOF

:fn_ResetErrorLevel
  VERIFY > NUL
  GOTO :EOF

:fn_SetErrorLevel
  VERIFY MAYBE 2> NUL
  GOTO :EOF

:usage
  ECHO.
  ECHO Usage: %~nx0 [configuration] [platform]
  ECHO.
  GOTO errors

:errors
  CALL :fn_SetErrorLevel
  ENDLOCAL
  ECHO.
  ECHO Build failure, errors were encountered.
  GOTO end_of_file

:no_errors
  CALL :fn_ResetErrorLevel
  ENDLOCAL
  ECHO.
  ECHO Build success, no errors were encountered.
  GOTO end_of_file

:end_of_file
%__ECHO% EXIT /B %ERRORLEVEL%

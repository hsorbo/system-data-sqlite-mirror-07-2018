﻿/********************************************************
 * ADO.NET 2.0 Data Provider for SQLite Version 3.X
 * Written by Robert Simpson (robert@blackcastlesoft.com)
 * 
 * Released to the public domain, use at your own risk!
 ********************************************************/

namespace System.Data.SQLite
{
  using System;
  using System.Data.Common;

#if !PLATFORM_COMPACTFRAMEWORK
  /// <summary>
  /// SQLite implementation of DbConnectionStringBuilder.
  /// </summary>
  public sealed class SQLiteConnectionStringBuilder : DbConnectionStringBuilder
  {
    /// <overloads>
    /// Constructs a new instance of the class
    /// </overloads>
    /// <summary>
    /// Default constructor
    /// </summary>
    public SQLiteConnectionStringBuilder()
    {
      Initialize(null);
    }

    /// <summary>
    /// Constructs a new instance of the class using the specified connection string.
    /// </summary>
    /// <param name="cnnString">The connection string to parse</param>
    public SQLiteConnectionStringBuilder(string cnnString)
    {
      Initialize(cnnString);
    }

    private void Initialize(string cnnString)
    {
      ConnectionString = cnnString;
      Reset();
    }

    internal void Reset()
    {
      if (this.ContainsKey("Version") == false)
        Version = 3;

      if (ContainsKey("UseUTF16Encoding") == false)
        UseUTF16Encoding = false;

      if (ContainsKey("Cache Size") == false)
        CacheSize = 2000;

      if (ContainsKey("Synchronous") == false)
        SyncMode = SyncMode.Normal;

      if (ContainsKey("DateTimeFormat") == false)
        DateTimeFormat = DateTimeFormat.ISO8601;

      if (ContainsKey("Page Size") == false)
        PageSize = 4096;
    }

    /// <summary>
    /// Gets/Sets the default version of the SQLite engine to instantiate.  Currently the only valid value is 3, indicating version 3 of the sqlite library.
    /// </summary>
    public int Version
    {
      get
      {
        return Convert.ToInt32(this["Version"]);
      }
      set
      {
        if (value != 3)
          throw new NotImplementedException();

        this["Version"] = value;
      }
    }

    /// <summary>
    /// Gets/Sets the synchronous mode of the connection string.  Default is "Normal".
    /// </summary>
    public SyncMode SyncMode
    {
      get
      {
        string s = this["Synchronous"].ToString().ToUpper();
        switch (s)
        {
          case "FULL":
            return SyncMode.Full;
          case "OFF":
            return SyncMode.Off;
          default:
            return SyncMode.Normal;
        }
      }
      set
      {
        string s = "Normal";
        if (value == SyncMode.Full) s = "Full";
        else if (value == SyncMode.Off) s = "Off";

        this["Synchronous"] = s;
      }
    }

    /// <summary>
    /// Gets/Sets the encoding for the connection string.  The default is "False" which indicates UTF-8 encoding.
    /// </summary>
    public bool UseUTF16Encoding
    {
      get
      {
        return (this["UseUTF16Encoding"].ToString().ToUpper() == "TRUE");
      }
      set
      {
        this["UseUTF16Encoding"] = ((value == true) ? "True" : "False");
      }
    }

    /// <summary>
    /// Gets/Sets the filename to open on the connection string.
    /// </summary>
    public string DataSource
    {
      get
      {
        return this["Data Source"].ToString();
      }
      set
      {
        this["Data Source"] = value;
      }
    }

    /// <summary>
    /// Gets/Sets the page size for the connection.
    /// </summary>
    public int PageSize
    {
      get
      {
        return Convert.ToInt32(this["Page Size"]);
      }
      set
      {
        this["Page Size"] = value;
      }
    }

    /// <summary>
    /// Gets/Sets the cache size for the connection.
    /// </summary>
    public int CacheSize
    {
      get
      {
        return Convert.ToInt32(this["Cache Size"]);
      }
      set
      {
        this["Cache Size"] = value;
      }
    }

    /// <summary>
    /// Gets/Sets the datetime format for the connection.
    /// </summary>
    public DateTimeFormat DateTimeFormat
    {
      get
      {
        switch(this["DateTimeFormat"].ToString().ToUpper())
        {
          case "TICKS":
            return DateTimeFormat.Ticks;
          default:
            return DateTimeFormat.ISO8601;
        }
      }
      set
      {
        switch (value)
        {
          case DateTimeFormat.Ticks:
            this["DateTimeFormat"] = "Ticks";
            break;
          case DateTimeFormat.ISO8601:
            this["DateTimeFormat"] = "ISO8601";
            break;
        }
      }
    }
  }
#endif
}

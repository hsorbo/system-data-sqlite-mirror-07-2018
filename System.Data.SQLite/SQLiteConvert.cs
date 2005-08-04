﻿/********************************************************
 * ADO.NET 2.0 Data Provider for SQLite Version 3.X
 * Written by Robert Simpson (robert@blackcastlesoft.com)
 * 
 * Released to the public domain, use at your own risk!
 ********************************************************/

namespace System.Data.SQLite
{
  using System;
  using System.Runtime.InteropServices;
  using System.Collections.Generic;

  /// <summary>
  /// SQLite has very limited types, and is inherently text-based.  The first 5 types below represent the sum of all types SQLite
  /// understands.  The DateTime extension to the spec is for internal use only.
  /// </summary>
  public enum TypeAffinity
  {
    /// <summary>
    /// Not used
    /// </summary>
    Uninitialized = 0,
    /// <summary>
    /// All integers in SQLite default to Int64
    /// </summary>
    Int64 = 1,
    /// <summary>
    /// All floating point numbers in SQLite default to double
    /// </summary>
    Double = 2,
    /// <summary>
    /// The default data type of SQLite is text
    /// </summary>
    Text = 3,
    /// <summary>
    /// Typically blob types are only seen when returned from a function
    /// </summary>
    Blob = 4,
    /// <summary>
    /// Null types can be returned from functions
    /// </summary>
    Null = 5,
    /// <summary>
    /// Used internally by this provider
    /// </summary>
    DateTime = 10,
    /// <summary>
    /// Used internally
    /// </summary>
    None = 256,
  }

  /// <summary>
  /// This implementation of SQLite for ADO.NET can process date/time fields in databases in only one of two formats.  Ticks and ISO8601.
  /// Ticks is inherently more accurate, but less compatible with 3rd party tools that query the database, and renders the DateTime field
  /// unreadable without post-processing.
  /// ISO8601 is more compatible, readable, fully-processable, but less accurate as it doesn't provide time down to fractions of a second.
  /// </summary>
  public enum DateTimeFormat
  {
    /// <summary>
    /// Using ticks is more accurate but less compatible with other viewers and utilities that access your database.
    /// </summary>
    Ticks = 0,
    /// <summary>
    /// The default format for this provider.  More compatible with SQLite's intended usage of datetimes, but overall less accurate than Ticks as it doesn't
    /// natively support times down to fractions of a second.
    /// </summary>
    ISO8601 = 1,
  }

  /// <summary>
  /// Struct used internally to determine the datatype of a column in a resultset
  /// </summary>
  internal struct SQLiteType
  {
    /// <summary>
    /// The DbType of the column, or DbType.Object if it cannot be determined
    /// </summary>
    internal DbType Type;
    /// <summary>
    /// The affinity of a column, used for expressions or when Type is DbType.Object
    /// </summary>
    internal TypeAffinity Affinity;
  }

  /// <summary>
  /// This base class provides datatype conversion services for the SQLite provider.
  /// </summary>
  public abstract class SQLiteConvert
  {
    /// <summary>
    /// An array of ISO8601 datetime formats we support conversion from
    /// </summary>
    private static string[] _datetimeFormats = new string[] {"yyyy-MM-dd HH:mm:ss",
																	  "yyyyMMddHHmmss",
																	  "yyyyMMddTHHmmssfffffff",
																	  "yyyy-MM-dd",
																	  "yy-MM-dd",
																	  "yyyyMMdd",
																	  "HH:mm:ss",
																	  "THHmmss"
															 };

    /// <summary>
    /// An UTF-8 Encoding instance, so we can convert strings to and from UTF8
    /// </summary>
    private static Text.UTF8Encoding _utf8 = new Text.UTF8Encoding();
    /// <summary>
    /// The default DateTime format for this instance
    /// </summary>
    private DateTimeFormat _datetimeFormat;
    /// <summary>
    /// Initializes the conversion class
    /// </summary>
    /// <param name="fmt">The default date/time format to use for this instance</param>
    internal SQLiteConvert(DateTimeFormat fmt)
    {
      _datetimeFormat = fmt;
    }

    #region UTF-8 Conversion Functions
    /// <summary>
    /// Converts a string to a UTF-8 encoded byte array sized to include a null-terminating character.
    /// </summary>
    /// <param name="sourceText">The string to convert to UTF-8</param>
    /// <returns>A byte array containing the converted string plus an extra 0 terminating byte at the end of the array.</returns>
    public static byte[] ToUTF8(string sourceText)
    {
      Byte[] byteArray;
      int nlen = _utf8.GetByteCount(sourceText) + 1;

      byteArray = new byte[nlen];
      nlen = _utf8.GetBytes(sourceText, 0, sourceText.Length, byteArray, 0);
      byteArray[nlen] = 0;

      return byteArray;
    }

    /// <summary>
    /// Convert a DateTime to a UTF-8 encoded, zero-terminated byte array.
    /// </summary>
    /// <remarks>
    /// This function is a convenience function, which first calls ToString() on the DateTime, and then calls ToUTF8() with the
    /// string result.
    /// </remarks>
    /// <param name="dateTimeValue">The DateTime to convert.</param>
    /// <returns>The UTF-8 encoded string, including a 0 terminating byte at the end of the array.</returns>
    public byte[] ToUTF8(DateTime dateTimeValue)
    {
      return ToUTF8(ToString(dateTimeValue));
    }

    /// <summary>
    /// Converts a UTF-8 encoded IntPtr of the specified length into a .NET string
    /// </summary>
    /// <param name="nativestring">The pointer to the memory where the UTF-8 string is encoded</param>
    /// <param name="nativestringlen">The number of bytes to decode</param>
    /// <returns>A string containing the translated character(s)</returns>
    public virtual string ToString(IntPtr nativestring, int nativestringlen)
    {
      if (nativestringlen == 0) return "";

      byte[] byteArray;

      byteArray = new byte[nativestringlen];
      Marshal.Copy(nativestring, byteArray, 0, nativestringlen);

      return _utf8.GetString(byteArray, 0, nativestringlen);
    }

    #endregion

    #region DateTime Conversion Functions
    /// <summary>
    /// Converts a string into a DateTime, using the current DateTimeFormat specified for the connection when it was opened.
    /// </summary>
    /// <remarks>
    /// Acceptable ISO8601 DateTime formats are:
    ///   yyyy-MM-dd HH:mm:ss
    ///   yyyyMMddHHmmss
    ///   yyyyMMddTHHmmssfffffff
    ///   yyyy-MM-dd
    ///   yy-MM-dd
    ///   yyyyMMdd
    ///   HH:mm:ss
    ///   THHmmss
    /// </remarks>
    /// <param name="dateText">The string containing either a Tick value or an ISO8601-format string</param>
    /// <returns>A DateTime value</returns>
    public DateTime ToDateTime(string dateText)
    {
      switch (_datetimeFormat)
      {
        case DateTimeFormat.Ticks:
          return new DateTime(Convert.ToInt64(dateText));
        default:
          return DateTime.ParseExact(dateText, _datetimeFormats, System.Globalization.DateTimeFormatInfo.InvariantInfo, System.Globalization.DateTimeStyles.None);
      }
    }

    ///// <summary>
    ///// Attempt to convert the specified string to a datetime value.
    ///// </summary>
    ///// <param name="strSrc">The string to parse into a datetime</param>
    ///// <param name="result">If successful, a valid datetime structure</param>
    ///// <returns>Returns true if the string was a valid ISO8601 datetime, false otherwise.</returns>
    //public bool TryToDateTime(string strSrc, out DateTime result)
    //{
    //  switch (_datetimeFormat)
    //  {
    //    case DateTimeFormat.ISO8601:
    //      return DateTime.TryParseExact(strSrc, _datetimeFormats, System.Globalization.DateTimeFormatInfo.InvariantInfo, System.Globalization.DateTimeStyles.None, out result);
    //    case DateTimeFormat.Ticks:
    //      {
    //        long n;
    //        if (long.TryParse(strSrc, out n) == true)
    //        {
    //          result = new DateTime(n);
    //          return true;
    //        }
    //      }
    //      break;
    //  }

    //  result = DateTime.Now;
    //  return false;
    //}

    /// <summary>
    /// Converts a DateTime to a string value, using the current DateTimeFormat specified for the connection when it was opened.
    /// </summary>
    /// <param name="dateValue">The DateTime value to convert</param>
    /// <returns>Either a string consisting of the tick count for DateTimeFormat.Ticks, or a date/time in ISO8601 format.</returns>
    public string ToString(DateTime dateValue)
    {
      switch (_datetimeFormat)
      {
        case DateTimeFormat.Ticks:
          return dateValue.Ticks.ToString();
        default:
          return dateValue.ToString(_datetimeFormats[0]);
      }
    }

    /// <summary>
    /// Internal function to convert a UTF-8 encoded IntPtr of the specified length to a DateTime.
    /// </summary>
    /// <remarks>
    /// This is a convenience function, which first calls ToString() on the IntPtr to convert it to a string, then calls
    /// ToDateTime() on the string to return a DateTime.
    /// </remarks>
    /// <param name="ptr">A pointer to the UTF-8 encoded string</param>
    /// <param name="len">The length in bytes of the string</param>
    /// <returns>The parsed DateTime value</returns>
    internal DateTime ToDateTime(IntPtr ptr, int len)
    {
      return ToDateTime(ToString(ptr, len));
    }
    #endregion

    /// <summary>
    /// Smart method of splitting a string.  Skips quoted elements, removes the quotes.
    /// </summary>
    /// <remarks>
    /// This split function works somewhat like the String.Split() function in that it breaks apart a string into
    /// pieces and returns the pieces as an array.  The primary differences are:
    /// <list type="bullet">
    /// <item><description>Only one character can be provided as a separator character</description></item>
    /// <item><description>Quoted text inside the string is skipped over when searching for the separator, and the quotes are removed.</description></item>
    /// </list>
    /// Thus, if splitting the following string looking for a comma:<br/>
    /// One,Two, "Three, Four", Five<br/>
    /// <br/>
    /// The resulting array would contain<br/>
    /// [0] One<br/>
    /// [1] Two<br/>
    /// [2] Three, Four<br/>
    /// [3] Five<br/>
    /// <br/>
    /// Note that the leading and trailing spaces were removed from each item during the split.
    /// </remarks>
    /// <param name="source">Source string to split apart</param>
    /// <param name="separator">Separator character</param>
    /// <returns>A string array of the split up elements</returns>
    public static string[] Split(string source, char separator)
    {
      char[] toks = new char[2] { '\"', separator };
      char[] quot = new char[1] { '\"' };
      int n = 0;
      List<string> ls = new List<string>();
      string s;

      while (source.Length > 0)
      {
        n = source.IndexOfAny(toks, n);
        if (n == -1) break;
        if (source[n] == toks[0])
        {
          source = source.Remove(n, 1);
          n = source.IndexOfAny(quot, n);
          if (n == -1)
          {
            source = "\"" + source;
            break;
          }
          source = source.Remove(n, 1);
        }
        else
        {
          s = source.Substring(0, n).Trim();
          source = source.Substring(n + 1).Trim();
          if (s.Length > 0) ls.Add(s);
          n = 0;
        }
      }
      if (source.Length > 0) ls.Add(source);

      string[] ar = new string[ls.Count];
      ls.CopyTo(ar, 0);

      return ar;
    }

    #region Type Conversions
    /// <summary>
    /// Determines the data type of a column in a statement
    /// </summary>
    /// <param name="stmt">The statement to retrieve information for</param>
    /// <param name="i">The column to retrieve type information on</param>
    /// <returns>Returns a SQLiteType struct</returns>
    internal static SQLiteType ColumnToType(SQLiteStatement stmt, int i)
    {
      SQLiteType typ;

      typ.Type = TypeNameToDbType(stmt._sql.ColumnType(stmt, i, out typ.Affinity));

      return typ;
    }

    /// <summary>
    /// Converts a SQLiteType to a .NET Type object
    /// </summary>
    /// <param name="t">The SQLiteType to convert</param>
    /// <returns>Returns a .NET Type object</returns>
    internal static Type SQLiteTypeToType(SQLiteType t)
    {
      if (t.Type != DbType.Object)
        return SQLiteConvert.DbTypeToType(t.Type);

      switch (t.Affinity)
      {
        case TypeAffinity.Null:
          return typeof(DBNull);
        case TypeAffinity.Int64:
          return typeof(Int64);
        case TypeAffinity.Double:
          return typeof(Double);
        case TypeAffinity.Blob:
          return typeof(byte[]);
        default:
          return typeof(string);
      }
    }

    /// <summary>
    /// For a given intrinsic type, return a DbType
    /// </summary>
    /// <param name="typ">The native type to convert</param>
    /// <returns>The corresponding (closest match) DbType</returns>
    internal static DbType TypeToDbType(Type typ)
    {
      switch (Type.GetTypeCode(typ))
      {
        case TypeCode.Int16:
          return DbType.Int16;
        case TypeCode.Int32:
          return DbType.Int32;
        case TypeCode.Int64:
          return DbType.Int64;
        case TypeCode.UInt16:
          return DbType.UInt16;
        case TypeCode.UInt32:
          return DbType.UInt32;
        case TypeCode.UInt64:
          return DbType.UInt64;
        case TypeCode.Double:
          return DbType.Double;
        case TypeCode.Single:
          return DbType.Single;
        case TypeCode.Decimal:
          return DbType.Decimal;
        case TypeCode.Boolean:
          return DbType.Boolean;
        case TypeCode.SByte:
        case TypeCode.Char:
          return DbType.SByte;
        case TypeCode.DateTime:
          return DbType.DateTime;
        case TypeCode.String:
          return DbType.String;
        case TypeCode.Object:
          if (typ == typeof(byte[])) return DbType.Binary;
          if (typ == typeof(Guid)) return DbType.Guid;
          return DbType.String;
      }

      return DbType.String;
    }

    /// <summary>
    /// Convert a DbType to a Type
    /// </summary>
    /// <param name="typ">The DbType to convert from</param>
    /// <returns>The closest-match .NET type</returns>
    internal static Type DbTypeToType(DbType typ)
    {
      switch (typ)
      {
        case DbType.Binary:
          return typeof(byte[]);
        case DbType.Boolean:
          return typeof(bool);
        case DbType.Byte:
          return typeof(byte);
        case DbType.Currency:
        case DbType.Decimal:
          return typeof(decimal);
        case DbType.DateTime:
          return typeof(DateTime);
        case DbType.Double:
          return typeof(double);
        case DbType.Guid:
          return typeof(Guid);
        case DbType.Int16:
        case DbType.UInt16:
          return typeof(Int16);
        case DbType.Int32:
        case DbType.UInt32:
          return typeof(Int32);
        case DbType.Int64:
        case DbType.UInt64:
          return typeof(Int64);
        case DbType.String:
          return typeof(string);
        case DbType.SByte:
          return typeof(char);
        case DbType.Single:
          return typeof(float);
      }
      return typeof(string);
    }

    /// <summary>
    /// For a given type, return the closest-match SQLite TypeAffinity, which only understands a very limited subset of types.
    /// </summary>
    /// <param name="typ">The type to evaluate</param>
    /// <returns>The SQLite type affinity for that type.</returns>
    internal static TypeAffinity TypeToAffinity(Type typ)
    {
      switch (Type.GetTypeCode(typ))
      {
        case TypeCode.DBNull:
          return TypeAffinity.Null;
        case TypeCode.String:
          return TypeAffinity.Text;
        case TypeCode.DateTime:
          return TypeAffinity.DateTime;
        case TypeCode.Int16:
        case TypeCode.Int32:
        case TypeCode.Int64:
        case TypeCode.UInt16:
        case TypeCode.UInt32:
        case TypeCode.UInt64:
        case TypeCode.Char:
        case TypeCode.SByte:
        case TypeCode.Byte:
        case TypeCode.Boolean:
          return TypeAffinity.Int64;
        case TypeCode.Double:
        case TypeCode.Single:
        case TypeCode.Decimal:
          return TypeAffinity.Double;
        case TypeCode.Object:
          if (typ == typeof(byte[])) return TypeAffinity.Blob;
          else return TypeAffinity.Text;
      }
      return TypeAffinity.Text;
    }

    /// <summary>
    /// For a given type name, return a closest-match .NET type
    /// </summary>
    /// <param name="Name">The name of the type to match</param>
    /// <returns>The .NET DBType the text evaluates to.</returns>
    internal static DbType TypeNameToDbType(string Name)
    {
      if (Name == null) return DbType.Object;

      Name = Name.ToUpper();

      if (Name.IndexOf("LONGTEXT") > -1) return DbType.String;
      if (Name.IndexOf("LONGCHAR") > -1) return DbType.String;
      if (Name.IndexOf("SMALLINT") > -1) return DbType.Int16;
      if (Name.IndexOf("BIGINT") > -1) return DbType.Int64;
      if (Name.IndexOf("COUNTER") > -1) return DbType.Int64;
      if (Name.IndexOf("AUTOINCREMENT") > -1) return DbType.Int64;
      if (Name.IndexOf("IDENTITY") > -1) return DbType.Int64;
      if (Name.IndexOf("LONG") > -1) return DbType.Int64;
      if (Name.IndexOf("TINYINT") > -1) return DbType.Byte;
      if (Name.IndexOf("INTEGER") > -1) return DbType.Int64;
      if (Name.IndexOf("INT") > -1) return DbType.Int32;
      if (Name.IndexOf("TEXT") > -1) return DbType.String;
      if (Name.IndexOf("DOUBLE") > -1) return DbType.Double;
      if (Name.IndexOf("FLOAT") > -1) return DbType.Double;
      if (Name.IndexOf("REAL") > -1) return DbType.Single;
      if (Name.IndexOf("BIT") > -1) return DbType.Boolean;
      if (Name.IndexOf("YESNO") > -1) return DbType.Boolean;
      if (Name.IndexOf("LOGICAL") > -1) return DbType.Boolean;
      if (Name.IndexOf("BOOL") > -1) return DbType.Boolean;
      if (Name.IndexOf("NUMERIC") > -1) return DbType.Decimal;
      if (Name.IndexOf("DECIMAL") > -1) return DbType.Decimal;
      if (Name.IndexOf("MONEY") > -1) return DbType.Decimal;
      if (Name.IndexOf("CURRENCY") > -1) return DbType.Decimal;
      if (Name.IndexOf("TIME") > -1) return DbType.DateTime;
      if (Name.IndexOf("DATE") > -1) return DbType.DateTime;
      if (Name.IndexOf("BLOB") > -1) return DbType.Binary;
      if (Name.IndexOf("BINARY") > -1) return DbType.Binary;
      if (Name.IndexOf("IMAGE") > -1) return DbType.Binary;
      if (Name.IndexOf("GENERAL") > -1) return DbType.Binary;
      if (Name.IndexOf("OLEOBJECT") > -1) return DbType.Binary;
      if (Name.IndexOf("GUID") > -1) return DbType.Guid;
      if (Name.IndexOf("UNIQUEIDENTIFIER") > -1) return DbType.Guid;
      if (Name.IndexOf("MEMO") > -1) return DbType.String;
      if (Name.IndexOf("NOTE") > -1) return DbType.String;
      if (Name.IndexOf("CHAR") > -1) return DbType.String;

      return DbType.Object;
    }
    #endregion
  }
}

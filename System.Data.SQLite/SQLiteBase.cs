﻿/********************************************************
 * ADO.NET 2.0 Data Provider for SQLite Version 3.X
 * Written by Robert Simpson (robert@blackcastlesoft.com)
 * 
 * Released to the public domain, use at your own risk!
 ********************************************************/

namespace System.Data.SQLite
{
  using System;
  using System.Data;
  using System.Runtime.InteropServices;
  using System.Collections.Generic;

  /// <summary>
  /// This internal class provides the foundation of SQLite support.  It defines all the abstract members needed to implement
  /// a SQLite data provider, and inherits from SQLiteConvert which allows for simple translations of string to and from SQLite.
  /// </summary>
  internal abstract class SQLiteBase : SQLiteConvert, IDisposable
  {
    internal SQLiteBase(DateTimeFormat fmt)
      : base(fmt) {}

    /// <summary>
    /// Returns a string representing the active version of SQLite
    /// </summary>
    internal abstract string Version { get; }
    /// <summary>
    /// Returns the number of changes the last executing insert/update caused.
    /// </summary>
    internal abstract int    Changes { get; }
    /// <summary>
    /// Opens a database.
    /// </summary>
    /// <remarks>
    /// Implementers should call SQLiteFunction.BindFunctions() and save the array after opening a connection
    /// to bind all attributed user-defined functions and collating sequences to the new connection.
    /// </remarks>
    /// <param name="strFilename">The filename of the database to open.  SQLite automatically creates it if it doesn't exist.</param>
    internal abstract void   Open(string strFilename);
    /// <summary>
    /// Closes the currently-open database.
    /// </summary>
    /// <remarks>
    /// After the database has been closed implemeters should call SQLiteFunction.UnbindFunctions() to deallocate all interop allocated
    /// memory associated with the user-defined functions and collating sequences tied to the closed connection.
    /// </remarks>
    internal abstract void   Close();
    /// <summary>
    /// Sets the busy timeout on the connection.  SQLiteCommand will call this before executing any command.
    /// </summary>
    /// <param name="nTimeoutMS">The number of milliseconds to wait before returning SQLITE_BUSY</param>
    internal abstract void   SetTimeout(int nTimeoutMS);
    /// <summary>
    /// Quick execute of a SQL command.  This is only executed internally, usually by SQLiteConnection when the connection
    /// is first opened to set the necessary startup pragmas.
    /// </summary>
    /// <param name="strSql">The SQL command text to execute</param>
    internal abstract void   Execute(string strSql);
    /// <summary>
    /// Returns the text of the last error issued by SQLite
    /// </summary>
    /// <returns></returns>
    internal abstract string SQLiteLastError();

    /// <summary>
    /// Prepares a SQL statement for execution.
    /// </summary>
    /// <param name="strSql">The SQL command text to prepare</param>
    /// <param name="nParamStart">When preparing multiple statements that are tied together into a single command,
    /// this value should be initialized to 0 for the first statement prepared.  On return from this function, the
    /// variable will automatically be incremented by 1 for each unnamed parameter that occurred in the statement.
    /// When implementing this function, one need only pass the nParamStart variable by reference to the SQLiteStatement()
    /// constructor.  SQLiteStatement will take care of it.</param>
    /// <param name="strRemain">The remainder of the statement that was not processed.  Each call to prepare parses the
    /// SQL up to to either the end of the text or to the first semi-colon delimiter.  The remaining text is returned
    /// here for a subsequent call to Prepare() until all the text has been processed.</param>
    /// <returns>Returns an initialized SQLiteStatement.</returns>
    internal abstract SQLiteStatement Prepare(string strSql, ref int nParamStart, out string strRemain);
    /// <summary>
    /// Steps through a prepared statement.
    /// </summary>
    /// <param name="stmt">The SQLiteStatement to step through</param>
    /// <returns>True if a row was returned, False if not.</returns>
    internal abstract bool Step(SQLiteStatement stmt);
    /// <summary>
    /// Finalizes a prepared statement.
    /// </summary>
    /// <param name="stmt">The statement to finalize</param>
    internal abstract void Finalize(SQLiteStatement stmt);
    /// <summary>
    /// Resets a prepared statement so it can be executed again.  If the error returned is SQLITE_SCHEMA, 
    /// transparently attempt to rebuild the SQL statement and throw an error if that was not possible.
    /// </summary>
    /// <param name="stmt">The statement to reset</param>
    /// <returns>Returns true if the schema changed while resetting, or false otherwise.</returns>
    internal abstract bool Reset(SQLiteStatement stmt);

    /// <summary>
    /// An interop-specific function, this call sets an internal flag in the sqlite.interop.dll which causes all column names
    /// of subsequently-prepared statements to return in Database.Table.Column format, ignoring all aliases that may have been applied
    /// to tables or columns in a resultset.
    /// </summary>
    /// <remarks>
    /// All statements prepared on this connection after this flag is changed are affected.  Existing statements are not.
    /// </remarks>
    /// <param name="bOn">Set to True to enable real column names, false to disable them.</param>
    internal abstract void SetRealColNames(bool bOn);

    internal abstract void Bind_Double(SQLiteStatement stmt, int index, double value);
    internal abstract void Bind_Int32(SQLiteStatement stmt, int index, Int32 value);
    internal abstract void Bind_Int64(SQLiteStatement stmt, int index, Int64 value);
    internal abstract void Bind_Text(SQLiteStatement stmt, int index, string value);
    internal abstract void Bind_Blob(SQLiteStatement stmt, int index, byte[] blobData);
    internal abstract void Bind_DateTime(SQLiteStatement stmt, int index, DateTime dt);
    internal abstract void Bind_Null(SQLiteStatement stmt, int index);

    internal abstract int    Bind_ParamCount(SQLiteStatement stmt);
    internal abstract string Bind_ParamName(SQLiteStatement stmt, int index);
    internal abstract int    Bind_ParamIndex(SQLiteStatement stmt, string paramName);

    internal abstract int    ColumnCount(SQLiteStatement stmt);
    internal abstract string ColumnName(SQLiteStatement stmt, int index);
    internal abstract string ColumnType(SQLiteStatement stmt, int index, out TypeAffinity nAffinity);
    internal abstract int    ColumnIndex(SQLiteStatement stmt, string columnName);

    internal abstract double   GetDouble(SQLiteStatement stmt, int index);
    internal abstract Int32    GetInt32(SQLiteStatement stmt, int index);
    internal abstract Int64    GetInt64(SQLiteStatement stmt, int index);
    internal abstract string   GetText(SQLiteStatement stmt, int index);
    internal abstract long     GetBytes(SQLiteStatement stmt, int index, int nDataoffset, byte[] bDest, int nStart, int nLength);
    internal abstract long     GetChars(SQLiteStatement stmt, int index, int nDataoffset, char[] bDest, int nStart, int nLength);
    internal abstract DateTime GetDateTime(SQLiteStatement stmt, int index);
    internal abstract bool     IsNull(SQLiteStatement stmt, int index);

    internal abstract int  CreateCollation(string strCollation, SQLiteCollation func);
    internal abstract int  CreateFunction(string strFunction, int nArgs, SQLiteCallback func, SQLiteCallback funcstep, SQLiteCallback funcfinal);
    internal abstract void FreeFunction(int nCookie);

    internal abstract int AggregateCount(int context);
    internal abstract int AggregateContext(int context);

    internal abstract long   GetParamValueBytes(int ptr, int nDataOffset, byte[] bDest, int nStart, int nLength);
    internal abstract double GetParamValueDouble(int ptr);
    internal abstract int    GetParamValueInt32(int ptr);
    internal abstract Int64  GetParamValueInt64(int ptr);
    internal abstract string GetParamValueText(int ptr);
    internal abstract TypeAffinity GetParamValueType(int ptr);

    internal abstract void ReturnBlob(int context, byte[] value);
    internal abstract void ReturnDouble(int context, double value);
    internal abstract void ReturnError(int context, string value);
    internal abstract void ReturnInt32(int context, Int32 value);
    internal abstract void ReturnInt64(int context, Int64 value);
    internal abstract void ReturnNull(int context);
    internal abstract void ReturnText(int context, string value);

    protected virtual void Dispose(bool bDisposing)
    {
    }
    public void Dispose()
    {
      Dispose(true);
      GC.SuppressFinalize(this);
    }
  }
}

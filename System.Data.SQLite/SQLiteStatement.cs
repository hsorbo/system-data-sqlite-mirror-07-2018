﻿/********************************************************
 * ADO.NET 2.0 Data Provider for SQLite Version 3.X
 * Written by Robert Simpson (robert@blackcastlesoft.com)
 * 
 * Released to the public domain, use at your own risk!
 ********************************************************/

namespace System.Data.SQLite
{
  using System;
  using System.Collections.Generic;

  /// <summary>
  /// Represents a single SQL statement in SQLite.
  /// </summary>
  internal sealed class SQLiteStatement : IDisposable
  {
    /// <summary>
    /// The underlying SQLite object this statement is bound to
    /// </summary>
    internal SQLiteBase                          _sql;
    /// <summary>
    /// The command text of this SQL statement
    /// </summary>
    internal string                              _sqlStatement;
    /// <summary>
    /// The actual statement pointer
    /// </summary>
    internal int                                 _sqlite_stmt;
    /// <summary>
    /// An index from which unnamed parameters begin
    /// </summary>
    internal int                                 _unnamedParameterStart;
    /// <summary>
    /// Names of the parameters as SQLite understands them to be
    /// </summary>
    internal string[]          _paramNames;
    /// <summary>
    /// Parameters for this statement
    /// </summary>
    internal SQLiteParameter[] _paramValues;

    /// <summary>
    /// Initializes the statement and attempts to get all information about parameters in the statement
    /// </summary>
    /// <param name="sqlbase">The base SQLite object</param>
    /// <param name="stmt">The statement</param>
    /// <param name="strCommand">The command text for this statement</param>
    /// <param name="nCmdStart">The index at which to start numbering unnamed parameters</param>
    internal SQLiteStatement(SQLiteBase sqlbase, int stmt, string strCommand, ref int nCmdStart)
    {
      _paramNames = null;
      _paramValues = null;

      _unnamedParameterStart   = nCmdStart;
      _sql     = sqlbase;
      _sqlite_stmt = stmt;
      _sqlStatement  = strCommand;

      // Determine parameters for this statement (if any) and prepare space for them.
      int n = _sql.Bind_ParamCount(this);
      int x;
      string s;

      if (n > 0)
      {
        _paramNames = new string[n];
        _paramValues = new SQLiteParameter[n];

        for (x = 0; x < n; x++)
        {
          s = _sql.Bind_ParamName(this, x + 1);
          if (s == null || s == "")
          {
            s = String.Format(";{0}", nCmdStart);
            nCmdStart++;
          }
          _paramNames[x] = s;
          _paramValues[x] = null;
        }
      }
    }

    /// <summary>
    /// Called by SQLiteParameterCollection, this function determines if the specified parameter name belongs to
    /// this statement, and if so, keeps a reference to the parameter so it can be bound later.
    /// </summary>
    /// <param name="s">The parameter name to map</param>
    /// <param name="p">The parameter to assign it</param>
    internal void MapParameter(string s, SQLiteParameter p)
    {
      if (_paramNames == null) return;

      int x = _paramNames.Length;
      for (int n = 0; n < x; n++)
      {
        if (String.Compare(_paramNames[n], s, true) == 0)
        {
          _paramValues[n] = p;
          break;
        }
      }
    }

    #region IDisposable Members
    /// <summary>
    /// Disposes and finalizes the statement
    /// </summary>
    public void Dispose()
    {
      _sql.Finalize(this);
      
      _paramNames = null;
      _paramValues = null;
      _sql = null;
      _sqlStatement = null;

      GC.SuppressFinalize(this);
    }
    #endregion
    
    /// <summary>
    ///  Bind all parameters, making sure the caller didn't miss any
    /// </summary>
    internal void BindParameters()
    {
      if (_paramNames == null) return;

      int x = _paramNames.Length;
      for (int n = 0; n < x; n++)
      {
        BindParameter(n + 1, _paramValues[n]);
      }
    }

    /// <summary>
    /// Perform the bind operation for an individual parameter
    /// </summary>
    /// <param name="index">The index of the parameter to bind</param>
    /// <param name="param">The parameter we're binding</param>
    private void BindParameter(int index, SQLiteParameter param)
    {
      object obj = param.Value;

      if (Convert.IsDBNull(obj) || obj == null)
      {
        _sql.Bind_Null(this, index);
        return;
      }

      switch (param.DbType)
      {
        case DbType.Date:
        case DbType.Time:
        case DbType.DateTime:
            _sql.Bind_DateTime(this, index, Convert.ToDateTime(obj));
          break;
        case DbType.Int64:
        case DbType.UInt64:
          _sql.Bind_Int64(this, index, Convert.ToInt64(obj));
          break;
        case DbType.Boolean:
        case DbType.Int16:
        case DbType.Int32:
        case DbType.UInt16:
        case DbType.UInt32:
        case DbType.SByte:
        case DbType.Byte:
            _sql.Bind_Int32(this, index, Convert.ToInt32(obj));
          break;
        case DbType.Single:
        case DbType.Double:
        case DbType.Currency:
        case DbType.Decimal:
          _sql.Bind_Double(this, index, Convert.ToDouble(obj));
          break;
        case DbType.Binary:
          _sql.Bind_Blob(this, index, (byte[])obj);
          break;
        default:
          _sql.Bind_Text(this, index, obj.ToString());
          break;
      }
    }
  }
}

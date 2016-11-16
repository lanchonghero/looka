#include <stdio.h>
#include "looka_mysql_wrapper.hpp"

MysqlWrapper::MysqlWrapper(const MysqlParams& tParams, bool initConnect):
  m_bSqlConnected(false), m_pMysqlResult(NULL), m_pMysqlFields(NULL), m_tMysqlRow(NULL), m_pMysqlLengths(NULL)
{
  m_tParams = tParams;
  char sBuf[1024];
  snprintf(sBuf, sizeof(sBuf), "sql://%s:***@%s:%d/%s",
    m_tParams.m_sUser.c_str(), m_tParams.m_sHost.c_str(),
    m_tParams.m_iPort, m_tParams.m_sDB.c_str());
  m_sSqlDSN = std::string(sBuf);

  if (initConnect && !m_bSqlConnected) {
    SqlConnect();
  }
}

MysqlWrapper::~MysqlWrapper()
{
  if (m_bSqlConnected) {
    SqlDisconnect();
  }
}

void MysqlWrapper::SqlDismissResult()
{
  if (!m_pMysqlResult)
    return;

  while (m_pMysqlResult) {
    mysql_free_result(m_pMysqlResult);
    m_pMysqlResult = NULL;

    // stored procedures might return multiple result sets
    // FIXME? we might want to index all of them
    // but for now, let's simply dismiss additional result sets
    if (mysql_next_result(&m_tMysqlDriver) == 0) {
      m_pMysqlResult = mysql_use_result(&m_tMysqlDriver);

      static bool bOnce = false;
      if ( !bOnce && m_pMysqlResult && mysql_num_rows(m_pMysqlResult)) {
        fprintf(stdout, "indexing of multiple result sets is not supported yet; some results sets were dismissed!" );
        bOnce = true;
      }
    }
  }

  m_pMysqlFields = NULL;
  m_pMysqlLengths = NULL;
}

bool MysqlWrapper::SqlQuery()
{
  const char* query = m_tParams.m_sQuery.c_str();
  if (mysql_query(&m_tMysqlDriver, query)) {
    if (m_tParams.m_bPrintQueries)
      fprintf( stdout, "SQL-QUERY: %s: FAIL\n", query);
    return false;
  }
  if (m_tParams.m_bPrintQueries)
    fprintf(stdout, "SQL-QUERY: %s: ok\n", query);

  m_pMysqlResult = mysql_use_result(&m_tMysqlDriver);
  m_pMysqlFields = NULL;
  return true;
}

bool MysqlWrapper::SqlQuery(const char* query)
{
  if (mysql_query(&m_tMysqlDriver, query)) {
    if (m_tParams.m_bPrintQueries)
      fprintf(stdout, "SQL-QUERY: %s: FAIL\n", query);
    return false;
  }
  if (m_tParams.m_bPrintQueries)
    fprintf(stdout, "SQL-QUERY: %s: ok\n", query);

  m_pMysqlResult = mysql_use_result(&m_tMysqlDriver);
  m_pMysqlFields = NULL;
  return true;
}

bool MysqlWrapper::SqlIsError()
{
  return mysql_errno(&m_tMysqlDriver) != 0;
}

const std::string MysqlWrapper::SqlError()
{
  return std::string(mysql_error(&m_tMysqlDriver));
}

bool MysqlWrapper::SqlConnect()
{
  // do not connect twice
  if (m_bSqlConnected)
    return true;

  mysql_init(&m_tMysqlDriver);

  if (!m_tParams.m_sSslKey.empty() || 
    !m_tParams.m_sSslCert.empty() || 
    !m_tParams.m_sSslCA.empty()) {
    mysql_ssl_set(&m_tMysqlDriver, 
      m_tParams.m_sSslKey.c_str(),
      m_tParams.m_sSslCert.c_str(),
      m_tParams.m_sSslCA.c_str(), NULL, NULL);
  }

  int  flags = (m_tParams.m_iFlags | CLIENT_MULTI_RESULTS); // we now know how to handle this

  bool bRes  = (NULL != mysql_real_connect(&m_tMysqlDriver, 
      m_tParams.m_sHost.c_str(), m_tParams.m_sUser.c_str(),
      m_tParams.m_sPass.c_str(), m_tParams.m_sDB.c_str(), m_tParams.m_iPort, 
      m_tParams.m_sUsock.c_str(), flags));
  if (m_tParams.m_bPrintQueries)
    fprintf(stdout, bRes ? "SQL-CONNECT: ok\n" : "SQL-CONNECT: FAIL\n");
  if (bRes) {
    m_bSqlConnected = true;
    for (size_t i=0; i<m_tParams.m_vQueryPre.size(); i++) {
      std::string query = m_tParams.m_vQueryPre[i];
      if (!query.empty()) {
        SqlQuery(query.c_str());
      }
    }
  } else {
    fprintf(stdout, "sql_connect: %s (DSN=%s)\n", SqlError().c_str(), m_sSqlDSN.c_str());
  }

  return bRes;
}

void MysqlWrapper::SqlDisconnect()
{
  if (!m_bSqlConnected)
    return;
  
  if (m_pMysqlResult) {
    SqlDismissResult();
  }

  for (size_t i=0; i<m_tParams.m_vQueryPost.size(); i++) {
    std::string query = m_tParams.m_vQueryPost[i];
    if (!query.empty()) {
      SqlQuery(query.c_str());
    }
  }
  
  if (m_pMysqlResult) {
    SqlDismissResult();
  }

  if (m_tParams.m_bPrintQueries)
    fprintf(stdout, "SQL-DISCONNECT\n");

  mysql_close(&m_tMysqlDriver);
  m_bSqlConnected = false;
}

int MysqlWrapper::SqlNumFields()
{
  if (!m_pMysqlResult)
    return -1;

  return mysql_num_fields(m_pMysqlResult);
}

bool MysqlWrapper::SqlFetchRow(std::vector<MysqlField>& vfs)
{
  vfs.clear();
  if (!SqlFetchRow())
    return false;

  int numFields;
  if ((numFields=SqlNumFields()) == -1)
    return false;

  for (int i=0; i<numFields; i++) {
    const char* pn = SqlFieldName(i);
    const char* pv = SqlColumn(i);
    if (!pn) return false;
    if (!pv) pv = "";
    MysqlField fs = {std::string(pn), std::string(pv)};
    vfs.push_back(fs);
  }

  return true;
}

bool MysqlWrapper::SqlFetchRow()
{
  if (!m_pMysqlResult)
    return false;

  m_tMysqlRow = mysql_fetch_row(m_pMysqlResult);
  return m_tMysqlRow != NULL;
}

unsigned long MysqlWrapper::SqlColumnLength(int iIndex)
{
  if (!m_pMysqlResult)
    return 0;

  if (!m_pMysqlLengths)
    m_pMysqlLengths = mysql_fetch_lengths(m_pMysqlResult);

  return m_pMysqlLengths[iIndex];
}


const char* MysqlWrapper::SqlColumn(int iIndex)
{
  if (!m_pMysqlResult)
    return NULL;

  return m_tMysqlRow[iIndex];
}


const char* MysqlWrapper::SqlFieldName(int iIndex)
{
  if (!m_pMysqlResult)
    return NULL;

  if (!m_pMysqlFields)
    m_pMysqlFields = mysql_fetch_fields(m_pMysqlResult);

  return m_pMysqlFields[iIndex].name;
}

const MysqlParams MysqlWrapper::GetParams() const
{
  return m_tParams;
}

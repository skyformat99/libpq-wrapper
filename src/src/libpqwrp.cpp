#ifndef LIBPQWRP_CPP_INCLUDED
#define LIBPQWRP_CPP_INCLUDED

#include <libpqwrp.h>     // name

namespace libpqwrp
{
///$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
/// 						process
///$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$



unsigned  libPQwrp::count_format_specifiers(std::string const& format)
{
  unsigned count = 0;
  for (size_t i = 0; i < format.size(); ++i)
  {
    if (format[i] == FORMAT_SPECIFIER)
    {
      if ((i != format.size() - 1) && (format[i+1] == FORMAT_SPECIFIER))
        ++i; // skip next % as well, it was consumed by this one
      else
      ++count;
    }
  }
  
  return count;
}



void libPQwrp::prepare(std::string const& format)
{
  if (count_format_specifiers(format) != 0)
    throw std::invalid_argument("number of arguments does not match the format string");
}




std::stringstream  libPQwrp::result()												/// retrieve values on the fields
{	

	char* tptr;
	std::stringstream sql;


    
	for ( int i= 0 ; i <cols ; i ++ )
	{
		tptr = PQgetvalue(res, rown, i);
		

		for (size_t i = 0; i < strlen(tptr); ++i)
		{
			if (tptr[i] == ' ')
			{
				tptr[i] =DeLiMiTaTioN;
			}
		}


		if ( i == 0 ) sql << tptr;
		else sql <<' '<< tptr;

	}
	return  sql;
}




void libPQwrp::connectDB(std::string info)											/// connect to the database
{
	
	clean = true ;
	
	conn= PQconnectdb(info.c_str());  												/// init PGconn
    if (PQstatus(conn) != CONNECTION_OK)
    {  
        char const* msg = PQerrorMessage(conn);
        throw std::runtime_error(std::string(msg));
    }
    res = PQgetResult(conn);														/// init PGresult
}



void libPQwrp::qexec( std::string sql)												/// PQexec 
{
	
	clean = true ;
	
    res = PQexec(conn, sql.c_str());

    if (!res || PQresultStatus(res) != PGRES_COMMAND_OK)
    {	PQclear(res); 
        char const* msg = PQresultErrorMessage(res);
        throw std::runtime_error(std::string(msg));
    }
}


void libPQwrp::query(std::string sql, bool binary )									/// PQquery
{
    int const fmt = binary ? 1 : 0;
    
	clean = true ;
	
    res = PQexecParams(conn, sql.c_str(), 0, 0, 0, 0, 0, fmt);
    
	if ( PGRES_BAD_RESPONSE == PQresultStatus(res) || \
		PGRES_NONFATAL_ERROR == PQresultStatus(res) || \
		PGRES_FATAL_ERROR ==PQresultStatus(res) )
		{
			PQclear(res);
			char const* msg =PQresultErrorMessage(res) ;      
			throw std::runtime_error(std::string(msg));
		}

	std::string operation = PQcmdStatus(res); 
	if ( operation =="FETCH 0"  ) fetchEOF =true; else fetchEOF =false ;   
    if ( operation =="UPDATE 0" ) fetchEOF =true; else fetchEOF =false ;  
    if ( operation =="SELECT 0" ) fetchEOF =true; else fetchEOF =false ;
    if ( operation =="DELETE 0" ) fetchEOF =true; else fetchEOF =false ;
    if ( operation =="INSERT 0" ) fetchEOF =true; else fetchEOF =false ;

}


char* libPQwrp::fetch( int row, int column)											/// get the value of the row and the column
{
	if (row < 0)
		throw std::invalid_argument("row position is negative");

	if (column < 0 || column > PQnfields(res) )
		throw std::invalid_argument("column position is negative");

	return PQgetvalue(res, row, column);
}



double libPQwrp::fetchDbl(int row, int column)										/// get the value of the row and the column
{
	if (row < 0)
		throw std::invalid_argument("row position is negative");

	if (column < 0 || column > PQnfields(res) )
		throw std::invalid_argument("column position is negative");

	return atof(PQgetvalue(res, row, column));
}



int libPQwrp::fetchInt(int row, int column)											/// get the value of the row and the column
{
	if (row < 0)
		throw std::invalid_argument("row position is negative");

	if (column < 0 || column > PQnfields(res) )
		throw std::invalid_argument("column position is negative");

	return atoi(PQgetvalue(res, row, column));

}


int libPQwrp::nfield(std::string field)												/// number column of the field
{
	int col =  PQfnumber(res, field.c_str());

	if ( col<0 )
	{
		std::string msg="bad name field :"  + field  ;      
		throw std::runtime_error(msg);
	}
	return col;
}


const char* libPQwrp::cfield(int nfield)											/// name of the field 
{
	if (nfield < 0 || nfield > PQnfields(res) )
	{
		std::string msg="bad n° field :"  + std::to_string(nfield) ;      
		throw std::runtime_error(msg);
    }
	return  PQfname(res,nfield);

}


int libPQwrp::countfield()															/// number of columns
{
	return  PQnfields(res);
}



int libPQwrp::countrow()															/// number of rows
{
	return PQntuples(res);	
}


bool libPQwrp::is_Table(const char* table)											/// if exist table of the database 
{
	static std::string const exist_Table = "SELECT count(*)  FROM pg_tables  WHERE  schemaname ='public' AND    tablename = '?'";
 
	const std::string sql = prepare(exist_Table ,table);

	bool binary = false;															/// false for return value count

	int const fmt = binary ? 1 : 0;
 
    res = PQexecParams(conn, sql.c_str(), 0, 0, 0, 0, 0, fmt);
    
	int count = atoi(PQgetvalue(res, 0, 0));
	
	PQclear(res);
	
	clean = false;
	
	
	return count;
}


void libPQwrp::closeDB()															/// close to the database 
{
	clearRes();
	PQfinish(conn);
}



void libPQwrp::begin()																/// start transction for commit
{
	PQexec(conn,"BEGIN");
}


void libPQwrp::commit()																/// commit for transaction init
{
	PQexec(conn,"COMMIT");
	clearRes();
}


void libPQwrp::rollback()															/// rollbck for transaction init
{
	PQexec(conn,"ROLLBACK");
	clearRes();
}


void libPQwrp::end()																/// end transaction for init 
{
	PQexec(conn,"END");
	clearRes();
}


void libPQwrp::clearRes()															/// clear PGresult
{
	if (clean) { PQclear(res); clean = false; }
}


void libPQwrp::savpoint()															/// point de sauvegarde
{
	PQexec(conn,"SAVEPOINT full_savepoint");
	clearRes(); 
}


void libPQwrp::savpointRollback()													/// roolback base savepoint
{
	PQexec(conn,"ROLLBACK TO SAVEPOINT full_savepoint");
	clearRes();
}


void libPQwrp::savpointRelease()													/// delete savepoint
{
	PQexec(conn,"RELEASE SAVEPOINT full_savepoint");
	clearRes();
}



///-------------------------------------------------------------------------------------------------------
///-------------- process  complex
///-------------------------------------------------------------------------------------------------------


void libPQwrp::fetchAll( std::string sql, std::string cursor)						/// fetch ALL include requete 
{
	std::string  ordreSQL;

	clean = true ;
	 
    ordreSQL =  "DECLARE " + cursor + " CURSOR FOR "+ sql;
 std::cout<<"code status  :"<<PQresultStatus(res)<<" OK:"<<PGRES_TUPLES_OK<<"  value result  "<<PQcmdStatus(res)<<"  PGRES_COMMAND_OK   "<<PGRES_COMMAND_OK<<std::endl;

	res = PQexec(conn, ordreSQL.c_str());
    if (!res || PQresultStatus(res) != PGRES_COMMAND_OK)
    {
		PQclear(res);
		char const* msg = PQresultErrorMessage(res);
		throw std::runtime_error(std::string(msg));
	}

	ordreSQL = "FETCH ALL in " + cursor ;	/// lecture full rc  ??? memory   
 	res = PQexec(conn, ordreSQL.c_str());
 std::cout<<"code status  :"<<PQresultStatus(res)<<" OK:"<<PGRES_TUPLES_OK<<"  value result  "<<PQcmdStatus(res)<<"  PGRES_COMMAND_OK   "<<PGRES_COMMAND_OK<<std::endl;

    if (!res || PQresultStatus(res) != PGRES_TUPLES_OK)
    {
		PQclear(res);
		char const* msg = PQresultErrorMessage(res);
		throw std::runtime_error(std::string(msg));
    }

	std::string operation = PQcmdStatus(res);  
	if ( !res ||  PQresultStatus(res) != PGRES_TUPLES_OK || operation =="FETCH 0"  )
	{
		fetchEOF  =true ;   				/// end of row or without row
		ordreSQL = "close" + cursor ;		/// close cursor
		PQexec(conn, ordreSQL.c_str());
		PQclear(res);
		clean = false ;
	}
	else fetchEOF  =false ;					/// no end of row


	rows	= PQntuples(res);
	cols	= PQnfields(res);
	rown	= 0;
	coln	= 0;


}



void libPQwrp::opensql( std::string sql, std::string cursor)						///  query for fetchsql record / record 
{
	std::string  ordreSQL;

	clean = true ;
		 
    ordreSQL =  "DECLARE " + cursor + " CURSOR FOR "+ sql;

    res = PQexec(conn, ordreSQL.c_str()); 
	if (!res || PQresultStatus(res) != PGRES_COMMAND_OK)
	{
		PQclear(res);
		char const* msg = PQresultErrorMessage(res);
		throw std::runtime_error(std::string(msg));
	}
	
	ordreSQL = "FETCH FIRST in " + cursor ;	/// read a line record only
	res = PQexec(conn, ordreSQL.c_str());    

	std::string operation = PQcmdStatus(res); 
	if (!res ||  PQresultStatus(res) != PGRES_TUPLES_OK || operation =="FETCH 0"  )
	{
		fetchEOF  =true ;					/// end of row or without row
		ordreSQL = "close" + cursor ;		/// close cursor
		PQexec(conn, ordreSQL.c_str());
		PQclear(res);
		clean = false ;
	}
	else
	{
		fetchEOF  =false ;					/// no end of row
	}
	rows	= PQntuples(res);
	cols	= PQnfields(res);
	rown	= 0;
	coln	= 0;

}




void libPQwrp::fechsql(std::string cursor )											/// fetch record use openSQL
{
	std::string  ordreSQL;

	clean = true ;
		
    ordreSQL = "FETCH NEXT in " + cursor ;	/// read a line record only

	res = PQexec(conn, ordreSQL.c_str());
 	std::string operation = PQcmdStatus(res);   
	if (!res ||  PQresultStatus(res) != PGRES_TUPLES_OK ||operation =="FETCH 0"  )
	{
		fetchEOF  =true ;					/// end of row or without row
		ordreSQL = "close" + cursor ;		/// close cursor
		PQexec(conn, ordreSQL.c_str());
		PQclear(res);
		clean = false;
	}
	else fetchEOF  =false ;					/// no end of row

	rows	= PQntuples(res);
	cols	= PQnfields(res);
	rown	= 0;
	coln	= 0; 
}




char* libPQwrp::DoubleToChar(double _X_ ,unsigned _precision_)						/// Double to char 
{

	char * X_value   = (char*) malloc(36  * sizeof(char*));   sprintf(X_value,"%c",'\0');
	char * X_cmd     = (char*) malloc(36  * sizeof(char*));
	char sign = _X_ >= 0 ? '+' : '-';

	if ( sign == '-')  // signed -
	{
		 sprintf(X_cmd,"%s%c%d.0%d%c","%c",'%',0,_precision_,'f');
		 
		 snprintf(X_value + strlen(X_value), 16, X_cmd , sign, fabs(_X_));
	}
	else // not signed +
	{
		sprintf(X_cmd,"%c%d.0%d%c",'%',0,_precision_,'f');
		
		snprintf(X_value + strlen(X_value), 16, X_cmd , fabs(_X_));
	}

	return (char*) X_value;

}



} // namespace PQWRP


#endif // LIBPQWRP_CPP_INCLUDED
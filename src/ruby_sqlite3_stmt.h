#ifndef RUBY_SQLITE_STMT_DECL
#define RUBY_SQLITE_STMT_DECL

#include "sqlite3.h"

extern "C" 
{
void init_sqlite3_stmt(mrb_state* mrb, RClass* module);
RClass* sqlite3_stmt_class();
mrb_value make_sqlite3_stmt(mrb_state* mrb, mrb_value connection, sqlite3* db, mrb_value sql);    
extern const mrb_data_type ruby_sqlite_statement_type;
}

#endif

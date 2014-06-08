#ifndef RUBY_SQLITE_CONNECTION_DECL
#define RUBY_SQLITE_CONNECTION_DECL

#include "sqlite3.h"

extern "C"
{

void init_sqlite3(mrb_state* vm, RClass* module);
RClass* sqlite3_class();
sqlite3* get_sqlite3_handle(mrb_value self);
mrb_value make_sqlite3_object(sqlite3* handle);

}

#endif

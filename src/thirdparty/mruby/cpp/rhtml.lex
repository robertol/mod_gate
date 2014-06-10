%{

#include "rhtml_2_parser.hpp"

#define YY_DECL int lexscan(rsp::Rhtml2Parser& s, yyscan_t yyscanner)
%}

%option noyywrap
%option reentrant

%x TEXT
%x CODE
%x INDENT
%x COMMENT

%%

\r\n|\n {

    yyless(0);
    BEGIN TEXT;
    //s.change(rsp::Rhtml2Parser::TEXT_STATE);
}

. {
    yyless(0);
    BEGIN TEXT;
}

<TEXT>"\\n" {

}

<TEXT>"\\r" {

}

<TEXT>"\\t" {

}

<TEXT>"<%" {
    s.change(rsp::Rhtml2Parser::CODE_STATE);

    BEGIN CODE;
}

<TEXT>"{%" {
    s.change(rsp::Rhtml2Parser::CODE_STATE);

    BEGIN CODE;
}

<TEXT>"<%=" {
    s.change(rsp::Rhtml2Parser::VAR_STATE);
    s.append("print((");

    BEGIN CODE;
}

<TEXT>"{%=" {
    s.change(rsp::Rhtml2Parser::VAR_STATE);
    s.append("print((");

    BEGIN CODE;
}

<CODE>"%>" {
    if(s.state() == rsp::Rhtml2Parser::VAR_STATE)
    {
        s.append("))");
    }

    s.change(rsp::Rhtml2Parser::TEXT_STATE);

    BEGIN TEXT;
}

<CODE>"%}" {
    if(s.state() == rsp::Rhtml2Parser::VAR_STATE)
    {
        s.append("))");
    }

    s.change(rsp::Rhtml2Parser::TEXT_STATE);

    BEGIN TEXT;
}

<TEXT>"<%--" {
    s.change(rsp::Rhtml2Parser::COMMENT_STATE);

    BEGIN COMMENT;
}

<COMMENT>"%>" {
    s.change(rsp::Rhtml2Parser::TEXT_STATE);

    BEGIN TEXT;
}


<TEXT>. {
    s.read(yytext);
}


<CODE>. {
    s.read(yytext);
}

<COMMENT>. {
    s.read(yytext);
}


<TEXT>\r\n|\n {
    s.end_line();
}

<CODE>\r\n|\n {
    s.end_line();
}

<COMMENT>\r\n|\n {
    s.end_line();
}

<COMMENT>"--%>" {
    BEGIN TEXT;
}

%%

using namespace rsp;

void Rhtml2Parser::init()
{
    line.clear();
    text.clear();
}

int Rhtml2Parser::finalize()
{    
    change(rsp::Rhtml2Parser::TEXT_STATE);

    return 0;
}

bool Rhtml2Parser::compile_file(const char* filename)
{
    // Check to see if file exists
    if(access(filename, R_OK) != 0)
    {
        return false;
    }

    init();

    yylex_init(&scanner);

    FILE* in;

    if(filename != NULL)
    {
        in = fopen(filename, "r");

        _file_name = filename;
    }
    else
    {
        in = stdin;
    }

    yyset_in(in, scanner);

    int ret = lexscan(*this, scanner);
    yylex_destroy(scanner);
    fclose(in);

    finalize();

    return true;
}

bool Rhtml2Parser::compile_text(const char* str)
{
    init();
    yylex_init(&scanner);
    YY_BUFFER_STATE lexstate = yy_scan_bytes(str, strlen(str), scanner);
    yy_switch_to_buffer(lexstate, scanner);
    int ret = lexscan(*this, scanner);
    finalize();
    yy_delete_buffer(lexstate, scanner);
    yylex_destroy(scanner);    

    return true;
}

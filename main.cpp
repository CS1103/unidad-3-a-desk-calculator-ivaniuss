#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <cctype>
#include <cmath>
using namespace std;

// Tokens accepted by the parser/lexer:
enum Token_value
{
    NAME,			NUMBER,			END,			FUNC = '$',
    ADD = '+', 		SUB = '-', 		MUL = '*', 		DIV = '/',
    PRINT = ';', 	ASSIGN = '=',	LP = '(',		RP = ')',
    EXPO = '^',		LCB = '{',		RCB = '}',		COMMA = ','
};

Token_value curr_tok = PRINT;

// Lexer input function:
Token_value get_token();

// Global variables that are used to get the value of a token:
double number_value;
string string_value;

// Error handling:
int no_of_errors;
double error(const string& s);

// Symbol table used to store variables and pre-defined constants:
map<string, double> globals;

struct Function
{
    Function() : arg_count(0) {}
    Function(const string& nm) : name(nm), arg_count(0) {}

    string name;			// name of function
    size_t arg_count;		// number of arguments a function accepts
    map<string, double> arg_table;	// Dictionary of argument name/values
    string body;			// a function's definition
};

// Symbol table for pre/user-defined functions:
map<string, Function> func_table;

// Returns whether a function with the given name exists in the func_table:
bool funcExists(const string& name);

// Parser functions(uses recursive descent):
double expr(bool);
double term(bool);
double prim(bool);

// Pointer to the input stream we're getting our input from:
istream* input;

map<string, double>* table;

int main(int argc, char* argv[])
{
    switch(argc)
    {
        case 1:					// Read input from standard input
            input = &cin;
            break;
        case 2:					// Read input from argument string
        {
            istringstream ss(argv[1]);
            input = &ss;
            break;
        }
        default:
            error("too many arguments");
            return 1;
    }

    // Setup the symbol table to point to the globals:
    table = &globals;

    // Set pre-defined constants:
    table->operator[]("pi") = 3.1415926535897932385;
    table->operator[]("e") = 2.7182818284590452354;

    while(*input)
    {
        get_token();
        if(curr_tok == END) break;
        if(curr_tok == PRINT) continue;
        cout << expr(false) << endl;
    }

    cout << endl;

    return no_of_errors;
}

// Error handling function:
double error(const string& s)
{
    ++no_of_errors;
    cerr << "Error: \"" << s << "\"" << endl;
    return 1;
}

// Lexer's input function:
Token_value get_token()
{
    char ch = 0;

    do
    {	// Skip whitespace except for '\n'
        if(!input->get(ch))
            return curr_tok = END;
    } while(ch != '\n' && isspace(ch));

    switch(ch)
    {
        case 0:
            return curr_tok = END;
        case ';':
        case '\n':
            return curr_tok = PRINT;
        case '+':
        case '-':
        case '*':
        case '/':
        case '(':
        case ')':
        case '=':
        case ',':
        case '^':
        case '}':
            return curr_tok = Token_value(ch);
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
        case '.':
            input->putback(ch);
            *input >> number_value;
            return curr_tok = NUMBER;
        case '$':						// FUNC definition or call:
        {
            if(!input->get(ch) || !isalpha(ch))
                return curr_tok = END;
            else
                string_value = ch;
            while(input->get(ch) && (isalnum(ch) || ch == '_'))
            {
                string_value += ch;
            }
            input->putback(ch);
            return curr_tok = FUNC;
        }
        case '{':
        {
            string_value.clear();
            while(input->get(ch) && ch != '}')
            {
                string_value += ch;
            }
            return curr_tok = LCB;
        }
        default:						// NAME, NAME =, or error:
            if(isalpha(ch))
            {
                string_value = ch;
                while(input->get(ch) && (isalnum(ch) || ch == '_'))
                    string_value.push_back(ch);
                input->putback(ch);
                return curr_tok = NAME;
            }
            error("bad token");
            return curr_tok = PRINT;
    }
}

// Handles addition and subtraction:
double expr(bool get)
{
    double left = term(get);

    for(;;)
    {
        switch(curr_tok)
        {
            case ADD:
            {
                left += term(true);
                break;
            }
            case SUB:
            {
                left -= term(true);
                break;
            }
            default:
                return left;
        }
    }
}

// Handles multiplication and division:
double term(bool get)
{
    double left = prim(get);

    for(;;)
    {
        switch(curr_tok)
        {
            case MUL:
            {
                left *= prim(true);
                break;
            }
            case DIV:
            {
                if(double d = prim(true))
                {
                    left /= d;
                    break;
                }
                return error("divide by zero");
            }
            case EXPO:
            {
                left = pow(left, prim(true));
                break;
            }
            default:
                return left;
        }
    }
}

// Handles primaries, like name or number:
double prim(bool get)
{
    if(get) get_token();

    switch(curr_tok)
    {
        case NUMBER:
        {
            double v = number_value;
            get_token();
            return v;
        }
        case NAME:
        {
            double& v = table->operator[](string_value);
            if(get_token() == ASSIGN)
                v = expr(true);
            return v;
        }
        case FUNC:
        {
            if(funcExists(string_value))	// Function call
            {
                Function f(func_table[string_value]);

                if(get_token() == LP)
                {
                    map<string, double>::iterator it = f.arg_table.begin();
                    while(get_token() != RP && curr_tok != END)
                    {
                        if(it == f.arg_table.end()) continue;
                        switch(curr_tok)
                        {
                            case NAME:
                                if(table->count(string_value) > 0)
                                {
                                    it->second = table->operator[](string_value);
                                    ++it;
                                    break;
                                }
                                else
                                    return error("unknown function argument");
                            case NUMBER:
                                it->second = number_value;
                                ++it;
                                break;
                            case COMMA:
                                continue;
                            default:
                                return error("invalid function argument");
                        }
                    }
                    // Check if the number of arguments are the same
                    // for the function call and the function definition:
                    if(f.arg_table.size() == f.arg_count)
                    {
                        // Now evaluate the function:
                        istringstream func(f.body);
                        istream* tmp = input;
                        input = &func;
                        map<string, double>* tmptable = table;
                        table = &f.arg_table;
                        it = tmptable->begin();
                        // Copy global variables to the symbol table:
                        while(it != tmptable->end())
                        {
                            table->operator[](it->first) = it->second;
                            ++it;
                        }
                        double value = 0;
                        while(*input)		// While their is input
                        {
                            get_token();
                            if(curr_tok == END) break;
                            if(curr_tok == PRINT) continue;
                            value = expr(false);
                        }
                        input = tmp;
                        table = tmptable;
                        return value;
                    }
                    else
                    {
                        return error("invalid number of arguments");
                    }
                }
                else
                {
                    return error("'(' expected in function call");
                }
            }
            else							// Function definition
            {
                Function f(string_value);
                if(get_token() == LP)		// Get arguments
                {
                    while(get_token() != RP && curr_tok != END)
                    {
                        switch(curr_tok)
                        {
                            case NAME:
                                f.arg_table[string_value];
                                break;
                            case COMMA:
                                continue;
                                break;
                            default:
                                return error("invalid function parameter");
                        }
                    }
                    f.arg_count = f.arg_table.size();
                    // Now read the body of the function:
                    if(get_token() == LCB)
                    {
                        f.body = string_value;
                    }
                    else
                    {
                        return error("undefined function; missing definition");
                    }
                    // Done reading in function, store it in symbol table:
                    func_table[f.name] = f;
                    return 0;
                }
                else
                {
                    return error("'(' expected in function definition");
                }

                return 0;
            }
        }
        case SUB:
            return -prim(true);
        case LP:
        {
            double e = expr(true);
            if(curr_tok != RP)
                return error("')' expected, but not found");
            get_token();		// Eat ')'
            return e;
        }
        default:
            return error("primary expected");
    }
}

bool funcExists(const string& name)
{
    return func_table.count(name) ? true : false;
}

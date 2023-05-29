// cisp.cpp 
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <list>
#include <map>

// return given number as a string
std::string stringify(long n) {
    std::ostringstream outputString;
    outputString << n;
    return outputString.str();
}

// return true if given character is '0'..'9'
bool isDigit(char c) {
    return isdigit(static_cast<unsigned char>(c)) != 0;
}


////////////////////// cell/token type

enum cellType {
    Symbol,
    Number,
    List,
    Proc,
    Lambda
};

struct environment; // forward declaration; cell and environment reference each other

// a variant that can hold any kind of lisp value
struct cell {
    // type definitions for readable types below 
    typedef cell(*procType)(const std::vector<cell>&);
    typedef std::vector<cell>::const_iterator iterator;
    typedef std::map<std::string, cell> map;

    // actual fields
    cellType type;
    std::string value;
    std::vector<cell> list;
    procType proc;
    environment* environment;

    // initializers
    cell(cellType type = Symbol) : type(type), environment(0) {}
    cell(cellType type, const std::string& val) : type(type), value(val), environment(0) {}
    cell(procType proc) : type(Proc), proc(proc), environment(0) {}
};

typedef std::vector<cell> cells;
typedef cells::const_iterator cellIterator;

const cell falseSymbol(Symbol, "False");
const cell trueSymbol(Symbol, "True"); // anything that isn't falseSymbol is true
const cell NIL(Symbol, "NIL");


////////////////////// environment

// a dictionary that (a) associates symbols with cells, and
// (b) can chain to an "outer" dictionary
struct environment {
    environment(environment* outer = 0) : outer_(outer) {}

    environment(const cells& parms, const cells& args, environment* outer)
        : outer_(outer)
    {
        cellIterator a = args.begin();
        for (cellIterator p = parms.begin(); p != parms.end(); ++p)
            env_[p->value] = *a++;
    }

    // map a variable name onto a cell
    typedef std::map<std::string, cell> map;

    // return a reference to the innermost environment where 'var' appears
    map& find(const std::string& var)
    {
        if (env_.find(var) != env_.end())
            return env_; // the symbol exists in this environment
        if (outer_)
            return outer_->find(var); // attempt to find the symbol in some "outer" env
        std::cout << "unbound symbol '" << var << "'\n";
        return env_;
    }

    // return a reference to the cell associated with the given symbol 'var'
    cell& operator[] (const std::string& var)
    {
        return env_[var];
    }

private:
    map env_; // inner symbol->cell mapping
    environment* outer_; // next adjacent outer env, or 0 if there are no further environments
};


////////////////////// built-in primitive procedures

cell addition(const cells& c)
{
    // parse the value of the first cell as a long integer
    long n(
        atol(
            c[0]
            .value
            .c_str()
        ));
    // adds up the 2 arguments of the `+` procedure
    for (cellIterator i = c.begin() + 1; i != c.end(); ++i)
        n += atol(i->value.c_str());
    return cell(Number, stringify(n));
}

cell substraction(const cells& c)
{
    long n(
        atol(c[0]
            .value
            .c_str()
        ));
    for (cellIterator i = c.begin() + 1; i != c.end(); ++i)
        n -= atol(i->value.c_str());
    return cell(Number, stringify(n));
}

cell multiplication(const cells& c)
{
    long n(1);
    for (cellIterator i = c.begin(); i != c.end(); ++i)
        n *= atol(i->value.c_str());
    return cell(Number, stringify(n));
}

cell division(const cells& c)
{
    long n(
        atol(
            c[0]
            .value
            .c_str()
        ));
    for (cellIterator i = c.begin() + 1; i != c.end(); ++i)
        n /= atol(i->value.c_str());
    return cell(Number, stringify(n));
}

cell greaterThan(const cells& c)
{
    long n(atol(
        c[0]
        .value
        .c_str()));
    for (cellIterator i = c.begin() + 1; i != c.end(); ++i)
        if (n <= atol(i->value.c_str()))
            return falseSymbol;
    return trueSymbol;
}

cell lessThan(const cells& c)
{
    long n(
        atol(
            c[0]
            .value
            .c_str()
        ));
    for (cellIterator i = c.begin() + 1; i != c.end(); ++i)
        if (n >= atol(i->value.c_str()))
            return falseSymbol;
    return trueSymbol;
}

cell lessOrEqualThan(const cells& c)
{
    long n(
        atol(
            c[0]
            .value
            .c_str()
        ));
    for (cellIterator i = c.begin() + 1; i != c.end(); ++i)
        if (n > atol(i->value.c_str()))
            return falseSymbol;
    return trueSymbol;
}

cell length(const cells& c) {
    return cell(Number, stringify(c[0].list.size()));
}
cell nullPointer(const cells& c) {
    return c[0].list.empty() ? trueSymbol : falseSymbol;
}
cell car(const cells& c) {
    return c[0].list[0];
}

cell cdr(const cells& c)
{
    if (c[0].list.size() < 2)
        return NIL;
    cell result(c[0]);
    result.list.erase(result.list.begin());
    return result;
}

cell append(const cells& c)
{
    cell result(List);
    result.list = c[0].list;
    for (cellIterator i = c[1].list.begin(); i != c[1].list.end(); ++i)
        result.list.push_back(*i);
    return result;
}


cell cons(const cells& c)
{
    cell result(List);
    result.list.push_back(c[0]);
    for (cellIterator i = c[1].list.begin(); i != c[1].list.end(); ++i)
        result.list.push_back(*i);
    return result;
}

cell list(const cells& c)
{
    cell result(List);
    result.list = c;
    return result;
}

cell display(const cells& c)
{
    std::cout << c[0].value << '\n';
    return NIL;
}

// define the bare minimum set of primintives necessary to pass the unit tests
void addGlobals(environment& env)
{
    env["nil"] = NIL;   env["False"] = falseSymbol;  env["True"] = trueSymbol;
    env["display"] = cell(&display);
    env["append"] = cell(&append);   env["car"] = cell(&car);
    env["cdr"] = cell(&cdr);      env["cons"] = cell(&cons);
    env["length"] = cell(&length);   env["list"] = cell(&list);
    env["null?"] = cell(&nullPointer);    env["+"] = cell(&addition);
    env["-"] = cell(&substraction);      env["*"] = cell(&multiplication);
    env["/"] = cell(&division);      env[">"] = cell(&greaterThan);
    env["<"] = cell(&lessThan);     env["<="] = cell(&lessOrEqualThan);
}


////////////////////// eval

cell eval(cell x, environment* env)
{
    if (x.type == Symbol)
        return env->find(x.value)[x.value];
    if (x.type == Number)
        return x;
    if (x.list.empty())
        return NIL;
    if (x.list[0].type == Symbol) {
        if (x.list[0].value == "quote")       // (quote exp)
            return x.list[1];
        if (x.list[0].value == "if")          // (if test conseq [alt])
            return eval(eval(x.list[1], env).value == "False" ? (x.list.size() < 4 ? NIL : x.list[3]) : x.list[2], env);
        if (x.list[0].value == "set!")        // (set! var exp)
            return env->find(x.list[1].value)[x.list[1].value] = eval(x.list[2], env);
        if (x.list[0].value == "define")      // (define var exp)
            return (*env)[x.list[1].value] = eval(x.list[2], env);
        if (x.list[0].value == "lambda") {    // (lambda (var*) exp)
            x.type = Lambda;
            // keep a reference to the environment that exists now (when the
            // lambda is being defined) because that's the outer environment
            // we'll need to use when the lambda is executed
            x.environment = env;
            return x;
        }
        if (x.list[0].value == "begin") {     // (begin exp*)
            for (size_t i = 1; i < x.list.size() - 1; ++i)
                eval(x.list[i], env);
            return eval(x.list[x.list.size() - 1], env);
        }
    }
    // (proc exp*)
    cell proc(eval(x.list[0], env));
    cells exps;
    for (cell::iterator exp = x.list.begin() + 1; exp != x.list.end(); ++exp)
        exps.push_back(eval(*exp, env));
    if (proc.type == Lambda) {
        // Create an environment for the execution of this lambda function
        // where the outer environment is the one that existed* at the time
        // the lambda was defined and the new inner associations are the
        // parameter names with the given arguments.
        // *Although the environmet existed at the time the lambda was defined
        // it wasn't necessarily complete - it may have subsequently had
        // more symbols defined in that environment.
        return eval(/*body*/proc.list[2], new environment(/*parms*/proc.list[1].list, /*args*/exps, proc.environment));
    }
    else if (proc.type == Proc)
        return proc.proc(exps);

    std::cout << "not a function\n";
    return NIL;
}


////////////////////// parse, read and user interaction

// convert given string to list of tokens
std::list<std::string> tokenize(const std::string& str)
{
    std::list<std::string> tokens;
    const char* s = str.c_str();
    while (*s) {
        while (*s == ' ')
            ++s;
        if (*s == '(' || *s == ')')
            tokens.push_back(*s++ == '(' ? "(" : ")");
        else {
            const char* t = s;
            while (*t && *t != ' ' && *t != '(' && *t != ')')
                ++t;
            tokens.push_back(std::string(s, t));
            s = t;
        }
    }
    return tokens;
}

// numbers become Numbers; every other token is a Symbol
cell atom(const std::string& token)
{
    if (isDigit(token[0]) || (token[0] == '-' && isDigit(token[1])))
        return cell(Number, token);
    return cell(Symbol, token);
}

// return the Lisp expression in the given tokens
cell readFrom(std::list<std::string>& tokens)
{
    // get the first token of this expression (it should be a `(`)
    const std::string token(tokens.front());
    // remove the first token, as it's supposed to be a `(`
    tokens.pop_front();
    // this must mean it is indeed an expression
    if (token == "(") {

        cell c(List);
        // continuously consume tokens until we reach `)` (which means the expression is finished)
        while (tokens.front() != ")")
            // pushes tokens into a Lisp expression/cell type
            c.list.push_back(readFrom(tokens));
        // due to recursion  ^^^^^^^^ above, this pops all the tokens after they were successfully consumed
        tokens.pop_front();
        return c;
    }
    // no `(`, it might be an atom, be it a number or a symbol
    else
        return atom(token);
}

// return the Lisp expression represented by the given string
cell read(const std::string& s)
{
    // tokenizes the string
    std::list<std::string> tokens(tokenize(s));
    // tokens -> Lisp expression
    return readFrom(tokens);
}

// convert given cell to a Lisp-readable string
std::string toString(const cell& exp)
{
    if (exp.type == List) {
        std::string s("(");
        // adds elements of the list as part of the output when evaluated
        for (cell::iterator e = exp.list.begin(); e != exp.list.end(); ++e)
            s += toString(*e) + ' ';
        // truncate last list item if it's somehow a whitespace
        if (s[s.size() - 1] == ' ')
            s.erase(s.size() - 1);
        // add closing bracket
        return s + ')';
    }
    // shows that a lambda was defined
    else if (exp.type == Lambda)
        return "<Lambda>";
    // shows a procedure was defined
    else if (exp.type == Proc)
        return "<Proc>";
    // if it's not a list, lambda, or procedure, it must be an atom (symbol or number)
    return exp.value;
}

// the default read-eval-print-loop
void repl(const std::string& prompt, environment* env)
{
    for (;;) {
        // prints the current prompt after previous instruction is done
        std::cout << prompt;
        // the `read` part of a read-eval-print-loop
        std::string line;
        std::getline(std::cin, line);
        // `eval`, stringify and `print` 
        std::cout << toString(eval(read(line), env)) << '\n';
    }
}

int main()
{
    environment globalEnvironment;
    addGlobals(globalEnvironment);
    repl("cisp > ", &globalEnvironment);
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file

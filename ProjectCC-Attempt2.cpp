#include <iostream>
#include <vector>
#include <regex>
#include <string>
#include <stdexcept>
#include <fstream>
#include <memory>
#include <stack>

using namespace std;

enum TokenType {
    KEYWORD,
    IDENTIFIER,
    LITERAL,
    OPERATOR,
    PUNCTUATION,
    UNKNOWN
};

struct Token {
    TokenType type;
    string value;
    int line;
};

class Lexer {
public:
    Lexer(const string& source) : source(source), position(0), line(1) {}

    vector<Token> tokenize() {
        vector<Token> tokens;
        regex tokenPatterns(
            "(std|ifstream|ofstream|fstream|string|while|if|else|return|int)" // Keywords
            "|([a-zA-Z_][a-zA-Z0-9_]*)"                                    // Identifiers
            "|(\".*?\")"                                                   // Literals
            "|(::|\\.|<<|>>|&&|\\+\\+|--|<=|>=|==|!=|\\+=|-=|/=|\\+|-|\\|/|<|>)" // Operators
            "|([;(){}<>\\[\\],])"                                          // Punctuation
            "|([ \t]+)"                                                    // Whitespace
            "|(\n)"                                                         // Newline
            "|(.)"                                                          // Unknown
        );



        auto words_begin = sregex_iterator(source.begin(), source.end(), tokenPatterns);
        auto words_end = sregex_iterator();
        for (auto it = words_begin; it != words_end; ++it) {
            smatch match = *it;
            if (match[1].matched) {
                tokens.push_back({ KEYWORD, match.str(), line });
            }
            else if (match[2].matched) {
                tokens.push_back({ IDENTIFIER, match.str(), line });
            }
            else if (match[3].matched) {
                tokens.push_back({ LITERAL, match.str(), line });
            }
            else if (match[4].matched) {
                tokens.push_back({ OPERATOR, match.str(), line });
            }
            else if (match[5].matched) {
                tokens.push_back({ PUNCTUATION, match.str(), line });
                if (match.str() == "{" || match.str() == "}") {
                    lineStack.push(line);
                }
            }
            // Skip whitespace
            else if (match[6].matched) {
                continue;
            }
            else if (match[7].matched) {  // Newline
                ++line;
                continue;  // Skip adding newline tokens
            }
            else {
                tokens.push_back({ UNKNOWN, match.str(), line });
            }
        }

       return tokens;
    }

private:
    string source;
    size_t position;
    int line;
    stack<int> lineStack;  // Stack to track line numbers for matching braces
};

struct ASTNode {
    virtual ~ASTNode() = default;
};

struct NumberNode : ASTNode {
    string value;
    explicit NumberNode(const string& value) : value(value) {}
};

struct IdentifierNode : ASTNode {
    string name;
    explicit IdentifierNode(const string& name) : name(name) {}
};

struct BinaryOperationNode : ASTNode {
    string op;
    unique_ptr<ASTNode> left;
    unique_ptr<ASTNode> right;

    BinaryOperationNode(string op, unique_ptr<ASTNode> left, unique_ptr<ASTNode> right)
    : op(op), left(std::move(left)), right(std::move(right)) {}
};

struct AssignmentNode : ASTNode {
    unique_ptr<IdentifierNode> identifier;
    unique_ptr<ASTNode> expression;

    AssignmentNode(unique_ptr<IdentifierNode> identifier, unique_ptr<ASTNode> expression)
    : identifier(std::move(identifier)), expression(std::move(expression)) {}
};

struct DeclarationNode : ASTNode {
    string type;
    vector<unique_ptr<IdentifierNode>> identifiers;

    DeclarationNode(string type, vector<unique_ptr<IdentifierNode>> identifiers)
    : type(type), identifiers(std::move(identifiers)) {}
};
struct UnaryOperationNode : ASTNode {
    string op;
    unique_ptr<ASTNode> right;

    UnaryOperationNode(string op, unique_ptr<ASTNode> right)
    : op(op), right(std::move(right)) {}
};

struct ForLoopNode : ASTNode {
    unique_ptr<ASTNode> initialization;
    unique_ptr<ASTNode> condition;
    unique_ptr<ASTNode> increment;
    unique_ptr<ASTNode> body;

    ForLoopNode(unique_ptr<ASTNode> initialization, unique_ptr<ASTNode> condition,
                unique_ptr<ASTNode> increment, unique_ptr<ASTNode> body)
    : initialization(std::move(initialization)), condition(std::move(condition)),
    increment(std::move(increment)), body(std::move(body)) {}
};

struct WhileLoopNode : ASTNode {
    unique_ptr<ASTNode> condition;
    unique_ptr<ASTNode> body;

    WhileLoopNode(unique_ptr<ASTNode> condition, unique_ptr<ASTNode> body)
    : condition(std::move(condition)), body(std::move(body)) {}
};

class Parser {
public:
    explicit Parser(const vector<Token>& tokens) : tokens(tokens), position(0) {}

    unique_ptr<ASTNode> parse() {
        return parseProgram();
    }

private:
    stack<char> bracketStack;
    stack<int> lineStack;  // Stack to track line numbers for matching braces

    unique_ptr<ASTNode> parseProgram() {
        while (!isAtEnd()) {
            parseStatement();
        }
        if (!bracketStack.empty()) {
            int errorLine = lineStack.top();
            throw runtime_error("Mismatched brackets detected at line " + to_string(errorLine));
        }
        return nullptr;
    }



    unique_ptr<ASTNode> parseVariableDeclaration() {
        string type = previous().value;
        vector<unique_ptr<IdentifierNode>> identifiers;
        do {
            if (match(IDENTIFIER)) {
                identifiers.push_back(make_unique<IdentifierNode>(previous().value));
            } else {
                throw runtime_error("Expected identifier in variable declaration at line " + to_string(peek().line));
            }
        } while (match(PUNCTUATION, ","));

        // Check if there is a semicolon at the end of the declaration
        if (!match(PUNCTUATION, ";")) {
            throw runtime_error("Expected ';' at the end of variable declaration at line " + to_string(peek().line));
        }

        return make_unique<DeclarationNode>(type, move(identifiers));
    }



    unique_ptr<ASTNode> parseFileDeclaration() {
        if (!match(IDENTIFIER)) {
            throw runtime_error("Expected identifier after file declaration keyword at line " + to_string(peek().line));
        }
        auto identifier = make_unique<IdentifierNode>(previous().value);
        if (!match(PUNCTUATION, "(")) {
            throw runtime_error("Expected '(' after file declaration identifier at line " + to_string(peek().line));
        }
        bracketStack.push('(');
        lineStack.push(previous().line);
        if (!match(LITERAL)) {
            throw runtime_error("Expected filename literal in file declaration at line " + to_string(peek().line));
        }
        if (!match(PUNCTUATION, ")")) {
            throw runtime_error("Expected ')' after filename literal in file declaration at line " + to_string(peek().line));
        }
        bracketStack.pop();
        lineStack.pop();
        if (!match(PUNCTUATION, ";")) {
            throw runtime_error("Expected ';' at the end of file declaration at line " + to_string(peek().line));
        }
        return identifier;
    }

    unique_ptr<ASTNode> parseFileOperation() {
        auto identifier = make_unique<IdentifierNode>(previous().value);
        if (match(PUNCTUATION, ".")) {
            if (!match(IDENTIFIER)) {
                throw runtime_error("Expected method name after '.' in file operation at line " + to_string(peek().line));
            }
            if (!match(PUNCTUATION, "(")) {
                throw runtime_error("Expected '(' after method name in file operation at line " + to_string(peek().line));
            }
            bracketStack.push('(');
            lineStack.push(previous().line);
            if (!match(PUNCTUATION, ")")) {
                throw runtime_error("Expected ')' in file operation at line " + to_string(peek().line));
            }
            bracketStack.pop();
            lineStack.pop();
            if (!match(PUNCTUATION, ";")) {
                throw runtime_error("Expected ';' at the end of file operation at line " + to_string(peek().line));
            }
        } else {
            throw runtime_error("Unexpected token after file identifier at line " + to_string(peek().line));
        }
        return identifier;
    }

    unique_ptr<ASTNode> parseIfStatement() {
        if (!match(PUNCTUATION, "(")) {
            throw runtime_error("Expected '(' after 'if' at line " + to_string(peek().line));
        }
        bracketStack.push('(');
        lineStack.push(previous().line);
        parseExpression();
        if (!match(PUNCTUATION, ")")) {
            throw runtime_error("Expected ')' after condition in 'if' statement at line " + to_string(peek().line));
        }
        bracketStack.pop();
        lineStack.pop();
        if (!match(PUNCTUATION, "{")) {
            throw runtime_error("Expected '{' after 'if' condition at line " + to_string(peek().line));
        }
        bracketStack.push('{');
        lineStack.push(previous().line);
        while (!match(PUNCTUATION, "}")) {
            parseStatement();
        }
        bracketStack.pop();
        lineStack.pop();
        return nullptr;
    }



    void parseStatement() {
        if (match(KEYWORD, "int")) {
            parseVariableDeclaration();
        }
        else if (match(KEYWORD, "ifstream") || match(KEYWORD, "ofstream") || match(KEYWORD, "fstream")) {
            parseFileDeclaration();
        }
        else if (match(IDENTIFIER)) {
            parseFileOperation();
        }
        else if (match(KEYWORD, "if")) {
            parseIfStatement();
        }
        else if (match(KEYWORD, "while")) {
            parseWhileStatement();
        }
        else if (match(KEYWORD, "for")) {
            parseForStatement();
        }
        else {
            throw runtime_error("Unexpected token: " + peek().value + " at line " + to_string(peek().line));
        }
    }


    unique_ptr<ASTNode> parseWhileStatement() {
        if (!match(PUNCTUATION, "(")) {
            throw runtime_error("Expected '(' after 'while' at line " + to_string(peek().line));
        }
        bracketStack.push('(');
        lineStack.push(previous().line);
        parseExpression();
        if (!match(PUNCTUATION, ")")) {
            throw runtime_error("Expected ')' after condition in 'while' statement at line " + to_string(peek().line));
        }
        bracketStack.pop();
        lineStack.pop();
        if (!match(PUNCTUATION, "{")) {
            throw runtime_error("Expected '{' after 'while' condition at line " + to_string(peek().line));
        }
        bracketStack.push('{');
        lineStack.push(previous().line);
        while (!match(PUNCTUATION, "}")) {
            parseStatement();
        }
        bracketStack.pop();
        lineStack.pop();
        return nullptr;
    }

    unique_ptr<ASTNode> parseForStatement() {
        if (!match(PUNCTUATION, "(")) {
            throw runtime_error("Expected '(' after 'for' at line " + to_string(peek().line));
        }
        bracketStack.push('(');
        lineStack.push(previous().line);
        parseExpression();
        if (!match(PUNCTUATION, ";")) {
            throw runtime_error("Expected ';' after initialization in 'for' statement at line " + to_string(peek().line));
        }
        parseExpression();
        if (!match(PUNCTUATION, ";")) {
            throw runtime_error("Expected ';' after condition in 'for' statement at line " + to_string(peek().line));
        }
        parseExpression();
        if (!match(PUNCTUATION, ")")) {
            throw runtime_error("Expected ')' after increment in 'for' statement at line " + to_string(peek().line));
        }
        bracketStack.pop();
        lineStack.pop();
        if (!match(PUNCTUATION, "{")) {
            throw runtime_error("Expected '{' after 'for' header at line " + to_string(peek().line));
        }
        bracketStack.push('{');
        lineStack.push(previous().line);
        while (!match(PUNCTUATION, "}")) {
            parseStatement();
        }
        bracketStack.pop();
        lineStack.pop();
        return nullptr;
    }

    unique_ptr<ASTNode> parseExpression() {
        if (match(OPERATOR, "!")) {
            auto op = previous().value;
            auto right = parseExpression();
            return make_unique<UnaryOperationNode>(op, std::move(right));
        }
        advance();
        return nullptr;
    }

    bool match(TokenType type, const string& value = "") {
        if (isAtEnd()) return false;
        Token token = peek();
        if (token.type != type) return false;
        if (!value.empty() && token.value != value) return false;
        advance();
        return true;
    }

    Token advance() {
        if (!isAtEnd()) position++;
        return previous();
    }

    bool isAtEnd() {
        return position >= tokens.size();
    }

    Token peek() {
        return tokens[position];
    }

    Token previous() {
        return tokens[position - 1];
    }

    const vector<Token>& tokens;
    size_t position;
};

void printTokens(const vector<Token>& tokens) {
    cout << "Lexer's Output:  " << endl;
    for (const auto& token : tokens) {
        cout << "Token: " << token.value << " Type: " << token.type << " Line: " << token.line << endl;
    }
}

int main() {
    cout << "\t\t\tCompiler Construction Project" << endl << endl;
    string code = R"(
     int a;
     int b,c;
     ifstream inputFile("input.txt");
     fstream outputFile;
     for ( int i = 0; i < 10; ++i )
    {
        cout << "For Loop Iteration: " << i << endl;
        outputFile << "For Loop Iteration: " << i << endl;
    }


    int i = 0;
    while (i < 5) {
        cout << "While Loop Iteration: " << i << endl;
        outputFile << "While Loop Iteration: " << i << endl;
        ++i;
    }

    if (d > 10) {
        cout << "d is greater than 10" << endl;
        outputFile << "d is greater than 10" << endl;
    } else {
        cout << "d is 10 or less" << endl;
        outputFile << "d is 10 or less" << endl;
    }

    string line;
    while (getline(inputFile, line)) {
        cout << line << endl;
        outputFile << line << endl;
    }

    inputFile.close(;
    outputFile.close();
    )";

    try {
        Lexer lexer(code);
        vector<Token> tokens = lexer.tokenize();
        printTokens(tokens);

        Parser parser(tokens);
        unique_ptr<ASTNode> syntaxTree = parser.parse();

        // Printing the syntax tree
        if (syntaxTree) {
            cout << "Parsing completed successfully." << endl;
        }
        else {
            cout << "Parsing failed." << endl;
        }
    }
    catch (const exception& e) {
        cerr << e.what() << endl;
    }

    return 0;
}

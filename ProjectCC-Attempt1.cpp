
#include <iostream>
#include <vector>
#include <regex>
#include <string>
#include <stdexcept>
#include <fstream>
#include <memory>

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
};

class Lexer {
public:
    Lexer(const string& source) : source(source), position(0) {}

    vector<Token> tokenize() {
        vector<Token> tokens;
        regex tokenPatterns(
            "(std|ifstream|ofstream|fstream|string|while|if|else|return)" // Keywords
            "|([a-zA-Z_][a-zA-Z0-9_]*)"                                    // Identifiers
            "|(\".*?\")"                                                   // Literals
            "|(::|\\.|<<|>>|&&|\\+\\+|--|<=|>=|==|!=|\\+=|-=|\\=|/=|\\+|-|\\|/|<|>)" // Operators
            "|([;(){}<>\\[\\],])"                                          // Punctuation
            "|([ \\t\\n]+)"                                                // Whitespace
            "|(.)"                                                         // Unknown
        );

        auto words_begin = sregex_iterator(source.begin(), source.end(), tokenPatterns);
        auto words_end = sregex_iterator();

        for (auto it = words_begin; it != words_end; ++it) {
            smatch match = *it;
            if (match[1].matched) {
                tokens.push_back({ KEYWORD, match.str() });
            }
            else if (match[2].matched) {
                tokens.push_back({ IDENTIFIER, match.str() });
            }
            else if (match[3].matched) {
                tokens.push_back({ LITERAL, match.str() });
            }
            else if (match[4].matched) {
                tokens.push_back({ OPERATOR, match.str() });
            }
            else if (match[5].matched) {
                tokens.push_back({ PUNCTUATION, match.str() });
            }
            // Skip whitespace
            else if (match[6].matched) {
                continue;
            }
            else {
                tokens.push_back({ UNKNOWN, match.str() });
            }
        }
        return tokens;
    }

private:
    string source;
    size_t position;
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
        return parseDeclaration();
    }

private:
    unique_ptr<ASTNode> parseDeclaration() {
        if (match(KEYWORD, "int")) {
            return parseVariableDeclaration();
        }
        else if (match(KEYWORD, "ifstream") || match(KEYWORD, "ofstream") || match(KEYWORD, "fstream")) {
            return parseFileDeclaration();
        }
        else if (match(IDENTIFIER)) {
            return parseFileOperation();
        }
        else if (match(KEYWORD, "if")) {
            return parseIfStatement();
        }
        else if (match(KEYWORD, "while")) {
            return parseWhileStatement();
        }
        return nullptr;
    }

    unique_ptr<ASTNode> parseVariableDeclaration() {
        string type = previous().value;
        vector<unique_ptr<IdentifierNode>> identifiers;
        do {
            if (match(IDENTIFIER)) {
                identifiers.push_back(make_unique<IdentifierNode>(previous().value));
            }
        } while (match(PUNCTUATION, ","));
        if (!match(PUNCTUATION, ";")) {
            throw runtime_error("Expected ';' at the end of variable declaration.");
        }
        return make_unique<DeclarationNode>(type, move(identifiers));
    }

    unique_ptr<ASTNode> parseFileDeclaration() {
        // Placeholder implementation
        return make_unique<ASTNode>();
    }

    unique_ptr<ASTNode> parseFileOperation() {
        // Placeholder implementation
        return make_unique<ASTNode>();
    }

    unique_ptr<ASTNode> parseIfStatement() {
        // Placeholder implementation
        return make_unique<ASTNode>();
    }

    unique_ptr<ASTNode> parseWhileStatement() {
        // Placeholder implementation
        return make_unique<ASTNode>();
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
        return peek().type == UNKNOWN;
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
        cout << "Token: " << token.value << " Type: " << token.type << endl;
    }
}

int main() {
    cout << "\t\t\tCompiler Construction Project" << endl << endl;
    string code = R"(

        int a, b, c;
    int d = 5;
    string filename = "example.txt";

    ifstream inputFile(filename);
    ofstream outputFile("output.txt");

    if (!inputFile) {
        cerr << "Error opening input file: " << filename << endl;
        return 1;
    }
    if (!outputFile) {
        cerr << "Error opening output file: output.txt" << endl;
        return 1;
    }

 
    for (int i = 0; i < 10; ++i) {
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




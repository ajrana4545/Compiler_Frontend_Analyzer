#include "DFALexer.h"
#include <cctype>
#include <stdexcept>
#include <iostream>

// Provide the definition of tokenTypeName used across the project
std::string tokenTypeName(TokenType t) {
    switch (t) {
        case TokenType::KW_INT:        return "KW_INT";
        case TokenType::KW_FLOAT:      return "KW_FLOAT";
        case TokenType::KW_CHAR:       return "KW_CHAR";
        case TokenType::KW_VOID:       return "KW_VOID";
        case TokenType::KW_IF:         return "KW_IF";
        case TokenType::KW_ELSE:       return "KW_ELSE";
        case TokenType::KW_WHILE:      return "KW_WHILE";
        case TokenType::KW_FOR:        return "KW_FOR";
        case TokenType::KW_RETURN:     return "KW_RETURN";
        case TokenType::KW_STRUCT:     return "KW_STRUCT";
        case TokenType::IDENTIFIER:    return "IDENTIFIER";
        case TokenType::INT_LITERAL:   return "INT_LITERAL";
        case TokenType::FLOAT_LITERAL: return "FLOAT_LITERAL";
        case TokenType::CHAR_LITERAL:  return "CHAR_LITERAL";
        case TokenType::STRING_LITERAL:return "STRING_LITERAL";
        case TokenType::PLUS:          return "PLUS";
        case TokenType::MINUS:         return "MINUS";
        case TokenType::STAR:          return "STAR";
        case TokenType::SLASH:         return "SLASH";
        case TokenType::PERCENT:       return "PERCENT";
        case TokenType::ASSIGN:        return "ASSIGN";
        case TokenType::EQ:            return "EQ";
        case TokenType::NEQ:           return "NEQ";
        case TokenType::LT:            return "LT";
        case TokenType::GT:            return "GT";
        case TokenType::LE:            return "LE";
        case TokenType::GE:            return "GE";
        case TokenType::AND:           return "AND";
        case TokenType::OR:            return "OR";
        case TokenType::NOT:           return "NOT";
        case TokenType::AMPERSAND:     return "AMPERSAND";
        case TokenType::PIPE:          return "PIPE";
        case TokenType::CARET:         return "CARET";
        case TokenType::LSHIFT:        return "LSHIFT";
        case TokenType::RSHIFT:        return "RSHIFT";
        case TokenType::PLUS_ASSIGN:   return "PLUS_ASSIGN";
        case TokenType::MINUS_ASSIGN:  return "MINUS_ASSIGN";
        case TokenType::STAR_ASSIGN:   return "STAR_ASSIGN";
        case TokenType::SLASH_ASSIGN:  return "SLASH_ASSIGN";
        case TokenType::INC:           return "INC";
        case TokenType::DEC:           return "DEC";
        case TokenType::LPAREN:        return "LPAREN";
        case TokenType::RPAREN:        return "RPAREN";
        case TokenType::LBRACE:        return "LBRACE";
        case TokenType::RBRACE:        return "RBRACE";
        case TokenType::LBRACKET:      return "LBRACKET";
        case TokenType::RBRACKET:      return "RBRACKET";
        case TokenType::SEMICOLON:     return "SEMICOLON";
        case TokenType::COLON:         return "COLON";
        case TokenType::COMMA:         return "COMMA";
        case TokenType::DOT:           return "DOT";
        case TokenType::ARROW:         return "ARROW";
        case TokenType::EOF_TOK:       return "EOF";
        case TokenType::COMMENT:       return "COMMENT";
        case TokenType::KW_CUSTOM:     return "KW_CUSTOM";
        default:                       return "UNKNOWN";
    }
}

// ─── DFALexer Implementation ─────────────────────────────────────────────────

DFALexer::DFALexer(const std::string& source) : _src(source) {}

char DFALexer::peek(int offset) const {
    size_t p = _pos + (size_t)offset;
    return (p < _src.size()) ? _src[p] : '\0';
}

char DFALexer::advance() {
    char c = _src[_pos++];
    if (c == '\n') { ++_line; _col = 1; } else { ++_col; }
    return c;
}

void DFALexer::skipWhitespace() {
    while (_pos < _src.size() && std::isspace((unsigned char)peek()))
        advance();
}

void DFALexer::skipLineComment() {
    // consume until newline
    while (_pos < _src.size() && peek() != '\n') advance();
}

void DFALexer::skipBlockComment() {
    // consume until */
    while (_pos < _src.size()) {
        if (peek() == '*' && peek(1) == '/') { advance(); advance(); return; }
        advance();
    }
    std::cerr << "[DFALexer] Warning: unterminated block comment\n";
    ++_m.errorCount;
}

Token DFALexer::readIdentifierOrKeyword() {
    int ln = _line, cn = _col;
    std::string val;
    while (_pos < _src.size() && (std::isalnum((unsigned char)peek()) || peek() == '_'))
        val += advance();
    auto& kw = keywords();
    auto  it = kw.find(val);
    if (it != kw.end()) return Token(it->second, val, ln, cn);
    // Check user-defined custom keywords
    if (_customKw.count(val)) return Token(TokenType::KW_CUSTOM, val, ln, cn);
    return Token(TokenType::IDENTIFIER, val, ln, cn);
}

Token DFALexer::readNumber() {
    int ln = _line, cn = _col;
    std::string val;
    bool isFloat = false;
    while (_pos < _src.size() && std::isdigit((unsigned char)peek()))
        val += advance();
    if (peek() == '.' && std::isdigit((unsigned char)peek(1))) {
        isFloat = true;
        val += advance(); // '.'
        while (_pos < _src.size() && std::isdigit((unsigned char)peek()))
            val += advance();
    }
    if ((peek() == 'e' || peek() == 'E') && (_pos + 1 < _src.size())) {
        isFloat = true;
        val += advance();
        if (peek() == '+' || peek() == '-') val += advance();
        while (_pos < _src.size() && std::isdigit((unsigned char)peek()))
            val += advance();
    }
    return Token(isFloat ? TokenType::FLOAT_LITERAL : TokenType::INT_LITERAL, val, ln, cn);
}

Token DFALexer::readCharLiteral() {
    int ln = _line, cn = _col;
    std::string val;
    val += advance(); // opening '
    while (_pos < _src.size() && peek() != '\'' && peek() != '\n') {
        if (peek() == '\\') val += advance();
        val += advance();
    }
    if (peek() == '\'') val += advance();
    else { std::cerr << "[DFALexer] unterminated char literal at " << ln << "\n"; ++_m.errorCount; }
    return Token(TokenType::CHAR_LITERAL, val, ln, cn);
}

Token DFALexer::readStringLiteral() {
    int ln = _line, cn = _col;
    std::string val;
    val += advance(); // opening "
    while (_pos < _src.size() && peek() != '"' && peek() != '\n') {
        if (peek() == '\\') val += advance();
        val += advance();
    }
    if (peek() == '"') val += advance();
    else { std::cerr << "[DFALexer] unterminated string literal at " << ln << "\n"; ++_m.errorCount; }
    return Token(TokenType::STRING_LITERAL, val, ln, cn);
}

Token DFALexer::readOperatorOrDelimiter() {
    int ln = _line, cn = _col;
    char c = peek();
    char c2 = peek(1);
    // Two-char operators
    if (c == '=' && c2 == '=') { advance(); advance(); return Token(TokenType::EQ, "==", ln, cn); }
    if (c == '!' && c2 == '=') { advance(); advance(); return Token(TokenType::NEQ, "!=", ln, cn); }
    if (c == '<' && c2 == '=') { advance(); advance(); return Token(TokenType::LE, "<=", ln, cn); }
    if (c == '>' && c2 == '=') { advance(); advance(); return Token(TokenType::GE, ">=", ln, cn); }
    if (c == '&' && c2 == '&') { advance(); advance(); return Token(TokenType::AND, "&&", ln, cn); }
    if (c == '|' && c2 == '|') { advance(); advance(); return Token(TokenType::OR, "||", ln, cn); }
    if (c == '<' && c2 == '<') { advance(); advance(); return Token(TokenType::LSHIFT, "<<", ln, cn); }
    if (c == '>' && c2 == '>') { advance(); advance(); return Token(TokenType::RSHIFT, ">>", ln, cn); }
    if (c == '+' && c2 == '+') { advance(); advance(); return Token(TokenType::INC, "++", ln, cn); }
    if (c == '-' && c2 == '-') { advance(); advance(); return Token(TokenType::DEC, "--", ln, cn); }
    if (c == '+' && c2 == '=') { advance(); advance(); return Token(TokenType::PLUS_ASSIGN, "+=", ln, cn); }
    if (c == '-' && c2 == '=') { advance(); advance(); return Token(TokenType::MINUS_ASSIGN, "-=", ln, cn); }
    if (c == '*' && c2 == '=') { advance(); advance(); return Token(TokenType::STAR_ASSIGN, "*=", ln, cn); }
    if (c == '/' && c2 == '=') { advance(); advance(); return Token(TokenType::SLASH_ASSIGN, "/=", ln, cn); }
    if (c == '-' && c2 == '>') { advance(); advance(); return Token(TokenType::ARROW, "->", ln, cn); }
    // Single-char
    advance();
    switch (c) {
        case '+': return Token(TokenType::PLUS,    "+", ln, cn);
        case '-': return Token(TokenType::MINUS,   "-", ln, cn);
        case '*': return Token(TokenType::STAR,    "*", ln, cn);
        case '/': return Token(TokenType::SLASH,   "/", ln, cn);
        case '%': return Token(TokenType::PERCENT, "%", ln, cn);
        case '=': return Token(TokenType::ASSIGN,  "=", ln, cn);
        case '<': return Token(TokenType::LT,      "<", ln, cn);
        case '>': return Token(TokenType::GT,      ">", ln, cn);
        case '!': return Token(TokenType::NOT,     "!", ln, cn);
        case '&': return Token(TokenType::AMPERSAND,"&",ln, cn);
        case '|': return Token(TokenType::PIPE,    "|", ln, cn);
        case '^': return Token(TokenType::CARET,   "^", ln, cn);
        case '(': return Token(TokenType::LPAREN,  "(", ln, cn);
        case ')': return Token(TokenType::RPAREN,  ")", ln, cn);
        case '{': return Token(TokenType::LBRACE,  "{", ln, cn);
        case '}': return Token(TokenType::RBRACE,  "}", ln, cn);
        case '[': return Token(TokenType::LBRACKET,"[", ln, cn);
        case ']': return Token(TokenType::RBRACKET,"]", ln, cn);
        case ';': return Token(TokenType::SEMICOLON,";",ln, cn);
        case ':': return Token(TokenType::COLON,   ":", ln, cn);
        case ',': return Token(TokenType::COMMA,   ",", ln, cn);
        case '.': return Token(TokenType::DOT,     ".", ln, cn);
        default:
            ++_m.errorCount;
            std::cerr << "[DFALexer] Unknown char '" << c << "' at line " << ln << "\n";
            return Token(TokenType::UNKNOWN, std::string(1, c), ln, cn);
    }
}

TokenStream DFALexer::tokenize() {
    Timer t; t.start();
    TokenStream ts;

    while (_pos < _src.size()) {
        skipWhitespace();
        if (_pos >= _src.size()) break;

        char c  = peek();
        char c2 = peek(1);

        // Comments
        if (c == '/' && c2 == '/') { advance(); advance(); skipLineComment(); continue; }
        if (c == '/' && c2 == '*') { advance(); advance(); skipBlockComment(); continue; }

        Token tok = (std::isalpha((unsigned char)c) || c == '_') ? readIdentifierOrKeyword()
                  : (std::isdigit((unsigned char)c))             ? readNumber()
                  : (c == '\'')                                  ? readCharLiteral()
                  : (c == '"')                                   ? readStringLiteral()
                  :                                                readOperatorOrDelimiter();

        // Update metrics
        _m.typeCounts[tokenTypeName(tok.type)]++;
        ts.push_back(std::move(tok));
    }

    ts.emplace_back(TokenType::EOF_TOK, "EOF", _line, _col);
    t.stop();
    _m.elapsedMs   = t.elapsedMs();
    _m.tokenCount  = (int)ts.size() - 1; // exclude EOF
    _m.memoryBytes = ts.size() * sizeof(Token);
    return ts;
}

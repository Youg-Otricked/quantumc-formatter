#include <iostream>
#include <filesystem>
#include <cstdlib>
#include <string>
#include <functional>
#include <unordered_map>
#include <vector>
#include <cstdio>
#include <fstream>
#include <unistd.h>
#include <limits.h>
#include <algorithm>
#include "yaml.hpp"
inline std::string whitespace = " \t\n\r";
inline std::string DIGITS = "0123456789";
inline std::string LETTERS = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
inline std::string LETTERSDIGITS = LETTERS + DIGITS;

enum class TokenType {
    IDENTIFIER, // Apply casing rules
    KEYWORD,    // (if, else, switch) Need specific trailing spaces
    OPERATOR,   // (+, -, *, /) Need spaces on both sides (mostly)
    ASSIGNMENT, // (=, +=, -=) Need spaces on both sides
    DELIMITER, // ({, (, [, ], ), }) Handle indentation or tight packing
    TERMINATOR, // (;) Usually forces a newline
    LITERAL,    // (strings, ints) Leave exactly as they are
    WHITESPACE, // Important to preserve or normalize
    COMMENT,    // Preserve and indent
	SEMICOLON,
    EOFT
};

class Token {
	public: 
	TokenType type;
	std::string value;
	Token();
	Token(TokenType t, std::string val) {
		this->type = t;
		this->value = val;
	}
	bool operator==(const Token&) const = default;
	std::string print() const;
};
bool isCharInSet(char c, const std::string &charSet) {
    return charSet.find(c) != std::string::npos;
}
class Lexer {
	public:
	int pos;
	std::string text;
	char current_char;
    Lexer(std::string text, std::string filename) {
        this->text = text;
		this->pos = -1;
        this->current_char = '\0';
        this->advance();
    }
    void advance() {
        if (this->pos < this->text.length()){
            this->current_char = this->text[this->pos];
        } else {
            this->current_char = '\0';
        }
    }
    Token make_number() {
        std::string num = "";
        int dot_count = 0;
        bool is_float = false;
        while (this->current_char != '\0' && isCharInSet(this->current_char, DIGITS + ".f")) {
            if (this->current_char == '.') {
                if (dot_count == 1) {
                    this->advance();
                    break;
                }
                dot_count ++;
                num += ".";
                this->advance();
            } else if (this->current_char == 'f') {
                is_float = true;
                this->advance();
                break;
            } else {
                num += this->current_char;
                this->advance();
            }
        }
        if (dot_count == 1) {
            if (is_float) {
                return Token(TokenType::LITERAL, num);
            }
            return Token(TokenType::LITERAL, num);
        }
        return Token(TokenType::LITERAL, num);
    }
    Token make_identifier() {
        std::string id = "";
        while (this->current_char != '\0' && 
            (isalnum(this->current_char) || this->current_char == '_')) {
            id += this->current_char;
            this->advance();
        }
        if (id == "int" || id == "float" || id == "double" || id == "bool" || id == "case" ||
            id == "string" || id == "qbool" || id == "void" || id == "char" || id == "break" ||
            id == "if" || id == "else" || id == "while" || id == "for" || id == "switch" ||
            id == "return" || id == "qif" || id == "qelse" || id == "qelif" || id == "qswitch" || 
            id == "const" || id == "default" || id == "class" || id == "struct" || id == "enum" || 
            id == "long" || id == "short" || id == "fn" || id == "continue" || id == "auto" || 
            id == "list" || id == "foreach" || id == "do" || id == "in" || id == "map" || 
            id == "type" || id == "public" || id == "protected" || id == "private" || id == "function" ||
            id == "namespace" || id == "keyword" || id == "operator" || id == "abstract" ||
            id == "final" ||id == "try" || id == "catch" || id == "nullptr") {
            return Token(TokenType::KEYWORD, id);
        }
        if (id == "true" || id == "false") {
            return Token(TokenType::LITERAL, id);
        }
        if (id == "qtrue" || id == "qfalse" || id == "both" || id == "none") {
            return Token(TokenType::LITERAL, id);
        }
        return Token(TokenType::IDENTIFIER, id);
    }
    Token make_string() {
        std::string str = "";
        bool escape_character = false;
        
        this->advance();

        while (this->current_char != '\0' && (this->current_char != '"' || escape_character)) {
            if (escape_character) {
                switch (this->current_char) {
                    case 'n': str += '\n'; break;
                    case 't': str += '\t'; break;
                    case 'r': str += '\r'; break;
                    case '\\': str += '\\'; break;
                    case '"': str += '\"'; break;
                    default: str += this->current_char; break;
                }
                escape_character = false;
            } else {
                if (this->current_char == '\\') {
                    escape_character = true;
                } else {
                    str += this->current_char;
                }
            }
            this->advance();
        }

        if (this->current_char != '"') {
            this->advance();
            throw "Invalid string.";
        }

        this->advance();
        return Token(TokenType::LITERAL, str);
    }
    Token make_char() {
        this->advance();

        std::string val = "";
        if (this->current_char == '\\') {
            this->advance();
            switch (this->current_char) {
                case 'n':  val = "\n"; break;
                case 't':  val = "\t"; break;
                case 'r':  val = "\r"; break;
                case '\'': val = "\'"; break;
                case '\\': val = "\\"; break;
                default:   val = std::string(1, this->current_char); break;
            }
            this->advance();
        } else {
            val = std::string(1, this->current_char);
            this->advance();
        }
        if (this->current_char != '\'') {
            throw "Invalid char";
        }
        this->advance();
        
        return Token(TokenType::LITERAL, val);
    }
    Token make_fstring() {
        this->advance(); 

        std::vector<std::string> parts;
        std::vector<std::string> exprs;

        std::string current = "";
        bool escape = false;

        while (this->current_char != '\0' && (this->current_char != '"' || escape)) {
            if (escape) {
                switch (this->current_char) {
                    case 'n': current += '\n'; break;
                    case 't': current += '\t'; break;
                    case 'r': current += '\r'; break;
                    case '\\': current += '\\'; break;
                    case '"': current += '"'; break;
                    default: current += this->current_char; break;
                }
                escape = false;
                this->advance();
                continue;
            }

            if (this->current_char == '\\') {
                escape = true;
                this->advance();
                continue;
            }

            if (this->current_char == '{' && this->text[this->pos + 1] == '{') {
                current += '{';
                this->advance();
                this->advance();
                continue;
            }

            if (this->current_char == '}' && this->text[this->pos + 1] == '}') {
                current += '}';
                this->advance();
                this->advance();
                continue;
            }

            if (this->current_char == '{') {
                parts.push_back(current);
                current = "";
                this->advance(); 

                std::string expr = "";
                int brace_depth = 1;

                while (this->current_char != '\0' && brace_depth > 0) {
                    if (this->current_char == '{') brace_depth++;
                    else if (this->current_char == '}') brace_depth--;

                    if (brace_depth > 0) expr += this->current_char;

                    this->advance();
                }

                if (brace_depth != 0)
                    throw "Invalid fstring";
                exprs.push_back(expr);
            } else {
                current += this->current_char;
                this->advance();
            }
        }
        parts.push_back(current);
        if (this->current_char != '"') throw "Invalid fstring";
        this->advance(); 
        std::string encoded = "";
        for (size_t i = 0; i < parts.size(); i++) {
            encoded += parts[i];
            if (i < exprs.size()) encoded += "\x01" + exprs[i] + "\x01";
        }
        return Token(TokenType::LITERAL, encoded);
    }
    std::vector<Token> make_tokens() {
        std::vector<Token> tokens;
        tokens.reserve(256); 
        while (this->current_char != '\0') {
            if (isCharInSet(this->current_char, whitespace)) {
				std::string ws = "";
                while (isCharInSet(this->current_char, whitespace)) {
                    ws += std::string(1, this->current_char);
                    this->advance();
                }
                tokens.push_back(Token(TokenType::WHITESPACE, ws));
                continue;
            } else if (isCharInSet(this->current_char, DIGITS)) {
                tokens.push_back(this->make_number());
                continue;
            } else if (this->current_char == 'f' && this->text[this->pos + 1] == '"') {
                this->advance();
                tokens.push_back(this->make_fstring());
            } else if (isCharInSet(this->current_char, LETTERS + "_")) {
                tokens.push_back(this->make_identifier());
            } else if (this->current_char == '"') {
                tokens.push_back(this->make_string());
                continue;
            } else if(this->current_char == '\'') {
                tokens.push_back(this->make_char());
            } else {
                switch (this->current_char) {
                    case '+':
                        this->advance();
                        if (current_char == '+') {
                            this->advance();
                            tokens.push_back(Token(TokenType::OPERATOR, "++"));
                        } else if (current_char == '=') {
                            this->advance();
                            tokens.push_back(Token(TokenType::ASSIGNMENT, "+="));
                        } else {
                            tokens.push_back(Token(TokenType::OPERATOR, "+"));
                        }
                        break;

                    case '-':
                        this->advance();
                        if (current_char == '-') {
                            this->advance();
                            tokens.push_back(Token(TokenType::OPERATOR, "--"));
                        } else if (current_char == '=') {
                            this->advance();
                            tokens.push_back(Token(TokenType::ASSIGNMENT, "-="));
                        }  else if (current_char == '>') {
                            this->advance();
                            tokens.push_back(Token(TokenType::OPERATOR, "->"));
                        } else {
                            tokens.push_back(Token(TokenType::OPERATOR, "-"));
                        }
                        break;
                    case '*':
                        this->advance();
                        if (current_char == '=') {
                            this->advance();
                            tokens.push_back(Token(TokenType::ASSIGNMENT, "*="));
                        } else {
                            tokens.push_back(Token(TokenType::OPERATOR, "*"));
                            break;
                        }
                        break;
                    case '/':
						this->advance();
						if (this->current_char == '/') {
							std::string comment_val = "//";
							this->advance();
							while (this->current_char != '\0' && this->current_char != '\n') {
								comment_val += this->current_char;
								this->advance();
							}
							tokens.push_back(Token(TokenType::COMMENT, comment_val));
							continue; 
						} else if (this->current_char == '*') {
							std::string comment_val = "/*";
							this->advance();
							while (this->current_char != '\0') {
								if (this->current_char == '*') {
									comment_val += "*";
									this->advance();
									if (this->current_char == '/') {
										comment_val += "/";
										this->advance();
										break;
									}
								} else {
									comment_val += this->current_char;
									this->advance();
								}
							}
							tokens.push_back(Token(TokenType::COMMENT, comment_val));
							continue;
						} else if (current_char == '=') {
                            this->advance();
                            tokens.push_back(Token(TokenType::ASSIGNMENT, "/="));
                        } else {
                            tokens.push_back(Token(TokenType::OPERATOR, "/"));
                        }
                        break;
                    case '=':
                        this->advance();
                        if (current_char == '=') {
                            this->advance();
                            if (current_char == '=') {
                                this->advance();
                                tokens.push_back(Token(TokenType::OPERATOR, "==="));
                            } else {
                                tokens.push_back(Token(TokenType::OPERATOR, "=="));
                            }
                        } else {
                            tokens.push_back(Token(TokenType::ASSIGNMENT, "="));
                            break;
                        }
                        break;
                    case '!':
                        this->advance();
                        if (current_char == '=') {
                            this->advance();
                            if (current_char == '=') {
                                this->advance();
                                tokens.push_back(Token(TokenType::OPERATOR, "!=="));
                            } else { 
                                tokens.push_back(Token(TokenType::OPERATOR, "!="));
                            }
                        } else if (current_char == '!') {
                            this->advance();
                            tokens.push_back(Token(TokenType::OPERATOR, "!!"));
                        } else {
                            tokens.push_back(Token(TokenType::OPERATOR, "!"));
                            break;
                        }
                        break;
                    case '>':
                        this->advance();
                        if (current_char == '=') {
                            this->advance();
                            tokens.push_back(Token(TokenType::OPERATOR, ">="));
                        } else if (this->current_char == '>') {
                            this->advance();
                            tokens.push_back(Token(TokenType::OPERATOR, ">>"));
                        } else {
                            tokens.push_back(Token(TokenType::OPERATOR, ">"));
                            break;
                        }
                        break;
                    case '<':
                        this->advance();
                        if (current_char == '<') {
                            this->advance();
                            tokens.push_back(Token(TokenType::OPERATOR, "<<"));
                        } else if (current_char == '=') {
                            this->advance();
                            tokens.push_back(Token(TokenType::OPERATOR, "<="));
                        } else {
                            tokens.push_back(Token(TokenType::OPERATOR, "<"));
                            break;
                        }
                        break;
                    case '(':
                        tokens.push_back(Token(TokenType::DELIMITER, "("));
                        this->advance();
                        break;
                    case ')':
                        tokens.push_back(Token(TokenType::DELIMITER, ")"));
                        this->advance();
                        break;
                    case '{':
                        tokens.push_back(Token(TokenType::DELIMITER, "{"));
                        this->advance();
                        break;
                    case '}':
                        tokens.push_back(Token(TokenType::DELIMITER, "}"));
                        this->advance();
                        break;
                    case '[':
                        tokens.push_back(Token(TokenType::DELIMITER, "["));
                        this->advance();
                        break;
                    case ']':
                        tokens.push_back(Token(TokenType::DELIMITER, "]"));
                        this->advance();
                        break;
                    case '%':
                        this->advance();
                        if (current_char == '=') {
                            this->advance();
                            tokens.push_back(Token(TokenType::ASSIGNMENT, "%="));
                        } else {
                            tokens.push_back(Token(TokenType::OPERATOR, "%"));
                            break;
                        }
                        break;
                    case '&':
                        this->advance();
                        if (current_char == '&') {
                            this->advance();
                            if (current_char == '&') {
                                this->advance();
                                tokens.push_back(Token(TokenType::OPERATOR, "&&&"));
                            } else {
                                tokens.push_back(Token(TokenType::OPERATOR, "&&"));
                            }
                        } else if (this->current_char == '|') {
                            this->advance();
                            if (this->current_char == '&') {
                                this->advance();
                                tokens.push_back(Token(TokenType::OPERATOR, "&|&"));
                            }
                        } else {
                            tokens.push_back(Token(TokenType::OPERATOR, "&"));
                            break;
                        }
                        break;
                    case '|':
                        this->advance();
                        if (current_char == '|') {
                            this->advance();
                            if (current_char == '|') {
                                this->advance();
                                tokens.push_back(Token(TokenType::OPERATOR, "|||"));
                            } else {
                                tokens.push_back(Token(TokenType::OPERATOR, "||"));
                            }
                        } else if (this->current_char == '&') {
                                this->advance();
                                if (this->current_char == '|') {
                                    this->advance();
                                    tokens.push_back(Token(TokenType::OPERATOR, "|&|"));
                                }
                        } else {
                            tokens.push_back(Token(TokenType::OPERATOR, "|"));
                            break;
                        }
                        break;
                    case '@':
                        this->advance();
                        tokens.push_back(Token(TokenType::OPERATOR, "@"));
                        break;
                    case '^':
                        this->advance();
                        if (current_char == '*') {
                            this->advance();
                            tokens.push_back(Token(TokenType::OPERATOR, "^*"));
                            break;
                        } else if (current_char == '^') {
                            this->advance();
                            tokens.push_back(Token(TokenType::OPERATOR, "^^"));
                        } else {
                            tokens.push_back(Token(TokenType::OPERATOR, "^"));
                        }
                        break;
                    case ',':
                        this->advance();
                        tokens.push_back(Token(TokenType::OPERATOR, ","));
                        break;
                    case ':':
                        this->advance();
                        if (current_char == ':') {
                            this->advance();
                            tokens.push_back(Token(TokenType::OPERATOR, "::"));
                            break;
                        } else {
                            tokens.push_back(Token(TokenType::OPERATOR, ":"));
                        }
                        break;
                    case ';':
                        tokens.push_back(Token(TokenType::SEMICOLON, ";"));
                        this->advance();
                        break;
                    case '.':
                        tokens.push_back(Token(TokenType::OPERATOR, "."));
                        this->advance();
                        break;
                    
                    default:
                        std::string unknown = std::string(1, this->current_char);
						throw "Invalid char " + unknown;
                }
            }
        }
        tokens.push_back(Token(TokenType::EOFT, "<eof>"));
        return tokens;

    }

};
const std::string VERSION = "v0.0.1";
void notImplemented(int count, char** args) {
	std::cout << "Not implemented" << '\n';
}
void help(int count, char** args) {
	std::cout << "QuantumC Formatter version " << VERSION << R"(
qconform help - This command.
qconform list <config file alias | default> - Lists formatting conventions for that alias.
qconform check [config file alias | default] <filename> - Checks formatting against formatting conventions. Current saved default unless specified file path / default
qconform apply [config file alias | default] <filename> - Same as above but also applys.
qconform setup - Creates all the folders and files for the cli to work, and adds it to your PATH.
qconform create - Adds formatting rules, creates the rules file, and adds it, with an alias.
qconform alias { <name> <path> | -l|--list | -d|--delete <name> } - Creates a quick alias for a rule file and stores it in the saved directorys in the .qconform folder. -d/--delete is the vis-versa, and -l/--list lists all aliases
)";
}
void setup(int count, char** args) {
	char* chome = std::getenv("HOME");
	if (chome == nullptr) {
		throw "HOME not set.";
	}
	std::string home(chome);
	std::filesystem::create_directories(home + "/.qconform/bin");
	std::filesystem::create_directories(home + "/.qconform/formats");
	if (std::filesystem::exists(home + "/.qconform/config.yaml")) {
		std::cout << "Config file already exists." << '\n';
	} else {
		auto file = std::ofstream(home + "/.qconform/config.yaml");
		file << "aliases:\n  default: " << home << "/.qconform/default.fmt.yaml";
		file.close();
	}
	if (std::filesystem::exists(home + "/.qconform/default.fmt.yaml")) {
		std::cout << "Default format already exists." << '\n';
	} else {
		auto file = std::ofstream(home + "/.qconform/default.fmt.yaml");
		file << R"(
formatting:
  indent_size: 4
  use_tabs: false
  braces:
    open_brace:
      before: " " 
      after: "\n"
    indent: true
    close_brace:
      before: "\n"
  operators:
    unops:
      before: ""
      after: ""
    binops:
      before: " "
      after: " "
  trailing_commas: no
  # params_per_line: null
  # max_width: null
naming:
  specifiers:
    priv_member:
      position: prefix
      value: __
      style: snake_case
  rules: 
    enforce:
      level: warning
      autocorrect: false
    vars: snake_case
    functions: camelCase
    user_types: PascalCase
    constants: SCREAMING_SNAKE_CASE
    private_members: priv_member
    private_methods: camel_Snake_Case
    namespaces: PascalCase
    internal_namespaces: Pascal_Snake_Case
    internal_functions: camel_Snake_Case
comments:
  specifiers:
    normal: //
    doc: ///
    top_level_doc: //!
  rules:
    enforce:
      level: error
    functions: doc
    file: top_level_doc
    namespaces: doc
    internal: /// No Include
newlines:
  style: lf
  final_newline: true
  collapse_blanks: true
includes:
  filepath:
    quotes: false
    relative: true
    absolute: false
    tilde_relative: true
namespaces:
  types:
    amount: 1
    enforce: warning
)";
		file.close();
	}
	std::string shell;
    const char* shell_raw = getenv("SHELL");
    if (shell_raw) shell = shell_raw;
    std::string rcFile;
    if (shell.ends_with("zsh")) rcFile = home + "/.zshrc";
    else if (shell.ends_with("fish")) rcFile = home + "/.config/fish/config.fish";
    else rcFile = home + "/.bashrc";
    std::string exportLine = "export PATH=\"$HOME/.qconform/bin:$PATH\"";
    if (shell.ends_with("fish")) {
        exportLine = "fish_add_path $HOME/.qconform/bin";
    }
    std::ifstream rcIn(rcFile);
    std::string rcContent((std::istreambuf_iterator<char>(rcIn)), std::istreambuf_iterator<char>());
    rcIn.close();
    char result[PATH_MAX];
    ssize_t cnt = readlink("/proc/self/exe", result, PATH_MAX);
    std::filesystem::copy_file(std::string(result, cnt), home + "/.qconform/bin/qconform", std::filesystem::copy_options::overwrite_existing);
    std::cout << "Run: source " << rcFile << "\n";
}
std::string strip(const std::string& s) {
    auto start = s.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\n\r");
    return s.substr(start, end - start + 1);
}
void create(int count, char** args) {
	char* chome = std::getenv("HOME");
	if (chome == nullptr) {
		throw "HOME not set";
	}
	std::string home(chome);
	if (!std::filesystem::exists(home + "/.qconform/formats")) {
		throw "Please run qconform setup";
	}
    if (!std::filesystem::exists(home + "/.qconform/config.yaml")) {
		throw "Please run qconform setup";
	}
	std::cout << "\033[?1049h\033[2J\033[H";
	std::cout << "This Wizard will create you your .fmt.yaml." << '\n';
	std::cout << "\n\n";
    std::cout << "rules name: (" << std::filesystem::current_path().filename() << ") ";
    std::string rules_name = "";
    std::getline(std::cin, rules_name);
    if (rules_name.empty() || std::all_of(rules_name.begin(), rules_name.end(), isspace)) {
        rules_name = std::filesystem::current_path().filename();
    }
	std::cout << '\n';
	std::cout << "\n\nFormatting rules:\n\n";
	std::cout << "  Enter spacing amount or tab for tabs: (4) ";
	std::string tabs;
	std::getline(std::cin, tabs);
	int tabc = 4;
	std::string use_tabs = "false";
	try {
		if (strip(tabs) != "tab") {
			std::stoi(tabs);
		} else {
			tabs = "4";
			use_tabs = "true";
		}
	} catch (...) {
		tabs = "4";
	}
	std::cout << '\n';
	std::cout << "  Enter max number of parameters per line (or null for none): (null) ";
	std::string param_max;
	std::getline(std::cin, param_max);
	try {
		std::stoi(param_max);
	} catch (...) {
		param_max = "null";
	}
	std::cout << '\n';
	std::cout << "  Enter max number of chars per line (or null for none): (null) ";
	std::string char_max;
	std::getline(std::cin, char_max);
	try {
		std::stoi(char_max);
	} catch (...) {
		char_max = "null";
	}
	std::cout << '\n' << '\n' << '\n';
	std::cout << "  Brace rules:\n\n";
	std::cout << "    What whitespace character should be before open braces (' ' or default): (\\n) ";
	std::string before_braces;
	std::getline(std::cin, before_braces);
	if (before_braces != " ") {
		before_braces = "\\n";
	}
	std::cout << "\n";
	std::cout << "    What level should brace rules be enforced (warning or error or not): (warning) ";
	std::string brace_enforcement;
	std::getline(std::cin, brace_enforcement);
	if (brace_enforcement != "error" && brace_enforcement != "warning" && brace_enforcement != "not") {
		brace_enforcement = "warning";
	}
	std::cout << "\n";
	std::cout << "    Should the apply command automatically fix brace spacing? (Y/n) ";
	std::string autocomplete_brace_fixes;
	std::getline(std::cin, autocomplete_brace_fixes);
	if (autocomplete_brace_fixes == "Y" || autocomplete_brace_fixes == "y") {
		autocomplete_brace_fixes = "true";
	} else {
		autocomplete_brace_fixes = "false";
	}
	std::cout << '\n' << '\n' << '\n';
	std::cout << "  Operator rules:\n\n";
	std::cout << "    Should unops have a space before them: (Y/n)";
	std::string unop_space_before;
	std::getline(std::cin, unop_space_before);
	if (unop_space_before == "Y" || unop_space_before == "y") {
		unop_space_before = " ";
	} else {
		unop_space_before = "";
	} 
	std::cout << "    Should unops have a space after them: (Y/n)";
	std::string unop_space_after;
	std::getline(std::cin, unop_space_after);
	if (unop_space_after == "Y" || unop_space_after == "y") {
		unop_space_after = " ";
	} else {
		unop_space_after = "";
	} 
	std::cout << "    Should binops have a space before them: (Y/n)";
	std::string binop_space_before;
	std::getline(std::cin, binop_space_before);
	if (binop_space_before == "Y" || binop_space_before == "y") {
		binop_space_before = " ";
	} else {
		binop_space_before = "";
	} 
	std::cout << "    Should binops have a space after them: (Y/n)";
	std::string binop_space_after;
	std::getline(std::cin, binop_space_after);
	if (binop_space_after == "Y" || binop_space_after == "y") {
		binop_space_after = " ";
	} else {
		binop_space_after = "";
	} 
	std::cout << '\n';
	std::cout << "\n\nNaming rules:\n\n";
		std::vector<std::string> casing_styles = {
		"snake_case", "camelCase", "PascalCase", 
		"SCREAMING_SNAKE_CASE", "camel_Snake_Case", "Pascal_Snake_Case"
	};
	struct Specifier {
		std::string name;
		std::string position;
		std::string value;
		std::string style;
	};
	std::vector<Specifier> specifiers;
	std::cout << "  Naming Specifiers (e.g., private_member prefix __):\n";
	while (true) {
		std::cout << "    Enter specifier name (or 'done'): ";
		std::string specName;
		std::getline(std::cin, specName);
		if (specName == "done" || specName.empty()) break;
		Specifier s;
		s.name = specName;
		std::cout << "      Position (prefix/suffix): ";
		std::getline(std::cin, s.position);
		std::cout << "      Value (e.g., __): ";
		std::getline(std::cin, s.value);
		std::cout << "      Internal Style (snake_case, etc): ";
		std::getline(std::cin, s.style);
		specifiers.push_back(s);
	}
	std::vector<std::string> rule_types = {
		"vars", "functions", "user_types", "constants", 
		"private_members", "private_methods", "namespaces"
	};
	struct RuleMapping {
		std::string rule_name;
		std::string target;
	};
	std::vector<RuleMapping> rule_mappings;

	std::cout << "\n  Mapping Rules:\n";
	for (const auto& rule : rule_types) {
		std::cout << "    Style for " << rule << ": (";
		for (std::string style : casing_styles) {
			if (style != "snake_case") {
				std::cout << " | ";
			}
			std::cout << style;
		}
		std::cout << ") ";
		std::string choice;
		std::getline(std::cin, choice);
		if (std::find(casing_styles.begin(), casing_styles.end(), strip(choice)) == casing_styles.end()) {
			bool is_speci = false;
			for (Specifier sp : specifiers) {
				if (strip(sp.name) == strip(choice)) {
					is_speci = true;
				}
			}
			if (!is_speci) {
				throw "Invalid casing style or specifier";
			}
		}
		rule_mappings.push_back({rule, strip(choice)});
	}
	std::cout << "\n  Naming Enforcement Level (warning/error/not): (warning) ";
	std::string name_enforce_level;
	std::getline(std::cin, name_enforce_level);
	if (name_enforce_level != "error" && name_enforce_level != "warning" && name_enforce_level != "not") {
		name_enforce_level = "warning";
	}
	std::cout << "  Autocorrect naming? (Y/n): ";
	std::string name_auto;
	std::getline(std::cin, name_auto);
	std::string name_autocorrect = (name_auto == "Y" || name_auto == "y") ? "true" : "false";
	std::cout << "\n\nComment Rules:\n\n";
	std::cout << "  Enter character sequence for normal comments: (//) ";
	std::string comment_normal;
	std::getline(std::cin, comment_normal);
	if (comment_normal.empty()) comment_normal = "//";
	std::cout << "  Enter character sequence for doc comments: (///) ";
	std::string comment_doc;
	std::getline(std::cin, comment_doc);
	if (comment_doc.empty()) comment_doc = "///";
	std::cout << "  Enter character sequence for top-level doc comments: (//!) ";
	std::string top_doc;
	std::getline(std::cin, top_doc);
	if (top_doc.empty()) top_doc = "///";
	std::cout << "  What level should comment rules be enforced (error/warning/not): (warning) ";
	std::string comment_level;
	std::getline(std::cin, comment_level);
	if (comment_level.empty()) {
		comment_level = "warning";
	}
	std::cout << "  What comment style should doc functions (doc, top_level_doc, or normal): (doc) ";
	std::string function_comment;
	std::getline(std::cin, function_comment);
	if (function_comment.empty()) function_comment = "doc";
	std::cout << "  What comment style should doc the top of files (doc, top_level_doc, or normal): (top_level_doc) ";
	std::string file_comment;
	std::getline(std::cin, file_comment);
	if (file_comment.empty()) file_comment = "top_level_doc";
	std::cout << "  What comment style should doc namespaces: (doc) ";
	std::string namespace_comment;
	std::getline(std::cin, namespace_comment);
	if (namespace_comment.empty()) namespace_comment = "doc";
	std::cout << "  What string or special sequence that starts a namespace comment should be used to mark a namespace as 'internal' (no include): (No Include) ";
	std::string internal_comment;
	std::getline(std::cin, internal_comment);
	if (internal_comment.empty()) internal_comment = "No Include";
	std::cout << "\n\nFile & Include Rules:\n\n";
	std::cout << "  Line ending style (lf/crlf): (lf) ";
	std::string nl_style;
	std::getline(std::cin, nl_style);
    if (nl_style.empty()) nl_style = "lf";
	std::cout << "  Force final newline at end of file? (Y/n) ";
	std::string final_nl_input;
	std::getline(std::cin, final_nl_input);
	std::string final_nl = (final_nl_input == "n" || final_nl_input == "N") ? "false" : "true";
	std::cout << "  Collapse blank lines in apply mode? (Y/n) ";
	std::string collapse_blanks_input;
	std::getline(std::cin, collapse_blanks_input);
	std::string collapse_blanks = (collapse_blanks_input == "n" || collapse_blanks_input == "N") ? "false" : "true";
	std::cout << "  Use quotes for includes? (y/N) ";
	std::string inc_quotes_input;
	std::getline(std::cin, inc_quotes_input);
	std::string inc_quotes = (inc_quotes_input == "y" || inc_quotes_input == "Y") ? "true" : "false";
	std::cout << "\n\nNamespace Rules:\n\n";
	std::cout << "  How many types per namespace: (1) ";
	std::string ns_amount;
	std::getline(std::cin, ns_amount);
	if (ns_amount.empty()) ns_amount = "1";
	std::cout << "\033[?1049l";
	std::ofstream rule_file(std::string(std::filesystem::current_path().c_str()) + "/" + rules_name + ".fmt.yaml");
	rule_file << "formatting:\n";
	rule_file << "  indent_size: " << tabc << "\n";
	rule_file << "  use_tabs: " << use_tabs << '\n';
	rule_file << "  params_per_line: " << param_max << '\n';
	rule_file << "  max_width: " << char_max << '\n';
	rule_file << "  braces:\n";
	rule_file << "    enforce:\n";
	rule_file << "      level: " << brace_enforcement << '\n';
	rule_file << "      autocomplete: " << autocomplete_brace_fixes << '\n';
	rule_file << "    open_brace:\n";
	rule_file << "      before: \"" << before_braces << '"' << '\n';
	rule_file << "  operators:\n    unops:\n";
	rule_file << "      before: \"" << unop_space_before << '"' << '\n';
	rule_file << "      after: \"" << unop_space_after << '"' << '\n';
	rule_file << "    binops:\n";
	rule_file << "      before: \"" << binop_space_before << '"' << '\n';
	rule_file << "      after: \"" << binop_space_after << '"' << '\n';
	rule_file << "naming:\n  specifiers:\n" << '\n';
	for (Specifier spec : specifiers) {
		rule_file << "    " << spec.name << ": \n";
		rule_file << "      position: " << spec.position << '\n';
		rule_file << "      value: " << spec.value << '\n';
		rule_file << "      style: " << spec.style << '\n';
	}
	rule_file << "  rules:\n    enforce:\n";
	rule_file << "      level: " << name_enforce_level << '\n';
	rule_file << "      autocorrect: " << name_autocorrect << '\n';
	for (RuleMapping rule : rule_mappings) {
		rule_file << "    " << rule.rule_name << ": " << rule.target << '\n';
	}
	rule_file << "comments:\n";
	rule_file << "  specifiers:\n";
	rule_file << "    normal: " << comment_normal << "\n";
	rule_file << "    doc: " << comment_doc << "\n";
	rule_file << "    top_level_doc: " << top_doc << "\n";
	rule_file << "  rules:\n";
	rule_file << "    enforce:\n";
	rule_file << "      level: " << comment_level << "\n";
	rule_file << "    functions: " << function_comment << "\n";
	rule_file << "    file: " << file_comment << "\n";
	rule_file << "    namespaces: " << namespace_comment << "\n";
	rule_file << "    internal: \"" << internal_comment << "\"\n";

	rule_file << "newlines:\n";
	rule_file << "  style: " << nl_style << "\n";
	rule_file << "  final_newline: " << final_nl << "\n";
	rule_file << "  collapse_blanks: " << collapse_blanks << "\n";

	rule_file << "includes:\n";
	rule_file << "  filepath:\n";
	rule_file << "    quotes: " << inc_quotes << "\n";
	rule_file << "namespaces:\n";
	rule_file << "  types:\n";
	rule_file << "    amount: " << ns_amount << "\n";
	rule_file << "    enforce: warning\n";
	rule_file.close();
	std::filesystem::rename(std::string(std::filesystem::current_path().c_str()) + "/" + rules_name + ".fmt.yaml", home + "/.qconform/formats/" + rules_name + ".fmt.yaml");
	auto yaml = fkyaml::node::deserialize(fopen((home + std::string("/.qconform/config.yaml")).c_str(), "r"));
    yaml["aliases"][rules_name] = home + "/.qconform/formats/" + rules_name + ".fmt.yaml";
    auto config = std::ofstream(home + "/.qconform/config.yaml");
    config << yaml;
    config.close();
	std::cout << "Rule .fmt.yaml file created successfuly!" << '\n';
}
void alias(int count, char** args) {
    if (count < 3) {
        throw "Too few args";
    }
    char* chome = std::getenv("HOME");
	if (chome == nullptr) {
		throw "HOME not set";
	}
	std::string home(chome);
    if (!std::filesystem::exists(home + "/.qconform/config.yaml")) {
        throw "Please run `qconform setup`";
    }
    auto config_node = fkyaml::node::deserialize(fopen((home + "/.qconform/config.yaml").c_str(), "r"));
    if (std::string(args[2]) == "-d" || std::string(args[2]) == "--delete") {
        if (count < 4) {
            throw "Too few args";
        }
        std::string target = args[3];
        if (config_node["aliases"].contains(target)) {
            if (target == "default") {
                std::cout << "Cannot delete the default alias." << std::endl;
            } else {
                std::filesystem::remove(config_node["aliases"][target].get_value<std::string>());
                config_node["aliases"].as_map().erase(target);
                auto config = std::ofstream(home + "/.qconform/config.yaml");
                config << fkyaml::node::serialize(config_node);
                config.close();
                std::cout << "Alias '" << target << "' removed." << std::endl;
            }
        } else {
            throw "Alias " + target + " doesn't exist.";
        }
    } else if (std::string(args[2]) == "-l" || std::string(args[2]) == "--list") {
        auto aliases = config_node["aliases"];
        std::cout << "Current Formatting Aliases:\n";
        std::cout << "---------------------------\n";
        for (auto it = aliases.begin(); it != aliases.end(); ++it) {
            std::cout << "  " << it.key().get_value<std::string>() << " -> " << it.value().get_value<std::string>() << "\n";
        }
    } else {
        if (count < 4) {
            throw "Too few args";
        }
        std::string alias_name = args[2];
        std::filesystem::path alias_path(args[3]);
        if (!alias_path.empty() && alias_path.string()[0] == '~') {
            alias_path = std::filesystem::path(home) / alias_path.string().substr(2);
        }
        std::cout << "Importing " << alias_path.filename() << " into ~/.qconform/formats/..." << '\n';
        std::filesystem::rename(alias_path, home + "/.qconform/formats/" + std::string(alias_path.filename().c_str()));
        config_node["aliases"][alias_name] = home + "/.qconform/formats/" + std::string(alias_path.filename().c_str());
        std::ofstream config_out(home + "/.qconform/config.yaml");
        config_out << fkyaml::node::serialize(config_node);
        config_out.close();
    }
}
void list_conventions(int count, char** args) {
    char* chome = std::getenv("HOME");
	if (chome == nullptr) {
		throw "HOME not set";
	}
    if (count < 3) {
        throw "Too few args";
    }
	std::string home(chome);
    if (!std::filesystem::exists(home + "/.qconform/config.yaml")) {
        throw "Please run `qconform setup`";
    }
    auto config = fkyaml::node::deserialize(fopen((home + "/.qconform/config.yaml").c_str(), "r"));
    std::string active_path = config["aliases"][args[2]].get_value<std::string>();
    auto rules = fkyaml::node::deserialize(fopen(active_path.c_str(), "r"));
    std::cout << "Active Conventions (" << active_path << "):\n";
    std::cout << "------------------------------------------\n";
    std::cout << "Formatting:\n";
    std::cout << "  Indentation: " << rules["formatting"]["indent_size"] 
              << (rules["formatting"]["use_tabs"].get_value<bool>() ? " (tabs)" : " (spaces)") << "\n";
    std::cout << "\nNaming Rules:\n";
    auto naming = rules["naming"]["rules"];
    for (auto it = naming.begin(); it != naming.end(); ++it) {
        if (it.key().get_value<std::string>() != "enforce") {
             std::cout << "  " << it.key().get_value<std::string>() 
                       << ": " << it.value().get_value<std::string>() << "\n";
        }
    }
    std::cout << "--- Formatting Rules (as raw yaml) ---" << '\n';
    std::cout << fkyaml::node::serialize(rules) << '\n';
}
struct Command {
	std::string name;
	std::function<void(int count, char** args)> command;
};
std::vector<Command> commands = {{"setup", setup}, {"help", help}, {"create", create}, {"alias", alias}, {"list", list_conventions}}; 
int main(int argc, char** argv) {
	if (argc < 2) {
		std::cout << "Too few args. Try qconform help" << '\n';
		return 1;
	}
	for (Command comm : commands) {
		if (argv[1] == comm.name) {
			try {
				comm.command(argc, argv);
				return 0;
			} catch (std::string e) {
				std::cout << "Error: " << e << '\n';
			} catch (const char* e) {
				std::cout << "Error: " << e << '\n';
			} catch (char* e) {
				std::cout << "Error: " << e << '\n';
			} catch (std::runtime_error e) {
				std::cout << "Error: " << e.what() << '\n';
			} catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << '\n';
            } catch (...) {
				std::cout << "Unknown error." << '\n';
			}
			return 1;
		}
	}
	std::cout << "Unknown command" << '\n';
	return 1;
}

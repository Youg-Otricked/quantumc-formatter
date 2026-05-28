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
#include <algorithem>
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
				tokens.push_back(Token(TokenType::WHITESPACE, std::string(1, this->current_char)));
                this->advance();
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
qconform list - Lists current default formatting conventions.
qconform conv <filename | default> - Changes formatting conventions to either default, or to the conventions stored in >filename>
qconform check [config file path | default] <filename> - Checks formatting against formatting conventions. Current saved default unless specified file path / default
qconform apply <filename> - Same as above but also applys.
qconform setup - Creates all the folders and files for the cli to work, and adds it to your PATH.
qconform create - Adds formatting rules, creates the rules file, and adds it, with an alias.
qconform alias [-d/--delete | -l/--list] - Creates a quick alias for a rule file and stores it in the saved directorys in the .qconform folder. -d/--delete is the vis-versa, and -l/--list lists all aliases
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
		file << "aliases:\n  - default:\n    " << home << "/.qconform/default.fmt.yaml";
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
void create(int count, char** args) {
	std::cout << "\033[?1049h\033[2J\033[H";
	std::cout << "This Wizard will create you your .fmt.yaml." << '\n';
	std::cout << "\n\n";
    std::cout << "rules name: (" << std::filesystem::current_path().filename() << ") ";
    std::string rulesName = "";
    std::getline(std::cin, rulesName);
    if (rulesName.empty() || std::all_of(rulesName.begin(), scopeName.end(), isspace)) {
        rulesName = std::filesystem::current_path().filename();
    }
	std::cout << '\n';
	std::cout << "enter spacing amount or tab for tabs: (4) ";
	std::string tabs;
	try {
		std::getline(std::cin, tabs);
	} catch (...) {
		throw "Invalid tab amount.";
	}
	std::cout << "\033[?1049l";
	std::cout << "Rule .fmt.yaml file created successfuly!" << '\n';
}
struct Command {
	std::string name;
	std::function<void(int count, char** args)> command;
};
std::vector<Command> commands = {{"setup", setup}, {"help", help}}; 
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
			} catch (...) {
				std::cout << "Unknown error." << '\n';
			}
		}
	}
	std::cout << "Unknown command" << '\n';
	return 1;
}

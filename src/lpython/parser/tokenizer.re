#include <limits>
#include <iostream>
#include <lpython/parser/parser_exception.h>
#include <lpython/parser/tokenizer.h>
#include <lpython/parser/parser.tab.hh>
#include <libasr/bigint.h>

namespace LFortran
{

template<int base>
bool adddgt(uint64_t &u, uint64_t d)
{
    if (u > (std::numeric_limits<uint64_t>::max() - d) / base) {
        return false;
    }
    u = u * base + d;
    return true;
}

bool lex_oct(const unsigned char *s, const unsigned char *e, uint64_t &u)
{
    for (u = 0, ++s; s < e; ++s) {
        if (!adddgt<8>(u, *s - 0x30u)) {
            return false;
        }
    }
    return true;
}

bool lex_dec(const unsigned char *s, const unsigned char *e, uint64_t &u)
{
    for (u = 0; s < e; ++s) {
        if (*s == '_') continue;
        if (!adddgt<10>(u, *s - 0x30u)) {
            return false;
        }
    }
    return true;
}

void lex_dec_int_large(Allocator &al, const unsigned char *s,
    const unsigned char *e, BigInt::BigInt &u)
{
    uint64_t ui;
    if (lex_dec(s, e, ui)) {
        if (ui <= BigInt::MAX_SMALL_INT) {
            u.from_smallint(ui);
            return;
        }
    }
    // TODO: remove underscore from large ints
    const unsigned char *start = s;
    Str num;
    num.p = (char*)start;
    num.n = e-start;
    u.from_largeint(al, num);
}


uint64_t get_value(char *s, int base, const Location &loc) {
    std::string str(s);
    uint64_t n;
    try {
        n = std::stoull(str, nullptr, base);
    } catch (const std::out_of_range &e) {
        throw parser_local::TokenizerError(
            "Integer too large to convert", {loc});
    }
    return n;
}

// Converts integer (Dec, Hex, Bin, Oct) from a string to BigInt `u`
// s ... the start of the integer
// e ... the character after the end
void lex_int(Allocator &al, const unsigned char *s,
    const unsigned char *e, BigInt::BigInt &u, const Location &loc)
{
    if (std::tolower(s[1]) == 'x') {
        // Hex
        s = s + 2;
        uint64_t n = get_value((char*)s, 16, loc);
        u.from_smallint(n);
    } else if (std::tolower(s[1]) == 'b') {
        // Bin
        s = s + 2;
        uint64_t n = get_value((char*)s, 2, loc);
        u.from_smallint(n);
    } else if ((std::tolower(s[1]) == 'o')) {
        // Oct
        s = s + 2;
        uint64_t n = get_value((char*)s, 8, loc);
        u.from_smallint(n);
    } else {
        lex_dec_int_large(al, s, e, u);
    }
    return;
}

// Tokenizes imag num of value 123j into `u` and `suffix`
// s ... the start of the integer
// e ... the character after the end
void lex_imag(Allocator &al, const unsigned char *s,
    const unsigned char *e, BigInt::BigInt &u, Str &suffix)
{
    const unsigned char *start = s;
    for (; s < e; ++s) {
        if ((*s == 'j') | (*s == 'J')) {
            suffix.p = (char*) s; // `j`
            suffix.n = 1;

            Str num;
            num.p = (char*)start;
            num.n = s-start;
            u.from_largeint(al, num);
            return;
        }
    }
}

void Tokenizer::set_string(const std::string &str)
{
    // The input string must be NULL terminated, otherwise the tokenizer will
    // not detect the end of string. After C++11, the std::string is guaranteed
    // to end with \0, but we check this here just in case.
    LFORTRAN_ASSERT(str[str.size()] == '\0');
    cur = (unsigned char *)(&str[0]);
    string_start = cur;
    cur_line = cur;
    line_num = 1;
}

void Tokenizer::record_paren(Location &loc, char c) {
    switch (c) {
        case '(':
        case '[':
        case '{':
            if(parenlevel >= MAX_PAREN_LEVEL) {
                throw parser_local::TokenizerError(
                    "Too many nested parentheses", {loc});
            }
            paren_stack[parenlevel] = c;
            parenlevel++;
            break;

        case ')':
        case ']':
        case '}':
            if(parenlevel < 1) {
                throw parser_local::TokenizerError(
                    "Parenthesis unexpected", {loc});
            }
            parenlevel--;

            char prev_paren = paren_stack[parenlevel];
            if(!((prev_paren == '(' && c == ')') ||
                 (prev_paren == '[' && c == ']') ||
                 (prev_paren == '{' && c == '}'))) {
                throw parser_local::TokenizerError(
                    "Parentheses does not match", {loc});
            }
            break;
    }
    return;
}

#define KW(x) token(yylval.string); RET(KW_##x);
#define RET(x) token_loc(loc); last_token=yytokentype::x; return yytokentype::x;


int Tokenizer::lex(Allocator &al, YYSTYPE &yylval, Location &loc, diag::Diagnostics &/*diagnostics*/)
{
    if(dedent == 1) {
        // Removes the indent completely i.e., to level 0
        if(!indent_length.empty()) {
            indent_length.pop_back();
            return yytokentype::TK_DEDENT;
        } else {
            dedent = 0;
        }
    } else if(dedent == 2) {
        // Reduce the indent to `last_indent_length`
        if((long int)indent_length.back() != last_indent_length) {
            indent_length.pop_back();
            loc.first = loc.last;
            return yytokentype::TK_DEDENT;
        } else {
            dedent = 0;
        }
    }

    for (;;) {
        tok = cur;

        /*
        Re2c has excellent documentation at:

        https://re2c.org/manual/manual_c.html

        The first paragraph there explains the basics:

        * If multiple rules match, the longest match takes precedence
        * If multiple rules match the same string, the earlier rule takes
          precedence
        * Default rule `*` should always be defined, it has the lowest priority
          regardless of its place and matches any code unit
        * We use the "Sentinel character" method for end of input:
            * The end of the input text is denoted with a null character \x00
            * Thus the null character cannot be part of the input otherwise
            * There is one rule to match \x00 to end the parser
            * No other rule is allowed to match \x00, otherwise the re2c block
              would parse past the end of the string and segfaults
            * A special case of the previous point are negated character
              ranges, such as [^"\x00], where one must include \x00 in it to
              ensure this rule does not match \x00 (all other rules simply do
              not mention \x00)
            * See the "Handling the end of input" section in the re2c
              documentation for more info

        The re2c block interacts with the rest of the code via just one pointer
        variable `cur`. On entering the re2c block, the `cur` variable must
        point to the first character of the token to be tokenized by the block.
        The re2c block below then executes on its own until a rule is matched:
        the action in {} is then executed. In that action `cur` points to the
        first character of the next token.

        Before the re2c block we save the current `cur` into `tok`, so that we
        can use `tok` and `cur` in the action in {} to extract the token that
        corresponds to the rule that got matched:

        * `tok` points to the first character of the token
        * `cur-1` points to the last character of the token
        * `cur` points to the first character of the next token
        * `cur-tok` is the length of the token

        In the action, we do one of:

        * call `continue` which executes another cycle in the for loop (which
          will parse the next token); we use this to skip a token
        * call `return` which returns from this function; we return a token
        * throw an exception (terminates the tokenizer)

        In the first two cases, `cur` points to first character of the next
        token, which becomes `tok` at the next iteration of the loop (either
        right away after `continue` or after the `lex` function is called again
        after `return`).

        See the manual for more details.
        */


        // These two variables are needed by the re2c block below internally,
        // initialization is not needed. One can think of them as local
        // variables of the re2c block.
        unsigned char *mar;//, *ctxmar;
        /*!re2c
            re2c:define:YYCURSOR = cur;
            re2c:define:YYMARKER = mar;
            // re2c:define:YYCTXMARKER = ctxmar;
            re2c:yyfill:enable = 0;
            re2c:define:YYCTYPE = "unsigned char";
            re2c:flags:utf-8 = 1;

            end = "\x00";
            whitespace = [ \t\v\r]+;
            newline = "\n";
            digit = [0-9];
            int_oct = "0"[oO][0-7]+;
            int_bin = "0"[bB][01]+;
            int_hex = "0"[xX][0-9a-fA-F]+;
            int_dec = digit+ (digit | "_" digit)*;
            char = [\x41-\x5a\x61-\x7a\xaa-\xaa\xb5-\xb5\xba-\xba\xc0-\xd6\xd8-\xf6\xf8-\u02c1\u02c6-\u02d1\u02e0-\u02e4\u02ec-\u02ec\u02ee-\u02ee\u0370-\u0374\u0376-\u0377\u037a-\u037d\u037f-\u037f\u0386-\u0386\u0388-\u038a\u038c-\u038c\u038e-\u03a1\u03a3-\u03f5\u03f7-\u0481\u048a-\u052f\u0531-\u0556\u0559-\u0559\u0561-\u0587\u05d0-\u05ea\u05f0-\u05f2\u0620-\u064a\u066e-\u066f\u0671-\u06d3\u06d5-\u06d5\u06e5-\u06e6\u06ee-\u06ef\u06fa-\u06fc\u06ff-\u06ff\u0710-\u0710\u0712-\u072f\u074d-\u07a5\u07b1-\u07b1\u07ca-\u07ea\u07f4-\u07f5\u07fa-\u07fa\u0800-\u0815\u081a-\u081a\u0824-\u0824\u0828-\u0828\u0840-\u0858\u08a0-\u08b2\u0904-\u0939\u093d-\u093d\u0950-\u0950\u0958-\u0961\u0971-\u0980\u0985-\u098c\u098f-\u0990\u0993-\u09a8\u09aa-\u09b0\u09b2-\u09b2\u09b6-\u09b9\u09bd-\u09bd\u09ce-\u09ce\u09dc-\u09dd\u09df-\u09e1\u09f0-\u09f1\u0a05-\u0a0a\u0a0f-\u0a10\u0a13-\u0a28\u0a2a-\u0a30\u0a32-\u0a33\u0a35-\u0a36\u0a38-\u0a39\u0a59-\u0a5c\u0a5e-\u0a5e\u0a72-\u0a74\u0a85-\u0a8d\u0a8f-\u0a91\u0a93-\u0aa8\u0aaa-\u0ab0\u0ab2-\u0ab3\u0ab5-\u0ab9\u0abd-\u0abd\u0ad0-\u0ad0\u0ae0-\u0ae1\u0b05-\u0b0c\u0b0f-\u0b10\u0b13-\u0b28\u0b2a-\u0b30\u0b32-\u0b33\u0b35-\u0b39\u0b3d-\u0b3d\u0b5c-\u0b5d\u0b5f-\u0b61\u0b71-\u0b71\u0b83-\u0b83\u0b85-\u0b8a\u0b8e-\u0b90\u0b92-\u0b95\u0b99-\u0b9a\u0b9c-\u0b9c\u0b9e-\u0b9f\u0ba3-\u0ba4\u0ba8-\u0baa\u0bae-\u0bb9\u0bd0-\u0bd0\u0c05-\u0c0c\u0c0e-\u0c10\u0c12-\u0c28\u0c2a-\u0c39\u0c3d-\u0c3d\u0c58-\u0c59\u0c60-\u0c61\u0c85-\u0c8c\u0c8e-\u0c90\u0c92-\u0ca8\u0caa-\u0cb3\u0cb5-\u0cb9\u0cbd-\u0cbd\u0cde-\u0cde\u0ce0-\u0ce1\u0cf1-\u0cf2\u0d05-\u0d0c\u0d0e-\u0d10\u0d12-\u0d3a\u0d3d-\u0d3d\u0d4e-\u0d4e\u0d60-\u0d61\u0d7a-\u0d7f\u0d85-\u0d96\u0d9a-\u0db1\u0db3-\u0dbb\u0dbd-\u0dbd\u0dc0-\u0dc6\u0e01-\u0e30\u0e32-\u0e33\u0e40-\u0e46\u0e81-\u0e82\u0e84-\u0e84\u0e87-\u0e88\u0e8a-\u0e8a\u0e8d-\u0e8d\u0e94-\u0e97\u0e99-\u0e9f\u0ea1-\u0ea3\u0ea5-\u0ea5\u0ea7-\u0ea7\u0eaa-\u0eab\u0ead-\u0eb0\u0eb2-\u0eb3\u0ebd-\u0ebd\u0ec0-\u0ec4\u0ec6-\u0ec6\u0edc-\u0edf\u0f00-\u0f00\u0f40-\u0f47\u0f49-\u0f6c\u0f88-\u0f8c\u1000-\u102a\u103f-\u103f\u1050-\u1055\u105a-\u105d\u1061-\u1061\u1065-\u1066\u106e-\u1070\u1075-\u1081\u108e-\u108e\u10a0-\u10c5\u10c7-\u10c7\u10cd-\u10cd\u10d0-\u10fa\u10fc-\u1248\u124a-\u124d\u1250-\u1256\u1258-\u1258\u125a-\u125d\u1260-\u1288\u128a-\u128d\u1290-\u12b0\u12b2-\u12b5\u12b8-\u12be\u12c0-\u12c0\u12c2-\u12c5\u12c8-\u12d6\u12d8-\u1310\u1312-\u1315\u1318-\u135a\u1380-\u138f\u13a0-\u13f4\u1401-\u166c\u166f-\u167f\u1681-\u169a\u16a0-\u16ea\u16f1-\u16f8\u1700-\u170c\u170e-\u1711\u1720-\u1731\u1740-\u1751\u1760-\u176c\u176e-\u1770\u1780-\u17b3\u17d7-\u17d7\u17dc-\u17dc\u1820-\u1877\u1880-\u18a8\u18aa-\u18aa\u18b0-\u18f5\u1900-\u191e\u1950-\u196d\u1970-\u1974\u1980-\u19ab\u19c1-\u19c7\u1a00-\u1a16\u1a20-\u1a54\u1aa7-\u1aa7\u1b05-\u1b33\u1b45-\u1b4b\u1b83-\u1ba0\u1bae-\u1baf\u1bba-\u1be5\u1c00-\u1c23\u1c4d-\u1c4f\u1c5a-\u1c7d\u1ce9-\u1cec\u1cee-\u1cf1\u1cf5-\u1cf6\u1d00-\u1dbf\u1e00-\u1f15\u1f18-\u1f1d\u1f20-\u1f45\u1f48-\u1f4d\u1f50-\u1f57\u1f59-\u1f59\u1f5b-\u1f5b\u1f5d-\u1f5d\u1f5f-\u1f7d\u1f80-\u1fb4\u1fb6-\u1fbc\u1fbe-\u1fbe\u1fc2-\u1fc4\u1fc6-\u1fcc\u1fd0-\u1fd3\u1fd6-\u1fdb\u1fe0-\u1fec\u1ff2-\u1ff4\u1ff6-\u1ffc\u2071-\u2071\u207f-\u207f\u2090-\u209c\u2102-\u2102\u2107-\u2107\u210a-\u2113\u2115-\u2115\u2119-\u211d\u2124-\u2124\u2126-\u2126\u2128-\u2128\u212a-\u212d\u212f-\u2139\u213c-\u213f\u2145-\u2149\u214e-\u214e\u2183-\u2184\u2c00-\u2c2e\u2c30-\u2c5e\u2c60-\u2ce4\u2ceb-\u2cee\u2cf2-\u2cf3\u2d00-\u2d25\u2d27-\u2d27\u2d2d-\u2d2d\u2d30-\u2d67\u2d6f-\u2d6f\u2d80-\u2d96\u2da0-\u2da6\u2da8-\u2dae\u2db0-\u2db6\u2db8-\u2dbe\u2dc0-\u2dc6\u2dc8-\u2dce\u2dd0-\u2dd6\u2dd8-\u2dde\u2e2f-\u2e2f\u3005-\u3006\u3031-\u3035\u303b-\u303c\u3041-\u3096\u309d-\u309f\u30a1-\u30fa\u30fc-\u30ff\u3105-\u312d\u3131-\u318e\u31a0-\u31ba\u31f0-\u31ff\u3400-\u4db5\u4e00-\u9fcc\ua000-\ua48c\ua4d0-\ua4fd\ua500-\ua60c\ua610-\ua61f\ua62a-\ua62b\ua640-\ua66e\ua67f-\ua69d\ua6a0-\ua6e5\ua717-\ua71f\ua722-\ua788\ua78b-\ua78e\ua790-\ua7ad\ua7b0-\ua7b1\ua7f7-\ua801\ua803-\ua805\ua807-\ua80a\ua80c-\ua822\ua840-\ua873\ua882-\ua8b3\ua8f2-\ua8f7\ua8fb-\ua8fb\ua90a-\ua925\ua930-\ua946\ua960-\ua97c\ua984-\ua9b2\ua9cf-\ua9cf\ua9e0-\ua9e4\ua9e6-\ua9ef\ua9fa-\ua9fe\uaa00-\uaa28\uaa40-\uaa42\uaa44-\uaa4b\uaa60-\uaa76\uaa7a-\uaa7a\uaa7e-\uaaaf\uaab1-\uaab1\uaab5-\uaab6\uaab9-\uaabd\uaac0-\uaac0\uaac2-\uaac2\uaadb-\uaadd\uaae0-\uaaea\uaaf2-\uaaf4\uab01-\uab06\uab09-\uab0e\uab11-\uab16\uab20-\uab26\uab28-\uab2e\uab30-\uab5a\uab5c-\uab5f\uab64-\uab65\uabc0-\uabe2\uac00-\ud7a3\ud7b0-\ud7c6\ud7cb-\ud7fb\uf900-\ufa6d\ufa70-\ufad9\ufb00-\ufb06\ufb13-\ufb17\ufb1d-\ufb1d\ufb1f-\ufb28\ufb2a-\ufb36\ufb38-\ufb3c\ufb3e-\ufb3e\ufb40-\ufb41\ufb43-\ufb44\ufb46-\ufbb1\ufbd3-\ufd3d\ufd50-\ufd8f\ufd92-\ufdc7\ufdf0-\ufdfb\ufe70-\ufe74\ufe76-\ufefc\uff21-\uff3a\uff41-\uff5a\uff66-\uffbe\uffc2-\uffc7\uffca-\uffcf\uffd2-\uffd7\uffda-\uffdc\U00010000-\U0001000b\U0001000d-\U00010026\U00010028-\U0001003a\U0001003c-\U0001003d\U0001003f-\U0001004d\U00010050-\U0001005d\U00010080-\U000100fa\U00010280-\U0001029c\U000102a0-\U000102d0\U00010300-\U0001031f\U00010330-\U00010340\U00010342-\U00010349\U00010350-\U00010375\U00010380-\U0001039d\U000103a0-\U000103c3\U000103c8-\U000103cf\U00010400-\U0001049d\U00010500-\U00010527\U00010530-\U00010563\U00010600-\U00010736\U00010740-\U00010755\U00010760-\U00010767\U00010800-\U00010805\U00010808-\U00010808\U0001080a-\U00010835\U00010837-\U00010838\U0001083c-\U0001083c\U0001083f-\U00010855\U00010860-\U00010876\U00010880-\U0001089e\U00010900-\U00010915\U00010920-\U00010939\U00010980-\U000109b7\U000109be-\U000109bf\U00010a00-\U00010a00\U00010a10-\U00010a13\U00010a15-\U00010a17\U00010a19-\U00010a33\U00010a60-\U00010a7c\U00010a80-\U00010a9c\U00010ac0-\U00010ac7\U00010ac9-\U00010ae4\U00010b00-\U00010b35\U00010b40-\U00010b55\U00010b60-\U00010b72\U00010b80-\U00010b91\U00010c00-\U00010c48\U00011003-\U00011037\U00011083-\U000110af\U000110d0-\U000110e8\U00011103-\U00011126\U00011150-\U00011172\U00011176-\U00011176\U00011183-\U000111b2\U000111c1-\U000111c4\U000111da-\U000111da\U00011200-\U00011211\U00011213-\U0001122b\U000112b0-\U000112de\U00011305-\U0001130c\U0001130f-\U00011310\U00011313-\U00011328\U0001132a-\U00011330\U00011332-\U00011333\U00011335-\U00011339\U0001133d-\U0001133d\U0001135d-\U00011361\U00011480-\U000114af\U000114c4-\U000114c5\U000114c7-\U000114c7\U00011580-\U000115ae\U00011600-\U0001162f\U00011644-\U00011644\U00011680-\U000116aa\U000118a0-\U000118df\U000118ff-\U000118ff\U00011ac0-\U00011af8\U00012000-\U00012398\U00013000-\U0001342e\U00016800-\U00016a38\U00016a40-\U00016a5e\U00016ad0-\U00016aed\U00016b00-\U00016b2f\U00016b40-\U00016b43\U00016b63-\U00016b77\U00016b7d-\U00016b8f\U00016f00-\U00016f44\U00016f50-\U00016f50\U00016f93-\U00016f9f\U0001b000-\U0001b001\U0001bc00-\U0001bc6a\U0001bc70-\U0001bc7c\U0001bc80-\U0001bc88\U0001bc90-\U0001bc99\U0001d400-\U0001d454\U0001d456-\U0001d49c\U0001d49e-\U0001d49f\U0001d4a2-\U0001d4a2\U0001d4a5-\U0001d4a6\U0001d4a9-\U0001d4ac\U0001d4ae-\U0001d4b9\U0001d4bb-\U0001d4bb\U0001d4bd-\U0001d4c3\U0001d4c5-\U0001d505\U0001d507-\U0001d50a\U0001d50d-\U0001d514\U0001d516-\U0001d51c\U0001d51e-\U0001d539\U0001d53b-\U0001d53e\U0001d540-\U0001d544\U0001d546-\U0001d546\U0001d54a-\U0001d550\U0001d552-\U0001d6a5\U0001d6a8-\U0001d6c0\U0001d6c2-\U0001d6da\U0001d6dc-\U0001d6fa\U0001d6fc-\U0001d714\U0001d716-\U0001d734\U0001d736-\U0001d74e\U0001d750-\U0001d76e\U0001d770-\U0001d788\U0001d78a-\U0001d7a8\U0001d7aa-\U0001d7c2\U0001d7c4-\U0001d7cb\U0001e800-\U0001e8c4\U0001ee00-\U0001ee03\U0001ee05-\U0001ee1f\U0001ee21-\U0001ee22\U0001ee24-\U0001ee24\U0001ee27-\U0001ee27\U0001ee29-\U0001ee32\U0001ee34-\U0001ee37\U0001ee39-\U0001ee39\U0001ee3b-\U0001ee3b\U0001ee42-\U0001ee42\U0001ee47-\U0001ee47\U0001ee49-\U0001ee49\U0001ee4b-\U0001ee4b\U0001ee4d-\U0001ee4f\U0001ee51-\U0001ee52\U0001ee54-\U0001ee54\U0001ee57-\U0001ee57\U0001ee59-\U0001ee59\U0001ee5b-\U0001ee5b\U0001ee5d-\U0001ee5d\U0001ee5f-\U0001ee5f\U0001ee61-\U0001ee62\U0001ee64-\U0001ee64\U0001ee67-\U0001ee6a\U0001ee6c-\U0001ee72\U0001ee74-\U0001ee77\U0001ee79-\U0001ee7c\U0001ee7e-\U0001ee7e\U0001ee80-\U0001ee89\U0001ee8b-\U0001ee9b\U0001eea1-\U0001eea3\U0001eea5-\U0001eea9\U0001eeab-\U0001eebb\U00020000-\U0002a6d6\U0002a700-\U0002b734\U0002b740-\U0002b81d\U0002f800-\U0002fa1d_];
            name = char (char | digit)*;
            significand = (digit+ "." digit*) | ("." digit+);
            exp = [eE][-+]? digit+;
            integer = int_dec | int_oct | int_bin | int_hex;
            real = (significand exp?) | (digit+ exp);
            imag_number = (real | digit+)[jJ];
            string1 = '"' ('\\"'|[^"\x00])* '"';
            string2 = "'" ("\\'"|[^'\x00])* "'";
            string3 = '"""' ( '\\"' | '"' [^"\x00] | '""' [^"\x00] | [^"\x00] )* '"""';
            string4 = "'''" ( "\\'" | "'" [^'\x00] | "''" [^'\x00] | [^'\x00] )* "'''";
            type_comment = "#" whitespace? "type:" whitespace? [^\n\x00]*;
            comment = "#" [^\n\x00]*;
            // docstring = newline whitespace? string1 | string2;
            ws_comment = whitespace? comment? newline;

            * { token_loc(loc);
                std::string t = token();
                throw parser_local::TokenizerError(diag::Diagnostic(
                    "Token '" + t + "' is not recognized",
                    diag::Level::Error, diag::Stage::Tokenizer, {
                        diag::Label("token not recognized", {loc})
                    })
                );
            }
            end {
                token_loc(loc);
                if(parenlevel) {
                    throw parser_local::TokenizerError(
                        "Parentheses was never closed", {loc});
                }
                RET(END_OF_FILE);
            }

            whitespace {
                if(cur[0] == '#') { continue; }
                if(last_token == yytokentype::TK_NEWLINE && cur[0] == '\n') {
                    continue;
                }
                if (indent) {
                    indent = false;
                    indent_length.push_back(cur-tok);
                    last_indent_length = cur-tok;
                    RET(TK_INDENT);
                } else {
                    if(last_token == yytokentype::TK_NEWLINE && cur[0] != ' '
                            && last_indent_length > cur-tok) {
                        last_indent_length = cur-tok;
                        dedent = 2;
                        if (!indent_length.empty()) {
                            indent_length.pop_back();
                        }
                        RET(TK_DEDENT);
                    }
                }
                continue;
             }

            // Keywords
            "as"       { KW(AS) }
            "assert"   { KW(ASSERT) }
            "async"    { KW(ASYNC) }
            "await"    { KW(AWAIT) }
            "break"    { KW(BREAK) }
            "class"    { KW(CLASS) }
            "continue" { KW(CONTINUE) }
            "def"      { KW(DEF) }
            "del"      { KW(DEL) }
            "elif"     { KW(ELIF) }
            "else"     { KW(ELSE) }
            "except"   { KW(EXCEPT) }
            "finally"  { KW(FINALLY) }
            "for"      { KW(FOR) }
            "from"     { KW(FROM) }
            "global"   { KW(GLOBAL) }
            "if"       { KW(IF) }
            "import"   { KW(IMPORT) }
            "in"       { KW(IN) }
            "is"       { KW(IS) }
            "lambda"   { KW(LAMBDA) }
            "None"     { KW(NONE) }
            "nonlocal" { KW(NONLOCAL) }
            "pass"     { KW(PASS) }
            "raise"    { KW(RAISE) }
            "return"   { KW(RETURN) }
            "try"      { KW(TRY) }
            "while"    { KW(WHILE) }
            "with"     { KW(WITH) }
            "yield"    { KW(YIELD) }
            "yield" whitespace "from" whitespace { KW(YIELD_FROM) }

            // Tokens
            newline {
                if(parenlevel) { continue; }
                if(cur[0] == '#') { RET(TK_NEWLINE); }
                if (last_token == yytokentype::TK_COLON
                        || colon_actual_last_token) {
                    colon_actual_last_token = false;
                    indent = true;
                } else if (cur[0] != ' ' && cur[0] != '\n'
                        && last_indent_length > cur-tok) {
                    last_indent_length = 0;
                    dedent = 1;
                }
                RET(TK_NEWLINE);
            }

            "\\" newline { continue; }

            // Single character symbols
            "(" { token_loc(loc); record_paren(loc, '('); RET(TK_LPAREN) }
            "[" { token_loc(loc); record_paren(loc, '['); RET(TK_LBRACKET) }
            "{" { token_loc(loc); record_paren(loc, '{'); RET(TK_LBRACE) }
            ")" { token_loc(loc); record_paren(loc, ')'); RET(TK_RPAREN) }
            "]" { token_loc(loc); record_paren(loc, ']'); RET(TK_RBRACKET) }
            "}" { token_loc(loc); record_paren(loc, '}'); RET(TK_RBRACE) }
            "+" { RET(TK_PLUS) }
            "-" { RET(TK_MINUS) }
            "=" { RET(TK_EQUAL) }
            ":" {
                    if(cur[0] == '\n' && !parenlevel){
                        colon_actual_last_token = true;
                    }
                    RET(TK_COLON);
                }
            ";" { RET(TK_SEMICOLON) }
            "/" { RET(TK_SLASH) }
            "%" { RET(TK_PERCENT) }
            "," { RET(TK_COMMA) }
            "*" { RET(TK_STAR) }
            "|" { RET(TK_VBAR) }
            "&" { RET(TK_AMPERSAND) }
            "." { RET(TK_DOT) }
            "~" { RET(TK_TILDE) }
            "^" { RET(TK_CARET) }
            "@" { RET(TK_AT) }

            // Multiple character symbols
            ">>" { RET(TK_RIGHTSHIFT) }
            "<<" { RET(TK_LEFTSHIFT) }
            "**" { RET(TK_POW) }
            "//" { RET(TK_FLOOR_DIV) }
            "+=" { RET(TK_PLUS_EQUAL) }
            "-=" { RET(TK_MIN_EQUAL) }
            "*=" { RET(TK_STAR_EQUAL) }
            "/=" { RET(TK_SLASH_EQUAL) }
            "%=" { RET(TK_PERCENT_EQUAL) }
            "&=" { RET(TK_AMPER_EQUAL) }
            "|=" { RET(TK_VBAR_EQUAL) }
            "^=" { RET(TK_CARET_EQUAL) }
            "@=" { RET(TK_ATEQUAL) }
            "->" { RET(TK_RARROW) }
            ":=" { RET(TK_COLONEQUAL) }
            "..." { RET(TK_ELLIPSIS) }
            "<<=" { RET(TK_LEFTSHIFT_EQUAL) }
            ">>=" { RET(TK_RIGHTSHIFT_EQUAL) }
            "**=" { RET(TK_POW_EQUAL) }
            "//=" { RET(TK_DOUBLESLASH_EQUAL) }

            // Relational operators
            "=="   { RET(TK_EQ) }
            "!="   { RET(TK_NE) }
            "<"    { RET(TK_LT) }
            "<="   { RET(TK_LE) }
            ">"    { RET(TK_GT) }
            ">="   { RET(TK_GE) }

            // Logical operators
            "not"  { RET(TK_NOT) }
            "and"  { RET(TK_AND) }
            "or"   { RET(TK_OR) }
            "is" whitespace "not" whitespace { RET(TK_IS_NOT) }
            "is" whitespace? "\\" newline whitespace? "not" whitespace { RET(TK_IS_NOT) }
            "not" whitespace "in" whitespace { RET(TK_NOT_IN) }
            "not" whitespace? "\\" newline whitespace? "in" whitespace { RET(TK_NOT_IN) }

            // True/False

            "True" { RET(TK_TRUE) }
            "False" { RET(TK_FALSE) }

            real { yylval.f = std::atof(token().c_str()); RET(TK_REAL) }
            integer {
                BigInt::BigInt n;
                token_loc(loc);
                lex_int(al, tok, cur, n, loc);
                yylval.n = n.n;
                RET(TK_INTEGER)
            }
            imag_number {
                yylval.f = std::atof(token().c_str());
                RET(TK_IMAG_NUM)
            }

            type_comment {
                if (last_token == yytokentype::TK_COLON && !parenlevel) {
                    indent = true;
                }
                token(yylval.string);
                RET(TK_TYPE_COMMENT);
            }

            comment {
                if(last_token == -1) { RET(TK_COMMENT); }
                if(parenlevel) { continue; }
                line_num++; cur_line=cur;
                token(yylval.string);
                // This is commented out because
                // the last character in the comment was skipped.
                // yylval.string.n--;
                token_loc(loc);
                if (last_token == yytokentype::TK_NEWLINE) {
                    return yytokentype::TK_COMMENT;
                } else {
                    if (last_token == yytokentype::TK_COLON) {
                        indent = true;
                    }
                    last_token=yytokentype::TK_NEWLINE;
                    return yytokentype::TK_EOLCOMMENT;
                }
            }
            //docstring { RET(TK_DOCSTRING) }

            string1 { token_str(yylval.string); RET(TK_STRING) }
            string2 { token_str(yylval.string); RET(TK_STRING) }
            string3 { token_str3(yylval.string); RET(TK_STRING) }
            string4 { token_str3(yylval.string); RET(TK_STRING) }

            name { token(yylval.string); RET(TK_NAME) }
        */
    }
}

#define T(tk, name) case (yytokentype::tk) : return name;

std::string token2text(const int token)
{
    if (0 < token && token < 256) {
        char t = token;
        return std::string(&t, 1);
    }
    switch (token) {
        T(END_OF_FILE, "end of file")
        T(TK_NEWLINE, "newline")
        T(TK_INDENT, "indent")
        T(TK_DEDENT, "dedent")
        T(TK_NAME, "identifier")
        T(TK_INTEGER, "integer")
        T(TK_REAL, "real")
        T(TK_IMAG_NUM, "imag number")

        T(TK_PLUS, "+")
        T(TK_MINUS, "-")
        T(TK_STAR, "*")
        T(TK_SLASH, "/")
        T(TK_COLON, ":")
        T(TK_SEMICOLON, ";")
        T(TK_COMMA, ",")
        T(TK_EQUAL, "=")
        T(TK_LPAREN, "(")
        T(TK_RPAREN, ")")
        T(TK_LBRACKET, "[")
        T(TK_RBRACKET, "]")
        T(TK_LBRACE, "{")
        T(TK_RBRACE, "}")
        T(TK_PERCENT, "%")
        T(TK_VBAR, "|")
        T(TK_AMPERSAND, "&")
        T(TK_DOT, ".")
        T(TK_TILDE, "~")
        T(TK_CARET, "^")
        T(TK_AT, "@")

        T(TK_STRING, "string")
        T(TK_COMMENT, "comment")
        T(TK_EOLCOMMENT, "eolcomment")
        T(TK_TYPE_COMMENT, "type_comment")

        T(TK_POW, "**")
        T(TK_FLOOR_DIV, "//")
        T(TK_RIGHTSHIFT, ">>")
        T(TK_LEFTSHIFT, "<<")
        T(TK_PLUS_EQUAL, "+=")
        T(TK_MIN_EQUAL, "-=")
        T(TK_STAR_EQUAL, "*=")
        T(TK_SLASH_EQUAL, "/=")
        T(TK_PERCENT_EQUAL, "%=")
        T(TK_AMPER_EQUAL, "&=")
        T(TK_VBAR_EQUAL, "|=")
        T(TK_CARET_EQUAL, "^=")
        T(TK_ATEQUAL, "@=")
        T(TK_RARROW, "->")
        T(TK_COLONEQUAL, ":=")
        T(TK_ELLIPSIS, "...")
        T(TK_LEFTSHIFT_EQUAL, "<<=")
        T(TK_RIGHTSHIFT_EQUAL, ">>=")
        T(TK_POW_EQUAL, "**=")
        T(TK_DOUBLESLASH_EQUAL, "//=")

        T(TK_EQ, "==")
        T(TK_NE, "!=")
        T(TK_LT, "<")
        T(TK_LE, "<=")
        T(TK_GT, ">")
        T(TK_GE, ">=")

        T(TK_NOT, "not")
        T(TK_AND, "and")
        T(TK_OR, "or")
        T(TK_IS_NOT, "is not")
        T(TK_NOT_IN, "not in")

        T(TK_TRUE, "True")
        T(TK_FALSE, "False")

        T(KW_AS, "as")
        T(KW_ASSERT, "assert")
        T(KW_ASYNC, "async")
        T(KW_AWAIT, "await")
        T(KW_BREAK, "break")
        T(KW_CLASS, "class")
        T(KW_CONTINUE, "continue")
        T(KW_DEF, "def")
        T(KW_DEL, "del")
        T(KW_ELIF, "elif")
        T(KW_ELSE, "else")
        T(KW_EXCEPT, "except")
        T(KW_FINALLY, "finally")
        T(KW_FOR, "for")
        T(KW_FROM, "from")
        T(KW_GLOBAL, "global")
        T(KW_IF, "if")
        T(KW_IMPORT, "import")
        T(KW_IN, "in")
        T(KW_IS, "is")
        T(KW_LAMBDA, "lambda")
        T(KW_NONE, "none")
        T(KW_NONLOCAL, "nonlocal")
        T(KW_PASS, "pass")
        T(KW_RAISE, "raise")
        T(KW_RETURN, "return")
        T(KW_TRY, "try")
        T(KW_WHILE, "while")
        T(KW_WITH, "with")
        T(KW_YIELD, "yield")
        T(KW_YIELD_FROM, "yield from")

        default : {
            std::cout << "TOKEN: " << token << std::endl;
            throw LCompilersException("Token conversion not implemented yet.");
        }
    }
}

Result<std::vector<int>> tokens(Allocator &al, const std::string &input,
        diag::Diagnostics &diagnostics,
        std::vector<YYSTYPE> *stypes,
        std::vector<Location> *locations)
{
    Tokenizer t;
    t.set_string(input);
    std::vector<int> tst;
    int token = yytokentype::END_OF_FILE + 1; // Something different from EOF
    while (token != yytokentype::END_OF_FILE) {
        YYSTYPE y;
        Location l;
        try {
            token = t.lex(al, y, l, diagnostics);
        } catch (const parser_local::TokenizerError &e) {
            diagnostics.diagnostics.push_back(e.d);
            return Error();
        }
        tst.push_back(token);
        if (stypes) stypes->push_back(y);
        if (locations) locations->push_back(l);
    }
    return tst;
}

std::string pickle_token(int token, const LFortran::YYSTYPE &yystype)
{
    std::string t;
    t += "(";
    if (token >= yytokentype::TK_INDENT && token <= TK_FALSE) {
        t += "TOKEN";
    } else if (token == yytokentype::TK_NEWLINE) {
        t += "NEWLINE";
        t += ")";
        return t;
    } else if (token == yytokentype::END_OF_FILE) {
        t += "EOF";
        t += ")";
        return t;
    } else {
        t += "KEYWORD";
    }
    t += " \"";
    t += token2text(token);
    t += "\"";
    if (token == yytokentype::TK_NAME) {
        t += " " + yystype.string.str();
    } else if (token == yytokentype::TK_INTEGER) {
        t += " " + BigInt::int_to_str(yystype.n);
    } else if (token == yytokentype::TK_REAL) {
        t += " " + std::to_string(yystype.f);
    } else if (token == yytokentype::TK_IMAG_NUM) {
        t += " " + std::to_string(yystype.f) + "j";
    } else if (token == yytokentype::TK_STRING) {
        t = t + " " + "\"" + yystype.string.str() + "\"";
    } else if (token == yytokentype::TK_TYPE_COMMENT) {
        t = t + " " + "\"" + yystype.string.str() + "\"";
    }
    t += ")";
    return t;
}


} // namespace LFortran

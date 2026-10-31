// modules.md "Module linkage, visibility and reachability".
export module token;

struct Token {
    int id;
    int twice() const { return id * 2; }
};

export Token make_token(int id) { return Token{id}; }

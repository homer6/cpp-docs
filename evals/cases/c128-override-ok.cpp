struct Base {
    virtual ~Base() = default;
    virtual int id() const { return 0; }
};
struct Derived final : Base {
    int id() const override { return 1; }
};

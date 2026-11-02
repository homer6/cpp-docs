// Checks claims made by docs/guidelines/core-guidelines/examples/class.md.
#include <algorithm>
#include <concepts>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <limits>
#include <memory>
#include <ranges>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>
#include <version>
#ifdef __cpp_lib_bit_cast
#include <bit>
#endif

#include <gtest/gtest.h>

#include "support/feature.hpp"

namespace {

// Assign through a function, so the self-assignment tests don't trip
// -Wself-assign / -Wself-move.
template <class T>
void copy_assign(T& to, const T& from) { to = from; }
template <class T>
void move_assign(T& to, T& from) { to = std::move(from); }

// Counts constructions, copies, moves and assignments of a member.
struct Tally {
    int constructed = 0, copied = 0, moved = 0, assigned = 0;
};
struct Counted {
    Tally* t;
    explicit Counted(Tally& tally) : t{&tally} { ++t->constructed; }
    Counted(const Counted& o) : t{o.t} { ++t->copied; }
    Counted(Counted&& o) noexcept : t{o.t} { ++t->moved; }
    Counted& operator=(const Counted& o) { t = o.t; ++t->assigned; return *this; }
    Counted& operator=(Counted&& o) noexcept { t = o.t; ++t->assigned; return *this; }
    ~Counted() = default;
};

// ---- C.2 ----
class Date_range {
public:
    Date_range(int first, int last) : first_{first}, last_{last} {
        if (first > last) throw std::invalid_argument{"first > last"};
    }
    int first() const { return first_; }
    int last() const { return last_; }
private:
    int first_;
    int last_;
};

// ---- C.2 (a struct) ----
struct Rgb { unsigned char r, g, b; };

// ---- C.5 ----
namespace shop { struct Price { long cents; }; long dollars(Price p) { return p.cents / 100; } }
namespace app5 { long total(shop::Price p) { return dollars(p); } }

// ---- C.9 ----
class Buffer {
public:
    void append(char c) { data_.push_back(c); }
    std::size_t size() const { return data_.size(); }
private:
    std::vector<char> data_;
};
template <class T>
concept exposes_data = requires(T& t) { t.data_; };

// ---- C.10, C.132 ----
struct Square { double side; double area() const { return side * side; } };
struct Shape_base { virtual ~Shape_base() = default; virtual double area() const = 0; };
struct Poly_square : Shape_base {
    double side = 0;
    double area() const override { return side * side; }
};

// ---- C.11 ----
struct Point {
    int x = 0;
    int y = 0;
    friend bool operator==(const Point&, const Point&) = default;
};

// ---- C.12 ----
struct Const_member {
    Const_member(std::string i, int l) : id{std::move(i)}, level{l} {}
    const std::string id;
    int level;
};
struct Ref_member {
    explicit Ref_member(int& t) : target{t} {}
    int& target;
};
class Employee {
public:
    Employee(std::string id, int level) : id_{std::move(id)}, level_{level} {}
    const std::string& id() const { return id_; }
    int level() const { return level_; }
private:
    std::string id_;
    int level_;
};

// ---- C.13 ----
class Samples13 {
    std::vector<int> data_;
    std::size_t count_;
public:
    explicit Samples13(std::vector<int> v) : data_{std::move(v)}, count_{data_.size()} {}
    std::size_t count() const { return count_; }
};

// ---- C.20 ----
struct Phone_book { std::vector<std::string> names; std::string owner; };

// ---- C.21 ----
struct Move_only_declared {
    Move_only_declared() = default;
    Move_only_declared(Move_only_declared&&) noexcept = default;
    std::vector<int> v;
};
struct Dtor_declared {
    explicit Dtor_declared(Tally& t) : c{t} {}
    ~Dtor_declared() {}   // a user destructor: no implicit moves
    Counted c;
};

// ---- C.22 ----
class Text {
public:
    explicit Text(const char* s) : p_{std::make_unique<std::string>(s)} {}
    Text(const Text& o) : p_{std::make_unique<std::string>(*o.p_)} {}
    Text& operator=(const Text& o) {
        Text tmp{o};
        p_.swap(tmp.p_);
        return *this;
    }
    Text(Text&&) noexcept = default;
    Text& operator=(Text&&) noexcept = default;
    ~Text() = default;
    std::string& str() { return *p_; }
    const std::string& str() const { return *p_; }
private:
    std::unique_ptr<std::string> p_;
};

// ---- C.30 ----
class On_exit {
public:
    explicit On_exit(std::function<void()> f) : f_{std::move(f)} {}
    On_exit(const On_exit&) = delete;
    On_exit& operator=(const On_exit&) = delete;
    On_exit(On_exit&&) = delete;
    On_exit& operator=(On_exit&&) = delete;
    ~On_exit() { f_(); }
private:
    std::function<void()> f_;
};
struct Name30 { std::string value; };
struct Name30_empty_dtor { std::string value; ~Name30_empty_dtor() {} };

// ---- C.31, C.33 ----
struct Release_counter {
    int* releases;
    void operator()(int* p) const noexcept { ++*releases; delete p; }
};
class Holder {
public:
    Holder(int& releases, bool fail) : r_{new int{0}, Release_counter{&releases}} {
        if (fail) throw std::runtime_error{"after acquiring"};
    }
private:
    std::unique_ptr<int, Release_counter> r_;
};
struct Tracked_pixels {
    int* destroyed;
    explicit Tracked_pixels(int& d) : destroyed{&d} {}
    Tracked_pixels(const Tracked_pixels&) = delete;
    Tracked_pixels& operator=(const Tracked_pixels&) = delete;
    ~Tracked_pixels() { ++*destroyed; }
};
class Image {
public:
    explicit Image(int& destroyed) : pixels_{new Tracked_pixels{destroyed}} {}
    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;
    Image(Image&&) = delete;
    Image& operator=(Image&&) = delete;
    ~Image() { delete pixels_; }
private:
    Tracked_pixels* pixels_;
};

// ---- C.35, C.121, C.127 ----
struct Sensor {
    virtual double read() const = 0;
    virtual ~Sensor() = default;
};
struct Thermometer : Sensor {
    int* destroyed;
    explicit Thermometer(int& d) : destroyed{&d} {}
    Thermometer(const Thermometer&) = delete;
    Thermometer& operator=(const Thermometer&) = delete;
    ~Thermometer() override { ++*destroyed; }
    double read() const override { return 21.5; }
};
class Counted_base {
public:
    int count() const { return n_; }
protected:
    Counted_base() = default;
    ~Counted_base() = default;
    void bump() { ++n_; }
private:
    int n_ = 0;
};
struct Clicks : Counted_base { void click() { bump(); } };

// ---- C.36, C.37 ----
struct Throwing_body {
    ~Throwing_body() { if (fail) report(); }   // no exception spec: implicitly noexcept
    bool fail = false;
    static void report() {}
};
struct Legacy { ~Legacy() noexcept(false) {} };
struct Session_implicit { Legacy legacy; };
class Session_explicit {
public:
    ~Session_explicit() noexcept {}
private:
    Legacy legacy_;
};

// ---- C.40 ----
class Fraction {
public:
    Fraction(int num, int den) : num_{num}, den_{den} {
        if (den == 0) throw std::invalid_argument{"zero denominator"};
    }
    double value() const { return static_cast<double>(num_) / den_; }
private:
    int num_;
    int den_;
};

// ---- C.41 ----
class Histogram {
public:
    explicit Histogram(int bins) : counts_(static_cast<std::size_t>(bins), 0) {}
    void add(int bin) { ++counts_.at(static_cast<std::size_t>(bin)); }
    int count(int bin) const { return counts_.at(static_cast<std::size_t>(bin)); }
private:
    std::vector<int> counts_;
};

// ---- C.42 ----
class Port {
public:
    explicit Port(int n) : n_{n} {
        if (n <= 0 || n >= 65536) throw std::out_of_range{"port"};
    }
    int number() const { return n_; }
private:
    int n_;
};

// ---- C.43 ----
struct Date {
    Date() = default;
    Date(int y, int m, int d) : y_{y}, m_{m}, d_{d} {}
    int y_ = 1970, m_ = 1, d_ = 1;
};
struct Date_no_default {
    Date_no_default(int y, int m, int d) : y_{y}, m_{m}, d_{d} {}
    int y_, m_, d_;
};

// ---- C.44 ----
class Int_buffer {
public:
    Int_buffer() noexcept = default;
    std::size_t size() const { return size_; }
    const int* data() const { return data_.get(); }
private:
    std::unique_ptr<int[]> data_;
    std::size_t size_ = 0;
};
class Allocating_buffer {
public:
    Allocating_buffer() : data_{std::make_unique<int[]>(64)} {}
    const int* data() const { return data_.get(); }
private:
    std::unique_ptr<int[]> data_;
};

// ---- C.45 ----
class Retry_policy {
public:
    int attempts() const { return attempts_; }
    int delay_ms() const { return delay_ms_; }
private:
    int attempts_ = 3;
    int delay_ms_ = 100;
};

// ---- C.46 ----
class Buffer_size {
public:
    explicit Buffer_size(std::size_t bytes) : bytes_{bytes} {}
    std::size_t bytes() const { return bytes_; }
private:
    std::size_t bytes_;
};
struct Implicit_size { Implicit_size(std::size_t b) : bytes{b} {} std::size_t bytes; };

// ---- C.47 ----
struct Logs_order {
    std::string* log;
    Logs_order(std::string& l, char c) : log{&l} { l += c; }
};
struct Two_members {
    explicit Two_members(std::string& log) : log_{&log}, b{log, 'b'} {}   // a isn't listed; it's built before b anyway
    std::string* log_;
    Logs_order a{*log_, 'a'};
    Logs_order b;
};

// ---- C.48 ----
class Connection_opts {
public:
    Connection_opts() = default;
    explicit Connection_opts(bool verbose) : verbose_{verbose} {}
    int timeout_ms() const { return timeout_ms_; }
    bool verbose() const { return verbose_; }
private:
    int timeout_ms_ = 500;
    bool verbose_ = false;
};

// ---- C.49 ----
struct Defaultable {
    Tally* t = nullptr;
    Defaultable() = default;
    explicit Defaultable(Tally& tally) : t{&tally} { ++t->constructed; }
    Defaultable& operator=(const Defaultable& o) { t = o.t; ++t->assigned; return *this; }
    Defaultable(const Defaultable&) = default;
};
struct Initializes {
    explicit Initializes(const Defaultable& d) : m{d} {}
    Defaultable m;
};
struct Assigns {
    explicit Assigns(const Defaultable& d) { m = d; }
    Defaultable m;
};

// ---- C.50 ----
class Plugin {
public:
    template <class T>
    static std::unique_ptr<T> create() {
        auto p = std::make_unique<T>(Token{});
        p->post_init();
        return p;
    }
    virtual ~Plugin() = default;
    virtual std::string describe() const = 0;
    const std::string& name() const { return name_; }
protected:
    struct Token {};
    explicit Plugin(Token) {}
private:
    void post_init() { name_ = describe(); }
    std::string name_;
};
class Spell_check : public Plugin {
public:
    explicit Spell_check(Token t) : Plugin{t} {}
    std::string describe() const override { return "spell-check"; }
};

// ---- C.51 ----
class Color {
public:
    Color(int r, int g, int b) : r_{r}, g_{g}, b_{b} {
        if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255) throw std::out_of_range{"rgb"};
    }
    explicit Color(int grey) : Color{grey, grey, grey} {}
    int red() const { return r_; }
    int sum() const { return r_ + g_ + b_; }
private:
    int r_, g_, b_;
};

// ---- C.52 ----
class Checked_vector : public std::vector<int> {
public:
    using std::vector<int>::vector;
    int& checked(std::size_t i) { return at(i); }
};

// ---- C.60, C.63 ----
class Matrix {
public:
    Matrix() = default;
    explicit Matrix(std::vector<double> c) : cells_{std::move(c)} {}
    Matrix(const Matrix&) = default;
    Matrix& operator=(const Matrix& other) { cells_ = other.cells_; return *this; }
    Matrix(Matrix&&) noexcept = default;
    Matrix& operator=(Matrix&& other) noexcept { cells_ = std::move(other.cells_); return *this; }
    ~Matrix() = default;
    const std::vector<double>& cells() const { return cells_; }
private:
    std::vector<double> cells_;
};

// ---- C.61, C.101 ----
class Polygon {
public:
    void add(int p) { pts_.push_back(p); }
    friend bool operator==(const Polygon&, const Polygon&) = default;
private:
    std::vector<int> pts_;
};

// ---- C.64 ----
class Name_list {
public:
    Name_list() = default;
    explicit Name_list(std::size_t n) : data_{new std::string[n]}, size_{n} {}
    Name_list(Name_list&& o) noexcept
        : data_{std::exchange(o.data_, nullptr)}, size_{std::exchange(o.size_, 0)} {}
    Name_list& operator=(Name_list&& o) noexcept {
        if (this != &o) {
            delete[] data_;
            data_ = std::exchange(o.data_, nullptr);
            size_ = std::exchange(o.size_, 0);
        }
        return *this;
    }
    Name_list(const Name_list&) = delete;
    Name_list& operator=(const Name_list&) = delete;
    ~Name_list() { delete[] data_; }
    std::size_t size() const { return size_; }
private:
    std::string* data_ = nullptr;
    std::size_t size_ = 0;
};

// ---- C.65 ----
class Owner {
public:
    Owner() = default;
    explicit Owner(int v) : p_{new int{v}} {}
    Owner(const Owner&) = delete;
    Owner& operator=(const Owner&) = delete;
    Owner(Owner&&) = delete;
    Owner& operator=(Owner&& o) noexcept {
        if (this != &o) {
            delete p_;
            p_ = o.p_;
            o.p_ = nullptr;
        }
        return *this;
    }
    ~Owner() { delete p_; }
    int value() const { return p_ ? *p_ : -1; }
private:
    int* p_ = nullptr;
};

// ---- C.66 ----
struct Noexcept_move {
    Tally* t;
    explicit Noexcept_move(Tally& tally) : t{&tally} {}
    Noexcept_move(const Noexcept_move& o) : t{o.t} { ++t->copied; }
    Noexcept_move(Noexcept_move&& o) noexcept : t{o.t} { ++t->moved; }
    Noexcept_move& operator=(const Noexcept_move&) = default;
    Noexcept_move& operator=(Noexcept_move&&) = default;
    ~Noexcept_move() = default;
};
struct Throwing_move {
    Tally* t;
    explicit Throwing_move(Tally& tally) : t{&tally} {}
    Throwing_move(const Throwing_move& o) : t{o.t} { ++t->copied; }
    Throwing_move(Throwing_move&& o) : t{o.t} { ++t->moved; }   // may throw, as far as vector knows
    Throwing_move& operator=(const Throwing_move&) = default;
    Throwing_move& operator=(Throwing_move&&) = default;
    ~Throwing_move() = default;
};

// ---- C.67, C.130, C.145 ----
class Shape {
public:
    Shape() = default;
    Shape(const Shape&) = delete;
    Shape& operator=(const Shape&) = delete;
    Shape(Shape&&) = delete;
    Shape& operator=(Shape&&) = delete;
    virtual ~Shape() = default;
    virtual std::unique_ptr<Shape> clone() const = 0;
    virtual double area() const = 0;
};
class Square_shape final : public Shape {
public:
    explicit Square_shape(double side) : side_{side} {}
    std::unique_ptr<Shape> clone() const override { return std::make_unique<Square_shape>(side_); }
    double area() const override { return side_ * side_; }
private:
    double side_;
};
struct Copyable_base { virtual ~Copyable_base() = default; virtual double area() const { return 0; } };
struct Copyable_square : Copyable_base {
    double side = 2;
    double area() const override { return side * side; }
};
double area_by_value(Copyable_base s) { return s.area(); }
double area_by_ref(const Copyable_base& s) { return s.area(); }

// ---- C.80 ----
struct Pixel_defaulted {
    Pixel_defaulted() = default;
    Pixel_defaulted(const Pixel_defaulted&) = default;
    Pixel_defaulted& operator=(const Pixel_defaulted&) = default;
    unsigned char r = 0, g = 0, b = 0;
};
struct Pixel_handwritten {
    Pixel_handwritten() {}
    Pixel_handwritten(const Pixel_handwritten& o) : r{o.r}, g{o.g}, b{o.b} {}
    Pixel_handwritten& operator=(const Pixel_handwritten& o) { r = o.r; g = o.g; b = o.b; return *this; }
    unsigned char r = 0, g = 0, b = 0;
};

// ---- C.81 ----
struct Move_only {
    Move_only() = default;
    Move_only(const Move_only&) = delete;
    Move_only& operator=(const Move_only&) = delete;
    Move_only(Move_only&&) = default;
    Move_only& operator=(Move_only&&) = default;
    ~Move_only() = default;
};

// ---- C.82 ----
struct Logger82 {
    explicit Logger82(std::string& log) { log += who(); }
    virtual ~Logger82() = default;
    virtual const char* who() const { return "base;"; }
};
struct File_logger82 : Logger82 {
    explicit File_logger82(std::string& log) : Logger82{log} { log += who(); }
    const char* who() const override { return "file;"; }
};

// ---- C.83, C.84, C.85 ----
namespace grid {
class Grid {
public:
    Grid() = default;
    explicit Grid(std::vector<int> c) : cells_{std::move(c)} {}
    void swap(Grid& o) noexcept { cells_.swap(o.cells_); ++swaps; }
    friend void swap(Grid& a, Grid& b) noexcept { a.swap(b); }
    const std::vector<int>& cells() const { return cells_; }
    static inline int swaps = 0;
private:
    std::vector<int> cells_;
};
}  // namespace grid
struct Tag_noexcept {
    std::string text;
    friend void swap(Tag_noexcept& a, Tag_noexcept& b) noexcept { a.text.swap(b.text); }
};
struct Tag_plain {
    std::string text;
    friend void swap(Tag_plain& a, Tag_plain& b) { a.text.swap(b.text); }
};

// ---- C.86 ----
struct Name86 {
    std::string value;
    friend bool operator==(const Name86& a, const Name86& b) noexcept { return a.value == b.value; }
};

// ---- C.87 ----
struct Animal {
    explicit Animal(std::string n) : name{std::move(n)} {}
    std::string name;
    virtual ~Animal() = default;
    friend bool operator==(const Animal& a, const Animal& b) { return a.name == b.name; }
};
struct Dog : Animal {
    std::string breed;
    Dog(std::string n, std::string b) : Animal{std::move(n)}, breed{std::move(b)} {}
};

// ---- C.89 ----
struct User_id { std::string value; };
struct User_id_hash {
    std::size_t operator()(const User_id& u) const noexcept { return std::hash<std::string>{}(u.value); }
};
struct User_id_eq {
    bool operator()(const User_id& a, const User_id& b) const noexcept { return a.value == b.value; }
};

// ---- C.90 ----
struct Record { std::string name; int score = 0; };

// ---- C.100, C.103, C.104 ----
class Bag {
public:
    using value_type = int;
    using iterator = std::vector<int>::iterator;
    using const_iterator = std::vector<int>::const_iterator;
    Bag() noexcept = default;
    Bag(std::initializer_list<int> il) : data_{il} {}
    iterator begin() { return data_.begin(); }
    iterator end() { return data_.end(); }
    const_iterator begin() const { return data_.begin(); }
    const_iterator end() const { return data_.end(); }
    std::size_t size() const { return data_.size(); }
    bool empty() const { return data_.empty(); }
    void push_back(int x) { data_.push_back(x); }
    const int* data() const { return data_.data(); }
    friend bool operator==(const Bag&, const Bag&) = default;
private:
    std::vector<int> data_;
};

// ---- C.109 ----
class Doc_handle {
public:
    explicit Doc_handle(std::shared_ptr<std::string> p) : p_{std::move(p)} {}
    std::string& operator*() const { return *p_; }
    std::string* operator->() const { return p_.get(); }
private:
    std::shared_ptr<std::string> p_;
};

// ---- C.121, C.122, C.127 ----
class Printer {
public:
    virtual void print(const std::string& s) = 0;
    virtual ~Printer() = default;
};
class Laser : public Printer {
public:
    explicit Laser(int& destroyed) : destroyed_{&destroyed} {}
    Laser(const Laser&) = delete;
    Laser& operator=(const Laser&) = delete;
    ~Laser() override { ++*destroyed_; }
    void print(const std::string& s) override { queue_ += s; }
private:
    int* destroyed_;
    std::string queue_;
};
class Storage {
public:
    virtual void write(const std::string& data) = 0;
    virtual std::string contents() const = 0;
    virtual ~Storage() = default;
};
class Memory_storage : public Storage {
public:
    void write(const std::string& d) override { data_ += d; }
    std::string contents() const override { return data_; }
private:
    std::string data_;
};
class Upper_storage : public Storage {
public:
    void write(const std::string& d) override {
        for (char c : d) data_ += (c >= 'a' && c <= 'z') ? static_cast<char>(c - 'a' + 'A') : c;
    }
    std::string contents() const override { return data_; }
private:
    std::string data_;
};
void save(Storage& dev, const std::string& data) { dev.write(data); }

// ---- C.128 ----
struct Base128 {
    virtual ~Base128() = default;
    virtual const char* scale(double) { return "base"; }
};
struct Drifted : Base128 {
    using Base128::scale;                                     // keeps -Woverloaded-virtual quiet; the point stands:
    virtual const char* scale(float) { return "drifted"; }   // a new function, not an override
};
struct Overrides : Base128 {
    const char* scale(double) override { return "override"; }
};

// ---- C.132 ----
class Vec2 {
public:
    double x() const { return x_; }
    double y() const { return y_; }
private:
    double x_ = 0, y_ = 0;
};
class Virtual_vec2 {
public:
    virtual ~Virtual_vec2() = default;
    virtual double x() const { return x_; }
    virtual double y() const { return y_; }
private:
    double x_ = 0, y_ = 0;
};

// ---- C.135 ----
class Reader { public: virtual ~Reader() = default; virtual std::string read() = 0; };
class Writer { public: virtual ~Writer() = default; virtual void write(const std::string&) = 0; };
class File : public Reader, public Writer {
public:
    std::string read() override { return data_; }
    void write(const std::string& s) override { data_ += s; }
private:
    std::string data_;
};
std::string slurp(Reader& r) { return r.read(); }
void put(Writer& w, const std::string& s) { w.write(s); }

// ---- C.136 ----
class Timestamped {
public:
    void touch(long now) { last_used_ = now; }
    long last_used() const { return last_used_; }
protected:
    ~Timestamped() = default;
private:
    long last_used_ = 0;
};
class Named {
public:
    const std::string& name() const { return name_; }
    void rename(std::string n) { name_ = std::move(n); }
protected:
    ~Named() = default;
private:
    std::string name_;
};
class Session136 : public Timestamped, public Named {};

// ---- C.137 ----
struct Utility { int uses = 0; };
struct Reader137 : virtual Utility { void read() { ++uses; } };
struct Writer137 : virtual Utility { void write() { ++uses; } };
struct File137 : Reader137, Writer137 {};

// ---- C.138 ----
struct Printer138 {
    const char* print(int) { return "int"; }
    const char* print(const std::string&) { return "string"; }
};
struct Hides : Printer138 {
    const char* print(int) { return "derived int"; }
};
struct Uses : Printer138 {
    using Printer138::print;
    const char* print(int) { return "derived int"; }
};
template <class T>
concept prints_string = requires(T& p) { p.print(std::string{}); };

// ---- C.139 ----
struct Shape139 { virtual ~Shape139() = default; virtual double area() const = 0; };
struct Circle139 : Shape139 { double r = 1; double area() const override { return 3.0 * r * r; } };
struct Labelled_circle : Circle139 { const char* label = "x"; };
struct Capped final : Shape139 { double area() const override { return 0; } };

// ---- C.140 ----
struct Logger140 {
    virtual ~Logger140() = default;
    virtual int log(int level = 1) { return level; }
};
struct Debug_logger140 : Logger140 {
    int log(int level = 3) override { return level * 10; }
};
struct Nvi_logger {
    virtual ~Nvi_logger() = default;
    int log() { return do_log(1); }
    int log(int level) { return do_log(level); }
private:
    virtual int do_log(int level) { return level; }
};
struct Nvi_debug : Nvi_logger {
private:
    int do_log(int level) override { return level * 10; }
};

// ---- C.146 - C.148, C.153 ----
struct Shape146 { virtual ~Shape146() = default; virtual double area() const = 0; };
struct Circle146 : Shape146 { double r = 1; double area() const override { return 3.0 * r * r; } };
struct Square146 : Shape146 { double side = 2; double area() const override { return side * side; } };
struct Rounded_square146 : Square146 { double area() const override { return side * side - 1; } };
double radius_or_zero(Shape146& s) {
    if (auto* c = dynamic_cast<Circle146*>(&s)) return c->r;
    return 0;
}

// ---- C.149 ----
struct Tracked {
    int* destroyed;
    explicit Tracked(int& d) : destroyed{&d} {}
    Tracked(const Tracked&) = delete;
    Tracked& operator=(const Tracked&) = delete;
    ~Tracked() { ++*destroyed; }
};
void throws_after(Tracked&) { throw std::runtime_error{"boom"}; }

// ---- C.152 ----
struct Base152 { int id = 0; };
struct Derived152 : Base152 { int extra = 0; };
int sum_ids(std::span<const Derived152> items) {
    int s = 0;
    for (const auto& d : items) s += d.id;
    return s;
}

// ---- C.160 ----
struct Money {
    long cents = 0;
    friend Money operator+(Money a, Money b) { return Money{a.cents + b.cents}; }
    friend bool operator==(const Money&, const Money&) = default;
};

// ---- C.161 ----
struct Meters {
    Meters(double v) : value{v} {}
    friend Meters operator+(const Meters& a, const Meters& b) { return a.value + b.value; }
    double value;
};
struct Member_meters {
    Member_meters(double v) : value{v} {}
    Member_meters operator+(const Member_meters& o) const { return value + o.value; }
    double value;
};
template <class L, class R>
concept addable = requires(L l, R r) { l + r; };

// ---- C.162 ----
std::string to_text(int v) { return "i" + std::to_string(v); }
std::string to_text(const std::string& v) { return "s" + v; }
template <class T>
std::string label(const T& v) { return "[" + to_text(v) + "]"; }

// ---- C.164 ----
class Path {
public:
    explicit Path(std::string s) : s_{std::move(s)} {}
    const char* c_str() const { return s_.c_str(); }
    explicit operator bool() const { return !s_.empty(); }
private:
    std::string s_;
};

// ---- C.165 ----
namespace custom {
struct Big { int value = 0; static inline int custom_swaps = 0; };
void swap(Big& a, Big& b) noexcept { std::swap(a.value, b.value); ++Big::custom_swaps; }
}  // namespace custom
template <class T>
void exchange_values(T& a, T& b) {
    using std::swap;
    swap(a, b);
}
template <class T>
void exchange_qualified(T& a, T& b) { std::swap(a, b); }

// ---- C.166 ----
struct Weird_address {
    int value = 0;
    int operator&() const { return value; }
};

// ---- C.167 ----
struct Point167 { int x, y; };
std::ostream& operator<<(std::ostream& os, const Point167& p) { return os << p.x << ',' << p.y; }

// ---- C.168 ----
namespace geo {
struct Point { int x, y; };
bool operator==(Point a, Point b) { return a.x == b.x && a.y == b.y; }
}  // namespace geo
namespace app {
bool same(geo::Point a, geo::Point b) { return a == b; }
}  // namespace app

// ---- C.180 ----
struct Token_wasteful { int kind; double number; char symbol; };
union Token_storage { double number; char symbol; };

// ---- C.182 ----
class Value {
public:
    explicit Value(int i) : kind_{Kind::integer}, i_{i} {}
    explicit Value(double d) : kind_{Kind::real}, d_{d} {}
    int as_int() const {
        if (kind_ != Kind::integer) throw std::logic_error{"not an int"};
        return i_;
    }
    double as_real() const {
        if (kind_ != Kind::real) throw std::logic_error{"not a real"};
        return d_;
    }
private:
    enum class Kind { integer, real } kind_;
    union { int i_; double d_; };
};

}  // namespace

// C.2: a class with an invariant rejects values that break it; a struct takes any combination.
TEST(CoreGuidelinesC, C2_ClassKeepsItsInvariant) {
    EXPECT_THROW(Date_range(5, 1), std::invalid_argument);
    const Date_range r{1, 5};
    EXPECT_LE(r.first(), r.last());
    static_assert(std::is_aggregate_v<Rgb> && !std::is_aggregate_v<Date_range>);
}

// C.5: a helper in the class's namespace is found by argument-dependent lookup.
TEST(CoreGuidelinesC, C5_HelperFoundByAdl) {
    EXPECT_EQ(app5::total(shop::Price{1250}), 12);
}

// C.9: private data can't be reached from outside; only the member functions change it.
TEST(CoreGuidelinesC, C9_PrivateDataIsNotExposed) {
    static_assert(!exposes_data<Buffer>);
    Buffer b;
    b.append('x');
    EXPECT_EQ(b.size(), 1u);
}

// C.10: a concrete type has no vtable and no indirection; the hierarchy version carries a vtable pointer.
TEST(CoreGuidelinesC, C10_ConcreteTypeIsSmallerAndNotPolymorphic) {
    static_assert(!std::is_polymorphic_v<Square> && std::is_polymorphic_v<Poly_square>);
    static_assert(sizeof(Square) < sizeof(Poly_square));
    static_assert(std::is_trivially_copyable_v<Square>);
    EXPECT_DOUBLE_EQ(Square{3}.area(), 9.0);
}

// C.11: a regular type can be default-constructed, copied and compared, and a copy compares equal.
TEST(CoreGuidelinesC, C11_RegularTypeCopiesCompareEqual) {
    static_assert(std::regular<Point>);
    const Point a{1, 2};
    Point b;
    b = a;
    EXPECT_EQ(a, b);
}

// C.12: a const or reference member deletes copy assignment, though copy construction still works.
TEST(CoreGuidelinesC, C12_ConstOrRefMemberDeletesAssignment) {
    static_assert(std::is_copy_constructible_v<Const_member> && !std::is_copy_assignable_v<Const_member>);
    static_assert(std::is_copy_constructible_v<Ref_member> && !std::is_copy_assignable_v<Ref_member>);
    static_assert(std::is_copy_assignable_v<Employee> && std::is_move_assignable_v<Employee>);
    Employee e{"a1", 1};
    e = Employee{"b2", 2};
    EXPECT_EQ(e.id(), "b2");
}

// C.13: members are initialized in declaration order, so a member may use one declared before it.
TEST(CoreGuidelinesC, C13_EarlierMemberIsReadyForLaterOne) {
    const Samples13 s{std::vector<int>{1, 2, 3}};
    EXPECT_EQ(s.count(), 3u);
}

// C.20: with no special members written, a class of self-managing members copies deeply and moves without throwing.
TEST(CoreGuidelinesC, C20_RuleOfZero) {
    static_assert(std::is_nothrow_move_constructible_v<Phone_book>);
    static_assert(std::is_copy_constructible_v<Phone_book>);
    Phone_book a{{"Ada"}, "me"};
    Phone_book b = a;
    b.names.push_back("Bob");
    EXPECT_EQ(a.names.size(), 1u);
    EXPECT_EQ(b.names.size(), 2u);
}

// C.21: declaring a move constructor deletes the copies; declaring a destructor turns moves into copies.
TEST(CoreGuidelinesC, C21_RuleOfFive) {
    static_assert(!std::is_copy_constructible_v<Move_only_declared>);
    Tally t;
    Dtor_declared a{t};
    Dtor_declared b = std::move(a);   // no move constructor was declared, so this copies
    (void)b;
    EXPECT_EQ(t.copied, 1);
    EXPECT_EQ(t.moved, 0);
}

// C.22: copy construction and copy assignment both make independent deep copies.
TEST(CoreGuidelinesC, C22_CopyOperationsAgree) {
    Text a{"one"};
    Text b{a};
    Text c{"x"};
    c = a;
    a.str() = "changed";
    EXPECT_EQ(b.str(), "one");
    EXPECT_EQ(c.str(), "one");
}

// C.30: a destructor is for an action at destruction; an empty one adds nothing and costs the implicit moves.
TEST(CoreGuidelinesC, C30_DestructorForAnAction) {
    int calls = 0;
    {
        On_exit guard{[&] { ++calls; }};
        EXPECT_EQ(calls, 0);
    }
    EXPECT_EQ(calls, 1);
    static_assert(std::is_nothrow_move_constructible_v<Name30>);
    static_assert(!std::is_nothrow_move_constructible_v<Name30_empty_dtor>);   // its "move" is a string copy
}

// C.31: a resource held by a member is released when the object dies, and when the constructor throws after acquiring it.
TEST(CoreGuidelinesC, C31_ResourcesReleasedByDestructor) {
    int releases = 0;
    { Holder h{releases, false}; }
    EXPECT_EQ(releases, 1);
    EXPECT_THROW(Holder(releases, true), std::runtime_error);
    EXPECT_EQ(releases, 2);
}

// C.33: a class with an owning pointer member deletes the owned object in its destructor.
TEST(CoreGuidelinesC, C33_OwningPointerDeletedByDestructor) {
    int destroyed = 0;
    { Image img{destroyed}; }
    EXPECT_EQ(destroyed, 1);
    static_assert(!std::is_copy_constructible_v<Image>);
}

// C.35: a public virtual destructor makes delete through the base run the derived destructor; a protected one forbids it.
TEST(CoreGuidelinesC, C35_BaseDestructorVirtualOrProtected) {
    static_assert(std::has_virtual_destructor_v<Sensor>);
    int destroyed = 0;
    { std::unique_ptr<Sensor> s = std::make_unique<Thermometer>(destroyed); }
    EXPECT_EQ(destroyed, 1);
    static_assert(!std::is_destructible_v<Counted_base> && !std::has_virtual_destructor_v<Counted_base>);
    Clicks c;
    c.click();
    EXPECT_EQ(c.count(), 1);
}

// C.36: a destructor without an exception specification is noexcept, so a throw from it would terminate.
TEST(CoreGuidelinesC, C36_DestructorsAreNoexceptByDefault) {
    static_assert(std::is_nothrow_destructible_v<Throwing_body>);
    static_assert(noexcept(std::declval<Throwing_body&>().~Throwing_body()));
    static_assert(std::is_nothrow_destructible_v<std::string> && std::is_nothrow_destructible_v<std::vector<int>>);
}

// C.37: one noexcept(false) member makes the implicit destructor noexcept(false); writing noexcept restores it.
TEST(CoreGuidelinesC, C37_ExplicitNoexceptDestructor) {
    static_assert(!std::is_nothrow_destructible_v<Legacy>);
    static_assert(!std::is_nothrow_destructible_v<Session_implicit>);
    static_assert(std::is_nothrow_destructible_v<Session_explicit>);
}

// C.40: a constructor establishes the invariant; an object breaking it is never created.
TEST(CoreGuidelinesC, C40_ConstructorEstablishesInvariant) {
    EXPECT_THROW(Fraction(1, 0), std::invalid_argument);
    EXPECT_DOUBLE_EQ(Fraction(1, 4).value(), 0.25);
    static_assert(!std::is_default_constructible_v<Fraction>);
}

// C.41: an object is usable as soon as its constructor returns, with no init() step.
TEST(CoreGuidelinesC, C41_FullyInitializedObject) {
    Histogram h{3};
    h.add(2);
    EXPECT_EQ(h.count(0), 0);
    EXPECT_EQ(h.count(2), 1);
    static_assert(!std::is_default_constructible_v<Histogram>);
}

// C.42: a constructor that can't make a valid object throws, so no invalid object exists.
TEST(CoreGuidelinesC, C42_ConstructorThrowsOnInvalidInput) {
    EXPECT_THROW(Port{0}, std::out_of_range);
    EXPECT_THROW(Port{70000}, std::out_of_range);
    EXPECT_EQ(Port{8080}.number(), 8080);
}

// C.43: a copyable type with a default constructor is semiregular and works in arrays and vector(n).
TEST(CoreGuidelinesC, C43_CopyableTypeHasDefault) {
    static_assert(std::semiregular<Date>);
    static_assert(std::copyable<Date_no_default> && !std::semiregular<Date_no_default>);
    std::vector<Date> days(3);
    EXPECT_EQ(days[2].y_, 1970);
    EXPECT_EQ(days[2].d_, 1);
}

// C.44: a simple default constructor is noexcept and allocates nothing; one that allocates can throw.
TEST(CoreGuidelinesC, C44_DefaultConstructorIsCheap) {
    static_assert(std::is_nothrow_default_constructible_v<Int_buffer>);
    static_assert(!std::is_nothrow_default_constructible_v<Allocating_buffer>);
    const Int_buffer b;
    EXPECT_EQ(b.data(), nullptr);
    EXPECT_EQ(b.size(), 0u);
}

// C.45: default member initializers give the compiler-generated default constructor the values, noexcept.
TEST(CoreGuidelinesC, C45_DefaultMemberInitializers) {
    static_assert(std::is_nothrow_default_constructible_v<Retry_policy>);
    const Retry_policy p;
    EXPECT_EQ(p.attempts(), 3);
    EXPECT_EQ(p.delay_ms(), 100);
}

// C.46: an explicit constructor isn't an implicit conversion; a non-explicit one is.
TEST(CoreGuidelinesC, C46_ExplicitConstructorBlocksConversion) {
    static_assert(!std::is_convertible_v<std::size_t, Buffer_size>);
    static_assert(std::is_constructible_v<Buffer_size, std::size_t>);
    static_assert(std::is_convertible_v<int, Implicit_size>);
    EXPECT_EQ(Buffer_size{4096}.bytes(), 4096u);
}

// C.47: members are initialized in declaration order, whatever the initializer list says.
TEST(CoreGuidelinesC, C47_DeclarationOrderInitialization) {
    std::string log;
    const Two_members m{log};
    (void)m;
    EXPECT_EQ(log, "ab");
}

// C.48: a default member initializer applies in every constructor that doesn't mention the member.
TEST(CoreGuidelinesC, C48_SharedDefaultInEveryConstructor) {
    EXPECT_EQ(Connection_opts{}.timeout_ms(), 500);
    EXPECT_EQ(Connection_opts{true}.timeout_ms(), 500);
    EXPECT_TRUE(Connection_opts{true}.verbose());
}

// C.49: initializing a member builds it once; assigning in the body default-constructs it and then assigns.
TEST(CoreGuidelinesC, C49_InitializationNotAssignment) {
    Tally t;
    const Defaultable d{t};
    const Initializes i{d};
    EXPECT_EQ(t.assigned, 0);
    const Assigns a{d};
    EXPECT_EQ(t.assigned, 1);
    (void)i;
    (void)a;
}

// C.50: a factory finishes initialization after construction, so the virtual call reaches the derived class.
TEST(CoreGuidelinesC, C50_FactoryForVirtualBehaviour) {
    const auto p = Plugin::create<Spell_check>();
    EXPECT_EQ(p->name(), "spell-check");
}

// C.51: a delegating constructor runs the target constructor's check too.
TEST(CoreGuidelinesC, C51_DelegatingConstructorSharesTheCheck) {
    EXPECT_THROW(Color{300}, std::out_of_range);
    EXPECT_EQ(Color{10}.sum(), 30);
    EXPECT_EQ(Color(1, 2, 3).red(), 1);
}

// C.52: using Base::Base imports every base constructor unchanged.
TEST(CoreGuidelinesC, C52_InheritingConstructors) {
    static_assert(std::is_constructible_v<Checked_vector, std::size_t, int>);
    static_assert(std::is_constructible_v<Checked_vector, std::initializer_list<int>>);
    Checked_vector v(3, 7);
    EXPECT_EQ(v.size(), 3u);
    EXPECT_EQ(v.checked(2), 7);
    EXPECT_THROW(v.checked(3), std::out_of_range);
    const Checked_vector w{1, 2};
    EXPECT_EQ(w.size(), 2u);
}

// C.60: copy assignment takes const& and returns a non-const reference to *this, so it chains.
TEST(CoreGuidelinesC, C60_CopyAssignmentShape) {
    Matrix a, b, c{std::vector<double>{1.0, 2.0}};
    static_assert(std::is_same_v<decltype(a = c), Matrix&>);
    a = b = c;
    EXPECT_EQ(a.cells().size(), 2u);
    EXPECT_EQ(&(a = c), &a);
    static_assert(!std::is_polymorphic_v<Matrix>);
}

// C.61: after a copy, the two compare equal and are independent.
TEST(CoreGuidelinesC, C61_CopyCopies) {
    Polygon a;
    a.add(1);
    Polygon b = a;
    EXPECT_EQ(a, b);
    b.add(2);
    EXPECT_NE(a, b);
}

// C.62: x = x leaves x unchanged.
TEST(CoreGuidelinesC, C62_SelfCopyAssignmentIsSafe) {
    Text t{"same"};
    copy_assign(t, t);
    EXPECT_EQ(t.str(), "same");
    Polygon p;
    p.add(5);
    const Polygon before = p;
    copy_assign(p, p);
    EXPECT_EQ(p, before);
}

// C.63: move assignment takes && and returns a non-const reference, and doesn't throw.
TEST(CoreGuidelinesC, C63_MoveAssignmentShape) {
    Matrix a, b{std::vector<double>{1.0}};
    static_assert(std::is_same_v<decltype(a = std::move(b)), Matrix&>);
    static_assert(std::is_nothrow_move_assignable_v<Matrix>);
    a = std::move(b);
    EXPECT_EQ(a.cells().size(), 1u);
}

// C.64: a moved-from object is valid (here empty, the default state) and can be reused.
TEST(CoreGuidelinesC, C64_MovedFromIsValid) {
    Name_list a{3};
    Name_list b = std::move(a);
    EXPECT_EQ(b.size(), 3u);
    EXPECT_EQ(a.size(), 0u);   // NOLINT(bugprone-use-after-move): the claim is about this state
    a = Name_list{2};
    EXPECT_EQ(a.size(), 2u);
}

// C.65: x = std::move(x) leaves x with its value.
TEST(CoreGuidelinesC, C65_SelfMoveAssignmentIsSafe) {
    Owner o{42};
    move_assign(o, o);
    EXPECT_EQ(o.value(), 42);
}

// C.66: vector moves noexcept-movable elements when it grows, but copies elements whose move may throw.
TEST(CoreGuidelinesC, C66_NoexceptMoveIsUsedByVector) {
    static_assert(std::is_nothrow_move_constructible_v<Noexcept_move>);
    static_assert(!std::is_nothrow_move_constructible_v<Throwing_move>);
    Tally fast, slow;
    std::vector<Noexcept_move> a;
    a.reserve(1);
    a.emplace_back(fast);
    a.emplace_back(fast);   // reallocates: the first element is moved
    EXPECT_EQ(fast.copied, 0);
    EXPECT_GE(fast.moved, 1);
    std::vector<Throwing_move> b;
    b.reserve(1);
    b.emplace_back(slow);
    b.emplace_back(slow);   // reallocates: the first element is copied
    EXPECT_GE(slow.copied, 1);
    EXPECT_EQ(slow.moved, 0);
}

// C.67: a polymorphic class with deleted copy and move can't be sliced by copying.
TEST(CoreGuidelinesC, C67_PolymorphicClassSuppressesCopy) {
    static_assert(std::is_polymorphic_v<Shape>);
    static_assert(!std::is_copy_constructible_v<Shape> && !std::is_copy_assignable_v<Shape>);
    static_assert(!std::is_move_constructible_v<Shape> && !std::is_move_assignable_v<Shape>);
    static_assert(std::is_copy_constructible_v<Copyable_base>);   // the BAD kind: slicing compiles
}

// C.80: =default keeps the compiler's semantics, including trivial copyability; hand-written copies lose it.
TEST(CoreGuidelinesC, C80_DefaultedKeepsTriviality) {
    static_assert(std::is_trivially_copyable_v<Pixel_defaulted>);
    static_assert(!std::is_trivially_copyable_v<Pixel_handwritten>);
    Pixel_defaulted a;
    a.r = 7;
    const Pixel_defaulted b = a;
    EXPECT_EQ(b.r, 7);
}

// C.81: =delete removes the copy operations while the moves stay.
TEST(CoreGuidelinesC, C81_DeletedCopy) {
    static_assert(!std::is_copy_constructible_v<Move_only> && !std::is_copy_assignable_v<Move_only>);
    static_assert(std::is_move_constructible_v<Move_only> && std::is_move_assignable_v<Move_only>);
}

// C.82: a virtual call in a base constructor runs the base's version, not the derived override.
TEST(CoreGuidelinesC, C82_VirtualCallInConstructorIsNotVirtual) {
    std::string log;
    const File_logger82 f{log};
    EXPECT_EQ(log, "base;file;");
    EXPECT_STREQ(f.who(), "file;");
}

// C.83: using std::swap; swap(a, b) finds the type's own noexcept swap.
TEST(CoreGuidelinesC, C83_TypeProvidesSwap) {
    grid::Grid a{std::vector<int>{1, 2}}, b{std::vector<int>{3}};
    const int before = grid::Grid::swaps;
    using std::swap;
    swap(a, b);
    EXPECT_EQ(grid::Grid::swaps, before + 1);
    EXPECT_EQ(a.cells().size(), 1u);
    EXPECT_EQ(b.cells().size(), 2u);
}

// C.84: a swap that exchanges handles copies nothing, so it can't fail: the element buffers change hands.
TEST(CoreGuidelinesC, C84_SwapExchangesHandles) {
    grid::Grid a{std::vector<int>{1, 2, 3}}, b{std::vector<int>{4}};
    const int* a_data = a.cells().data();
    const int* b_data = b.cells().data();
    a.swap(b);
    EXPECT_EQ(a.cells().data(), b_data);
    EXPECT_EQ(b.cells().data(), a_data);
}

// C.85: a noexcept swap makes the type nothrow-swappable; an unmarked one doesn't.
TEST(CoreGuidelinesC, C85_SwapIsNoexcept) {
    static_assert(std::is_nothrow_swappable_v<Tag_noexcept>);
    static_assert(std::is_nothrow_swappable_v<grid::Grid>);
    static_assert(!std::is_nothrow_swappable_v<Tag_plain>);
    Tag_noexcept a{"a"}, b{"b"};
    Tag_plain c{"c"}, d{"d"};
    swap(a, b);
    swap(c, d);
    EXPECT_EQ(a.text, "b");
    EXPECT_EQ(c.text, "d");
}

// C.86: a non-member == accepts const operands on both sides and is noexcept.
TEST(CoreGuidelinesC, C86_EqualityIsSymmetricAndNoexcept) {
    const Name86 a{"x"}, b{"x"}, c{"y"};
    static_assert(noexcept(a == b));
    EXPECT_TRUE(a == b);
    EXPECT_TRUE(b == a);
    EXPECT_FALSE(a == c);
    EXPECT_FALSE(c == a);
}

// C.87: == on a base compares only the base part, so different derived objects compare equal.
TEST(CoreGuidelinesC, C87_BaseEqualityIgnoresDerivedPart) {
    const Dog collie{"Rex", "collie"};
    const Dog poodle{"Rex", "poodle"};
    EXPECT_TRUE(static_cast<const Animal&>(collie) == static_cast<const Animal&>(poodle));
    EXPECT_NE(collie.breed, poodle.breed);
}

// C.89: a noexcept hash is nothrow-invocable and works in an unordered container.
TEST(CoreGuidelinesC, C89_HashIsNoexcept) {
    static_assert(std::is_nothrow_invocable_v<User_id_hash, const User_id&>);
    std::unordered_set<User_id, User_id_hash, User_id_eq> ids{{"a"}, {"b"}};
    EXPECT_EQ(ids.count(User_id{"a"}), 1u);
    EXPECT_EQ(ids.count(User_id{"c"}), 0u);
}

// C.90: a class with a std::string isn't trivially copyable, so memcpy/memset on it is UB; assignment does it right.
TEST(CoreGuidelinesC, C90_ConstructorsNotMemset) {
    static_assert(!std::is_trivially_copyable_v<Record>);
    Record r{"Ada", 10};
    r = Record{};
    EXPECT_TRUE(r.name.empty());
    EXPECT_EQ(r.score, 0);
    const Record from{"Bob", 3};
    Record to;
    to = from;
    EXPECT_EQ(to.name, "Bob");
}

// C.100: a container with the STL's names works with range-for, the algorithms and the ranges library.
TEST(CoreGuidelinesC, C100_StlStyleContainer) {
    static_assert(std::ranges::forward_range<Bag> && std::ranges::sized_range<Bag>);
    const Bag b{3, 1, 2};
    int sum = 0;
    for (int x : b) sum += x;
    EXPECT_EQ(sum, 6);
    EXPECT_EQ(*std::ranges::max_element(b), 3);
    EXPECT_EQ(std::ranges::size(b), 3u);
}

// C.101: copying a container copies its elements: the copy is equal and independent.
TEST(CoreGuidelinesC, C101_ContainerValueSemantics) {
    Bag a{1, 2};
    Bag b = a;
    EXPECT_EQ(a, b);
    EXPECT_NE(a.data(), b.data());
    b.push_back(3);
    EXPECT_EQ(a.size(), 2u);
}

// C.102: moving a container hands over its buffer instead of copying the elements, without throwing.
TEST(CoreGuidelinesC, C102_ContainerMoveKeepsBuffer) {
    static_assert(std::is_nothrow_move_constructible_v<Bag>);
    Bag a{1, 2, 3};
    const int* buffer = a.data();
    const Bag b = std::move(a);
    EXPECT_EQ(b.data(), buffer);
}

// C.103: an initializer_list constructor accepts a braced list of values.
TEST(CoreGuidelinesC, C103_InitializerListConstructor) {
    const Bag primes{2, 3, 5, 7};
    EXPECT_EQ(primes.size(), 4u);
    EXPECT_EQ(*primes.begin(), 2);
}

// C.104: a default-constructed container is empty and creating it can't throw.
TEST(CoreGuidelinesC, C104_DefaultIsEmpty) {
    static_assert(std::is_nothrow_default_constructible_v<Bag>);
    const Bag b;
    EXPECT_TRUE(b.empty());
    static_assert(std::regular<Bag>);
}

// C.109: a handle with pointer semantics supports * and ->, and copies share the object.
TEST(CoreGuidelinesC, C109_HandleHasStarAndArrow) {
    const Doc_handle h{std::make_shared<std::string>("doc")};
    const Doc_handle copy = h;
    EXPECT_EQ(h->size(), 3u);
    *copy += "!";
    EXPECT_EQ(*h, "doc!");
}

// C.121: an interface class is abstract with a virtual destructor, so deleting through it destroys the whole object.
TEST(CoreGuidelinesC, C121_PureAbstractInterface) {
    static_assert(std::is_abstract_v<Printer> && std::has_virtual_destructor_v<Printer>);
    int destroyed = 0;
    { std::unique_ptr<Printer> p = std::make_unique<Laser>(destroyed); p->print("x"); }
    EXPECT_EQ(destroyed, 1);
}

// C.122: code written against an abstract interface works with any implementation.
TEST(CoreGuidelinesC, C122_InterfaceSeparatesImplementations) {
    static_assert(std::is_abstract_v<Storage>);
    Memory_storage m;
    Upper_storage u;
    save(m, "hi");
    save(u, "hi");
    EXPECT_EQ(m.contents(), "hi");
    EXPECT_EQ(u.contents(), "HI");
}

// C.127: with a virtual destructor, deleting through the base runs the derived destructor.
TEST(CoreGuidelinesC, C127_VirtualDestructorWithVirtualFunctions) {
    static_assert(std::is_polymorphic_v<Sensor> && std::has_virtual_destructor_v<Sensor>);
    int destroyed = 0;
    Sensor* s = new Thermometer{destroyed};
    EXPECT_DOUBLE_EQ(s->read(), 21.5);
    delete s;
    EXPECT_EQ(destroyed, 1);
}

// C.128: without override, a drifted signature silently declares a new function; calls through the base miss it.
TEST(CoreGuidelinesC, C128_OverrideCatchesDrift) {
    Drifted d;
    Overrides o;
    Base128& bd = d;
    Base128& bo = o;
    EXPECT_STREQ(bd.scale(1.0), "base");       // the "override" never runs
    EXPECT_STREQ(bo.scale(1.0), "override");
}

// C.130: clone copies the most-derived object, while the base can't be copied.
TEST(CoreGuidelinesC, C130_VirtualClone) {
    const Square_shape sq{3};
    const Shape& s = sq;
    const auto copy = s.clone();
    const Shape& copied = *copy;
    EXPECT_EQ(typeid(copied), typeid(Square_shape));
    EXPECT_DOUBLE_EQ(copy->area(), 9.0);
    EXPECT_NE(copy.get(), &s);
    static_assert(!std::is_copy_constructible_v<Shape>);
}

// C.132: a class without virtual functions isn't polymorphic and carries no vtable pointer.
TEST(CoreGuidelinesC, C132_NoVirtualWithoutReason) {
    static_assert(!std::is_polymorphic_v<Vec2> && std::is_polymorphic_v<Virtual_vec2>);
    static_assert(sizeof(Vec2) < sizeof(Virtual_vec2));
    EXPECT_DOUBLE_EQ(Vec2{}.x() + Vec2{}.y(), 0.0);
}

// C.135: one class can implement several interfaces, each usable on its own.
TEST(CoreGuidelinesC, C135_MultipleInterfaces) {
    static_assert(std::is_base_of_v<Reader, File> && std::is_base_of_v<Writer, File>);
    File f;
    put(f, "abc");
    EXPECT_EQ(slurp(f), "abc");
}

// C.136: mixin bases combine their state and operations in one class.
TEST(CoreGuidelinesC, C136_MixinsCombineAttributes) {
    Session136 s;
    s.touch(42);
    s.rename("ada");
    EXPECT_EQ(s.last_used(), 42);
    EXPECT_EQ(s.name(), "ada");
}

// C.137: a virtual base is shared: both branches see one subobject.
TEST(CoreGuidelinesC, C137_VirtualBaseIsShared) {
    File137 f;
    f.read();
    f.write();
    EXPECT_EQ(f.uses, 2);
    EXPECT_EQ(&static_cast<Utility&>(static_cast<Reader137&>(f)),
              &static_cast<Utility&>(static_cast<Writer137&>(f)));
}

// C.138: a derived function hides every base overload of that name; using Base::name brings them back.
TEST(CoreGuidelinesC, C138_UsingRestoresOverloadSet) {
    static_assert(!prints_string<Hides>);
    static_assert(prints_string<Uses>);
    Uses u;
    EXPECT_STREQ(u.print(std::string{"x"}), "string");
    EXPECT_STREQ(u.print(1), "derived int");
}

// C.139: final forbids derivation; a class left open can be extended.
TEST(CoreGuidelinesC, C139_FinalCapsTheHierarchy) {
    static_assert(std::is_final_v<Capped> && !std::is_final_v<Circle139>);
    static_assert(std::is_base_of_v<Circle139, Labelled_circle>);
    const Labelled_circle c;
    EXPECT_DOUBLE_EQ(c.area(), 3.0);
}

// C.140: default arguments come from the static type, the body from the dynamic type; NVI keeps one default.
TEST(CoreGuidelinesC, C140_DefaultArgumentsFromStaticType) {
    Debug_logger140 d;
    Logger140& base = d;
    EXPECT_EQ(base.log(), 10);   // Logger140's default (1), Debug_logger140's body
    EXPECT_EQ(d.log(), 30);      // Debug_logger140's default (3)
    Nvi_debug n;
    Nvi_logger& nb = n;
    EXPECT_EQ(nb.log(), n.log());
}

// C.145: by value, a polymorphic object is sliced and calls the base; by reference it calls the override.
TEST(CoreGuidelinesC, C145_AccessThroughReferences) {
    const Copyable_square sq;
    EXPECT_DOUBLE_EQ(area_by_value(sq), 0.0);
    EXPECT_DOUBLE_EQ(area_by_ref(sq), 4.0);
}

// C.146: dynamic_cast checks the real type at run time and returns null when it doesn't match.
TEST(CoreGuidelinesC, C146_DynamicCastIsChecked) {
    Circle146 c;
    Square146 s;
    EXPECT_DOUBLE_EQ(radius_or_zero(c), 1.0);
    EXPECT_DOUBLE_EQ(radius_or_zero(s), 0.0);
}

// C.147: dynamic_cast to a reference throws std::bad_cast when the object isn't of that type.
TEST(CoreGuidelinesC, C147_ReferenceCastThrows) {
    Square146 s;
    Shape146& shape = s;
    EXPECT_THROW((void)dynamic_cast<Circle146&>(shape), std::bad_cast);
    EXPECT_NO_THROW((void)dynamic_cast<Square146&>(shape));
}

// C.148: dynamic_cast to a pointer returns null on failure, which can be tested.
TEST(CoreGuidelinesC, C148_PointerCastReturnsNull) {
    Square146 s;
    Shape146* shape = &s;
    EXPECT_EQ(dynamic_cast<Circle146*>(shape), nullptr);
    EXPECT_NE(dynamic_cast<Square146*>(shape), nullptr);
}

// C.149: a unique_ptr deletes its object on every path, including an exception.
TEST(CoreGuidelinesC, C149_SmartPointerDeletesOnException) {
    int destroyed = 0;
    try {
        auto p = std::make_unique<Tracked>(destroyed);
        throws_after(*p);
    } catch (const std::runtime_error&) {
    }
    EXPECT_EQ(destroyed, 1);
}

// C.150: make_unique creates the object owned by a unique_ptr.
TEST(CoreGuidelinesC, C150_MakeUnique) {
    const auto greeting = std::make_unique<std::string>("hello");
    static_assert(std::is_same_v<decltype(greeting), const std::unique_ptr<std::string>>);
    EXPECT_EQ(*greeting, "hello");
}

// C.151: make_shared creates the object with a single owner to start.
TEST(CoreGuidelinesC, C151_MakeShared) {
    const auto name = std::make_shared<std::string>("Ada");
    EXPECT_EQ(name.use_count(), 1);
    EXPECT_EQ(*name, "Ada");
}

// C.152: a derived object is bigger than its base, so a Base* into a Derived array steps wrongly; a span<Derived> doesn't.
TEST(CoreGuidelinesC, C152_NoBasePointerToDerivedArray) {
    static_assert(sizeof(Derived152) > sizeof(Base152));
    Derived152 items[3];
    for (int i = 0; i < 3; ++i) items[i].id = i + 1;
    EXPECT_EQ(sum_ids(items), 6);
}

// C.153: a virtual call reaches the most-derived override; a cast to an intermediate class does not.
TEST(CoreGuidelinesC, C153_VirtualBeatsCasting) {
    const Rounded_square146 r;
    const Shape146& s = r;
    EXPECT_DOUBLE_EQ(s.area(), 3.0);
    const auto* sq = dynamic_cast<const Square146*>(&s);
    ASSERT_NE(sq, nullptr);
    EXPECT_DOUBLE_EQ(sq->Square146::area(), 4.0);   // the "Square" answer is wrong for a rounded square
}

// C.160: + doesn't change its operands, and a copy compares equal.
TEST(CoreGuidelinesC, C160_ConventionalOperators) {
    const Money a{100}, b{50};
    EXPECT_EQ(a + b, b + a);
    EXPECT_EQ(a.cents, 100);
    const Money c = a;
    EXPECT_EQ(c, a);
}

// C.161: a non-member operator converts either operand; a member one only the right.
TEST(CoreGuidelinesC, C161_NonMemberSymmetricOperators) {
    static_assert(addable<Meters, double> && addable<double, Meters>);
    static_assert(addable<Member_meters, double> && !addable<double, Member_meters>);
    EXPECT_DOUBLE_EQ((1.5 + Meters{2}).value, 3.5);
}

// C.162: one overloaded name lets generic code call the right version for each type.
TEST(CoreGuidelinesC, C162_OverloadEquivalentOperations) {
    EXPECT_EQ(label(4), "[i4]");
    EXPECT_EQ(label(std::string{"x"}), "[sx]");
}

// C.164: an explicit conversion operator isn't used implicitly, but works in a condition or a cast.
TEST(CoreGuidelinesC, C164_ExplicitConversion) {
    static_assert(!std::is_convertible_v<Path, bool>);
    static_assert(!std::is_convertible_v<Path, const char*>);
    const Path p{"/tmp"};
    EXPECT_TRUE(static_cast<bool>(p));
    EXPECT_FALSE(Path{""});
    EXPECT_STREQ(p.c_str(), "/tmp");
}

// C.165: using std::swap; swap(a, b) picks the type's own swap; std::swap(a, b) never does.
TEST(CoreGuidelinesC, C165_UsingForCustomizationPoints) {
    custom::Big a{1}, b{2};
    custom::Big::custom_swaps = 0;
    exchange_values(a, b);
    EXPECT_EQ(custom::Big::custom_swaps, 1);
    exchange_qualified(a, b);
    EXPECT_EQ(custom::Big::custom_swaps, 1);
    EXPECT_EQ(a.value, 1);
    int x = 1, y = 2;
    exchange_values(x, y);
    EXPECT_EQ(x, 2);
}

// C.166: once unary & is overloaded, &x isn't the address; only std::addressof still is.
TEST(CoreGuidelinesC, C166_OverloadedAddressOfBreaksAmpersand) {
    Weird_address w{7};
    static_assert(std::is_same_v<decltype(&w), int>);
    EXPECT_EQ(&w, 7);
    EXPECT_EQ(static_cast<void*>(std::addressof(w)), static_cast<void*>(&w.value));
}

// C.167: operator<< for output composes with any ostream and with other << calls.
TEST(CoreGuidelinesC, C167_ConventionalOutputOperator) {
    std::ostringstream os;
    os << "p=" << Point167{1, 2} << ';';
    EXPECT_EQ(os.str(), "p=1,2;");
}

// C.168: an operator in its operands' namespace is found by argument-dependent lookup from anywhere.
TEST(CoreGuidelinesC, C168_OperatorInOperandNamespace) {
    EXPECT_TRUE(app::same(geo::Point{1, 2}, geo::Point{1, 2}));
    EXPECT_FALSE(app::same(geo::Point{1, 2}, geo::Point{2, 1}));
}

// C.170: one generic lambda handles several argument types.
TEST(CoreGuidelinesC, C170_GenericLambda) {
    const auto describe = [](const auto& v) -> std::string {
        if constexpr (std::is_arithmetic_v<std::remove_cvref_t<decltype(v)>>) return std::to_string(v);
        else return v;
    };
    EXPECT_EQ(describe(42), "42");
    EXPECT_EQ(describe(std::string{"x"}), "x");
}

// C.180: a union is as big as its largest member, smaller than a struct holding them all.
TEST(CoreGuidelinesC, C180_UnionSavesMemory) {
    static_assert(sizeof(Token_storage) == sizeof(double));
    static_assert(sizeof(Token_storage) < sizeof(Token_wasteful));
    Token_storage t;
    t.symbol = '+';
    EXPECT_EQ(t.symbol, '+');
}

// C.181: std::variant records the active member and rejects reading another.
TEST(CoreGuidelinesC, C181_VariantInsteadOfNakedUnion) {
    std::variant<int, double> v = 42;
    EXPECT_TRUE(std::holds_alternative<int>(v));
    EXPECT_THROW((void)std::get<double>(v), std::bad_variant_access);
    EXPECT_EQ(std::get_if<double>(&v), nullptr);
    v = 1.5;
    EXPECT_DOUBLE_EQ(std::get<double>(v), 1.5);
}

// C.182: a class wrapping a tag and an anonymous union checks every access against the tag.
TEST(CoreGuidelinesC, C182_TaggedUnionClass) {
    const Value i{3};
    const Value d{2.5};
    EXPECT_EQ(i.as_int(), 3);
    EXPECT_DOUBLE_EQ(d.as_real(), 2.5);
    EXPECT_THROW((void)i.as_real(), std::logic_error);
    EXPECT_THROW((void)d.as_int(), std::logic_error);
}

// C.183: std::bit_cast gives an object's bits without union punning: 1.0f is 0x3F800000 in IEEE 754.
TEST(CoreGuidelinesC, C183_BitCastNotUnionPunning) {
    CPPDOCS_REQUIRE(__cpp_lib_bit_cast);
#ifdef __cpp_lib_bit_cast
    static_assert(std::numeric_limits<float>::is_iec559);
    static_assert(std::bit_cast<std::uint32_t>(1.0f) == 0x3F800000u);
    EXPECT_EQ(std::bit_cast<std::uint32_t>(-2.0f), 0xC0000000u);
#endif
}

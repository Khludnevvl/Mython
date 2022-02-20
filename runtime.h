#pragma once

#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace runtime
{
class Context
{
public:
    virtual std::ostream &GetOutputStream() = 0;

protected:
    ~Context() = default;
};

class Object
{
public:
    virtual ~Object() = default;

    virtual void Print(std::ostream &os, Context &context) = 0;
};

class ObjectHolder
{
public:
    ObjectHolder() = default;

    // Возвращает ObjectHolder, владеющий объектом типа T
    // Тип T - конкретный класс-наследник Object.
    // object копируется или перемещается в кучу
    template <typename T>
    [[nodiscard]] static ObjectHolder Own(T &&object)
    {
        return ObjectHolder(std::make_shared<T>(std::forward<T>(object)));
    }

    // Создаёт ObjectHolder, не владеющий объектом (аналог слабой ссылки)
    [[nodiscard]] static ObjectHolder Share(Object &object);

    // Создаёт пустой ObjectHolder, соответствующий значению None
    [[nodiscard]] static ObjectHolder None();

    Object &operator*() const;

    Object *operator->() const;

    [[nodiscard]] Object *Get() const;

    // Возвращает указатель на объект типа T либо nullptr, если внутри ObjectHolder не хранится
    // объект данного типа
    template <typename T>
    [[nodiscard]] T *TryAs() const
    {
        return dynamic_cast<T *>(this->Get());
    }

    explicit operator bool() const;

private:
    explicit ObjectHolder(std::shared_ptr<Object> data);
    void AssertIsValid() const;

    std::shared_ptr<Object> data_;
};

template <typename T>
class ValueObject : public Object
{
public:
    ValueObject(T v) 
        : value_(v)
    {
    }

    void Print(std::ostream &os, [[maybe_unused]] Context &context) override
    {
        os << value_;
    }

    [[nodiscard]] const T &GetValue() const
    {
        return value_;
    }

private:
    T value_;
};

using Closure = std::unordered_map<std::string, ObjectHolder>;

// Для отличных от нуля чисел, True и непустых строк возвращается true. В остальных случаях - false.
bool IsTrue(const ObjectHolder &object);

class Executable
{
public:
    virtual ~Executable() = default;

    virtual ObjectHolder Execute(Closure &closure, Context &context) = 0;
};

using String = ValueObject<std::string>;
using Number = ValueObject<int>;

class Bool : public ValueObject<bool>
{
public:
    using ValueObject<bool>::ValueObject;

    void Print(std::ostream &os, Context &context) override;
};

struct Method
{
    std::string name;
    std::vector<std::string> formal_params;
    std::unique_ptr<Executable> body;
};

class Class : public Object
{
public:
    // Создаёт класс с именем name и набором методов methods, унаследованный от класса parent
    // Если parent равен nullptr, то создаётся базовый класс
    explicit Class(std::string name, std::vector<Method> methods, const Class *parent);

    // Возвращает указатель на метод name или nullptr, если метод с таким именем отсутствует
    [[nodiscard]] const Method *GetMethod(const std::string &name) const;

    [[nodiscard]] const std::string &GetName() const;

    void Print(std::ostream &os, Context &context) override;

private:
    std::string name_;
    std::vector<Method> methods_;
    const Class *parent_ = nullptr;
    std::unordered_map<std::string, const Method *> metod_name_to_ptr_;
    Closure arguments_;
};

class ClassInstance : public Object
{
public:
    explicit ClassInstance(const Class &cls);

    /*
     * Если у объекта есть метод __str__, выводит в os результат, возвращённый этим методом.
     * В противном случае в os выводится адрес объекта.
     */
    void Print(std::ostream &os, [[maybe_unused]] Context &context) override;

    /*
     * Вызывает у объекта метод method, передавая ему actual_args параметров.
     * Параметр context задаёт контекст для выполнения метода.
     * Если ни сам класс, ни его родители не содержат метод method, метод выбрасывает исключение
     * runtime_error
     */
    ObjectHolder Call(const std::string &method, const std::vector<ObjectHolder> &actual_args,
                      Context &context);

    [[nodiscard]] bool HasMethod(const std::string &method, size_t argument_count) const;

    [[nodiscard]] Closure &Fields();

    [[nodiscard]] const Closure &Fields() const;

private:
    const Class &class_;
    Closure fields_;
};

/*
 * Возвращает true, если lhs и rhs содержат одинаковые числа, строки или значения типа Bool.
 * Если lhs - объект с методом __eq__, функция возвращает результат вызова lhs.__eq__(rhs),
 * приведённый к типу Bool. Если lhs и rhs имеют значение None, функция возвращает true.
 * В остальных случаях функция выбрасывает исключение runtime_error.
 */
bool Equal(const ObjectHolder &lhs, const ObjectHolder &rhs, Context &context);

/*
 * Если lhs и rhs - числа, строки или значения bool, функция возвращает результат их сравнения
 * оператором <.
 * Если lhs - объект с методом __lt__, возвращает результат вызова lhs.__lt__(rhs),
 * приведённый к типу bool. В остальных случаях функция выбрасывает исключение runtime_error.
 */
bool Less(const ObjectHolder &lhs, const ObjectHolder &rhs, Context &context);

bool NotEqual(const ObjectHolder &lhs, const ObjectHolder &rhs, Context &context);

bool Greater(const ObjectHolder &lhs, const ObjectHolder &rhs, Context &context);

bool LessOrEqual(const ObjectHolder &lhs, const ObjectHolder &rhs, Context &context);

bool GreaterOrEqual(const ObjectHolder &lhs, const ObjectHolder &rhs, Context &context);

struct DummyContext : Context
{
    std::ostream &GetOutputStream() override
    {
        return output;
    }

    std::ostringstream output;
};

class SimpleContext : public runtime::Context
{
public:
    explicit SimpleContext(std::ostream &output) : output_(output) {}

    std::ostream &GetOutputStream() override
    {
        return output_;
    }

private:
    std::ostream &output_;
};

} // namespace runtime
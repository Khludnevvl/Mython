#include "runtime.h"

#include <cassert>
#include <iostream>
#include <sstream>
#include <variant>

using namespace std;

namespace runtime
{

namespace
{
const std::string STR_METHOD = "__str__";
const std::string LESS_THAN_METHOD = "__lt__";
const std::string EQUAL_METHOD = "__eq__";
} // namespace

ObjectHolder::ObjectHolder(std::shared_ptr<Object> data) : data_(std::move(data)) {}

void ObjectHolder::AssertIsValid() const
{
    assert(data_ != nullptr);
}

ObjectHolder ObjectHolder::Share(Object &object)
{
    return ObjectHolder(std::shared_ptr<Object>(&object, [](auto * /*p*/) { /* do nothing */ }));
}

ObjectHolder ObjectHolder::None()
{
    return ObjectHolder();
}

Object &ObjectHolder::operator*() const
{
    AssertIsValid();
    return *Get();
}

Object *ObjectHolder::operator->() const
{
    AssertIsValid();
    return Get();
}

Object *ObjectHolder::Get() const
{
    return data_.get();
}

ObjectHolder::operator bool() const
{
    return Get() != nullptr;
}

bool IsTrue(const ObjectHolder &object)
{
#define CONVERT_TO_BOOL(type)                                                                                \
    {                                                                                                        \
        auto obj_ptr = dynamic_cast<type *>(object.Get());                                                   \
        if (obj_ptr)                                                                                         \
            return obj_ptr->GetValue();                                                                      \
    }
    CONVERT_TO_BOOL(Bool);
    CONVERT_TO_BOOL(Number);

#undef CONVERT_TO_BOOL

    auto string_ptr = dynamic_cast<String *>(object.Get());
    if (string_ptr != nullptr)
    {
        return !string_ptr->GetValue().empty();
    }

    return false;
}

void ClassInstance::Print(std::ostream &os, Context &context)
{
    if (this->HasMethod(STR_METHOD, 0U))
    {
        this->Call(STR_METHOD, {}, context)->Print(os, context);
    }
    else
    {
        os << this;
    }
}

bool ClassInstance::HasMethod(const std::string &method_name, size_t argument_count) const
{
    const Method *method = class_.GetMethod(method_name);
    return method != nullptr && method->formal_params.size() == argument_count;
}

Closure &ClassInstance::Fields()
{
    return fields_;
}

const Closure &ClassInstance::Fields() const
{
    return fields_;
}

ClassInstance::ClassInstance(const Class &cls) : class_(cls) {}

ObjectHolder ClassInstance::Call(const std::string &method_name, const std::vector<ObjectHolder> &actual_args,
                                 Context &context)
{
    if (!HasMethod(method_name, actual_args.size()))
    {
        throw std::runtime_error("Cannot call method");
    }
    const Method *method = class_.GetMethod(method_name);
    Closure args;
    args["self"] = ObjectHolder::Share(*this);
    for (size_t i = 0; i < method->formal_params.size(); i++)
    {
        args[method->formal_params[i]] = actual_args[i];
    }
    return method->body.get()->Execute(args, context);
}

Class::Class(std::string name, std::vector<Method> methods, const Class *parent)
    : name_(name), methods_(std::move(methods)), parent_(parent)
{
    for (const auto &method : methods_)
    {
        metod_name_to_ptr_[method.name] = &method;
    }
}

const Method *Class::GetMethod(const std::string &name) const
{
    auto iter = metod_name_to_ptr_.find(name);
    if (iter != metod_name_to_ptr_.end())
    {
        return iter->second;
    }
    else if (parent_ != nullptr)
    {
        return parent_->GetMethod(name);
    }
    return nullptr;
}

[[nodiscard]] const std::string &Class::GetName() const
{
    return name_;
}

void Class::Print(ostream &os, [[maybe_unused]] Context &context)
{
    os << "Class " << name_;
}

void Bool::Print(std::ostream &os, [[maybe_unused]] Context &context)
{
    os << (GetValue() ? "True"sv : "False"sv);
}

bool Equal(const ObjectHolder &lhs, const ObjectHolder &rhs, Context &context)
{
    if (lhs.Get() == nullptr && rhs.Get() == nullptr)
    {
        return true;
    }

    auto lhs_class_ptr = dynamic_cast<ClassInstance *>(lhs.Get());
    if (lhs_class_ptr != nullptr)
    {
        if (lhs_class_ptr->HasMethod(EQUAL_METHOD, 1U))
        {
            return IsTrue(lhs_class_ptr->Call(EQUAL_METHOD, {rhs}, context));
        }
        else
        {
            throw std::runtime_error("Cannot compare objects for equality"s);
        }
    }

#define COMPARABLE_VALUE(type)                                                                               \
    {                                                                                                        \
        auto lhs_ptr = dynamic_cast<type *>(lhs.Get());                                                      \
        auto rhs_ptr = dynamic_cast<type *>(rhs.Get());                                                      \
        if (lhs_ptr && rhs_ptr)                                                                              \
            return lhs_ptr->GetValue() == rhs_ptr->GetValue();                                               \
    }
    COMPARABLE_VALUE(Bool);
    COMPARABLE_VALUE(Number);
    COMPARABLE_VALUE(String);

#undef COMPARABLE_VALUE

    throw std::runtime_error("Cannot compare objects for equality"s);
}

bool Less(const ObjectHolder &lhs, const ObjectHolder &rhs, Context &context)
{
    auto lhs_class_ptr = dynamic_cast<ClassInstance *>(lhs.Get());
    if (lhs_class_ptr != nullptr)
    {
        if (lhs_class_ptr->HasMethod(LESS_THAN_METHOD, 1U))
        {
            return IsTrue(lhs_class_ptr->Call(LESS_THAN_METHOD, {rhs}, context));
        }
        throw std::runtime_error("Cannot compare objects for less"s);
    }

#define COMPARABLE_VALUE(type)                                                                               \
    {                                                                                                        \
        auto lhs_ptr = dynamic_cast<type *>(lhs.Get());                                                      \
        auto rhs_ptr = dynamic_cast<type *>(rhs.Get());                                                      \
        if (lhs_ptr && rhs_ptr)                                                                              \
            return lhs_ptr->GetValue() < rhs_ptr->GetValue();                                                \
    }
    COMPARABLE_VALUE(Bool);
    COMPARABLE_VALUE(Number);
    COMPARABLE_VALUE(String);

#undef COMPARABLE_VALUE

    throw std::runtime_error("Cannot compare objects for less"s);
}

bool NotEqual(const ObjectHolder &lhs, const ObjectHolder &rhs, Context &context)
{
    return !Equal(lhs, rhs, context);
}

bool Greater(const ObjectHolder &lhs, const ObjectHolder &rhs, Context &context)
{
    return !(Less(lhs, rhs, context) || Equal(lhs, rhs, context));
}

bool LessOrEqual(const ObjectHolder &lhs, const ObjectHolder &rhs, Context &context)
{
    return !(Greater(lhs, rhs, context));
}

bool GreaterOrEqual(const ObjectHolder &lhs, const ObjectHolder &rhs, Context &context)
{
    return !(Less(lhs, rhs, context));
}

} // namespace runtime
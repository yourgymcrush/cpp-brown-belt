#include <iostream>
#include <vector>
#include <string>

class MyClass {
public:
    int value;
    std::string name;

    void printIntMember(const int MyClass::* memberPtr, const MyClass& obj) {
      std::cout << "Value: " << this->*memberPtr << std::endl;
    }

    void print()
    {
      std::cout << "HI" << std::endl;
    }
};

// Функция принимает указатель на член класса типа int
void a(void (MyClass::* memberPtr) (), MyClass& obj) {
  (obj.*memberPtr)();
}


int main() {
    MyClass obj;
    obj.value = 5;
    obj.name = "Another Test";

    // Передача указателя на член класса 'value'
    obj.printIntMember(&MyClass::value, obj); // Выведет: Value: 5
    void (MyClass::*ptr)() = &MyClass::print;
    (obj.*ptr)();
    a(&MyClass::print, obj);
    return 0;
}
#include <iostream>

namespace RAII
{
template<typename Provider>
class Booking
{
public:
  Booking(Provider* provider, int& counter) : m_provider(provider), m_counter(counter)
  {
  }
  Booking(const Booking& other) = delete;
  Booking(Booking&& other) : m_provider(std::move(other.m_provider)), m_counter(other.m_counter)
  {
    other.m_provider = nullptr;
  }
  Booking& operator=(const Booking& other) = delete;
  Booking& operator=(Booking&& other)
  {
    m_provider = std::move(other.m_provider);
    m_counter = other.m_counter;
    other.m_provider = nullptr;
    
    return *this;
  }
  ~Booking()
  {
    if(m_provider)
      m_provider->CancelOrComplete(*this);
  }

private:
  Provider* m_provider;
  int& m_counter;
};
}

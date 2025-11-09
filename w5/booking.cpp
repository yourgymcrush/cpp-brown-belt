#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <functional>

class BookingProvider
{
private:
  struct Booking
  {
    int64_t time;
    int client_id;
    int room_count;
  };
  class HotelInfo
  {
  private:
    static const int TIME_WINDOW_SIZE = 86400;
    std::deque<Booking> m_bookings;
    int m_roomCount;
    std::unordered_map<int, int> m_clientIdToBookingCount;
  public:
    void Book(const Booking& booking)
    {
      m_bookings.push_back(booking);
      m_roomCount += booking.room_count;
      ++m_clientIdToBookingCount[booking.client_id];
    }
    int ComputeClientsCount(int64_t currentTime)
    {
      RemoveOldBookings(currentTime);
      return m_clientIdToBookingCount.size();
    }
    int ComputeRoomsCount(int64_t currentTime)
    {
      RemoveOldBookings(currentTime);
      return m_roomCount;
    }

    void RemoveOldBookings(int64_t currentTime)
    {
      while(!m_bookings.empty() && m_bookings.front().time <= currentTime - TIME_WINDOW_SIZE)
      {
        PopBooking();
      }
    }

    void PopBooking()
    {
      const Booking& booking = m_bookings.front();
      const auto clienIt = m_clientIdToBookingCount.find(booking.client_id);

      m_roomCount -= booking.room_count;
      if(--clienIt->second == 0)
      {
        m_clientIdToBookingCount.erase(clienIt);
      }

      m_bookings.pop_front();
    }
  };
  int64_t m_currentTime = 0;
  std::unordered_map<std::string, HotelInfo> m_hotels;

public:
  void Book(int64_t time, std::string hotel_name, int client_id, int room_count)
  {
    m_currentTime = time;
    m_hotels[hotel_name].Book({time, client_id, room_count});
  }

  int Clients(const std::string& hotelMame)
  {
    return m_hotels[hotelMame].ComputeClientsCount(m_currentTime);
  }

  int Rooms(const std::string& hotelMame)
  {
    return m_hotels[hotelMame].ComputeRoomsCount(m_currentTime);
  }
};

int main()
{
  std::ios::sync_with_stdio(false);
  std::cin.tie(nullptr);

  BookingProvider bookingProvider;
  int queryCount;

  std::cin >> queryCount;
  for(int i = 0; i < queryCount; i++)
  {
    std::string queryType;

    std::cin >> queryType;
    if(queryType == "BOOK")
    {
      int64_t time;
      std::string hotel_name;
      int client_id;
      int room_count;

      std::cin >> time >> hotel_name >> client_id >> room_count;
      bookingProvider.Book(time, hotel_name, client_id, room_count);
    }
    else
    {
      std::string hotel_name;

      std::cin >> hotel_name;
      if(queryType == "CLIENTS")
      {
        std::cout << bookingProvider.Clients(hotel_name) << std::endl;
      }
      else
      {
        std::cout << bookingProvider.Rooms(hotel_name) << std::endl;
      }
    }
    
  }

  return 0;
}
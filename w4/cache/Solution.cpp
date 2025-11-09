#include <map>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <string>
#include <list>
#include <mutex>
#include <iostream>
#include "Common.h"

using namespace std;

class LruCache : public ICache
{
private:
  using ListIt = std::list<std::string>::iterator;

  const Settings& m_settings {}; 
  shared_ptr<IBooksUnpacker> m_BooksUnpacker {};

  size_t m_cacheSize {0};
  std::unordered_map<std::string, ListIt> m_cachedBookNames {};
  std::list<std::string> m_sortedBooks;
  std::unordered_map<std::string, BookPtr> m_chachedBooks {};

  std::mutex m_cacheMutex;
public:
  LruCache(shared_ptr<IBooksUnpacker> books_unpacker, const Settings& settings) : 
           m_settings(settings), m_BooksUnpacker(books_unpacker)
  {
    // DONE
  }

  BookPtr GetBook(const string& book_name) override
  {
    std::lock_guard l(m_cacheMutex);
    if(m_cachedBookNames.count(book_name))
    {
      // FIND BOOK
      auto book = m_chachedBooks[book_name];

      // MAKE BOOK RECENT USED
      auto it = m_cachedBookNames[book_name];
      m_sortedBooks.erase(it);
      m_sortedBooks.push_front(book_name);
      m_cachedBookNames[book_name] = m_sortedBooks.begin();

      return book;
    }
    else
    {
      auto book = m_BooksUnpacker->UnpackBook(book_name);
      auto bookSize = book->GetContent().size();

      while(!m_cachedBookNames.empty())
      {
        if(m_cacheSize + bookSize <= m_settings.max_memory)
        {
          // INSERT BOOK
          m_sortedBooks.push_front(book_name);
          m_cachedBookNames[book_name] = m_sortedBooks.begin();
          m_chachedBooks[book_name] = std::move(book);
          m_cacheSize += bookSize;
          
          return m_chachedBooks[book_name];
        }
        // DELETE BOOK
        auto book_name = m_sortedBooks.back();
        auto bookSize = m_chachedBooks[book_name]->GetContent().size();

        m_sortedBooks.pop_back();
        m_cachedBookNames.erase(book_name);
        m_chachedBooks.erase(book_name);
        m_cacheSize -= bookSize;
      }
      if(m_cacheSize + bookSize <= m_settings.max_memory)
      {
        // INSERT BOOK
        m_sortedBooks.push_front(book_name);
        m_cachedBookNames[book_name] = m_sortedBooks.begin();
        m_chachedBooks[book_name] = std::move(book);
        m_cacheSize += bookSize;
        
        return m_chachedBooks[book_name];
      }

      return book;
    }
  }
};


unique_ptr<ICache> MakeCache(shared_ptr<IBooksUnpacker> books_unpacker, const ICache::Settings& settings)
{
  auto cache = make_unique<LruCache>(books_unpacker, settings);

  return std::move(cache);
}

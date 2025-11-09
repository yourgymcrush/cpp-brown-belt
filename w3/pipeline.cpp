#include "test_runner.h"
#include <functional>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

struct Email {
  string from;
  string to;
  string body;
};

std::istream& operator >>(std::istream& is, Email &email) {
  getline(is, email.from);
  getline(is, email.to);
  getline(is, email.body);

  return is;
}

std::ostream& operator <<(std::ostream& os, Email &email) {
  os << email.from << "\n" << email.to << "\n" <<email.body << "\n";

  return os;
}


class Worker {
private:
  std::optional<std::unique_ptr<Worker>> mNext {std::nullopt};
public:
  virtual ~Worker() = default;
  virtual void Process(unique_ptr<Email> email) = 0;
  virtual void Run() {
    throw logic_error("Unimplemented");
  }

protected:
  
  void PassOn(unique_ptr<Email> email) const {
    if (mNext) {
      (*mNext)->Process(std::move(email));
    }
  }

public:
  void SetNext(unique_ptr<Worker> next) {
    mNext = std::move(next);
  }
};


class Reader : public Worker {
private:
  std::istream& mIs;
public:
  Reader(std::istream &is) : mIs(is) {}

  virtual void Run() override {
    
    Email email;

    while(mIs >> email) {
      PassOn(std::make_unique<Email>(email));
    }
  }

  virtual void Process(unique_ptr<Email> email) override {}
};


class Filter : public Worker {
public:
  using Function = function<bool(const Email&)>;
private:
  Function mFilter {};
public:
  Filter(const Function &filter) : mFilter(filter) {}
  
  virtual void Process(std::unique_ptr<Email> email) override {
    if (mFilter(*email)) {
      PassOn(std::move(email));
    }
  }
};


class Copier : public Worker {
private:
  std::string mAddres {};
public:
  Copier(const std::string& addres) : mAddres(addres) {}

  virtual void Process(std::unique_ptr<Email> email) override {
    if (mAddres != email->to) {
      auto copy = std::make_unique<Email>(*email);

      copy->to = mAddres;
      PassOn(std::move(email));
      PassOn(std::move(copy));
    }
    else {
      PassOn(std::move(email));
    }
  }
};


class Sender : public Worker {
private:
  std::ostream& mOs;
public:
  Sender(std::ostream &os) : mOs(os) {}

  virtual void Process(std::unique_ptr<Email> email) override {
    mOs << *email;
    PassOn(move(email)); 
  }
};


class PipelineBuilder {
  std::vector<std::unique_ptr<Worker>> pipe;
public:
  explicit PipelineBuilder(istream& in) {
    pipe.push_back(std::make_unique<Reader>(in));
  }

  // добавляет новый обработчик Filter
  PipelineBuilder& FilterBy(Filter::Function filter) {
    pipe.push_back(std::make_unique<Filter>(filter));
    return *this;
  }

  // добавляет новый обработчик Copier
  PipelineBuilder& CopyTo(string recipient) {
    pipe.push_back(std::make_unique<Copier>(recipient));
    return *this;
  }

  // добавляет новый обработчик Sender
  PipelineBuilder& Send(ostream& out) {
    pipe.push_back(std::make_unique<Sender>(out));
    return *this;
  }

  // возвращает готовую цепочку обработчиков
  unique_ptr<Worker> Build() {
    for (int i = pipe.size() - 1; i > 0; i--) {
      pipe[i - 1]->SetNext(std::move(pipe[i]));
    }
    return std::move(pipe.front());
  }
};


void TestSanity() {
  string input = (
    "erich@example.com\n"
    "richard@example.com\n"
    "Hello there\n"

    "erich@example.com\n"
    "ralph@example.com\n"
    "Are you sure you pressed the right button?\n"

    "ralph@example.com\n"
    "erich@example.com\n"
    "I do not make mistakes of that kind\n"
  );
  istringstream inStream(input);
  ostringstream outStream;

  PipelineBuilder builder(inStream);
  builder.FilterBy([](const Email& email) {
    return email.from == "erich@example.com";
  });
  builder.CopyTo("richard@example.com");
  builder.Send(outStream);
  auto pipeline = builder.Build();

  pipeline->Run();

  string expectedOutput = (
    "erich@example.com\n"
    "richard@example.com\n"
    "Hello there\n"

    "erich@example.com\n"
    "ralph@example.com\n"
    "Are you sure you pressed the right button?\n"

    "erich@example.com\n"
    "richard@example.com\n"
    "Are you sure you pressed the right button?\n"
  );

  ASSERT_EQUAL(expectedOutput, outStream.str());
}

int main() {
  TestRunner tr;
  RUN_TEST(tr, TestSanity);
  return 0;
}

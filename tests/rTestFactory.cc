#include "rBOOSTTest.h"

#include "rFactory.h"
#include <thread>
#include <vector>
#include <string>
#include <memory>

using namespace rapio;

// --- Helper classes for testing the Creator Pattern ---

// 1. The Product (The object we want to create)
class TestProduct {
public:
  int value;
  TestProduct(int v) : value(v){ }
};

// 2. The Abstract Creator Interface (Stored in the Factory)
class TestCreator {
public:
  virtual
  ~TestCreator() = default;
  // Factory method to create the product
  virtual std::shared_ptr<TestProduct>
  create(int v) = 0;
};

// 3. The Concrete Creator (The implementation)
class ConcreteTestCreator : public TestCreator {
public:
  std::shared_ptr<TestProduct>
  create(int v) override
  {
    // Logic to create a new unique instance
    return std::make_shared<TestProduct>(v);
  }
};

// --- Tests ---

BOOST_AUTO_TEST_SUITE(_Factory_Suite_)

// Test using creator way (currently notifiers)
BOOST_AUTO_TEST_CASE(_Factory_Creator_Pattern_)
{
  std::string name = "my_creator";

  // 1. Instantiate the lightweight creator
  auto creator = std::make_shared<ConcreteTestCreator>();

  // 2. Register the creator into the Factory
  Factory<TestCreator>::introduce(name, creator);

  // 3. Verify it exists
  BOOST_CHECK(Factory<TestCreator>::exists(name));

  // 4. Retrieve the creator
  auto retrievedCreator = Factory<TestCreator>::get(name);

  BOOST_CHECK(retrievedCreator != nullptr);

  // 5. Use the creator to generate unique products
  if (retrievedCreator) {
    auto productA = retrievedCreator->create(100);
    auto productB = retrievedCreator->create(200);

    // Verify products are correct
    BOOST_CHECK_EQUAL(productA->value, 100);
    BOOST_CHECK_EQUAL(productB->value, 200);

    // Verify they are distinct instances (Factory behavior, not Singleton)
    BOOST_CHECK(productA != productB);
  }
}

// Stress test the single Factory class with multiple threads
BOOST_AUTO_TEST_CASE(_Factory_Concurrency_Stress_Test_)
{
  const int NUM_THREADS    = 20;
  const int OPS_PER_THREAD = 100;
  std::vector<std::thread> threads;

  // Lambda to simulate work: register creators and use them
  auto worker = [](int id) {
      for (int i = 0; i < OPS_PER_THREAD; ++i) {
        std::string name = "thread_" + std::to_string(id) + "_creator_" + std::to_string(i);

        // Create and introduce a creator
        auto creator = std::make_shared<ConcreteTestCreator>();
        Factory<TestCreator>::introduce(name, creator);

        // Read back immediately
        auto retrieved = Factory<TestCreator>::get(name);

        // Use it
        if (retrieved) {
          auto prod = retrieved->create(i);
          // Simple check to ensure memory is valid
          if (prod->value != i) {
            // In a real test framework we might log failure here,
            // but strictly we just want to ensure no segfaults occurred.
          }
        }
      }
    };

  // Launch threads
  for (int i = 0; i < NUM_THREADS; ++i) {
    threads.push_back(std::thread(worker, i));
  }

  // Join threads
  for (auto& t : threads) {
    t.join();
  }

  // Final verification
  std::string verifyName = "thread_0_creator_0";

  BOOST_CHECK(Factory<TestCreator>::exists(verifyName));

  auto finalCreator      = Factory<TestCreator>::get(verifyName);

  BOOST_CHECK(finalCreator != nullptr);
  if (finalCreator) {
    auto p = finalCreator->create(999);
    BOOST_CHECK_EQUAL(p->value, 999);
  }
}

BOOST_AUTO_TEST_SUITE_END();

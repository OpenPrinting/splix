#include <gtest/gtest.h>
#include "printer.h"
#include "ppdfile.h"
#include "sp_semaphore.h"
#include "band.h"
#include "bandplane.h"
#include "algo0x11.h"
#include "request.h"
#include "sp_portable.h"
#include <string_view>
#include <utility>
#include <thread>
#include <vector>
#include <span>

// ────────────────────────────────────────────────────────────────────────────
// Helper Subclass for Testing Protected Members
// ────────────────────────────────────────────────────────────────────────────

class TestablePrinter : public Printer {
public:
    void setManufacturer(const std::string& m) { _manufacturer = m; }
    void setModel(const std::string& m) { _model = m; }
};

// ────────────────────────────────────────────────────────────────────────────
// Printer Class Tests
// ────────────────────────────────────────────────────────────────────────────

TEST(PrinterTest, Initialization) {
    TestablePrinter printer;
    printer.setManufacturer("Samsung");
    printer.setModel("ML-2010");
    
    EXPECT_EQ(printer.manufacturer(), "Samsung");
    EXPECT_EQ(printer.model(), "ML-2010");
}

TEST(PrinterTest, MoveSemantics) {
    TestablePrinter p1;
    p1.setManufacturer("Xerox");
    p1.setModel("Phaser 3117");
    
    Printer p2 = std::move(p1);
    
    EXPECT_EQ(p2.manufacturer(), "Xerox");
    // p1's strings should be empty after move
    EXPECT_TRUE(p1.manufacturer().empty());
}

TEST(PrinterTest, DefaultInitialization) {
    Printer p;
    EXPECT_EQ(p.qpdlVersion(), 0);
    EXPECT_FALSE(p.color());
    EXPECT_EQ(p.bandHeight(), 0);
}

// ────────────────────────────────────────────────────────────────────────────
// PPDFile Value Tests
// ────────────────────────────────────────────────────────────────────────────

TEST(PPDValueTest, StringSafety) {
    PPDValue v1("TestValue");
    PPDValue v2 = v1; // Copy assignment
    
    EXPECT_STREQ(static_cast<const char*>(v1), "TestValue");
    EXPECT_STREQ(static_cast<const char*>(v2), "TestValue");
    
    v1 = PPDValue("NewValue");
    EXPECT_STREQ(static_cast<const char*>(v1), "NewValue");
    EXPECT_STREQ(static_cast<const char*>(v2), "TestValue"); // v2 should remain unchanged
}

// ────────────────────────────────────────────────────────────────────────────
// Synchronization Tests (SP::Semaphore)
// ────────────────────────────────────────────────────────────────────────────

TEST(SemaphoreTest, BasicAcquireRelease) {
    SP::Semaphore sem(0);
    sem.release();
    EXPECT_TRUE(sem.try_acquire());
    EXPECT_FALSE(sem.try_acquire());
}

TEST(SemaphoreTest, MultiRelease) {
    SP::Semaphore sem(0);
    sem.release(3);
    EXPECT_TRUE(sem.try_acquire());
    EXPECT_TRUE(sem.try_acquire());
    EXPECT_TRUE(sem.try_acquire());
    EXPECT_FALSE(sem.try_acquire());
}

TEST(SemaphoreTest, ThreadSync) {
    SP::Semaphore sem(0);
    bool workDone = false;
    
    std::thread t([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        workDone = true;
        sem.release();
    });
    
    sem.acquire();
    EXPECT_TRUE(workDone);
    t.join();
}

// ────────────────────────────────────────────────────────────────────────────
// Compression Tests (Algo0x11)
// ────────────────────────────────────────────────────────────────────────────

TEST(Algo0x11Test, CompressionCorrectness) {
    Algo0x11 algo;
    Request request;
    
    // Sample input: 1024 bytes of repeating data (highly compressible)
    std::vector<uint8_t> src(1024, 0xAA);
    
    auto result = algo.compress(request, src, 128, 64);
    ASSERT_TRUE(result.has_value());
    EXPECT_NE(result.value(), nullptr);
}

TEST(Algo0x11Test, EmptyInput) {
    Algo0x11 algo;
    Request request;
    std::vector<uint8_t> src;
    
    auto result = algo.compress(request, src, 0, 0);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), SP::Error::InvalidArgument);
}

TEST(Algo0x11Test, LargeEmptyBuffer) {
    Algo0x11 algo;
    Request request;
    std::vector<uint8_t> input(1000, 0);
    // New signature expects: request, span, width, height
    auto result = algo.compress(request, input, 100, 10); 
    EXPECT_TRUE(result.has_value());
    EXPECT_NE(result.value(), nullptr);
}

// Phase 14: Modernization Tests
// StringCaseCompare test removed as SP_STRCASECMP was eliminated in favor of std::ranges::equal


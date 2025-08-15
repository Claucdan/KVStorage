#include <gtest/gtest.h>
#include <KVStorage.h>

// Мок для часов
struct MockClock {
    static int currentTime;

    int now() { return currentTime; }

    void advance(int sec) { currentTime += sec; }

    MockClock(int time) {
        currentTime = time;
    }
};

int MockClock::currentTime = 0;

using TestStorage = InMemoryKVStorage<std::string, std::string, int, MockClock>;

TEST(InMemoryKVStorageTest, SetAndGetValueWithoutTTL) {
    MockClock clock(0);
    TestStorage storage(clock);

    storage.set("key1", "value1", 0); // бесконечный TTL
    auto val = storage.get("key1");
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(val.value(), "value1");
}

TEST(InMemoryKVStorageTest, SetAndGetValueWithTTL_NotExpired) {
    MockClock clock(0);
    TestStorage storage(clock);

    storage.set("key1", "value1", 10); // TTL 10
    auto val = storage.get("key1");
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(val.value(), "value1");
}

TEST(InMemoryKVStorageTest, SetAndGetValueWithTTL_Expired) {
    MockClock clock(0);
    TestStorage storage(clock);

    storage.set("key1", "value1", 5); // TTL 5
    clock.advance(6); // продвигаем время
    auto val = storage.get("key1");
    EXPECT_FALSE(val.has_value()); // запись должна быть недоступна
}

TEST(InMemoryKVStorageTest, RemoveExistingKey) {
    MockClock clock(0);
    TestStorage storage(clock);

    storage.set("key1", "value1", 0);
    bool removed = storage.remove("key1");
    EXPECT_TRUE(removed);

    auto val = storage.get("key1");
    EXPECT_FALSE(val.has_value());
}

TEST(InMemoryKVStorageTest, RemoveNonExistingKey) {
    MockClock clock(0);
    TestStorage storage(clock);

    bool removed = storage.remove("key_not_exist");
    EXPECT_FALSE(removed);
}

TEST(InMemoryKVStorageTest, UpdateTTL) {
    MockClock clock(0);
    TestStorage storage(clock);

    storage.set("key1", "value1", 5);
    clock.advance(3);
    storage.set("key1", "value1_new", 10); // обновляем TTL

    clock.advance(4);
    auto val = storage.get("key1");
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(val.value(), "value1_new");

    clock.advance(7);
    val = storage.get("key1");
    EXPECT_FALSE(val.has_value()); // TTL истёк
}

TEST(InMemoryKVStorageTest, GetManySortedBasic) {
    MockClock clock(0);
    TestStorage storage(clock);

    storage.set("a", "val1", 0);
    storage.set("b", "val2", 0);
    storage.set("d", "val3", 0);
    storage.set("e", "val4", 0);

    auto result = storage.getManySorted("c", 2);
    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0].first, "d");
    EXPECT_EQ(result[0].second, "val3");
    EXPECT_EQ(result[1].first, "e");
    EXPECT_EQ(result[1].second, "val4");
}

TEST(InMemoryKVStorageTest, GetManySortedCountExceedsRemaining) {
    MockClock clock(0);
    TestStorage storage(clock);

    storage.set("a", "val1", 0);
    storage.set("b", "val2", 0);
    storage.set("c", "val3", 0);

    auto result = storage.getManySorted("b", 5);
    ASSERT_EQ(result.size(), 2); // "b" и "c"
    EXPECT_EQ(result[0].first, "b");
    EXPECT_EQ(result[1].first, "c");
}

TEST(InMemoryKVStorageTest, GetManySortedKeyAfterAll) {
    MockClock clock(0);
    TestStorage storage(clock);

    storage.set("a", "val1", 0);
    storage.set("b", "val2", 0);

    auto result = storage.getManySorted("z", 3);
    EXPECT_TRUE(result.empty()); // нет ключей после "z"
}

TEST(InMemoryKVStorageTest, GetManySortedSkipsExpired) {
    MockClock clock(0);
    TestStorage storage(clock);

    storage.set("a", "val1", 5);
    storage.set("b", "val2", 0); // бесконечный TTL
    clock.advance(6); // "a" истёк

    auto result = storage.getManySorted("a", 2);
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].first, "b");
    EXPECT_EQ(result[0].second, "val2");
}

TEST(InMemoryKVStorageTest, GetManySortedEmptyStorage) {
    MockClock clock(0);
    TestStorage storage(clock);

    auto result = storage.getManySorted("a", 3);
    EXPECT_TRUE(result.empty());
}

TEST(InMemoryKVStorageTest, GetManySortedFromFirstKey) {
    MockClock clock(0);
    TestStorage storage(clock);

    storage.set("a", "val1", 0);
    storage.set("b", "val2", 0);
    storage.set("c", "val3", 0);

    auto result = storage.getManySorted("a", 2);
    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0].first, "a");
    EXPECT_EQ(result[1].first, "b");
}

TEST(InMemoryKVStorageTest, GetManySortedCountZero) {
    MockClock clock(0);
    TestStorage storage(clock);

    storage.set("a", "val1", 0);
    storage.set("b", "val2", 0);

    auto result = storage.getManySorted("a", 0);
    EXPECT_TRUE(result.empty());
}

TEST(InMemoryKVStorageTest, GetManySortedAllExpired) {
    MockClock clock(0);
    TestStorage storage(clock);

    storage.set("a", "val1", 5);
    storage.set("b", "val2", 5);
    clock.advance(6); // все TTL истекли

    auto result = storage.getManySorted("a", 5);
    EXPECT_TRUE(result.empty());
}

TEST(InMemoryKVStorageTest, GetManySortedFromExistingKey) {
    MockClock clock(0);
    TestStorage storage(clock);

    storage.set("a", "val1", 0);
    storage.set("b", "val2", 0);
    storage.set("c", "val3", 0);
    storage.set("d", "val4", 0);

    auto result = storage.getManySorted("b", 3);
    ASSERT_EQ(result.size(), 3);
    EXPECT_EQ(result[0].first, "b");
    EXPECT_EQ(result[1].first, "c");
    EXPECT_EQ(result[2].first, "d");
}

TEST(InMemoryKVStorageTest, GetManySortedIgnoresRemovedKeys) {
    MockClock clock(0);
    TestStorage storage(clock);

    storage.set("a", "val1", 0);
    storage.set("b", "val2", 0);
    storage.set("c", "val3", 0);

    // Удаляем ключ "b"
    storage.remove("b");

    auto result = storage.getManySorted("a", 3);
    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0].first, "a");
    EXPECT_EQ(result[1].first, "c");
}

TEST(InMemoryKVStorageTest, GetManySortedIgnoresExpiredKeys) {
    MockClock clock(0);
    TestStorage storage(clock);

    storage.set("a", "val1", 5);
    storage.set("b", "val2", 5);
    storage.set("c", "val3", 0); // бесконечный TTL

    // Продвигаем время, чтобы "a" и "b" истекли
    clock.advance(6);

    auto result = storage.getManySorted("a", 5);
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].first, "c");
}

TEST(InMemoryKVStorageTest, GetManySortedMixRemovedAndExpired) {
    MockClock clock(0);
    TestStorage storage(clock);

    storage.set("a", "val1", 3);
    storage.set("b", "val2", 0);
    storage.set("c", "val3", 2);
    storage.set("d", "val4", 0);

    storage.remove("b");  // удаляем ключ "b"
    clock.advance(4);     // "a" и "c" истекли

    auto result = storage.getManySorted("a", 5);
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].first, "d");
}

TEST(InMemoryKVStorageTest, RemoveOneExpiredEntry_RemovesExpired) {
    MockClock clock(0);
    TestStorage storage(clock);

    storage.set("key1", "value1", 5); // TTL 5
    clock.advance(6);

    auto removed = storage.removeOneExpiredEntry();
    ASSERT_TRUE(removed.has_value());
    EXPECT_EQ(removed->first, "key1");
    EXPECT_EQ(removed->second, "value1");

    // Проверяем, что запись больше не доступна
    auto val = storage.get("key1");
    EXPECT_FALSE(val.has_value());
}

TEST(InMemoryKVStorageTest, RemoveOneExpiredEntry_RemovesAnyExpiredIfMultiple) {
    MockClock clock(0);
    TestStorage storage(clock);

    storage.set("key1", "value1", 5);
    storage.set("key2", "value2", 3);
    storage.set("key3", "value3", 10);
    clock.advance(6);

    auto removed = storage.removeOneExpiredEntry();
    ASSERT_TRUE(removed.has_value());

    // Удалённая запись должна быть либо key1, либо key2
    EXPECT_TRUE(removed->first == "key1" || removed->first == "key2");

    // Убедимся, что удалённая запись больше не доступна
    auto val = storage.get(removed->first);
    EXPECT_FALSE(val.has_value());
}

TEST(InMemoryKVStorageTest, RemoveOneExpiredEntry_NoExpiredReturnsNullopt) {
    MockClock clock(0);
    TestStorage storage(clock);

    storage.set("key1", "value1", 10);
    storage.set("key2", "value2", 15);

    auto removed = storage.removeOneExpiredEntry();
    EXPECT_FALSE(removed.has_value());
}

TEST(InMemoryKVStorageTest, RemoveOneExpiredEntry_RemoveAllExpiredSequentially) {
    MockClock clock(0);
    TestStorage storage(clock);

    storage.set("a", "va", 1);
    storage.set("b", "vb", 2);
    storage.set("c", "vc", 3);

    clock.advance(5);

    // Удаляем до тех пор, пока есть протухшие
    int removedCount = 0;
    while (true) {
        auto removed = storage.removeOneExpiredEntry();
        if (!removed.has_value()) break;
        removedCount++;
    }

    EXPECT_EQ(removedCount, 3); // Все три должны быть удалены
}

TEST(InMemoryKVStorageTest, RemoveOneExpiredEntry_MixedTTLs) {
    MockClock clock(0);
    TestStorage storage(clock);

    storage.set("x", "vx", 1); // протухнет
    storage.set("y", "vy", 10); // живёт
    clock.advance(2);

    auto removed = storage.removeOneExpiredEntry();
    ASSERT_TRUE(removed.has_value());
    EXPECT_EQ(removed->first, "x");

    // "y" всё ещё доступен
    auto val = storage.get("y");
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(val.value(), "vy");

    // Повторный вызов — уже nullopt
    removed = storage.removeOneExpiredEntry();
    EXPECT_FALSE(removed.has_value());
}

TEST(InMemoryKVStorageTest, RemoveOneExpiredEntry_EmptyStorage) {
    MockClock clock(0);
    TestStorage storage(clock);

    auto removed = storage.removeOneExpiredEntry();
    EXPECT_FALSE(removed.has_value());
}

TEST(InMemoryKVStorageTest, RemoveOneExpiredEntry_ExpiredNotReturnedTwice) {
    MockClock clock(0);
    TestStorage storage(clock);

    storage.set("k", "vk", 1);
    clock.advance(5);

    auto removed1 = storage.removeOneExpiredEntry();
    ASSERT_TRUE(removed1.has_value());
    EXPECT_EQ(removed1->first, "k");

    auto removed2 = storage.removeOneExpiredEntry();
    EXPECT_FALSE(removed2.has_value()); // Уже удалена
}

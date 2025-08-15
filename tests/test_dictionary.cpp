#include <gtest/gtest.h>
#include <KVStorage.h>
#include <string>
#include <optional>

struct ConflictHash {
    size_t operator()(const std::string& key) const {
        return 1;
    }
};

template<typename Dict>
void FillDictionary(Dict& dict, int count, const std::string& prefix) {
    for (int i = 0; i < count; ++i) {
        dict.set(make_shared<std::string>(prefix + std::to_string(i)),
                 make_unique<std::string>("val_" + std::to_string(i)));
    }
}

template<typename Dict>
void CheckValues(Dict& dict, int count, const std::string& prefix) {
    for (int i = 0; i < count; ++i) {
        auto val = dict.get(prefix + std::to_string(i));
        ASSERT_TRUE(val.has_value());
        EXPECT_EQ(*val, "val_" + std::to_string(i));
    }
}

TEST(DictionaryTest, ConstuctWithBasicSize) {
    EXPECT_NO_THROW((Dictionary<std::string, std::string>()));
}

TEST(DictionaryTest, ValidateSizeDuringConstructWithBasicSize) {
    Dictionary<std::string, std::string> dict;

    ASSERT_EQ(dict.size(), 0);
}

TEST(DictionaryTest, ConstrucWithNotBasicSize) {
    EXPECT_NO_THROW((Dictionary<std::string, std::string>(10)));
}

TEST(DictionaryTest, ValidateSizeDuringConstructWithNotBasicSize) {
    Dictionary<std::string, std::string> dict(10);

    ASSERT_EQ(dict.size(), 0);
}

TEST(DictionaryTest, EmptyAfterConstruction) {
    Dictionary<std::string, std::string> dict(10);

    ASSERT_TRUE(dict.empty());
}

TEST(DictionaryTest, NotEmptyAfterSetKeyValue) {
    Dictionary<std::string, std::string> dict;

    FillDictionary(dict, 1, "val");

    ASSERT_FALSE(dict.empty());
}

TEST(DictionaryTest, ValidateValueAfterSetKeyValue) {
    Dictionary<std::string, std::string> dict;

    FillDictionary(dict, 1, "val");

    CheckValues(dict, 1, "val");
}

TEST(DictionaryTest, ValidateValueAfterSetConflictKeyValue) {
    Dictionary<std::string, std::string, ConflictHash> dict;

    FillDictionary(dict, 2, "val");

    CheckValues(dict,2, "val");
    ASSERT_EQ(dict.size(), 2);
}

TEST(DictionaryTest, ValidateValueAfterResetKeyValue){
    std::string key = "key";
    std::string value1 = "value1", value2 = "value2";
    Dictionary<std::string, std::string> dict;

    dict.set(std::make_shared<std::string>(key), std::make_shared<std::string>(value1));
    dict.set(std::make_shared<std::string>(key), std::make_shared<std::string>(value2));


    ASSERT_EQ(dict.get(key), value2);
    ASSERT_EQ(dict.size(), 1);
}

TEST(DictionaryTest, EmptyAfterRemoveKeyValue){

    std::string key = "key";
    std::string value1 = "value1";
    Dictionary<std::string, std::string> dict;

    dict.set(std::make_shared<std::string>(key), std::make_shared<std::string>(value1));
    dict.remove(key);

    ASSERT_TRUE(dict.empty());
}

TEST(DictionaryTest, NotEmptyAfterRemoveKeyValueWithConflict){
    std::string key1 = "key1", key2 = "key2";
    std::string value1 = "value1", value2 = "value2";

    Dictionary<std::string, std::string, ConflictHash> dict;

    dict.set(std::make_shared<std::string>(key1), std::make_shared<std::string>(value1));
    dict.set(std::make_shared<std::string>(key2), std::make_shared<std::string>(value2));
    dict.remove(key1);


    ASSERT_EQ(dict.get(key2), value2);
    ASSERT_FALSE(dict.empty());
}

TEST(DictionaryTest, NotEmptyAfterRemoveKeyValueWithoutConflict){
    std::string key1 = "key1", key2 = "key2";
    std::string value1 = "value1", value2 = "value2";

    Dictionary<std::string, std::string> dict;

    dict.set(std::make_shared<std::string>(key1), std::make_shared<std::string>(value1));
    dict.set(std::make_shared<std::string>(key2), std::make_shared<std::string>(value2));
    dict.remove(key1);


    ASSERT_EQ(dict.get(key2), value2);
    ASSERT_FALSE(dict.empty());
}


TEST(DictionaryAdvancedTest, RehashKeepsDataWhenGrowing) {
    Dictionary<std::string, std::string> dict(4);

    FillDictionary(dict, 10, "k");
    CheckValues(dict, 10, "k");

    EXPECT_GE(dict.size(), 10);
}

TEST(DictionaryAdvancedTest, RehashKeepsDataWhenShrinking) {
    Dictionary<std::string, std::string> dict(16);
    FillDictionary(dict, 14, "key");

    for (int i = 0; i < 13; ++i) {
        dict.remove("key" + std::to_string(i));
    }

    auto val = dict.get("key13");
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, "val_13");
}

TEST(DictionaryAdvancedTest, RehashDuringSimultaneousInsertAndRemove) {
    Dictionary<std::string, std::string> dict(4);
    FillDictionary(dict, 20, "old");

    for (int i = 0; i < 20; ++i) {
        dict.set(make_shared<std::string>("new" + std::to_string(i)),
                 make_unique<std::string>("val_" + std::to_string(i)));
        dict.remove("old" + std::to_string(i));
    }

    CheckValues(dict, 20, "new");

    for (int i = 0; i < 20; ++i) {
        EXPECT_FALSE(dict.get("old" + std::to_string(i)).has_value());
    }
}

TEST(DictionaryAdvancedTest, OverwritingValues) {
    Dictionary<std::string, std::string> dict;
    dict.set(std::make_shared<std::string>("dup"), std::make_unique<std::string>("first"));
    dict.set(std::make_shared<std::string>("dup"), std::make_unique<std::string>("second"));

    auto val = dict.get("dup");
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, "second");
}

TEST(DictionaryAdvancedTest, EmptyDictionaryWithRehashInProgress) {
    Dictionary<std::string, std::string> dict(4);

    FillDictionary(dict, 10, "tmp");
    for (int i = 0; i < 10; ++i) {
        dict.remove("tmp" + std::to_string(i));
    }

    EXPECT_TRUE(dict.empty());
    EXPECT_EQ(dict.size(), 0);

    dict.set(std::make_shared<std::string>("x"), std::make_unique<std::string>("y"));
    auto val = dict.get("x");
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, "y");
}

TEST(DictionaryAdvancedTest, FillFactorMatchesExpected) {
    Dictionary<std::string, std::string> dict(8);
    FillDictionary(dict, 6, "ff");

    double expectedFillFactor = static_cast<double>(dict.size()) / 8.0;
    EXPECT_GT(expectedFillFactor, 0.0);
    EXPECT_LT(expectedFillFactor, 1.0);
}

TEST(DictionaryFindTest, FindExistingKeyReturnsCorrectPair) {
    Dictionary<std::string, std::string> dict;
    auto keyPtr = std::make_shared<std::string>("alpha");
    auto valPtr = std::make_unique<std::string>("beta");

    dict.set(keyPtr, std::move(valPtr));

    auto result = dict.find("alpha");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result->first, "alpha");
    EXPECT_EQ(*result->second, "beta");
}

TEST(DictionaryFindTest, FindNonExistingKeyReturnsNullopt) {
    Dictionary<std::string, std::string> dict;
    dict.set(std::make_shared<std::string>("a"), std::make_unique<std::string>("b"));

    auto result = dict.find("zzz");
    EXPECT_FALSE(result.has_value());
}

TEST(DictionaryFindTest, FindDuringRehashFromAlternativeDictionary) {
    Dictionary<std::string, std::string> dict(4);

    // Заполняем словарь, чтобы вызвать rehash на рост
    for (int i = 0; i < 20; ++i) {
        dict.set(make_shared<std::string>("k" + std::to_string(i)),
                 make_unique<std::string>("v" + std::to_string(i)));
    }

    // Принудительно переводим в состояние rehash (сработает при следующем preCommandActions)
    dict.set(std::make_shared<std::string>("rehash_trigger"), std::make_unique<std::string>("value_trigger"));

    // Теперь добавляем ключ, который попадёт в alternativeDictionary
    dict.set(std::make_shared<std::string>("special_key"), std::make_unique<std::string>("special_value"));

    // Должен найтись через alternativeDictionary
    auto result = dict.find("special_key");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result->first, "special_key");
    EXPECT_EQ(*result->second, "special_value");
}

TEST(DictionaryFindTest, ReturnedSharedPointersModifyOriginal) {
    Dictionary<std::string, std::string> dict;
    dict.set(std::make_shared<std::string>("x"), std::make_unique<std::string>("y"));

    auto result = dict.find("x");
    ASSERT_TRUE(result.has_value());

    // Меняем значение через shared_ptr
    *result->second = "modified";
    auto check = dict.get("x");
    ASSERT_TRUE(check.has_value());
    EXPECT_EQ(*check, "modified");
}

TEST(DictionaryFindTest, FindOnEmptyDictionaryReturnsNullopt) {
    Dictionary<std::string, std::string> dict;
    EXPECT_FALSE(dict.find("anything").has_value());
}
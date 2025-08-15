#ifndef KVSTORAGE_INMEMORYKVSTORAGE_H
#define KVSTORAGE_INMEMORYKVSTORAGE_H

#include "IKVStorage.h"
#include "dict/Dictionary.h"

#include <set>
#include <queue>
#include <memory>

/**
 * @brief Хранилище ключ-значение в памяти.
 *
 * Предоставляет хранение пар ключ-значение с поддержкой опционального TTL (время жизни записи).
 * Поддерживаются базовые операции: добавление, получение, удаление и получение нескольких записей в отсортированном виде.
 *
 * @tparam Key Тип ключей.
 * @tparam Value Тип значений.
 * @tparam Ttl Тип для хранения времени жизни записи.
 * @tparam Clock Тип часов, используемых для определения текущего времени.
 */
template<typename Key, typename Value, typename Ttl, typename Clock>
class InMemoryKVStorage : public IKVStorage<Key, Value, Ttl> {
private:

    /**
     * @brief Компаратор для shared_ptr ключей.
     *
     * Используется для сортировки ключей, хранящихся как shared_ptr в std::set.
     */
    struct ComparatorPtrKey {
        bool operator()(const std::shared_ptr<Key> &lhs,
                        const std::shared_ptr<Key> &rhs) const {
            return *lhs < *rhs;
        }
    };

    Dictionary<Key, Value> _keyValueDictionary;
    Dictionary<Key, Ttl> _keyTtlDictionary;
    std::set<std::shared_ptr<Key>, ComparatorPtrKey> _setSortedKeyPtr;
    std::set<std::pair<std::shared_ptr<Key>, std::shared_ptr<Ttl>>> _setSortedTtlPtr;

    Clock _clock;
protected:

    /**
     * @brief Проверяет существование ключа в хранилище.
     *
     * @param key Ключ для проверки.
     * @return true если ключ существует, false иначе.
     */
    bool isExist(Key key) {
        auto value = _keyValueDictionary.get(key);
        return value.has_value();
    }

    /**
     * @brief Проверяет, истек ли срок жизни ключа.
     *
     * @param key Ключ для проверки.
     * @return true если ключ существует и его TTL истек, false иначе.
     */
    bool isExpired(Key key) {
        auto ttl = _keyTtlDictionary.get(key);
        if (ttl.has_value()) {
            return ttl.value() < _clock.now();
        }

        return false;
    }

public:

    /**
     * @brief Создает пустое хранилище с указанными часами.
     *
     * @param clock Экземпляр часов, используемый для управления TTL.
     */
    explicit InMemoryKVStorage(Clock clock = Clock()) : _clock(clock) {
    }

    /**
     * @brief Создает хранилище и инициализирует его списком записей.
     *
     * @param entries Span кортежей (Key, Value, Ttl) для заполнения хранилища.
     */
    explicit InMemoryKVStorage(std::span<std::tuple<Key, Value, Ttl>> entries) {
        for (auto &[key, value, ttl]: entries) {
            _set(key, value, ttl);
        }
    }

    /**
     * @brief Добавляет или обновляет запись с TTL.
     *
     * Если TTL равен 0, запись считается бессрочной.
     *
     * @param key Ключ записи.
     * @param value Значение для хранения.
     * @param ttl Время жизни записи в секундах; 0 означает бессрочно.
     */
    virtual void set(Key key, Value value, Ttl ttl) override {
        auto pKey = std::make_shared<Key>(key);
        auto pValue = std::make_shared<Value>(value);
        auto pTtl = std::make_shared<Ttl>(ttl);

        _keyValueDictionary.set(pKey, pValue);
        _setSortedKeyPtr.insert(pKey);
        if (ttl != 0) {
            *pTtl += _clock.now();
            _keyTtlDictionary.set(pKey, pTtl);
            _setSortedTtlPtr.insert({pKey, pTtl});
        }
    }

    /**
     * @brief Удаляет запись по ключу.
     *
     * @param key Ключ записи для удаления.
     * @return true если запись существовала и была удалена, false иначе.
     */
    bool remove(Key key) override {
        if (!isExist(key))
            return false;

        auto pKey = std::make_shared<Key>(key);
        auto oTtl = _keyValueDictionary.get(key);

        _keyValueDictionary.remove(key);
        auto it = _setSortedKeyPtr.find(pKey);
        if (it != _setSortedKeyPtr.end()) {
            _setSortedKeyPtr.erase(it);
        }

        if (oTtl.has_value()) {
            _keyTtlDictionary.remove(key);

            auto it = std::find_if(
                    _setSortedTtlPtr.begin(),
                    _setSortedTtlPtr.end(),
                    [&](const auto &pair) {
                        return *pair.first == key;
                    }
            );

            if (it != _setSortedTtlPtr.end()) {
                _setSortedTtlPtr.erase(it);
            }
        }

        return true;
    }

    /**
     * @brief Получает значение по ключу.
     *
     * @param key Ключ записи.
     * @return std::optional с значением, если ключ существует и не истек, std::nullopt иначе.
     */
    std::optional<Value> get(Key key) override {
        if (!isExist(key) || isExpired(key))
            return std::nullopt;

        return _keyValueDictionary.get(key).value();
    }

    /**
     * @brief Получает несколько записей, начиная с указанного ключа, в лексикографическом порядке.
     *
     * @param key Ключ, с которого начинать поиск (включительно).
     * @param count Максимальное количество записей для получения.
     * @return Вектор пар ключ-значение в лексикографическом порядке.
     */
    std::vector<std::pair<Key, Value>> getManySorted(std::string_view key, uint32_t count) override {
        std::vector<std::pair<Key, Value>> result;
        auto it = _setSortedKeyPtr.lower_bound(std::make_shared<std::string>(key));

        for (size_t i = 0; i < count && it != _setSortedKeyPtr.end(); ++i, ++it) {
            if (isExpired(**it)) {
                --i;
                continue;
            }

            result.push_back({**it, get(**it).value()});
        }
        return result;
    }

    /**
     * @brief Удаляет одну протухшую запись из хранилища.
     *
     * Если несколько записей истекли, может быть удалена любая из них.
     *
     * @return std::optional с удаленной парой ключ-значение, std::nullopt если нет протухших записей.
     */
    std::optional<std::pair<Key, Value>> removeOneExpiredEntry() override {
        auto it = _setSortedTtlPtr.begin();
        if (it != _setSortedTtlPtr.end() && *(it->second) < _clock.now()) {
            Key key = *(it->first);
            Value value = _keyValueDictionary.get(key).value();
            remove(key);
            return std::make_pair(key, value);
        }
        return std::nullopt;
    }
};

#endif //KVSTORAGE_INMEMORYKVSTORAGE_H

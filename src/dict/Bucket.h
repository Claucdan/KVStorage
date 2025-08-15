#ifndef KVSTORAGE_BUCKET_H
#define KVSTORAGE_BUCKET_H

#include <memory>

/**
 * @brief Бакет для хранения ключ-значение.
 *
 * Используется для реализации хэш-таблиц с отдельными цепочками
 * (chaining) для разрешения коллизий.
 *
 * @tparam Key Тип ключа.
 * @tparam Value Тип значения.
 */
template<typename Key, typename Value>
struct Bucket {
    std::shared_ptr<Key> key;
    std::shared_ptr<Value> value;
    Bucket<Key, Value> *next = nullptr;

    Bucket(std::shared_ptr<Key> key, std::shared_ptr<Value> value)
            : key(std::move(key)), value(std::move(value)) {}
};

#endif //KVSTORAGE_BUCKET_H

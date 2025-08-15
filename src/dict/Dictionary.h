#ifndef KVSTORAGE_DICTIONARY_H
#define KVSTORAGE_DICTIONARY_H

#include "Bucket.h"

#include <memory>
#include <cstddef>
#include <optional>
#include <functional>

#include <spdlog/spdlog.h>


constexpr size_t BASIC_DICTIONARY_SIZE = 8;
constexpr size_t DICTIONARY_REHASH_BUCKET_COUNT = 32;
constexpr double DICTIONARY_LOW_LIMIT_REHASH = 0.01f;
constexpr double DICTIONARY_HIGH_LIMIT_REHASH = 1.2f;

/**
 * @class Dictionary
 * @brief Шаблонный класс хеш-таблицы с открытым хешированием (цепочками) и поддержкой
 *        динамического изменения размера (rehash) при изменении коэффициента заполнения.
 *
 * @tparam Key  Тип ключа
 * @tparam Value Тип значения
 * @tparam Hash  Функция хеширования (по умолчанию std::hash<Key>)
 *
 * Класс реализует ассоциативное хранилище пар ключ-значение с возможностью
 * добавления, удаления и поиска элементов. Внутренняя структура — массив
 * бакетов (цепочек), которые хранят элементы с одинаковыми хешами.
 *
 * Поддерживается автоматический rehash в сторону увеличения или уменьшения
 * размера таблицы при выходе коэффициента заполнения за допустимые границы.
 *
 * @note Не копируемый (copy constructor и operator= удалены).
 */
template<typename Key, typename Value, typename Hash = std::hash<Key>>
class Dictionary {
private:
    Bucket<Key, Value> **_dictionary;
    std::size_t _dictionarySize;
    std::size_t _dictionaryUsed = 0;

    Bucket<Key, Value> **_alternativeDictionary;
    std::size_t _alternativeDictionarySize = 0;

    Hash _hashFunction;
    std::size_t _rehashIndex = -1;


    /*---------------------------------Common methods------------------------------------*/

    /**
     * @brief Вычисляет индекс бакета по ключу.
     *
     * @param key Ключ для хеширования.
     * @param dictionarySize Размер массива бакетов.
     * @return Индекс бакета.
     */
    std::size_t getBucketIndex(const Key &key, const std::size_t &dictionarySize) {
        std::size_t bucketIndex = _hashFunction(key) % dictionarySize;
        spdlog::debug("Bucket index for key [{}] is [{}]", key, bucketIndex);
        return bucketIndex;
    }

    /**
     * @brief Вычисляет коэффициент заполнения словаря.
     *
     * @return Доля занятого пространства (dictionaryUsed / dictionarySize).
     */
    double fillFactor() {
        return static_cast<double>(_dictionaryUsed) / _dictionarySize;
    }

    /*----------------------------Commands with dictionaries-----------------------------*/

    /**
     * @brief Внутренний метод добавления пары (ключ, значение) в указанный словарь бакетов.
     */
    Bucket<Key, Value> *_set(
            std::shared_ptr<Key> key,
            std::shared_ptr<Value> value,
            Bucket<Key, Value> **pDictionary,
            std::size_t dictionarySize) {
        std::size_t index = getBucketIndex(*key, dictionarySize);

        if (pDictionary[index] == nullptr) {
            spdlog::debug("Creat new bucket with: key = {}, value = {} ", *key, *value);
            pDictionary[index] = new Bucket{std::move(key), std::move(value)};
            ++_dictionaryUsed;
            return pDictionary[index];
        }

        Bucket<Key, Value> *pBucket = pDictionary[index];
        while (pBucket != nullptr && *(pBucket->key) != *key) {
            pBucket = pBucket->next;
        }

        if (pBucket != nullptr) {
            pBucket->value = std::move(value);
            return pBucket;
        }

        Bucket<Key, Value> *tmp = new Bucket{std::move(key), std::move(value)};
        tmp->next = _dictionary[index];
        pDictionary[index] = tmp;
        ++_dictionaryUsed;
        return tmp;
    }

    /**
     * @brief Внутренний метод поиска бакета по ключу в указанном словаре.
     */
    std::optional<Bucket<Key, Value> *> _get(Key key, Bucket<Key, Value> **pDictionary, std::size_t dictionarySize) {
        std::size_t index = getBucketIndex(key, dictionarySize);

        Bucket<Key, Value> *tmp = pDictionary[index];
        while (tmp != nullptr && *(tmp->key) != key) {
            tmp = tmp->next;
        }

        if (tmp == nullptr) {
            spdlog::debug("Bucket was not found with: key = {}", key);
            return std::nullopt;
        }

        spdlog::debug("Bucket was found with: key = {}, value = {}", key, *tmp->value);
        return std::optional<Bucket<Key, Value> *>(tmp);
    }

    /**
     * @brief Внутренний метод удаления элемента по ключу в указанном словаре.
     */
    bool _remove(Key key, Bucket<Key, Value> **pDictionary, std::size_t dictionarySize) {
        std::size_t index = getBucketIndex(key, dictionarySize);

        Bucket<Key, Value> *current = pDictionary[index];
        Bucket<Key, Value> *prev = nullptr;

        while (current != nullptr && *current->key != key) {
            prev = current;
            current = current->next;
        }

        if (current == nullptr)
            return false;

        if (prev == nullptr) {
            pDictionary[index] = current->next;
        } else {
            prev->next = current->next;
        }

        spdlog::debug("Delete bucket with: key = {}, value = {}", *current->key, *current->value);
        delete current;
        --_dictionaryUsed;
        return true;
    }

    /*------------------------Commands for rehash dictionary-----------------------------*/

    /**
     * @brief Проверяет, нужно ли выполнять rehash на основе коэффициента заполнения.
     */
    bool isNeedRehash() {
        return _dictionarySize > BASIC_DICTIONARY_SIZE && _dictionaryUsed != 0 &&
               (fillFactor() < DICTIONARY_LOW_LIMIT_REHASH || fillFactor() > DICTIONARY_HIGH_LIMIT_REHASH);
    }

    /**
     * @brief Проверяет, выполняется ли в данный момент rehash.
     */
    bool isRehashing() {
        return _rehashIndex != -1;
    }

    /**
     * @brief Подготавливает вспомогательный словарь для rehash.
     */
    void prepareRehash() {
        if (fillFactor() < DICTIONARY_LOW_LIMIT_REHASH) {
            _alternativeDictionarySize = _dictionarySize / 2;
        } else {
            _alternativeDictionarySize = _dictionarySize * 2;
        }

        _alternativeDictionary = new Bucket<Key, Value> *[_alternativeDictionarySize](nullptr);
        _rehashIndex = 0;
    }

    /**
     * @brief Переносит часть бакетов из основного словаря в процесс rehash.
     */
    void rehash() {
        for (std::size_t i = 0;
             i < DICTIONARY_REHASH_BUCKET_COUNT && _rehashIndex < _dictionarySize;
             ++i, ++_rehashIndex) {

            if (_dictionary[_rehashIndex] == nullptr) {
                continue;
            }

            Bucket<Key, Value> *tmp = _dictionary[_rehashIndex];
            Bucket<Key, Value> *next;
            _dictionary[_rehashIndex] = nullptr;

            while (tmp != nullptr) {
                next = tmp->next;
                delete tmp;
                tmp = next;
            }
        }

        if (_rehashIndex == _dictionarySize) {
            _rehashIndex = -1;
            delete[] _dictionary;
            _dictionary = _alternativeDictionary;
            _alternativeDictionary = nullptr;
        }
    }

    /**
     * @brief Выполняет необходимые действия перед основной командой (set, get, remove).
     *        Если идёт rehash — продолжает его, иначе проверяет необходимость rehash.
     */
    void preCommandActions() {
        if (isRehashing()) {
            rehash();
            return;
        }

        if (isNeedRehash()) {
            prepareRehash();
            rehash();
        }
    }

public:
    Dictionary(const Dictionary &) = delete;

    Dictionary &operator=(const Dictionary &) = delete;

    /**
     * @brief Конструктор словаря.
     * @param size Начальный размер массива бакетов (по умолчанию BASIC_DICTIONARY_SIZE).
     */
    explicit Dictionary(std::size_t size = BASIC_DICTIONARY_SIZE) :
            _dictionary(new Bucket<Key, Value> *[size]{nullptr}),
            _dictionarySize(size),
            _alternativeDictionary(nullptr) {
        spdlog::debug("Dictionary has been created with: size = {};", size);
    }

    /**
     * @brief Деструктор словаря.
     *        Освобождает память, занятую всеми бакетами.
     */
    ~Dictionary() {
        spdlog::debug("Start of destroying dictionary with: size = {}", _dictionarySize);
        for (std::size_t i = 0; i < _dictionarySize; ++i) {
            if (_dictionary[i] == nullptr) {
                continue;
            }

            Bucket<Key, Value> *nextBucket;

            do {
                nextBucket = _dictionary[i]->next;
                delete _dictionary[i];
                _dictionary[i] = nextBucket;
            } while (_dictionary[i] != nullptr);
        }
        spdlog::debug("End of destroying dictionary with: size = {}", _dictionarySize);
    }

    /**
     * @brief Записывает пару (ключ, значение) в словарь.
     *        Если ключ уже существует — обновляет значение.
     *
     * @param key   Умный указатель на ключ.
     * @param value Умный указатель на значение.
     */
    void set(std::shared_ptr<Key> key, std::shared_ptr<Value> value) {
        preCommandActions();

        if (isRehashing()) {
            _set(std::move(key), std::move(value), _alternativeDictionary, _alternativeDictionarySize);
            _remove(*key, _dictionary, _dictionarySize);
            return;
        }

        _set(std::move(key), std::move(value), _dictionary, _dictionarySize);
    }

    /**
     * @brief Удаляет элемент по ключу.
     *
     * @param key Ключ для удаления.
     * @return true  если элемент был найден и удалён.
     * @return false если элемент не найден.
     */
    bool remove(Key key) {
        preCommandActions();

        if (isRehashing() && _remove(key, _alternativeDictionary, _alternativeDictionarySize)) {
            return true;
        }

        return _remove(key, _dictionary, _dictionarySize);
    }

    /**
     * @brief Получает значение по ключу.
     *
     * @param key Ключ для поиска.
     * @return std::optional<Value> Значение, если найдено, иначе std::nullopt.
     */
    std::optional<Value> get(Key key) {
        preCommandActions();

        if (isRehashing()) {
            std::optional<Bucket<Key, Value> *> alternativeResult = _get(key, _alternativeDictionary,
                                                                         _alternativeDictionarySize);
            if (alternativeResult.has_value())
                return std::optional<Value>(*(alternativeResult.value()->value));
        }

        std::optional<Bucket<Key, Value> *> result = _get(key, _dictionary, _dictionarySize);
        return result.has_value() ? std::optional<Value>(*(result.value()->value)) : std::nullopt;
    }


    /**
     * @brief Получает ключ и значение по ключу.
     *
     * @param key Ключ для поиска.
     * @return std::optional<std::pair<std::shared_ptr<Key>, std::shared_ptr<Value>>>
     *         Пара ключ-значение, если найдено, иначе std::nullopt.
     */
    std::optional<std::pair<std::shared_ptr<Key>, std::shared_ptr<Value>>> find(Key key) {
        preCommandActions();

        if (isRehashing()) {
            std::optional<Bucket<Key, Value> *> alternativeResult = _get(key, _alternativeDictionary,
                                                                         _alternativeDictionarySize);
            if (alternativeResult.has_value()) {
                std::optional<std::pair<std::shared_ptr<Key>, std::shared_ptr<Value>>> result =
                        std::optional<std::pair<std::shared_ptr<Key>, std::shared_ptr<Value>>>(
                                std::make_pair(
                                        alternativeResult.value()->key, alternativeResult.value()->value
                                )
                        );
                return result;
            }
        }

        std::optional<Bucket<Key, Value> *> result = _get(key, _dictionary, _dictionarySize);
        if (!result.has_value())
            return std::nullopt;

        return std::optional<std::pair<std::shared_ptr<Key>, std::shared_ptr<Value>>>(
                std::make_pair(
                        result.value()->key, result.value()->value
                )
        );
    }


    /**
     * @brief Проверяет, пуст ли словарь.
     *
     * @return true  если словарь пуст.
     * @return false если содержит хотя бы один элемент.
     */
    bool empty() {
        return _dictionaryUsed == 0;
    }

    /**
     * @brief Возвращает количество элементов в словаре.
     *
     * @return Размер словаря (число элементов).
     */
    std::size_t size() {
        return _dictionaryUsed;
    }
};

#endif //KVSTORAGE_DICTIONARY_H
#ifndef KVSTORAGE_IKVSTORAGE_H
#define KVSTORAGE_IKVSTORAGE_H

#include <string>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>
#include <future>

/**
 * @brief Интерфейс хранилища ключ-значение.
 *
 * Определяет базовые операции для работы с KV-хранилищем:
 * добавление, удаление, получение значений, получение нескольких записей в отсортированном виде и удаление протухших записей.
 *
 * @tparam Key Тип ключей.
 * @tparam Value Тип значений.
 * @tparam Ttl Тип для хранения времени жизни записи.
 */
template<typename Key, typename Value, typename Ttl>
class IKVStorage {
public:
    virtual ~IKVStorage() = default;

    /**
     * @brief Добавляет или обновляет запись с TTL.
     *
     * Если TTL равен 0, запись считается бессрочной.
     * Метод безусловно обновляет существующую запись.
     *
     * @param key Ключ записи.
     * @param value Значение для хранения.
     * @param ttl Время жизни записи в секундах; 0 означает бессрочно.
     */
    virtual void set(Key key, Value value, Ttl ttl) = 0;

    /**
     * @brief Удаляет запись по ключу.
     *
     * @param key Ключ записи для удаления.
     * @return true если запись существовала и была удалена, false если ключ отсутствовал.
     */
    virtual bool remove(Key key) = 0;

    /**
     * @brief Получает значение по ключу.
     *
     * @param key Ключ записи.
     * @return std::optional с значением, если ключ существует и не истек, std::nullopt иначе.
     */
    virtual std::optional<Value> get(Key key) = 0;

    /**
     * @brief Получает несколько записей, начиная с указанного ключа, в лексикографическом порядке.
     *
     * @param key Ключ, с которого начинать поиск (включительно).
     * @param count Максимальное количество записей для получения.
     * @return Вектор пар ключ-значение в лексикографическом порядке.
     *
     * Пример:
     * ("a", "val1"), ("b", "val2"), ("d", "val3"), ("e", "val4")
     * getManySorted("c", 2) -> ("d", "val3"), ("e", "val4")
     */
    virtual std::vector<std::pair<Key, Value>> getManySorted(std::string_view key, uint32_t count) = 0;

    /**
     * @brief Удаляет одну протухшую запись из хранилища.
     *
     * Если несколько записей истекли, может быть удалена любая из них.
     *
     * @return std::optional с удаленной парой ключ-значение, std::nullopt если нет протухших записей.
     */
    virtual std::optional<std::pair<Key, Value>> removeOneExpiredEntry() = 0;
};

#endif //KVSTORAGE_IKVSTORAGE_H
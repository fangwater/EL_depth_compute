#include <iostream>
#include <vector>
#include <bitset>
#include <boost/intrusive/set.hpp>
#include <array>

template<typename T, size_t PoolSize>
class MemoryPool {
private:
    std::vector<T> pool;
    std::bitset<PoolSize> occupied;
    size_t cursor;

    // Helper function to find an available slot
    size_t findAvailableSlot() {
        for (size_t i = 0; i < PoolSize; ++i) {
            if (!occupied[i]) return i;
        }
        return PoolSize; // Indicate that no slot is available
    }

public:
    MemoryPool() : pool(PoolSize), cursor(0) {}

    T* alloc() {
        if (cursor < PoolSize) {
            occupied[cursor] = true;
            return &pool[cursor++];
        } else {
            // When cursor reaches its limit, find an available slot
            size_t availableSlot = findAvailableSlot();
            if (availableSlot == PoolSize) return nullptr; // No available memory

            occupied[availableSlot] = true;
            return &pool[availableSlot];
        }
    }

    void dealloc(T* ptr) {
        size_t index = std::distance(pool.data(), ptr);
        if (index >= 0 && index < PoolSize) {
            occupied[index] = false;
        }
    }
};



#include <iostream>
#include <boost/intrusive/set.hpp>

class PriceLevel : public boost::intrusive::set_base_hook<> {
public:
    int32_t price;
    int32_t quantity;
    int64_t vol;
    PriceLevel(int32_t p) : price(p), quantity(0),vol(0) {}
    PriceLevel(int32_t pr, int32_t qt) : price(pr), quantity(qt),vol(pr * qt) {}
};

// 买单的比较函数：降序
struct BidCompare {
    bool operator() (const PriceLevel &a, const PriceLevel &b) const {
        return a.price > b.price;
    }
};

// 卖单的比较函数：升序
struct AskCompare {
    bool operator() (const PriceLevel &a, const PriceLevel &b) const {
        return a.price < b.price;
    }
};

namespace boost {
    namespace intrusive {
        template<>
        struct key_of_value<PriceLevel> {
            const int32_t& operator()(const PriceLevel& value) const {
                return value.price;
            }
        };
    }
}

typedef boost::intrusive::set<PriceLevel, boost::intrusive::compare<BidCompare>> BidSet;
typedef boost::intrusive::set<PriceLevel, boost::intrusive::compare<AskCompare>> AskSet;

template<std::size_t N>
struct depth{
    using level_info = std::pair<int32_t,int32_t>;
    std::array<level_info,N> level_infos;
};

class MergeOrderBook{
public:
private:
    BidSet bid_side;
    AskSet ask_side;
private:
    PriceLevel* find_or_insert_price_level(BidSet &side, int32_t price, int32_t quantity) {
        auto it = side.find(price);
        if (it == side.end()) {
            PriceLevel* newLevel = new PriceLevel(price, 0);
            side.insert(*newLevel);
            return newLevel;
        }
        return &(*it);
    }
    
    PriceLevel* find_or_insert_price_level(AskSet &side, int32_t price, int32_t quantity) {
        auto it = side.find(price);
        if (it == side.end()) {
            PriceLevel* newLevel = new PriceLevel(price, 0);
            side.insert(*newLevel);
            return newLevel;
        }
        return &(*it);
    }
public:
    inline int32_t best_bid(){
        return bid_side.begin()->price;
    }

    inline int32_t best_ask(){
        return ask_side.begin()->price;
    }

    bool order_add(int32_t price, int32_t quantity, bool is_bid) {
        //增加新的报价
        if (is_bid) {
            //执行买单，逐个迭代成交，直到价格不满足，或者quantity耗尽
            while(!ask_side.empty() && quantity > 0 && best_ask() <= price){
                PriceLevel* ask_level = &(*ask_side.begin());
                if (ask_level->quantity <= quantity) {
                    quantity -= ask_level->quantity;
                    ask_side.erase(ask_side.begin());
                }else {
                    ask_level->quantity -= quantity;
                    ask_level->vol = ask_level->price * ask_level->quantity;
                    quantity = 0;
                }
            }
            if (quantity > 0) {
                PriceLevel* level = find_or_insert_price_level(bid_side, price, quantity);
                level->quantity += quantity;
                level->vol = level->price * level->quantity;
            }
        } else {
            while(!bid_side.empty() && quantity > 0 && best_bid() >= price){
                PriceLevel* bid_level = &(*bid_side.rbegin());
                if (bid_level->quantity <= quantity) {
                    quantity -= bid_level->quantity;
                    bid_side.erase(std::prev(bid_side.end()));
                } else {
                    bid_level->quantity -= quantity;
                    bid_level->vol = bid_level->price * bid_level->quantity;
                    quantity = 0;
                }
            }
            if (quantity > 0) {
                PriceLevel* level = find_or_insert_price_level(ask_side, price, quantity);
                level->quantity += quantity;
                level->vol = level->price * level->quantity;
            }
        }
        return true;
    }
    bool order_del(int32_t price, int32_t quantity, bool is_bid){
        if(is_bid){
            auto iter = bid_side.find(price);
            if (iter->quantity == quantity) {
                ask_side.erase(iter);
            } else {
                iter->quantity -= quantity;
                iter->vol = iter->price * iter->quantity;
            }
        }else{
            auto iter = ask_side.find(price);
            if (iter->quantity == quantity) {
                bid_side.erase(iter);
            } else {
                iter->quantity -= quantity;
                iter->vol = iter->price * iter->quantity;
            }
        }
        return true;
    }
    template<size_t N>
    depth<N> cal_depth_bid();

    template<size_t N>
    depth<N> cal_depth_ask();
};

template<size_t N>
depth<N> MergeOrderBook::cal_depth_bid() {
    depth<N> result;
    size_t count = 0;

    // 从最高的买价开始
    for (auto it = bid_side.rbegin(); it != bid_side.rend() && count < N; ++it, ++count) {
        result.level_infos[count] = std::make_pair(it->price, it->quantity);
    }

    // 如果数量不足N，用0补齐
    for (; count < N; ++count) {
        result.level_infos[count] = std::make_pair(0, 0);
    }

    return result;
}

template<size_t N>
depth<N> MergeOrderBook::cal_depth_ask() {
    depth<N> result;
    size_t count = 0;

    // 从最低的卖价开始
    for (auto it = ask_side.begin(); it != ask_side.end() && count < N; ++it, ++count) {
        result.level_infos[count] = std::make_pair(it->price, it->quantity);
    }

    // 如果数量不足N，用0补齐
    for (; count < N; ++count) {
        result.level_infos[count] = std::make_pair(0, 0);
    }

    return result;
}


int main() {
    MergeOrderBook book;

    // 添加测试订单
    book.order_add(105, 5, true);   // 买单：价格105，数量5
    book.order_add(100, 10, true);  // 买单：价格100，数量10
    book.order_add(90, 15, true);   // 买单：价格90，数量15

    book.order_add(112, 6, false);  // 卖单：价格112，数量6
    book.order_add(110, 8, false);  // 卖单：价格110，数量8
    book.order_add(115, 7, false);  // 卖单：价格115，数量7

    // 显示最佳买卖价格
    std::cout << "Best Bid: " << book.best_bid() << std::endl;
    std::cout << "Best Ask: " << book.best_ask() << std::endl;

    // 计算并显示买单深度
    auto bid_depth = book.cal_depth_bid<3>();
    std::cout << "Top 3 Bids:" << std::endl;
    for (const auto& level : bid_depth.level_infos) {
        std::cout << "Price: " << level.first << ", Quantity: " << level.second << std::endl;
    }

    // 计算并显示卖单深度
    auto ask_depth = book.cal_depth_ask<3>();
    std::cout << "Top 3 Asks:" << std::endl;
    for (const auto& level : ask_depth.level_infos) {
        std::cout << "Price: " << level.first << ", Quantity: " << level.second << std::endl;
    }

    // 删除测试订单
    book.order_del(100, 5, true);  // 删除买单：价格100，数量5
    book.order_del(110, 4, false); // 删除卖单：价格110，数量4

    // 再次显示最佳买卖价格
    std::cout << "\nAfter deletion:" << std::endl;
    std::cout << "Best Bid: " << book.best_bid() << std::endl;
    std::cout << "Best Ask: " << book.best_ask() << std::endl;

    // 计算并显示买单深度
    bid_depth = book.cal_depth_bid<3>();
    std::cout << "Top 3 Bids:" << std::endl;
    for (const auto& level : bid_depth.level_infos) {
        std::cout << "Price: " << level.first << ", Quantity: " << level.second << std::endl;
    }

    // 计算并显示卖单深度
    ask_depth = book.cal_depth_ask<3>();
    std::cout << "Top 3 Asks:" << std::endl;
    for (const auto& level : ask_depth.level_infos) {
        std::cout << "Price: " << level.first << ", Quantity: " << level.second << std::endl;
    }

    //增加一个可能和ask成交的单子  
    book.order_add(113, 9, true);  // 删除买单：价格100，数量5
    std::cout << "\nAfter deletion:" << std::endl;
    std::cout << "Best Bid: " << book.best_bid() << std::endl;
    std::cout << "Best Ask: " << book.best_ask() << std::endl;

    // 计算并显示买单深度
    bid_depth = book.cal_depth_bid<3>();
    std::cout << "Top 3 Bids:" << std::endl;
    for (const auto& level : bid_depth.level_infos) {
        std::cout << "Price: " << level.first << ", Quantity: " << level.second << std::endl;
    }

    // 计算并显示卖单深度
    ask_depth = book.cal_depth_ask<3>();
    std::cout << "Top 3 Asks:" << std::endl;
    for (const auto& level : ask_depth.level_infos) {
        std::cout << "Price: " << level.first << ", Quantity: " << level.second << std::endl;
    }
    return 0;
}
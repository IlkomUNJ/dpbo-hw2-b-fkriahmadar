#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <chrono>
#include <ctime>
#include <cstdint>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <unordered_map>
#include <iomanip>

struct Account {
    std::string username;
    std::string password;
    std::string name;
    std::string address;
    std::string phone;
    std::string email;
    bool isSeller = false;
    int sellerId = 0;
    std::string storeName;
};

struct Item {
    int id = 0;
    std::string name;
    double price = 0.0;
    int quantity = 0;
};

struct Order {
    int id = 0;
    int buyerIndex = 0;
    int sellerIndex = 0;
    int itemId = 0;
    std::string itemName;
    int quantity = 0;
    double total = 0.0;
    std::time_t time = 0;
    enum Status { PAID = 0, COMPLETED = 1, CANCELLED = 2 } status = PAID;
};

struct Transaction {
    double amount = 0.0;      
    std::time_t time = 0;
    std::string note;
};

struct BankAccount {
    bool exists = false;
    double balance = 0.0;
    std::vector<Transaction> transactions;

    void create(double initial = 0.0) {
        exists = true;
        balance = initial;
        transactions.clear();
        if (initial != 0.0) {
            transactions.push_back({initial, std::time(nullptr), "Initial deposit"});
        }
    }
    void topup(double a, const std::string &note="Topup") {
        if (!exists) return;
        balance += a;
        transactions.push_back({a, std::time(nullptr), note});
    }
    bool withdraw(double a, const std::string &note="Withdraw") {
        if (!exists) return false;
        if (a > balance) return false;
        balance -= a;
        transactions.push_back({-a, std::time(nullptr), note});
        return true;
    }
    std::pair<double,double> cashflow_since(std::chrono::system_clock::time_point since) const {
        double in = 0.0, out = 0.0;
        std::time_t since_t = std::chrono::system_clock::to_time_t(since);
        for (const auto &tx : transactions) {
            if (tx.time >= since_t) {
                if (tx.amount >= 0) in += tx.amount;
                else out += -tx.amount;
            }
        }
        return {in, out};
    }
};

struct Patient {
    int id = 0;
    std::string name;
    int age = 0;

    std::string serialize() const {
        std::ostringstream ss;
        ss << id << '|' << name << '|' << age;
        return ss.str();
    }
    static Patient deserialize(const std::string &line) {
        Patient p{};
        std::istringstream ss(line);
        std::string token;
        if (std::getline(ss, token, '|')) p.id = std::stoi(token);
        if (std::getline(ss, token, '|')) p.name = token;
        if (std::getline(ss, token, '|')) p.age = std::stoi(token);
        return p;
    }
};

extern std::vector<Account> accounts;
extern std::vector<std::vector<Item>> inventories;
extern std::vector<BankAccount> bankAccounts;                  
extern std::vector<std::vector<Order>> sellerOrders;           
extern std::vector<std::vector<Order>> buyerOrders;            
extern int nextOrderId;
extern std::vector<Patient> patients;

inline std::string timestr(std::time_t t) {
    char buf[64];
    std::tm tm = *std::localtime(&t);
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    return std::string(buf);
}
inline void bank_list_transactions_last_week() {
    std::time_t now = std::time(nullptr);
    std::time_t weekAgo = now - 7 * 24 * 3600;
    std::cout << "\nTransactions in the last 7 days (newest first)\n";
    std::cout << std::string(60, '-') << "\n";
    bool any = false;
    size_t n = std::min(accounts.size(), bankAccounts.size());
    for (size_t i = 0; i < n; ++i) {
        if (!bankAccounts[i].exists) continue;
        for (auto it = bankAccounts[i].transactions.rbegin(); it != bankAccounts[i].transactions.rend(); ++it) {
            if (it->time >= weekAgo) {
                any = true;
                std::cout << "AcctIdx: " << i
                          << " | User: " << (i < accounts.size() ? accounts[i].username : std::string("<unknown>"))
                          << " | Time: " << timestr(it->time)
                          << " | " << (it->amount >= 0 ? "+" : "-") << std::fixed << std::setprecision(2) << std::fabs(it->amount)
                          << " | " << it->note << "\n";
            }
        }
    }
    if (!any) std::cout << "(no transactions in the last week)\n";
    std::cout << "\n";
}

inline void bank_list_customers() {
    std::cout << "\nBank customers:\n";
    std::cout << std::left << std::setw(6) << "Idx" << std::setw(20) << "Username" << std::setw(10) << "HasAcct" << std::setw(12) << "Balance" << "\n";
    std::cout << std::string(60, '-') << "\n";
    size_t n = std::min(accounts.size(), bankAccounts.size());
    for (size_t i = 0; i < n; ++i) {
        std::cout << std::left << std::setw(6) << i
                  << std::setw(20) << accounts[i].username
                  << std::setw(10) << (bankAccounts[i].exists ? "Yes" : "No")
                  << std::right << std::setw(12) << std::fixed << std::setprecision(2) << (bankAccounts[i].exists ? bankAccounts[i].balance : 0.0)
                  << "\n";
    }
    std::cout << "\n";
}

inline void bank_list_dormant_accounts() {
    std::time_t now = std::time(nullptr);
    std::time_t monthAgo = now - 30 * 24 * 3600;
    std::cout << "\nDormant accounts (no transaction within 30 days):\n";
    std::cout << std::left << std::setw(6) << "Idx" << std::setw(20) << "Username" << std::setw(12) << "LastTx" << "\n";
    std::cout << std::string(60, '-') << "\n";
    bool any = false;
    size_t n = std::min(accounts.size(), bankAccounts.size());
    for (size_t i = 0; i < n; ++i) {
        if (!bankAccounts[i].exists) {
            // treat as dormant (no transactions)
            any = true;
            std::cout << std::left << std::setw(6) << i << std::setw(20) << accounts[i].username << std::setw(12) << "(no tx)" << "\n";
            continue;
        }
        if (bankAccounts[i].transactions.empty()) {
            any = true;
            std::cout << std::left << std::setw(6) << i << std::setw(20) << accounts[i].username << std::setw(12) << "(no tx)" << "\n";
            continue;
        }
        std::time_t last = bankAccounts[i].transactions.back().time;
        if (last < monthAgo) {
            any = true;
            std::cout << std::left << std::setw(6) << i << std::setw(20) << accounts[i].username << std::setw(12) << timestr(last) << "\n";
        }
    }
    if (!any) std::cout << "(no dormant accounts)\n";
    std::cout << "\n";
}

inline void bank_top_user_most_tx_today() {
    std::time_t now = std::time(nullptr);
    std::tm today = *std::localtime(&now);
    today.tm_hour = 0; today.tm_min = 0; today.tm_sec = 0;
    std::time_t startOfDay = std::mktime(&today);
    std::vector<int> counts;
    size_t n = std::min(accounts.size(), bankAccounts.size());
    counts.assign(n, 0);
    for (size_t i = 0; i < n; ++i) {
        if (!bankAccounts[i].exists) continue;
        for (const auto &tx : bankAccounts[i].transactions) {
            if (tx.time >= startOfDay) ++counts[i];
        }
    }
    int bestIdx = -1;
    int bestCount = 0;
    for (size_t i = 0; i < n; ++i) {
        if (counts[i] > bestCount) {
            bestCount = counts[i];
            bestIdx = static_cast<int>(i);
        }
    }
    std::cout << "\nTop user by number of bank transactions today:\n";
    if (bestIdx < 0) {
        std::cout << "(no transactions today)\n\n";
        return;
    }
    std::cout << "Idx: " << bestIdx << " | User: " << accounts[bestIdx].username << " | Tx today: " << bestCount << "\n\n";
}

inline void writeString(std::ofstream &out, const std::string &s) {
    uint32_t len = static_cast<uint32_t>(s.size());
    out.write(reinterpret_cast<const char*>(&len), sizeof(len));
    if (len) out.write(s.data(), len);
}
inline std::string readString(std::ifstream &in) {
    uint32_t len = 0;
    if (!in.read(reinterpret_cast<char*>(&len), sizeof(len))) return {};
    std::string s;
    if (len) {
        s.resize(len);
        in.read(&s[0], len);
    }
    return s;
}

inline void savePatients(const std::string &path = "patients.dat") {
    std::ofstream out(path, std::ios::trunc);
    for (const auto &p : patients) out << p.serialize() << '\n';
}

inline void loadPatients(const std::string &path = "patients.dat") {
    std::ifstream in(path);
    if (!in.good()) return;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        patients.push_back(Patient::deserialize(line));
    }
}

inline void saveState(const std::string &path = "state.bin") {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) return;

    uint32_t accCount = static_cast<uint32_t>(accounts.size());
    out.write(reinterpret_cast<const char*>(&accCount), sizeof(accCount));
    for (const auto &a : accounts) {
        writeString(out, a.username);
        writeString(out, a.password);
        writeString(out, a.name);
        writeString(out, a.address);
        writeString(out, a.phone);
        writeString(out, a.email);
        uint8_t sellerFlag = a.isSeller ? 1 : 0;
        out.write(reinterpret_cast<const char*>(&sellerFlag), sizeof(sellerFlag));
        int32_t sId = a.sellerId;
        out.write(reinterpret_cast<const char*>(&sId), sizeof(sId));
        writeString(out, a.storeName);
    }

    for (const auto &inv : inventories) {
        uint32_t invCount = static_cast<uint32_t>(inv.size());
        out.write(reinterpret_cast<const char*>(&invCount), sizeof(invCount));
        for (const auto &it : inv) {
            int32_t id = it.id;
            out.write(reinterpret_cast<const char*>(&id), sizeof(id));
            writeString(out, it.name);
            out.write(reinterpret_cast<const char*>(&it.price), sizeof(it.price));
            int32_t qty = it.quantity;
            out.write(reinterpret_cast<const char*>(&qty), sizeof(qty));
        }
    }

    for (const auto &b : bankAccounts) {
        uint8_t exists = b.exists ? 1 : 0;
        out.write(reinterpret_cast<const char*>(&exists), sizeof(exists));
        out.write(reinterpret_cast<const char*>(&b.balance), sizeof(b.balance));
        uint32_t txCount = static_cast<uint32_t>(b.transactions.size());
        out.write(reinterpret_cast<const char*>(&txCount), sizeof(txCount));
        for (const auto &tx : b.transactions) {
            out.write(reinterpret_cast<const char*>(&tx.amount), sizeof(tx.amount));
            int64_t t = static_cast<int64_t>(tx.time);
            out.write(reinterpret_cast<const char*>(&t), sizeof(t));
            writeString(out, tx.note);
        }
    }

    for (const auto &orders : sellerOrders) {
        uint32_t ordCount = static_cast<uint32_t>(orders.size());
        out.write(reinterpret_cast<const char*>(&ordCount), sizeof(ordCount));
        for (const auto &o : orders) {
            int32_t id = o.id; out.write(reinterpret_cast<const char*>(&id), sizeof(id));
            int32_t bi = o.buyerIndex; out.write(reinterpret_cast<const char*>(&bi), sizeof(bi));
            int32_t si = o.sellerIndex; out.write(reinterpret_cast<const char*>(&si), sizeof(si));
            int32_t iid = o.itemId; out.write(reinterpret_cast<const char*>(&iid), sizeof(iid));
            writeString(out, o.itemName);
            int32_t q = o.quantity; out.write(reinterpret_cast<const char*>(&q), sizeof(q));
            out.write(reinterpret_cast<const char*>(&o.total), sizeof(o.total));
            int64_t t = static_cast<int64_t>(o.time); out.write(reinterpret_cast<const char*>(&t), sizeof(t));
            int32_t st = static_cast<int32_t>(o.status); out.write(reinterpret_cast<const char*>(&st), sizeof(st));
        }
    }

    for (const auto &orders : buyerOrders) {
        uint32_t ordCount = static_cast<uint32_t>(orders.size());
        out.write(reinterpret_cast<const char*>(&ordCount), sizeof(ordCount));
        for (const auto &o : orders) {
            int32_t id = o.id; out.write(reinterpret_cast<const char*>(&id), sizeof(id));
            int32_t bi = o.buyerIndex; out.write(reinterpret_cast<const char*>(&bi), sizeof(bi));
            int32_t si = o.sellerIndex; out.write(reinterpret_cast<const char*>(&si), sizeof(si));
            int32_t iid = o.itemId; out.write(reinterpret_cast<const char*>(&iid), sizeof(iid));
            writeString(out, o.itemName);
            int32_t q = o.quantity; out.write(reinterpret_cast<const char*>(&q), sizeof(q));
            out.write(reinterpret_cast<const char*>(&o.total), sizeof(o.total));
            int64_t t = static_cast<int64_t>(o.time); out.write(reinterpret_cast<const char*>(&t), sizeof(t));
            int32_t st = static_cast<int32_t>(o.status); out.write(reinterpret_cast<const char*>(&st), sizeof(st));
        }
    }

    int32_t nid = nextOrderId;
    out.write(reinterpret_cast<const char*>(&nid), sizeof(nid));
}

inline void loadState(const std::string &path = "state.bin") {
    std::ifstream in(path, std::ios::binary);
    if (!in.good()) return;

    uint32_t accCount = 0;
    if (!in.read(reinterpret_cast<char*>(&accCount), sizeof(accCount))) return;

    accounts.clear();
    inventories.clear();
    bankAccounts.clear();
    sellerOrders.clear();
    buyerOrders.clear();

    accounts.resize(accCount);
    inventories.resize(accCount);
    bankAccounts.resize(accCount);
    sellerOrders.resize(accCount);
    buyerOrders.resize(accCount);

    for (uint32_t i = 0; i < accCount; ++i) {
        Account a;
        a.username = readString(in);
        a.password = readString(in);
        a.name = readString(in);
        a.address = readString(in);
        a.phone = readString(in);
        a.email = readString(in);
        uint8_t sellerFlag = 0; in.read(reinterpret_cast<char*>(&sellerFlag), sizeof(sellerFlag));
        a.isSeller = sellerFlag ? true : false;
        int32_t sId = 0; in.read(reinterpret_cast<char*>(&sId), sizeof(sId));
        a.sellerId = sId;
        a.storeName = readString(in);
        accounts[i] = std::move(a);
    }

    for (uint32_t i = 0; i < accCount; ++i) {
        uint32_t invCount = 0; in.read(reinterpret_cast<char*>(&invCount), sizeof(invCount));
        inventories[i].clear();
        for (uint32_t j = 0; j < invCount; ++j) {
            Item it;
            int32_t id = 0; in.read(reinterpret_cast<char*>(&id), sizeof(id));
            it.id = id;
            it.name = readString(in);
            in.read(reinterpret_cast<char*>(&it.price), sizeof(it.price));
            int32_t qty = 0; in.read(reinterpret_cast<char*>(&qty), sizeof(qty));
            it.quantity = qty;
            inventories[i].push_back(std::move(it));
        }
    }

    for (uint32_t i = 0; i < accCount; ++i) {
        uint8_t exists = 0; in.read(reinterpret_cast<char*>(&exists), sizeof(exists));
        bankAccounts[i].exists = exists ? true : false;
        in.read(reinterpret_cast<char*>(&bankAccounts[i].balance), sizeof(bankAccounts[i].balance));
        uint32_t txCount = 0; in.read(reinterpret_cast<char*>(&txCount), sizeof(txCount));
        bankAccounts[i].transactions.clear();
        for (uint32_t t = 0; t < txCount; ++t) {
            Transaction tx;
            in.read(reinterpret_cast<char*>(&tx.amount), sizeof(tx.amount));
            int64_t tt = 0; in.read(reinterpret_cast<char*>(&tt), sizeof(tt));
            tx.time = static_cast<std::time_t>(tt);
            tx.note = readString(in);
            bankAccounts[i].transactions.push_back(std::move(tx));
        }
    }

    for (uint32_t i = 0; i < accCount; ++i) {
        uint32_t ordCount = 0; in.read(reinterpret_cast<char*>(&ordCount), sizeof(ordCount));
        sellerOrders[i].clear();
        for (uint32_t j = 0; j < ordCount; ++j) {
            Order o;
            int32_t id=0; in.read(reinterpret_cast<char*>(&id), sizeof(id)); o.id = id;
            int32_t bi=0; in.read(reinterpret_cast<char*>(&bi), sizeof(bi)); o.buyerIndex = bi;
            int32_t si=0; in.read(reinterpret_cast<char*>(&si), sizeof(si)); o.sellerIndex = si;
            int32_t iid=0; in.read(reinterpret_cast<char*>(&iid), sizeof(iid)); o.itemId = iid;
            o.itemName = readString(in);
            int32_t q=0; in.read(reinterpret_cast<char*>(&q), sizeof(q)); o.quantity = q;
            in.read(reinterpret_cast<char*>(&o.total), sizeof(o.total));
            int64_t t=0; in.read(reinterpret_cast<char*>(&t), sizeof(t)); o.time = static_cast<std::time_t>(t);
            int32_t st=0; in.read(reinterpret_cast<char*>(&st), sizeof(st)); o.status = static_cast<Order::Status>(st);
            sellerOrders[i].push_back(std::move(o));
        }
    }

    for (uint32_t i = 0; i < accCount; ++i) {
        uint32_t ordCount = 0; in.read(reinterpret_cast<char*>(&ordCount), sizeof(ordCount));
        buyerOrders[i].clear();
        for (uint32_t j = 0; j < ordCount; ++j) {
            Order o;
            int32_t id=0; in.read(reinterpret_cast<char*>(&id), sizeof(id)); o.id = id;
            int32_t bi=0; in.read(reinterpret_cast<char*>(&bi), sizeof(bi)); o.buyerIndex = bi;
            int32_t si=0; in.read(reinterpret_cast<char*>(&si), sizeof(si)); o.sellerIndex = si;
            int32_t iid=0; in.read(reinterpret_cast<char*>(&iid), sizeof(iid)); o.itemId = iid;
            o.itemName = readString(in);
            int32_t q=0; in.read(reinterpret_cast<char*>(&q), sizeof(q)); o.quantity = q;
            in.read(reinterpret_cast<char*>(&o.total), sizeof(o.total));
            int64_t t=0; in.read(reinterpret_cast<char*>(&t), sizeof(t)); o.time = static_cast<std::time_t>(t);
            int32_t st=0; in.read(reinterpret_cast<char*>(&st), sizeof(st)); o.status = static_cast<Order::Status>(st);
            buyerOrders[i].push_back(std::move(o));
        }
    }

    int32_t nid = 0;
    if (in.read(reinterpret_cast<char*>(&nid), sizeof(nid))) nextOrderId = nid;
}

inline void print_most_frequent_items(int topN = 10) {
    if (sellerOrders.empty()) {
        std::cout << "(no transactions)\n";
        return;
    }
    std::unordered_map<std::string, int> cnt;
    int total = 0;
    for (const auto &orders : sellerOrders) {
        for (const auto &o : orders) {
            if (o.status == Order::PAID || o.status == Order::COMPLETED) {
                cnt[o.itemName]++;
                ++total;
            }
        }
    }
    if (cnt.empty()) {
        std::cout << "(no qualifying transactions)\n";
        return;
    }
    std::vector<std::pair<std::string,int>> v;
    v.reserve(cnt.size());
    for (auto &p : cnt) v.emplace_back(p.first, p.second);
    std::sort(v.begin(), v.end(), [](auto &a, auto &b){ return a.second > b.second; });
    std::cout << "\nMost frequent items (top " << topN << ")\n";
    std::cout << std::left << std::setw(40) << "Item Name" << std::right << std::setw(10) << "Count" << "   " << "Pct\n";
    std::cout << std::string(60, '-') << "\n";
    int shown = 0;
    for (auto &p : v) {
        if (shown++ >= topN) break;
        double pct = total ? (100.0 * p.second / total) : 0.0;
        std::cout << std::left << std::setw(40) << p.first
                  << std::right << std::setw(10) << p.second
                  << "   " << std::fixed << std::setprecision(1) << pct << "%\n";
    }
    std::cout << "\n";
}

inline void print_most_active_buyers_last_days(int days = 1, int topN = 10) {
    if (days <= 0) days = 1;
    std::time_t since = std::time(nullptr) - static_cast<std::time_t>(days) * 24 * 3600;
    std::unordered_map<int,int> cnt; // buyerIndex -> count
    for (const auto &orders : sellerOrders) {
        for (const auto &o : orders) {
            if ((o.status == Order::PAID || o.status == Order::COMPLETED) && o.time >= since) {
                cnt[o.buyerIndex]++;
            }
        }
    }
    if (cnt.empty()) {
        std::cout << "(no transactions in the given period)\n";
        return;
    }
    std::vector<std::pair<int,int>> v;
    v.reserve(cnt.size());
    for (auto &p : cnt) v.emplace_back(p.first, p.second);
    std::sort(v.begin(), v.end(), [](auto &a, auto &b){ return a.second > b.second; });
    std::cout << "\nMost active buyers (last " << days << " day(s))\n";
    std::cout << std::left << std::setw(6) << "Index" << std::setw(30) << "Buyer Name" << std::right << std::setw(12) << "Tx Count" << "   " << "Tx/day\n";
    std::cout << std::string(70, '-') << "\n";
    int shown = 0;
    for (auto &p : v) {
        if (shown++ >= topN) break;
        int idx = p.first;
        std::string name = (idx >= 0 && static_cast<size_t>(idx) < accounts.size()) ? accounts[idx].name : ("#"+std::to_string(idx));
        double perDay = static_cast<double>(p.second) / days;
        std::cout << std::left << std::setw(6) << idx << std::setw(30) << name
                  << std::right << std::setw(12) << p.second
                  << "   " << std::fixed << std::setprecision(2) << perDay << "\n";
    }
    std::cout << "\n";
}
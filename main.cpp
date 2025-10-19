#include <iostream>
#include <sstream>
#include <ctime>
#include <cmath>
#include <algorithm>
#include "bank_customer.h"
#include "buyer.h"
#include "state.h"

enum PrimaryPrompt{LOGIN, REGISTER, EXIT, BANK}; // added BANK as last
enum RegisterPrompt{CREATE_BUYER, CREATE_SELLER, BACK};
using namespace std;

std::vector<Account> accounts;
std::vector<std::vector<Item>> inventories;
std::vector<BankAccount> bankAccounts;
std::vector<std::vector<Order>> sellerOrders;
std::vector<std::vector<Order>> buyerOrders;
int nextOrderId = 1;
std::vector<Patient> patients;

int main(int argc, char** argv) {
    
    loadState();      
    loadPatients();   

    PrimaryPrompt prompt = LOGIN;
    RegisterPrompt regPrompt = CREATE_BUYER;

    while (prompt != EXIT) {
        cout << "Select an option: " << endl;
        cout << "1. Login" << endl;
        cout << "2. Register" << endl;
        cout << "3. Exit" << endl;
        cout << "4. Bank (reports)" << endl; 
        int choice;
        if (!(cin >> choice)) break;
        prompt = static_cast<PrimaryPrompt>(choice - 1);
        switch (prompt) {
            case LOGIN: {
                cout << "Login selected." << endl;

                string username, password;
                cout << "Enter username: ";
                cin >> username;
                cout << "Enter password: ";
                cin >> password;

                bool found = false;
                int foundIndex = -1;
                for (size_t i = 0; i < accounts.size(); ++i) {
                    if (accounts[i].username == username && accounts[i].password == password) {
                        found = true;
                        foundIndex = static_cast<int>(i);
                        break;
                    }
                }

                if (found) {
                    if (static_cast<int>(inventories.size()) <= foundIndex) inventories.resize(foundIndex + 1);
                    if (static_cast<int>(bankAccounts.size()) <= foundIndex) bankAccounts.resize(foundIndex + 1);
                    if (static_cast<int>(sellerOrders.size()) <= foundIndex) sellerOrders.resize(foundIndex + 1);
                    if (static_cast<int>(buyerOrders.size()) <= foundIndex) buyerOrders.resize(foundIndex + 1);

                    Account &loggedInAccount = accounts[foundIndex];
                    BankAccount &myBank = bankAccounts[foundIndex];
                    cout << "Login successful!" << endl;

                    if (loggedInAccount.isSeller) {
                        bool sellerLoggedIn = true;
                        while (sellerLoggedIn) {
                            cout << "\nSeller Menu:" << endl;
                            cout << "1. Check Account / Store Status" << endl;
                            cout << "2. View Inventory" << endl;
                            cout << "3. Add Item to Inventory" << endl;
                            cout << "4. Manage Bank Account" << endl;
                            cout << "5. Manage Orders" << endl;
                            cout << "6. Discard Item from Store" << endl;
                            cout << "7. List most frequent items (store-wide)" << endl;
                            cout << "8. List most active buyers (last 1 day)" << endl;
                            cout << "9. Logout" << endl;
                            int sChoice;
                            cin >> sChoice;
                            switch (sChoice) {
                                case 1:
                                    cout << "\n--- Account / Store Status ---" << endl;
                                    cout << "Account Type: Seller" << endl;
                                    cout << "Store Name: " << loggedInAccount.storeName << endl;
                                    cout << "Seller ID: " << loggedInAccount.sellerId << endl;
                                    cout << "Name: " << loggedInAccount.name << endl;
                                    cout << "Username: " << loggedInAccount.username << endl;
                                    cout << "Address: " << loggedInAccount.address << endl;
                                    cout << "Phone: " << loggedInAccount.phone << endl;
                                    cout << "Email: " << loggedInAccount.email << endl;
                                    cout << "-------------------------------\n" << endl;
                                    break;
                                case 2: {
                                    auto &inv = inventories[foundIndex];
                                    cout << "\n--- Inventory for '" << loggedInAccount.storeName << "' ---" << endl;
                                    if (inv.empty()) {
                                        cout << "(No items in inventory)" << endl;
                                    } else {
                                        for (const auto &it : inv) {
                                            cout << "ID: " << it.id
                                                 << " | Name: " << it.name
                                                 << " | Price: $" << it.price
                                                 << " | Qty: " << it.quantity << endl;
                                        }
                                    }
                                    cout << "------------------------------------\n" << endl;
                                    break;
                                }
                                case 3: {
                                    auto &inv = inventories[foundIndex];
                                    Item newItem;
                                    cout << "Enter Item ID (numeric): ";
                                    cin >> newItem.id;
                                    cin.ignore();
                                    cout << "Enter Item Name: ";
                                    getline(cin, newItem.name);
                                    cout << "Enter Item Price: ";
                                    cin >> newItem.price;
                                    cout << "Enter Item Quantity: ";
                                    cin >> newItem.quantity;
                                    bool updated = false;
                                    for (auto &it : inv) {
                                        if (it.id == newItem.id) {
                                            it.quantity += newItem.quantity;
                                            it.price = newItem.price;
                                            updated = true;
                                            break;
                                        }
                                    }
                                    if (!updated) inv.push_back(newItem);
                                    cout << "Item saved to inventory." << endl;
                                    break;
                                }
                                case 4: {
                                    bool backBA = false;
                                    while (!backBA) {
                                        cout << "\nBank Account Menu:" << endl;
                                        if (!myBank.exists) {
                                            cout << "No bank account found for this user." << endl;
                                            cout << "1. Create Bank Account" << endl;
                                            cout << "2. Back" << endl;
                                            int baChoice; cin >> baChoice;
                                            if (baChoice == 1) {
                                                double init; cout << "Initial deposit amount: "; cin >> init;
                                                myBank.create(init);
                                                cout << "Bank account created. Balance: $" << myBank.balance << endl;
                                            } else backBA = true;
                                        } else {
                                            cout << "1. Top-up" << endl;
                                            cout << "2. Withdraw" << endl;
                                            cout << "3. View Cash Flow (last 30 days)" << endl;
                                            cout << "4. View Balance & Transactions" << endl;
                                            cout << "5. Back" << endl;
                                            int baChoice; cin >> baChoice;
                                            if (baChoice == 1) {
                                                double amt; cout << "Top-up amount: "; cin >> amt;
                                                myBank.topup(amt);
                                                cout << "Top-up successful. New balance: $" << myBank.balance << endl;
                                            } else if (baChoice == 2) {
                                                double amt; cout << "Withdraw amount: "; cin >> amt;
                                                if (myBank.withdraw(amt)) cout << "Withdraw successful. New balance: $" << myBank.balance << endl;
                                                else cout << "Withdraw failed. Insufficient funds." << endl;
                                            } else if (baChoice == 3) {
                                                auto now = std::chrono::system_clock::now();
                                                auto since = now - std::chrono::hours(24*30);
                                                auto cf = myBank.cashflow_since(since);
                                                cout << "\nCash flow -- last 30 days:" << endl;
                                                cout << "Total Inflows: $" << cf.first << endl;
                                                cout << "Total Outflows: $" << cf.second << endl;
                                                cout << "Net: $" << (cf.first - cf.second) << endl;
                                            } else if (baChoice == 4) {
                                                cout << "\nBalance: $" << myBank.balance << endl;
                                                cout << "Transactions:" << endl;
                                                if (myBank.transactions.empty()) cout << "(no transactions)" << endl;
                                                for (const auto &tx : myBank.transactions) {
                                                    cout << timestr(tx.time)
                                                         << " | " << (tx.amount >= 0 ? "+" : "-")
                                                         << std::fabs(tx.amount) << " | " << tx.note << endl;
                                                }
                                            } else backBA = true;
                                        }
                                    }
                                    break;
                                }
                                case 5: {
                                    auto &orders = sellerOrders[foundIndex];
                                    if (orders.empty()) {
                                        cout << "\n(No orders yet)\n";
                                        break;
                                    }
                                    cout << "\n--- Orders for store '" << loggedInAccount.storeName << "' ---\n";
                                    for (const auto &o : orders) {
                                        cout << "Order #" << o.id
                                             << " | BuyerIdx: " << o.buyerIndex
                                             << " | Item: " << o.itemName
                                             << " | Qty: " << o.quantity
                                             << " | Total: $" << o.total
                                             << " | Status: " << (o.status==Order::PAID?"PAID": o.status==Order::COMPLETED?"COMPLETED":"CANCELLED")
                                             << " | Time: " << timestr(o.time) << "\n";
                                    }
                                    cout << "Enter order id to manage (0 = back): ";
                                    int oid; cin >> oid;
                                    if (oid == 0) break;
                                    // find order in sellerOrders
                                    auto it = std::find_if(orders.begin(), orders.end(), [&](const Order &x){ return x.id == oid; });
                                    if (it == orders.end()) { cout << "Order not found.\n"; break; }
                                    int action = 0;
                                    cout << "1. Mark Completed\n2. Cancel (refund)\n3. Back\nChoose: ";
                                    cin >> action;
                                    if (action == 1) {
                                        if (it->status == Order::PAID) {
                                            it->status = Order::COMPLETED;
                                            // update buyer copy
                                            auto &borders = buyerOrders[it->buyerIndex];
                                            auto ib = std::find_if(borders.begin(), borders.end(), [&](const Order &x){ return x.id == oid; });
                                            if (ib!=borders.end()) ib->status = Order::COMPLETED;
                                            cout << "Order marked COMPLETED.\n";
                                        } else cout << "Order not in PAID state.\n";
                                    } else if (action == 2) {
                                        if (it->status == Order::PAID) {
                                            // refund buyer
                                            int bidx = it->buyerIndex;
                                            if (static_cast<int>(bankAccounts.size()) <= bidx) bankAccounts.resize(bidx+1);
                                            BankAccount &buyerBank = bankAccounts[bidx];
                                            if (buyerBank.exists) {
                                                buyerBank.topup(it->total, "Refund for order #" + std::to_string(it->id));
                                                // restore stock
                                                auto &inv = inventories[foundIndex];
                                                auto itemIt = std::find_if(inv.begin(), inv.end(), [&](const Item &itx){ return itx.id == it->itemId; });
                                                if (itemIt != inv.end()) itemIt->quantity += it->quantity;
                                                // change status in both seller and buyer copies
                                                it->status = Order::CANCELLED;
                                                auto &borders = buyerOrders[bidx];
                                                auto ib = std::find_if(borders.begin(), borders.end(), [&](const Order &x){ return x.id == oid; });
                                                if (ib!=borders.end()) ib->status = Order::CANCELLED;
                                                cout << "Order cancelled and buyer refunded.\n";
                                            } else {
                                                cout << "Cannot refund: buyer has no bank account. Manual refund required.\n";
                                            }
                                        } else cout << "Order not in PAID state.\n";
                                    }
                                    break;
                                }
                                case 6: {
                                    auto &inv = inventories[foundIndex];
                                    if (inv.empty()) {
                                        cout << "No items to discard.\n";
                                        break;
                                    }
                                    cout << "\n--- Inventory ---\n";
                                    for (const auto &it : inv) {
                                        cout << "ID: " << it.id << " | " << it.name << " | Qty: " << it.quantity << "\n";
                                    }
                                    cout << "Enter Item ID to discard (0 = back): ";
                                    int rid; cin >> rid;
                                    if (rid == 0) break;
                                    auto rit = std::find_if(inv.begin(), inv.end(), [&](const Item &x){ return x.id == rid; });
                                    if (rit == inv.end()) {
                                        cout << "Item not found.\n";
                                        break;
                                    }
                                    cout << "Confirm discard item '" << rit->name << "' (y/n): ";
                                    char conf; cin >> conf;
                                    if (conf == 'y' || conf == 'Y') {
                                        inv.erase(rit);
                                        cout << "Item discarded from store inventory.\n";
                                    } else {
                                        cout << "Discard cancelled.\n";
                                    }
                                    break;
                                }
                                case 7: {
                                    print_most_frequent_items(10);
                                    break;
                                }
                                case 8: {
                                    print_most_active_buyers_last_days(1, 10);
                                    break;
                                }
                                case 9:
                                    cout << "Logging out..." << endl;
                                    sellerLoggedIn = false;
                                    break;
                                default:
                                    cout << "Invalid option." << endl;
                                    break;
                            }
                        }
                    } else {
                        bool loggedIn = true;
                        while (loggedIn) {
                            cout << "\nBuyer Menu:" << endl;
                            cout << "1. Check Account Status" << endl;
                            cout << "2. Manage Bank Account" << endl;
                            cout << "3. Visit Stores / Buy Item" << endl;
                            cout << "4. Upgrade Account to Seller" << endl;
                            cout << "5. Logout" << endl;
                            int buyerChoice;
                            cin >> buyerChoice;
                            switch (buyerChoice) {
                                case 1:
                                    cout << "\n--- Account Status ---" << endl;
                                    cout << "Account Type: Buyer only" << endl;
                                    cout << "\nBuyer Details:" << endl;
                                    cout << "Name: " << loggedInAccount.name << endl;
                                    cout << "Username: " << loggedInAccount.username << endl;
                                    cout << "Address: " << loggedInAccount.address << endl;
                                    cout << "Phone: " << loggedInAccount.phone << endl;
                                    cout << "Email: " << loggedInAccount.email << endl;
                                    cout << "\nBanking Account Details:" << endl;
                                    if (!myBank.exists) cout << "(No bank account)" << endl;
                                    else cout << "Balance: $" << myBank.balance << endl;
                                    cout << "----------------------\n" << endl;
                                    break;
                                case 2: {
                                    bool backBA = false;
                                    while (!backBA) {
                                        cout << "\nBank Account Menu:" << endl;
                                        if (!myBank.exists) {
                                            cout << "No bank account found for this user." << endl;
                                            cout << "1. Create Bank Account" << endl;
                                            cout << "2. Back" << endl;
                                            int baChoice; cin >> baChoice;
                                            if (baChoice == 1) {
                                                double init; cout << "Initial deposit amount: "; cin >> init;
                                                myBank.create(init);
                                                cout << "Bank account created. Balance: $" << myBank.balance << endl;
                                            } else backBA = true;
                                        } else {
                                            cout << "1. Top-up" << endl;
                                            cout << "2. Withdraw" << endl;
                                            cout << "3. View Cash Flow (last 30 days)" << endl;
                                            cout << "4. View Balance & Transactions" << endl;
                                            cout << "5. Back" << endl;
                                            int baChoice; cin >> baChoice;
                                            if (baChoice == 1) {
                                                double amt; cout << "Top-up amount: "; cin >> amt;
                                                myBank.topup(amt);
                                                cout << "Top-up successful. New balance: $" << myBank.balance << endl;
                                            } else if (baChoice == 2) {
                                                double amt; cout << "Withdraw amount: "; cin >> amt;
                                                if (myBank.withdraw(amt)) cout << "Withdraw successful. New balance: $" << myBank.balance << endl;
                                                else cout << "Withdraw failed. Insufficient funds." << endl;
                                            } else if (baChoice == 3) {
                                                auto now = std::chrono::system_clock::now();
                                                auto since = now - std::chrono::hours(24*30);
                                                auto cf = myBank.cashflow_since(since);
                                                cout << "\nCash flow -- last 30 days:" << endl;
                                                cout << "Total Inflows: $" << cf.first << endl;
                                                cout << "Total Outflows: $" << cf.second << endl;
                                                cout << "Net: $" << (cf.first - cf.second) << endl;
                                            } else if (baChoice == 4) {
                                                cout << "\nBalance: $" << myBank.balance << endl;
                                                cout << "Transactions:" << endl;
                                                if (myBank.transactions.empty()) cout << "(no transactions)" << endl;
                                                for (const auto &tx : myBank.transactions) {
                                                    cout << timestr(tx.time)
                                                         << " | " << (tx.amount >= 0 ? "+" : "-")
                                                         << std::fabs(tx.amount) << " | " << tx.note << endl;
                                                }
                                            } else backBA = true;
                                        }
                                    }
                                    break;
                                }
                                case 3: {
                                    cout << "\nAvailable stores:\n";
                                    for (size_t i = 0; i < accounts.size(); ++i) {
                                        if (accounts[i].isSeller) {
                                            cout << i << ". " << accounts[i].storeName << " (Owner: " << accounts[i].name << ")\n";
                                        }
                                    }
                                    cout << "Enter seller index to visit (-1 = back): ";
                                    int sidx; cin >> sidx;
                                    if (sidx < 0) break;
                                    if (static_cast<size_t>(sidx) >= accounts.size() || !accounts[sidx].isSeller) {
                                        cout << "Invalid seller.\n"; break;
                                    }
                                    if (static_cast<int>(inventories.size()) <= sidx) inventories.resize(sidx+1);
                                    auto &sInv = inventories[sidx];
                                    if (sInv.empty()) { cout << "Store has no items.\n"; break; }
                                    cout << "\nItems in store '" << accounts[sidx].storeName << "':\n";
                                    for (const auto &it : sInv) {
                                        cout << "ID: " << it.id << " | " << it.name << " | $" << it.price << " | Qty: " << it.quantity << "\n";
                                    }
                                    cout << "Enter item ID to buy (0 = back): ";
                                    int iid; cin >> iid;
                                    if (iid == 0) break;
                                    auto itemIt = std::find_if(sInv.begin(), sInv.end(), [&](const Item &x){ return x.id == iid; });
                                    if (itemIt == sInv.end()) { cout << "Item not found.\n"; break; }
                                    cout << "Enter quantity: ";
                                    int q; cin >> q;
                                    if (q <= 0) { cout << "Invalid quantity.\n"; break; }
                                    if (q > itemIt->quantity) { cout << "Insufficient stock.\n"; break; }
                                    double total = itemIt->price * q;
                                    // check buyer bank
                                    if (!myBank.exists) { cout << "You have no bank account. Create one before purchase.\n"; break; }
                                    if (myBank.balance < total) { cout << "Insufficient balance.\n"; break; }
                                    // perform payment
                                    bool ok = myBank.withdraw(total, "Purchase to " + accounts[sidx].storeName + " (order)");
                                    if (!ok) { cout << "Payment failed.\n"; break; }
                                    // reduce stock
                                    itemIt->quantity -= q;
                                    // record order
                                    Order o;
                                    o.id = nextOrderId++;
                                    o.buyerIndex = foundIndex;
                                    o.sellerIndex = sidx;
                                    o.itemId = itemIt->id;
                                    o.itemName = itemIt->name;
                                    o.quantity = q;
                                    o.total = total;
                                    o.time = std::time(nullptr);
                                    o.status = Order::PAID;
                                    if (static_cast<int>(sellerOrders.size()) <= sidx) sellerOrders.resize(sidx+1);
                                    if (static_cast<int>(buyerOrders.size()) <= foundIndex) buyerOrders.resize(foundIndex+1);
                                    sellerOrders[sidx].push_back(o);
                                    buyerOrders[foundIndex].push_back(o);
                                    cout << "Purchase successful. Order ID: " << o.id << " Status: PAID\n";
                                    break;
                                }
                                case 4:
                                    if (!loggedInAccount.isSeller) {
                                        cout << "Enter Seller ID (numeric): ";
                                        cin >> loggedInAccount.sellerId;
                                        cout << "Enter Store Name: ";
                                        cin.ignore();
                                        getline(cin, loggedInAccount.storeName);
                                        loggedInAccount.isSeller = true;
                                        if (static_cast<int>(inventories.size()) <= foundIndex) inventories.resize(foundIndex + 1);
                                        if (static_cast<int>(bankAccounts.size()) <= foundIndex) bankAccounts.resize(foundIndex + 1);
                                        if (static_cast<int>(sellerOrders.size()) <= foundIndex) sellerOrders.resize(foundIndex + 1);
                                        if (static_cast<int>(buyerOrders.size()) <= foundIndex) buyerOrders.resize(foundIndex + 1);
                                        cout << "Seller account created and linked to your buyer account!" << endl;
                                    } else {
                                        cout << "Already a Seller." << endl;
                                    }
                                    break;
                                case 5:
                                    cout << "Logging out..." << endl;
                                    loggedIn = false;
                                    break;
                                default:
                                    cout << "Invalid option." << endl;
                                    break;
                            }
                        }
                    }
                } else {
                    cout << "Login failed. Invalid username/password." << endl;
                }
                break;
            }
            case REGISTER: {
                regPrompt = CREATE_BUYER;
                while (regPrompt != BACK){
                    cout << "Register selected. " << endl;
                    cout << "Select an option: " << endl;
                    cout << "1. Create Buyer Account" << endl;
                    cout << "2. Create Seller Account" << endl;
                    cout << "3. Back" << endl;
                    int regChoice;
                    cin >> regChoice;
                    regPrompt = static_cast<RegisterPrompt>(regChoice - 1);
                    switch (regPrompt) {
                        case CREATE_BUYER: {
                            cout << "Create Buyer Account selected." << endl;
                            Account newAcc;
                            cout << "Enter username: ";
                            cin >> newAcc.username;
                            cout << "Enter password: ";
                            cin >> newAcc.password;
                            cin.ignore();
                            cout << "Enter Name: ";
                            getline(cin, newAcc.name);
                            cout << "Enter Home Address: ";
                            getline(cin, newAcc.address);
                            cout << "Enter Phone Number: ";
                            getline(cin, newAcc.phone);
                            cout << "Enter Email: ";
                            getline(cin, newAcc.email);
                            newAcc.isSeller = false;
                            newAcc.sellerId = 0;
                            newAcc.storeName = "";
                            accounts.push_back(newAcc);
                            inventories.push_back({});
                            bankAccounts.push_back({});
                            sellerOrders.push_back({});
                            buyerOrders.push_back({});
                            cout << "\nBuyer Account Created and Saved!" << endl;
                            break;
                        }
                        case CREATE_SELLER: {
                            cout << "Create Seller Account selected." << endl;
                            Account newAcc;
                            cout << "Enter username: ";
                            cin >> newAcc.username;
                            cout << "Enter password: ";
                            cin >> newAcc.password;
                            cin.ignore();
                            cout << "Enter Owner Name: ";
                            getline(cin, newAcc.name);
                            cout << "Enter Store Name: ";
                            getline(cin, newAcc.storeName);
                            cout << "Enter Seller ID (numeric): ";
                            cin >> newAcc.sellerId;
                            cin.ignore();
                            cout << "Enter Home Address: ";
                            getline(cin, newAcc.address);
                            cout << "Enter Phone Number: ";
                            getline(cin, newAcc.phone);
                            cout << "Enter Email: ";
                            getline(cin, newAcc.email);
                            newAcc.isSeller = true;
                            accounts.push_back(newAcc);
                            inventories.push_back({});
                            bankAccounts.push_back({});
                            sellerOrders.push_back({});
                            buyerOrders.push_back({});
                            cout << "\nSeller Account Created and Saved!" << endl;
                            break;
                        }
                        case BACK: {
                            cout << "Back selected." << endl;
                            break;
                        }
                        default: {
                            cout << "Invalid option." << endl;
                            break;
                        }
                    }
                }
                break;
            }
            case EXIT: {
                cout << "Exiting." << endl;
                break;
            }
            case BANK: {
                bool bankMenu = true;
                while (bankMenu) {
                    cout << "\nBank Menu:\n";
                    cout << "1. List transactions within last week\n";
                    cout << "2. List all bank customers\n";
                    cout << "3. List dormant accounts (no tx within 30 days)\n";
                    cout << "4. Top user (most bank tx today)\n";
                    cout << "5. Back\n";
                    int bch; cin >> bch;
                    switch (bch) {
                        case 1: bank_list_transactions_last_week(); break;
                        case 2: bank_list_customers(); break;
                        case 3: bank_list_dormant_accounts(); break;
                        case 4: bank_top_user_most_tx_today(); break;
                        case 5: bankMenu = false; break;
                        default: cout << "Invalid option.\n"; break;
                    }
                }
                break;
            }
            default: {
                cout << "Invalid option." << endl;
                break;
            }
        }
        cout << endl;
    }
    saveState();    
    savePatients();
    return 0;
}
